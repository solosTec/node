#include <tasks/en13757.h>

#include <smf/mbus/field_definitions.h>
#include <smf/mbus/flag_id.h>
#include <smf/mbus/radio/decode.h>
#include <smf/mbus/reader.h>
#include <smf/mbus/server_id.h>
#include <smf/obis/defs.h>
#include <smf/sml/reader.h>
#include <smf/sml/readout.h>
#include <smf/sml/unpack.h>

#include <cyng/io/serialize.h>
#include <cyng/log/record.h>
#include <cyng/net/client.hpp>
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
            std::bind(&en13757::push, this),	//	3 - "push"
            std::bind(&en13757::stop, this, std::placeholders::_1)		//	4
    }
		, channel_(wp)
		, ctl_(ctl)
        , client_factory_(ctl)
		, logger_(logger)
        , cfg_(config)
        , cfg_sml_(config)
        , cfg_cache_(config, lmn_type::WIRELESS)
        , parser_([this](mbus::radio::header const& h, mbus::radio::tplayer const& t, cyng::buffer_t const& data) {

            //
            // process wireless data header
            //
            this->decode(h, t, data);
        })
    {

        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"receive", "reset-data-sinks", "add-data-sink", "push"});

            CYNG_LOG_TRACE(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    void en13757::stop(cyng::eod) { CYNG_LOG_INFO(logger_, "[EN-13757] stopped"); }

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

    void en13757::decode(mbus::radio::header const &h, mbus::radio::tplayer const &tpl, cyng::buffer_t const &data) {

        //
        //  update cache
        //
        if (cfg_cache_.is_enabled()) {
            update_cache(h, tpl, data);
        }

        //
        //  update load profile
        //
        update_load_profile(h, tpl, data);
    }

    void en13757::update_cache(mbus::radio::header const &h, mbus::radio::tplayer const &tpl, cyng::buffer_t const &data) {

        //
        //  complete payload with header
        //
        auto payload = mbus::radio::restore_data(h, tpl, data);

        //
        //  write into cache
        //
        cfg_.get_cache().access(
            [&](cyng::table *tbl) {
                auto const address = h.get_server_id_as_buffer();
                auto const srv_id = h.get_server_id();

                if (tbl->merge(
                        cyng::key_generator(address),
                        cyng::data_generator(
                            payload, // payload
                            h.get_frame_type(),
                            get_manufacturer_flag(srv_id), // flag
                            h.get_version(),
                            get_medium(srv_id),
                            std::chrono::system_clock::now()),
                        1u,
                        cfg_.get_tag())) {
                    CYNG_LOG_TRACE(logger_, "[roCache] data of meter " << to_string(srv_id) << " inserted");

                } else {
                    CYNG_LOG_TRACE(logger_, "[roCache] data of meter " << to_string(srv_id) << " updated");
                }
            },
            cyng::access::write("mbusCache"));
    }

    void en13757::update_load_profile(mbus::radio::header const &h, mbus::radio::tplayer const &tpl, cyng::buffer_t const &data) {
        auto const auto_cfg = cfg_sml_.is_auto_config();
        auto const def_profile = cfg_sml_.get_default_profile();

        //
        //   get the AES key from table "TMeterMBus"
        //
        cfg_.get_cache().access(
            [&](cyng::table *tbl,
                cyng::table *tbl_readout,
                cyng::table *tbl_data,
                cyng::table *tbl_collector,
                cyng::table *tbl_mirror) {
                cyng::crypto::aes_128_key aes_key; // default is 0

                //
                //  lookup table "meterMBus"
                //
                auto const now = std::chrono::system_clock::now();
                auto const key = cyng::key_generator(h.get_server_id_as_buffer());
                auto const rec = tbl->lookup(key);
                if (rec.empty()) {
                    //
                    //  wireless M-Bus meter not found - insert
                    //

                    //
                    //	start with some known AES keys
                    //
                    auto const id = to_string(h.get_server_id());

#if defined(_DEBUG)
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

                    CYNG_LOG_INFO(logger_, "[EN-13757] insert meter: " << id);
                    tbl->insert(
                        key,
                        cyng::data_generator(
                            now,   // lastSeen
                            "---", // class
                            true,  // visible
                            0u,    // status
                            cyng::make_buffer({0, 0}),
                            cyng::make_buffer({0}),
                            aes_key,
                            //  same as pk
                            h.get_server_id_as_buffer()),
                        1,
                        cfg_.get_tag());
                }

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
                    //
                    //  working with secondary address
                    //
                    auto const meter = to_buffer(tpl.get_secondary_address());
                    BOOST_ASSERT(meter.size() == 9);
                    auto const key2 = cyng::key_generator(meter);
                    auto const rec2 = tbl->lookup(key2);
                    if (rec2.empty()) {
                        CYNG_LOG_INFO(
                            logger_, "[EN-13757] insert meter: " << to_string(tpl.get_secondary_address()) << " (secondary)");
                        tbl->insert(
                            key2,
                            cyng::data_generator(
                                now,
                                "---",
                                true, // visible
                                0u,
                                cyng::make_buffer({0, 0}),
                                cyng::make_buffer({0}),
                                aes_key,
                                meter),
                            1,
                            cfg_.get_tag());

                        //
                        //  Update the adaptor with the primary address
                        //
                        tbl->modify(key, cyng::make_param("secondary", meter), cfg_.get_tag());
                    } else {
                        tbl->modify(key2, cyng::make_param("lastSeen", now), cfg_.get_tag());
                    }
                    decode(
                        tbl_readout,
                        tbl_data,
                        tbl_collector,
                        tbl_mirror,
                        tpl.get_secondary_address(),
                        tpl.get_access_no(),
                        h.get_frame_type(),
                        data,
                        aes_key,
                        //  start auto configuration only for new meters
                        auto_cfg && rec2.empty(),
                        def_profile);
                } else {
                    //
                    //  working with primary address
                    //
                    decode(
                        tbl_readout,
                        tbl_data,
                        tbl_collector,
                        tbl_mirror,
                        h.get_server_id(),
                        tpl.get_access_no(),
                        h.get_frame_type(),
                        data,
                        aes_key,
                        //  start auto configuration only for new meters
                        auto_cfg && rec.empty(),
                        def_profile);
                }
                //}
            },
            cyng::access::write("meterMBus"),
            cyng::access::write("readout"),
            cyng::access::write("readoutData"),
            //  auto configuration
            cyng::access::write("dataCollector"),
            cyng::access::write("dataMirror"));
    }

    void en13757::decode(
        cyng::table *tbl_readout,
        cyng::table *tbl_data,
        cyng::table *tbl_collector,
        cyng::table *tbl_mirror,
        srv_id_t address,
        std::uint8_t access_no,
        std::uint8_t frame_type,
        cyng::buffer_t const &data,
        cyng::crypto::aes_128_key const &key,
        bool auto_cfg,
        cyng::obis def_profile) {

        auto const flag_id = get_manufacturer_code(address);
        auto const manufacturer = mbus::decode(flag_id.first, flag_id.second);
        auto const id = get_id(address); //  meter id as string

        CYNG_LOG_TRACE(
            logger_,
            "[EN-13757] read meter: " << to_string(address) << " (" << manufacturer << ", " << get_medium_name(address) << ", "
                                      << mbus::field_name(static_cast<mbus::ci_field>(frame_type)) << ")");

        auto const payload = mbus::radio::decode(address, access_no, key, data);
        if (mbus::radio::is_decoded(payload)) {
            CYNG_LOG_DEBUG(logger_, "[EN-13757] " << to_string(address) << " payload: " << payload);

            //
            //  update table "readout" with latest readout meta data
            //
            tbl_readout->merge(
                cyng::key_generator(to_buffer(address)),
                cyng::data_generator(
                    payload, frame_type, std::chrono::system_clock::now(), cfg_.get_operating_time(), true // encrypted
                    ),
                1u,
                cfg_.get_tag());

            if (auto_cfg) {
                //
                //  configure a the data collector
                //
                auto const meter = to_buffer(address);
                BOOST_ASSERT(meter.size() == 9);
                tbl_collector->insert(
                    cyng::key_generator(meter, static_cast<std::uint8_t>(1)), //  start with nr = 1
                    cyng::data_generator(def_profile, true, 100u, std::chrono::seconds(0)),
                    1,
                    cfg_.get_tag());
            }

            switch (frame_type) {
            case mbus::FIELD_CI_HEADER_LONG:
            case mbus::FIELD_CI_HEADER_SHORT: {
                //
                //  decode M-Bus format
                //
                read_mbus(
                    tbl_data,
                    tbl_mirror,
                    address,
                    id, //	meter id as string
                    get_medium(address),
                    manufacturer,
                    frame_type,
                    payload,
                    auto_cfg,
                    def_profile);
            } break;
            case mbus::FIELD_CI_RES_LONG_DLMS:
            case mbus::FIELD_CI_RES_SHORT_DLSM:
                // read_dlms(address, payload);
                break;
            case mbus::FIELD_CI_RES_LONG_SML:
            case mbus::FIELD_CI_RES_SHORT_SML:
                //
                //  decode SML format
                //
                read_sml(
                    tbl_data,
                    tbl_mirror,
                    address,
                    id, //	meter id as string
                    payload,
                    auto_cfg,
                    def_profile);
                break;
            default: break;
            }
        } else {

            //
            //  store raw data
            //
            tbl_readout->merge(
                cyng::key_generator(to_buffer(address)),
                cyng::data_generator(
                    data, frame_type, std::chrono::system_clock::now(), cfg_.get_operating_time(), false // not encrypted
                    ),
                1u,
                cfg_.get_tag());

            //
            //  cannot provide encrypted readout data
            //
        }
    }

    void en13757::read_sml(
        cyng::table *tbl_data,
        cyng::table *tbl_mirror,
        srv_id_t const &address,
        std::string const &id,
        cyng::buffer_t const &payload,
        bool auto_cfg,
        cyng::obis def_profile) {

        //
        //  get the binary server id
        //
        auto const meter = to_buffer(address);
        BOOST_ASSERT(meter.size() == 9);

        //
        //  remove previous stuff from table "readoutData"
        //
        auto const removed = tbl_data->erase_if(
            [&](cyng::record &&rec) -> bool {
                //
                //  remove all records with the same meter id
                //
                return rec.value("meterID", meter) == meter;
            },
            cfg_.get_tag());

        if (removed != 0) {
            CYNG_LOG_TRACE(logger_, "[EN-13757] sml removed " << removed << " readout data records of " << id);
        }

        smf::sml::unpack p(
            [&, this](std::string trx, std::uint8_t, std::uint8_t, smf::sml::msg_type type, cyng::tuple_t msg, std::uint16_t crc) {
                CYNG_LOG_TRACE(logger_, "[EN-13757] sml " << smf::sml::get_name(type) << ": " << trx << ", " << msg);

                std::uint8_t idx = 0;
                auto const [client, server, code, tp1, tp2, data] = sml::read_get_list_response(msg);
                for (auto const &m : data) {
                    CYNG_LOG_TRACE(logger_, "[EN-13757] sml " << m.first << ": " << m.second);

                    //
                    //	extract values
                    //
                    auto const reader = cyng::make_reader(m.second);
                    auto const obj = reader.get("raw");
                    auto const value = reader.get("value", ""); //  string
                    auto const unit = cyng::numeric_cast<std::uint8_t>(reader.get("unit"), 0u);
                    auto const unit_name = cyng::value_cast(reader.get("unit-name"), "");
                    auto const scaler = cyng::value_cast<std::int8_t>(reader.get("scaler"), 0u);
                    auto const status = cyng::value_cast<std::uint32_t>(reader.get("status"), 0u);
                    auto const val_time = cyng::value_cast(reader.get("valTime"), std::chrono::system_clock::now());

                    //
                    //	store data readout cache
                    //
                    if (tbl_data->insert(
                            cyng::key_generator(meter, m.first),
                            cyng::data_generator(value, obj.tag(), scaler, unit, status),
                            1,
                            cfg_.get_tag())) {

                        BOOST_ASSERT(idx < 0xFF);
                        ++idx;

                        if (auto_cfg) {

                            CYNG_LOG_INFO(logger_, "[EN-13757] sml auto config " << id << ": " << m.first);
                            //
                            // create an entry in table "dataMirror" assuming the
                            // nr is 1.
                            //
                            tbl_mirror->insert(
                                cyng::key_generator(meter, static_cast<std::uint8_t>(1), idx), //  start with nr = 1
                                cyng::data_generator(m.first),
                                1,
                                cfg_.get_tag());
                        }
                    }
                }

                if (auto_cfg) {
                    //
                    //  push always UTC time (01 00 00 09 0b 00 -  CURRENT_UTC) and second index (81 00 00 09 0b 00 -
                    //  ACT_SENSOR_TIME)
                    //
                    tbl_mirror->insert(
                        cyng::key_generator(meter, static_cast<std::uint8_t>(1), idx), //  start with nr = 1
                        cyng::data_generator(OBIS_CURRENT_UTC),
                        1,
                        cfg_.get_tag());
                    tbl_mirror->insert(
                        cyng::key_generator(meter, static_cast<std::uint8_t>(1), idx), //  start with nr = 1
                        cyng::data_generator(OBIS_ACT_SENSOR_TIME),
                        1,
                        cfg_.get_tag());
                }

                // count = data.size();
                CYNG_LOG_INFO(
                    logger_, "[EN-13757] sml " << server << ": " << data.size() << "/" << tbl_data->size() << " records merged");
            });
        p.read(payload.begin() + 2, payload.end());
    }

    void en13757::read_mbus(
        cyng::table *tbl_data,
        cyng::table *tbl_mirror,
        srv_id_t const &address,
        std::string const &id,
        std::uint8_t medium,
        std::string const &manufacturer,
        std::uint8_t frame_type,
        cyng::buffer_t const &payload,
        bool auto_cfg,
        cyng::obis def_profile) {

        //
        //  get the binary server id
        //
        auto const meter = to_buffer(address);
        BOOST_ASSERT(meter.size() == 9);

        //
        //  remove previous stuff from table "readoutData"
        //
        auto const removed = tbl_data->erase_if(
            [&](cyng::record &&rec) -> bool {
                //
                //  remove all records with the same meter id
                //
                return rec.value("meterID", meter) == meter;
            },
            cfg_.get_tag());

        if (removed != 0) {
            CYNG_LOG_TRACE(logger_, "[EN-13757] wmbus removed " << removed << " readout data records of " << id);
        }

        std::size_t offset = 2;
        std::uint8_t idx = 0;

        while (offset < payload.size()) {
            // std::size_t, cyng::obis, cyng::object, std::int8_t, unit, bool
            auto const [index, code, obj, scaler, u, valid] = smf::mbus::read(payload, offset, 1);
            if (valid) {

                //
                //  scaling the value
                //
                auto const value = sml::read_value(code, scaler, obj);

                //
                //	store data to readout cache
                //
                auto const val = cyng::io::to_plain(obj);
                CYNG_LOG_TRACE(
                    logger_, "[EN-13757] wmbus " << code << ": " << val << "e" << +scaler << " " << smf::mbus::get_name(u));

                if (tbl_data->insert(
                        cyng::key_generator(to_buffer(address), code),
                        cyng::data_generator(val, obj.tag(), scaler, mbus::to_u8(u), static_cast<std::uint32_t>(0)),
                        1,
                        cfg_.get_tag())) {
                    ++idx;

                    if (auto_cfg) {
                        CYNG_LOG_INFO(logger_, "[EN-13757] wmbus auto config " << id << ": " << code);
                        //
                        // create an entry in table "dataMirror" assuming the
                        // nr is 1.
                        //
                        tbl_mirror->insert(
                            cyng::key_generator(meter, static_cast<std::uint8_t>(1), idx), //  start with nr = 1
                            cyng::data_generator(code),
                            1,
                            cfg_.get_tag());
                    }
                }
            }

            offset = index;
        }

        if (auto_cfg) {
            //
            //  push always UTC time (01 00 00 09 0b 00 -  CURRENT_UTC) and second index (81 00 00 09 0b 00 -
            //  ACT_SENSOR_TIME)
            //
            tbl_mirror->insert(
                cyng::key_generator(meter, static_cast<std::uint8_t>(1), idx), //  start with nr = 1
                cyng::data_generator(OBIS_CURRENT_UTC),
                1,
                cfg_.get_tag());
            tbl_mirror->insert(
                cyng::key_generator(meter, static_cast<std::uint8_t>(1), idx), //  start with nr = 1
                cyng::data_generator(OBIS_ACT_SENSOR_TIME),
                1,
                cfg_.get_tag());
        }
        CYNG_LOG_INFO(
            logger_, "[EN-13757] wmbus " << to_string(address) << ": " << +idx << "/" << tbl_data->size() << " records merged");
    }

    void en13757::push() {

        if (auto sp = channel_.lock(); sp) {
            //
            //  restart push
            //
            auto const period = cfg_cache_.get_period();
            sp->suspend(period, "push");

            //
            //  send data
            //
            auto proxy = client_factory_.create_proxy<boost::asio::ip::tcp::socket, 2048>(
                [](std::size_t) -> std::pair<std::chrono::seconds, bool> {
                    return {std::chrono::seconds(0), false};
                },
                [&](boost::asio::ip::tcp::endpoint ep, cyng::channel_ptr cp) {
                    CYNG_LOG_TRACE(logger_, "[EN-13757] successful connected to " << ep);

                    //
                    //  send data
                    //
                    // cp->dispatch("send", cyng::make_buffer("hello"));
                    push_data(cp);
                },
                [&](cyng::buffer_t data) {
                    //  should not receive anything - send only
                    CYNG_LOG_INFO(logger_, "[EN-13757] received " << data.size() << " bytes");
                },
                [&](boost::system::error_code ec) {
                    //	fill async
                    CYNG_LOG_WARNING(logger_, "[EN-13757] connection lost " << ec.message());
                });

            auto const host = cfg_cache_.get_push_host();
            auto const service = cfg_cache_.get_push_service();
            CYNG_LOG_INFO(logger_, "[EN-13757] open connection to " << host << ":" << service);
            proxy.connect(host, service);
        }
    }

    void en13757::push_data(cyng::channel_ptr cp) {
        cfg_.get_cache().access(
            [&](cyng::table *tbl) {
                tbl->loop([=, this](cyng::record &&rec, std::size_t) -> bool {
                    auto const id = rec.value("meterID", cyng::make_buffer());
                    auto const payload = rec.value("payload", cyng::make_buffer());
                    CYNG_LOG_TRACE(logger_, "[roCache] send data of meter " << id);
                    cp->dispatch("send");
                    return true;
                });
                //
                //  remove all data
                //
                tbl->clear(cfg_.get_tag());
            },
            cyng::access::write("mbusCache"));
    }
} // namespace smf
