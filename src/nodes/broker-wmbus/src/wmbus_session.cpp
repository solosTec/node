#include <wmbus_session.h>

#include <smf/mbus/flag_id.h>
#include <smf/mbus/radio/decode.h>
#include <smf/mbus/radio/header.h>
#include <smf/mbus/reader.h>
#include <smf/sml/msg.h>
#include <smf/sml/reader.h>
#include <smf/sml/readout.h>
#include <smf/sml/unpack.h>

#include <cyng/io/io_buffer.h>
#include <cyng/io/serialize.h>
#include <cyng/log/record.h>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/container_factory.hpp>

#include <iostream>

#ifdef _DEBUG_BROKER_WMBUS
#include <cyng/io/hex_dump.hpp>
#include <iostream>
#include <sstream>
#endif

#include <boost/uuid/nil_generator.hpp>

namespace smf {

    wmbus_session::wmbus_session(
        boost::asio::ip::tcp::socket socket,
        bus &cluster_bus,
        cyng::mesh &fabric,
        cyng::logger logger,
        std::shared_ptr<db> db,
        cyng::key_t key,
        cyng::channel_ptr writer)
        : base_t(
              std::move(socket),
              cluster_bus,
              fabric,
              logger,
              mbus::radio::parser([this](mbus::radio::header const &h, mbus::radio::tplayer const &t, cyng::buffer_t const &data) {
                  this->decode(h, t, data);
              }),
              mbus::serializer{})
        , db_(db)
        , key_gw_wmbus_(key)
        , writer_(writer) {}

    wmbus_session::~wmbus_session() {
#ifdef _DEBUG_BROKER_WMBUS
        std::cout << "wmbus_session(~)" << std::endl;
#endif
    }

    void wmbus_session::stop() {
        CYNG_LOG_WARNING(logger_, "stop session");
        close_socket();
    }

    void wmbus_session::decode(mbus::radio::header const &h, mbus::radio::tplayer const &t, cyng::buffer_t const &data) {

        if (bus_.is_connected()) {

            auto const meter = mbus::to_string(h);
            CYNG_LOG_TRACE(logger_, "[wmbus] meter: " << meter << " (" << key_gw_wmbus_ << ")");

            // key_gw_wmbus_
            bus_.req_db_update(
                "gwwMBus",
                key_gw_wmbus_,
                cyng::param_map_factory()("meter", meter) //  meter name
                ("state", static_cast<std::uint16_t>(2))  //  status: reading
            );

            if (h.has_secondary_address()) {
                decode(t.get_secondary_address(), t.get_access_no(), h.get_frame_type(), data);
            } else {
                decode(h.get_server_id(), t.get_access_no(), h.get_frame_type(), data);
            }
        } else {
            CYNG_LOG_WARNING(logger_, "[wmbus] meter: " << mbus::to_string(h) << " cannot decode data since offline");
        }
    }

    void wmbus_session::decode(srv_id_t address, std::uint8_t access_no, std::uint8_t frame_type, cyng::buffer_t const &data) {

        auto const flag_id = get_manufacturer_code(address);
        auto const manufacturer = mbus::decode(flag_id.first, flag_id.second);

        if (db_) {

            //
            //	if tag is "nil" no meter configuration was found
            //
            auto const id = get_id(address);
            auto const [key, tag] = db_->lookup_meter(id);
            if (!tag.is_nil()) {

                auto const payload = mbus::radio::decode(address, access_no, key, data);

                if (mbus::radio::is_decoded(payload)) {

                    switch (frame_type) {
                    case mbus::FIELD_CI_HEADER_LONG:
                    case mbus::FIELD_CI_HEADER_SHORT: {
                        auto const count = read_mbus(
                            address,
                            id, //	meter id
                            get_medium(address),
                            manufacturer,
                            frame_type,
                            payload);
                        bus_.req_db_insert_auto(
                            "wMBusUplink",
                            cyng::data_generator(
                                std::chrono::system_clock::now(),
                                id, //	meter id
                                get_medium(address),
                                manufacturer,
                                frame_type,
                                std::to_string(count) + " records: " + cyng::io::to_hex(payload),
                                boost::uuids::nil_uuid()));
                    } break;
                    case mbus::FIELD_CI_RES_LONG_SML:
                    case mbus::FIELD_CI_RES_SHORT_SML: {
                        auto const count = read_sml(address, payload);
                        bus_.req_db_insert_auto(
                            "wMBusUplink",
                            cyng::data_generator(
                                std::chrono::system_clock::now(),
                                id, //	meter id
                                get_medium(address),
                                manufacturer,
                                frame_type,
                                std::to_string(count) + " records: " + cyng::io::to_hex(payload),
                                boost::uuids::nil_uuid()));
                    } break;
                    default:
                        //	unknown frame type
                        //
                        //	insert uplink data
                        //
                        bus_.req_db_insert_auto(
                            "wMBusUplink",
                            cyng::data_generator(
                                std::chrono::system_clock::now(),
                                id, //	meter id
                                get_medium(address),
                                manufacturer,
                                frame_type,
                                cyng::io::to_hex(payload), //	"payload",
                                boost::uuids::nil_uuid()));

                        break;
                    }
                }

                //
                //	update config data
                //	ip adress, port and last seen
                //
                auto const ep = get_remote_endpoint();
                bus_.req_db_update(
                    "meterwMBus",
                    cyng::key_generator(tag),
                    cyng::param_map_factory()("address", ep.address())("port", ep.port())(
                        "lastSeen", std::chrono::system_clock::now()));

                switch (frame_type) {
                case mbus::FIELD_CI_RES_LONG_SML:  // 0x7E - long header
                case mbus::FIELD_CI_RES_SHORT_SML: // 0x7F - short header
                    push_sml_data(payload);
                    break;
                case mbus::FIELD_CI_RES_LONG_DLMS:  // 0x7C
                case mbus::FIELD_CI_RES_SHORT_DLSM: // 0x7D - short header
                    push_dlsm_data(payload);
                    break;
                case mbus::FIELD_CI_HEADER_LONG:  // 0x72 - 12 byte header followed by variable format data (EN 13757-3)
                case mbus::FIELD_CI_HEADER_SHORT: // 0x7A - 4 byte header followed by variable format data (EN 13757-3)
                    push_data(payload);
                    break;

                default: break;
                }

            } else {
                bus_.sys_msg(cyng::severity::LEVEL_WARNING, "[wmbus]", to_string(address), "has no AES key");
            }
        } else {
            CYNG_LOG_ERROR(logger_, "[wmbus] no database");
            bus_.sys_msg(cyng::severity::LEVEL_ERROR, "[wmbus] no database");
        }
    }

    void wmbus_session::push_sml_data(cyng::buffer_t const &payload) {
#ifdef _DEBUG_BROKER_WMBUS
        //
        //	read SML data
        //
        // smf::sml::unpack p([this](std::string trx, std::uint8_t, std::uint8_t, smf::sml::msg_type type, cyng::tuple_t msg,
        // std::uint16_t crc) {

        //	CYNG_LOG_DEBUG(logger_, "SML> " << smf::sml::get_name(type) << ": " << trx << ", " << msg);

        //	});
        // p.read(payload.begin() + 2, payload.end());
#endif
    }
    void wmbus_session::push_dlsm_data(cyng::buffer_t const &payload) {
        ctl_.get_registry().dispatch(
            "push",
            "send.dlsm",
            //  handle dispatch errors
            std::bind(&bus::log_dispatch_error, &bus_, std::placeholders::_1, std::placeholders::_2),
            cyng::make_tuple());
    }
    void wmbus_session::push_data(cyng::buffer_t const &payload) {}

    std::size_t wmbus_session::read_mbus(
        srv_id_t const &address,
        std::string const &id,
        std::uint8_t medium,
        std::string const &manufacturer,
        std::uint8_t frame_type,
        cyng::buffer_t const &payload) {

        CYNG_LOG_TRACE(logger_, "[wmbus] payload " << payload);

        //
        //	open CSV file
        //
        //  handle dispatch errors
        writer_->dispatch("open", std::bind(&bus::log_dispatch_error, &bus_, std::placeholders::_1, std::placeholders::_2), id);

        std::size_t offset = 2;
        cyng::obis code;
        cyng::object obj;
        std::int8_t scaler = 0;
        smf::mbus::unit u;
        std::stringstream ss;
        bool valid = false;
        bool init = false;
        std::size_t count{0};
        cyng::tuple_t sml_list;
        while (offset < payload.size()) {
            std::tie(offset, code, obj, scaler, u, valid) = smf::mbus::read(payload, offset, 1);
            if (valid) {

                //
                //  scaling the value
                //
                auto const value = sml::read_value(code, scaler, obj);

                //
                //	store data to csv file
                //
                //  handle dispatch errors
                writer_->dispatch(
                    "store",
                    std::bind(&bus::log_dispatch_error, &bus_, std::placeholders::_1, std::placeholders::_2),
                    code,
                    value,
                    smf::mbus::get_name(u));

                if (!init) {
                    init = true;
                } else {
                    ss << ", ";
                }
                ss << code << ": " << obj << "e" << +scaler << " " << smf::mbus::get_name(u);
                ++count;

                //
                //  produce a SML_ListEntry
                // ToDo: extra handling of time points
                // How to serialize time points in SML Lists?
                //
                if (obj.tag() != cyng::TC_TIME_POINT) {
                    sml_list.push_back(cyng::make_object(sml::list_entry(code, 0, mbus::to_u8(u), scaler, obj)));
                }
            }
        }

        //
        //	close CSV file
        //
        //  handle dispatch errors
        writer_->dispatch("commit", std::bind(&bus::log_dispatch_error, &bus_, std::placeholders::_1, std::placeholders::_2));

        auto const msg = ss.str();
        CYNG_LOG_TRACE(logger_, "[sml] CI_HEADER_SHORT: " << msg);

        ctl_.get_registry().dispatch(
            "push",
            "send.mbus",
            //  handle dispatch errors
            std::bind(&bus::log_dispatch_error, &bus_, std::placeholders::_1, std::placeholders::_2),
            cyng::buffer_t(address.begin(), address.end()),
            sml_list);
        return count;
    }

    std::size_t wmbus_session::read_sml(srv_id_t const &address, cyng::buffer_t const &payload) {

        //
        //	get meter id
        //
        auto const id = get_id(address);

        //
        //	open CSV file
        //
        writer_->dispatch("open", std::bind(&bus::log_dispatch_error, &bus_, std::placeholders::_1, std::placeholders::_2), id);

        std::size_t count{0};
        smf::sml::unpack p(
            [this,
             &count](std::string trx, std::uint8_t, std::uint8_t, smf::sml::msg_type type, cyng::tuple_t msg, std::uint16_t crc) {
                CYNG_LOG_TRACE(logger_, "[sml] " << smf::sml::get_name(type) << ": " << trx << ", " << msg);

                auto const [client, server, code, tp1, tp2, data] = sml::read_get_list_response(msg);
                for (auto const &m : data) {
                    CYNG_LOG_TRACE(logger_, "[sml] " << m.first << ": " << m.second);

                    //
                    //	extract values
                    //
                    auto const reader = cyng::make_reader(m.second);
                    auto const value = cyng::io::to_plain(reader.get("value"));
                    auto const unit_name = cyng::value_cast(reader.get("unit.name"), "");

                    //
                    //	store data to csv file
                    //
                    //  handle dispatch errors
                    writer_->dispatch(
                        "store",
                        std::bind(&bus::log_dispatch_error, &bus_, std::placeholders::_1, std::placeholders::_2),
                        cyng::make_tuple(m.first, value, unit_name));
                }

                count = data.size();
            });
        p.read(payload.begin() + 2, payload.end());

        //
        //	close CSV file
        //
        writer_->dispatch("commit", std::bind(&bus::log_dispatch_error, &bus_, std::placeholders::_1, std::placeholders::_2));

        //
        //	remove trailing 0x2F
        //
        cyng::buffer_t data(payload.begin() + 2, payload.end());
        if (!data.empty()) {
            auto index = data.size();
            for (; index > 0 && data.at(index - 1) == 0x2F; --index)
                ;
            data.resize(index);
        }

        ctl_.get_registry().dispatch(
            "push",
            "send.sml",
            //  handle dispatch errors
            std::bind(&bus::log_dispatch_error, &bus_, std::placeholders::_1, std::placeholders::_2),
            cyng::make_tuple(cyng::buffer_t(address.begin(), address.end()), data));

        return count;
    }

} // namespace smf
