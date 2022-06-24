#include <tasks/en13757.h>

#include <smf/mbus/field_definitions.h>
#include <smf/mbus/flag_id.h>
#include <smf/mbus/radio/decode.h>
#include <smf/mbus/reader.h>
#include <smf/mbus/server_id.h>
#include <smf/sml/reader.h>
#include <smf/sml/readout.h>
#include <smf/sml/unpack.h>

#include <cyng/io/serialize.h>
#include <cyng/log/record.h>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/intrinsics/buffer.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/controller.h>

#ifdef _DEBUG_SEGW
#include <cyng/io/hex_dump.hpp>
#endif

namespace smf {
    en13757::en13757(cyng::channel_weak wp
		, cyng::controller& ctl
		, cyng::logger logger
		, cfg& config)
	: sigs_{
            std::bind(&en13757::receive, this, std::placeholders::_1),	//	0
            std::bind(&en13757::reset_target_channels, this),	//	1 - "reset-data-sinks"
            std::bind(&en13757::add_target_channel, this, std::placeholders::_1),	//	2 - "add-data-sink"
            std::bind(&en13757::stop, this, std::placeholders::_1)		//	4
    }
		, channel_(wp)
		, ctl_(ctl)
		, logger_(logger)
        , cfg_(config)
        , parser_([this](mbus::radio::header const& h, mbus::radio::tplayer const& t, cyng::buffer_t const& data) {

            this->decode(h, t, data);
            //   auto const res = smf::mbus::radio::decode(h.get_server_id(), t.get_access_no(), aes, payload);
        })
    {

        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"receive", "reset-data-sinks", "add-data-sink"});

            CYNG_LOG_TRACE(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    void en13757::stop(cyng::eod) {
        CYNG_LOG_INFO(logger_, "[EN-13757] stopped");
        boost::system::error_code ec;
    }

    void en13757::receive(cyng::buffer_t data) {
        CYNG_LOG_TRACE(logger_, "[EN-13757] received " << data.size() << " bytes");
        parser_.read(std::begin(data), std::end(data));
    }

    void en13757::reset_target_channels() {
        // targets_.clear();
        CYNG_LOG_TRACE(logger_, "[EN-13757] clear target channels");
    }

    void en13757::add_target_channel(std::string name) {
        // targets_.insert(name);
        CYNG_LOG_TRACE(logger_, "[EN-13757] add target channel: " << name);
    }

    // void en13757::update_statistics() { CYNG_LOG_TRACE(logger_, "[EN-13757] update statistics"); }

    void en13757::decode(mbus::radio::header const &h, mbus::radio::tplayer const &t, cyng::buffer_t const &data) {
        // auto const flag_id = get_manufacturer_code(h.get_server_id());
        // CYNG_LOG_TRACE(
        //     logger_,
        //     "[EN-13757] read meter: " << get_id(h.get_server_id()) << " (" << mbus::decode(flag_id.first, flag_id.second) << ", "
        //                               << get_medium_name(h.get_server_id()) << ")");

        //
        //   get the AES key from table "TMeterMBus"
        //
        cfg_.get_cache().access(
            [&](cyng::table *tbl) {
                cyng::crypto::aes_128_key aes_key;
                auto const now = std::chrono::system_clock::now();
                auto const key = cyng::key_generator(h.get_server_id_as_buffer());
                auto const rec = tbl->lookup(key);
                if (rec.empty()) {
                //
                //  wireless M-Bus meter not found - insert
                //

#if defined(_DEBUG)
                    //
                    //	start with some known AES keys
                    //
                    auto const id = srv_id_to_str(h.get_server_id());
                    // auto const id = sml::from_server_id(dev_id);
                    if (boost::algorithm::equals(id, "01-e61e-29436587-bf-03") ||
                        boost::algorithm::equals(id, "01-e61e-13090016-3c-07")) {
                        aes_key.key_ = {
                            0x51, 0x72, 0x89, 0x10, 0xE6, 0x6D, 0x83, 0xF8, 0x51, 0x72, 0x89, 0x10, 0xE6, 0x6D, 0x83, 0xF8};
                    } else if (id == "01-a815-74314504-01-02") {
                        //	23A84B07EBCBAF948895DF0E9133520D
                        aes_key.key_ = {
                            0x23, 0xA8, 0x4B, 0x07, 0xEB, 0xCB, 0xAF, 0x94, 0x88, 0x95, 0xDF, 0x0E, 0x91, 0x33, 0x52, 0x0D};
                    } else if (
                        boost::algorithm::equals(id, "01-e61e-79426800-02-0e") ||
                        boost::algorithm::equals(id, "01-e61e-57140621-36-03")) {
                        //	6140B8C066EDDE3773EDF7F8007A45AB
                        aes_key.key_ = {
                            0x61, 0x40, 0xB8, 0xC0, 0x66, 0xED, 0xDE, 0x37, 0x73, 0xED, 0xF7, 0xF8, 0x00, 0x7A, 0x45, 0xAB};
                    } else {
                        aes_key.key_ = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf};
                    }
#endif

                    tbl->insert(
                        key,
                        cyng::data_generator(now, "---", true, 0u, cyng::make_buffer({0, 0}), cyng::make_buffer({0}), aes_key),
                        1,
                        cfg_.get_tag());
                } else {
                    //
                    //  get AES key
                    //
                    aes_key = rec.value("aes", aes_key);

                    //
                    //  update
                    //
                    tbl->modify(rec.key(), cyng::make_param("lastSeen", now), cfg_.get_tag());

                    BOOST_ASSERT_MSG(!cyng::is_null(aes_key), "no aes key");
                    if (h.has_secondary_address()) {
                        decode(t.get_secondary_address(), t.get_access_no(), h.get_frame_type(), data, aes_key);
                    } else {
                        decode(h.get_server_id(), t.get_access_no(), h.get_frame_type(), data, aes_key);
                    }
                }
            },
            cyng::access::write("meterMBus"));
    }

    void en13757::decode(
        srv_id_t address,
        std::uint8_t access_no,
        std::uint8_t frame_type,
        cyng::buffer_t const &data,
        cyng::crypto::aes_128_key const &key) {

        auto const flag_id = get_manufacturer_code(address);
        auto const manufacturer = mbus::decode(flag_id.first, flag_id.second);
        CYNG_LOG_TRACE(
            logger_,
            "[EN-13757] read meter: " << get_id(address) << " (" << manufacturer << ", " << get_medium_name(address) << ", "
                                      << mbus::field_name(static_cast<mbus::ci_field>(frame_type)) << ")");

        auto const payload = mbus::radio::decode(address, access_no, key, data);
        if (mbus::radio::is_decoded(payload)) {
            CYNG_LOG_DEBUG(logger_, "[EN-13757] payload: " << payload);
            switch (frame_type) {
            case mbus::FIELD_CI_HEADER_LONG:
            case mbus::FIELD_CI_HEADER_SHORT: {
                auto const id = get_id(address);
                read_mbus(
                    address,
                    id, //	meter id
                    get_medium(address),
                    manufacturer,
                    frame_type,
                    payload);
            } break;
            case mbus::FIELD_CI_RES_LONG_DLMS:
            case mbus::FIELD_CI_RES_SHORT_DLSM:
                // read_dlms(address, payload);
                break;
            case mbus::FIELD_CI_RES_LONG_SML:
            case mbus::FIELD_CI_RES_SHORT_SML: read_sml(address, payload); break;
            default: break;
            }
        }
    }
    std::size_t en13757::read_sml(srv_id_t const &address, cyng::buffer_t const &payload) {
        smf::sml::unpack p(
            [&, this](std::string trx, std::uint8_t, std::uint8_t, smf::sml::msg_type type, cyng::tuple_t msg, std::uint16_t crc) {
                CYNG_LOG_TRACE(logger_, "[sml] " << smf::sml::get_name(type) << ": " << trx << ", " << msg);

                auto const [client, server, code, tp1, tp2, data] = sml::read_get_list_response(msg);
                for (auto const &m : data) {
                    CYNG_LOG_TRACE(logger_, "[sml] " << m.first << ": " << m.second);

                    //
                    //	extract values
                    //
                    auto const reader = cyng::make_reader(m.second);
                    auto const value = cyng::io::to_plain(reader.get("value"));
                    auto const unit_name = cyng::value_cast(reader.get("unit-name"), "");

                    //
                    //	store data to csv file
                    //
                    // writer_->dispatch("store", cyng::make_tuple(m.first, value, unit_name));
                }

                // count = data.size();
            });
        p.read(payload.begin() + 2, payload.end());
        return 0;
    }

    std::size_t en13757::read_mbus(
        srv_id_t const &address,
        std::string const &id,
        std::uint8_t medium,
        std::string const &manufacturer,
        std::uint8_t frame_type,
        cyng::buffer_t const &payload) {

        // CYNG_LOG_TRACE(logger_, "[wmbus] payload " << payload);
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
                // writer_->dispatch("store", cyng::make_tuple(code, value, smf::mbus::get_unit_name(u)));

                if (!init) {
                    init = true;
                } else {
                    ss << ", ";
                }
                ss << code << ": " << obj << "e" << +scaler << " " << smf::mbus::get_unit_name(u);
                CYNG_LOG_TRACE(logger_, "[wmbus] " << code << ": " << obj << "e" << +scaler << " " << smf::mbus::get_unit_name(u));
                ++count;

                //
                //  produce a SML_ListEntry
                // ToDo: extra handling of time points
                // How to serialize time points in SML Lists?
                //
                if (obj.tag() != cyng::TC_TIME_POINT) {
                    // sml_list.push_back(cyng::make_object(sml::list_entry(code, 0, mbus::to_u8(u), scaler, obj)));
                }
            }
        }
        return 0;
    }
} // namespace smf
