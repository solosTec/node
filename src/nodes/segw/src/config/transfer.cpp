#include <config/transfer.h>

#include <config/cfg_nms.h>
#include <smf.h>
#include <storage_functions.h>

#include <smf/ipt/config.h>
#include <smf/ipt/scramble_key_format.h>
#include <smf/mbus/server_id.h>
#include <smf/obis/db.h>
#include <smf/obis/defs.h>
#include <smf/obis/profile.h>

//
#include <cyng/db/storage.h>
#include <cyng/io/serialize.h>
#include <cyng/obj/algorithm/dom_transform.h>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/container_factory.hpp>
#include <cyng/obj/util.hpp>
#include <cyng/obj/vector_cast.hpp>
#include <cyng/parse/duration.h>
#include <cyng/parse/string.h>
#include <cyng/sql/sql.hpp>

#ifdef _DEBUG_SEGW
#include <iostream>
#endif

#include <boost/uuid/string_generator.hpp>

namespace smf {
    void read_json_config(cyng::db::session &db, cyng::object &&cfg) {
        BOOST_ASSERT(db.is_alive());

        auto const reader = cyng::make_reader(std::move(cfg));
        if (reader.size() == 0) {
            std::cout << "***warning: no config data" << std::endl;
        } else {
            std::cout << "***info   : transfer " << reader.size() << " records into database" << std::endl;
        }

        //
        //	start transaction
        //
        cyng::db::transaction trx(db);

        //
        //	prepare SQL INSERT statement
        //
        auto const ms = get_table_cfg();
        auto const sql_insert = cyng::sql::insert(db.get_dialect(), ms).bind_values(ms)();
        std::cout << "***info   : prepare " << sql_insert << std::endl;

        auto stmt_insert = db.create_statement();
        std::pair<int, bool> const r_insert = stmt_insert->prepare(sql_insert);

        if (r_insert.second) {

            //
            //	prepare SQL SELECT statement
            //
            auto const sql_select = cyng::sql::select(db.get_dialect(), ms).all().from().where(cyng::sql::pk())();
            std::cout << "***info   : prepare " << sql_select << std::endl;

            auto stmt_select = db.create_statement();
            std::pair<int, bool> const r_select = stmt_select->prepare(sql_select);
            if (r_select.second) {

                //
                //	transfer some global entries
                //
                // insert_config_record(
                //    stmt_insert,
                //    stmt_select,
                //    cyng::to_path(cfg::sep, "country.code"),
                //    reader["country.code"].get(),
                //    "country code");

                // insert_config_record(
                //     stmt_insert,
                //     stmt_select,
                //     cyng::to_path(cfg::sep, "language.code"),
                //     reader["language.code"].get(),
                //     "language code");

                // std::cout << "***info   : insert " << cyng::to_path(cfg::sep, "generate-profile") << std::endl;
                insert_config_record(
                    stmt_insert,
                    stmt_select,
                    cyng::to_path(cfg::sep, "generate-profile"),
                    reader["generate-profile"].get(),
                    "generate profiles");

                auto const obj = reader["tag"].get();
                if (cyng::is_of_type<cyng::TC_STRING>(obj)) {
                    auto const str = cyng::value_cast(obj, "00000000-0000-0000-0000-000000000000");
                    if (str.size() == 36 && str.at(8) == '-' && str.at(13) == '-' && str.at(18) == '-' && str.at(23) == '-') {

                        //
                        //	convert string into UUID
                        //
                        boost::uuids::string_generator sgen;
                        insert_config_record(
                            stmt_insert,
                            stmt_select,
                            cyng::to_path(cfg::sep, "tag"),
                            cyng::make_object(sgen(str)),
                            "unique app id");
                    } else {
#ifdef _DEBUG_SEGW
                        std::cerr << "**warning: invalid tag: " << str << std::endl;
#endif
                    }
                }

                //
                //	transfer network/server configuration
                //
                read_net(stmt_insert, stmt_select, cyng::container_cast<cyng::param_map_t>(reader["net"].get()));

                //
                //	transfer IP-T configuration
                //
                read_ipt_config(stmt_insert, stmt_select, cyng::container_cast<cyng::vector_t>(reader["ipt"]["config"].get()));

                //
                //	transfer IP-T parameter
                //
                read_ipt_params(stmt_insert, stmt_select, cyng::container_cast<cyng::param_map_t>(reader["ipt"]["param"].get()));

                //
                //	transfer hardware configuration
                //
                read_hardware(stmt_insert, stmt_select, cyng::container_cast<cyng::param_map_t>(reader["hardware"].get()));

                //
                //	transfer SML server configuration
                //	%(("accept.all.ids":false),("account":operator),("address":0.0.0.0),("discover":5798),("enabled":true),("pwd":operator),("service":7259))
                //
                read_sml(stmt_insert, stmt_select, cyng::container_cast<cyng::param_map_t>(reader["sml"].get()));

                //
                //	transfer NMS server configuration
                //
                read_nms(stmt_insert, stmt_select, cyng::container_cast<cyng::param_map_t>(reader["nms"].get()));

                //
                //	transfer LMN configuration
                //
                read_lnm(stmt_insert, stmt_select, cyng::container_cast<cyng::vector_t>(reader["lmn"].get()));

                //
                //	transfer GPIO configuration
                //
                read_gpio(stmt_insert, stmt_select, cyng::container_cast<cyng::param_map_t>(reader["gpio"].get()));

                //
                //	transfer virtual meter configuration
                //
                // transfer_vmeter(stmt, cyng::container_cast<cyng::param_map_t>(reader["virtual-meter"].get()));
            } else {

                std::cerr << "**error: prepare statement failed: " << sql_select << std::endl;
            }

        } else {

            std::cerr << "**error: prepare statement failed" << sql_insert << std::endl;
        }
    }

    void read_net(cyng::db::statement_ptr stmt_insert, cyng::db::statement_ptr stmt_select, cyng::param_map_t &&pmap) {

        insert_config_record(
            stmt_insert,
            stmt_select,
            cyng::to_path(cfg::sep, "net", "mac"),
            cyng::make_object(cyng::find_value(pmap, std::string("mac"), std::string())),
            "HAN MAC");

        insert_config_record(
            stmt_insert,
            stmt_select,
            cyng::to_path(cfg::sep, OBIS_SERVER_ID),
            cyng::make_object(cyng::find_value(pmap, cyng::to_string(OBIS_SERVER_ID), std::string())),
            "Server ID");

        auto const vec = cyng::vector_cast<std::string>(cyng::find(pmap, "local-links"), std::string("0000::0000:0000:0000:0000"));
        std::size_t counter{0};
        for (auto link : vec) {

            insert_config_record(
                stmt_insert,
                stmt_select,
                cyng::to_path(cfg::sep, "net", "local-link", std::to_string(++counter)),
                cyng::make_object(link),
                "local link (computed)");
        }
    }

    void read_ipt_config(cyng::db::statement_ptr stmt_insert, cyng::db::statement_ptr stmt_select, cyng::vector_t &&vec) {

        std::uint8_t idx{1};
        for (auto const &cfg : vec) {
            //
            //	%(("account":segw),
            //	("def.sk":0102030405060708090001020304050607080900010203040506070809000001),
            //	("host":127.0.0.1),
            //	("pwd":NODE_PWD),
            //	("scrambled":true),
            //	("service":26862))
            //
            // std::cout << "IP-T: " << cfg << std::endl;
            auto const srv = ipt::read_config(cyng::container_cast<cyng::param_map_t>(cfg));

            insert_config_record(
                stmt_insert,
                stmt_select,
                cyng::to_path(
                    cfg::sep,
                    OBIS_ROOT_IPT_PARAM,
                    cyng::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
                    cyng::make_obis(0x81, 0x49, 0x17, 0x07, 0x00, idx)),
                cyng::make_object(srv.host_),
                "IP-T server address");

            insert_config_record(
                stmt_insert,
                stmt_select,
                cyng::to_path(
                    cfg::sep,
                    OBIS_ROOT_IPT_PARAM,
                    cyng::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
                    cyng::make_obis(0x81, 0x49, 0x1A, 0x07, 0x00, idx)),
                cyng::make_object(srv.service_),
                "IP-T server target port");

            insert_config_record(
                stmt_insert,
                stmt_select,
                cyng::to_path(
                    cfg::sep,
                    OBIS_ROOT_IPT_PARAM,
                    cyng::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
                    cyng::make_obis(0x81, 0x49, 0x19, 0x07, 0x00, idx)),
                cyng::make_object(static_cast<std::uint16_t>(0u)),
                "IP-T server source port");

            insert_config_record(
                stmt_insert,
                stmt_select,
                cyng::to_path(
                    cfg::sep,
                    OBIS_ROOT_IPT_PARAM,
                    cyng::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
                    cyng::make_obis(0x81, 0x49, 0x63, 0x3C, 0x01, idx)),
                cyng::make_object(srv.account_),
                "login account");

            insert_config_record(
                stmt_insert,
                stmt_select,
                cyng::to_path(
                    cfg::sep,
                    OBIS_ROOT_IPT_PARAM,
                    cyng::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
                    cyng::make_obis(0x81, 0x49, 0x63, 0x3C, 0x02, idx)),
                cyng::make_object(srv.pwd_),
                "login password");

            insert_config_record(
                stmt_insert,
                stmt_select,
                cyng::to_path(
                    cfg::sep,
                    OBIS_ROOT_IPT_PARAM,
                    cyng::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
                    cyng::make_obis(0x81, 0x49, 0x63, 0x3C, 0x03, idx)),
                cyng::make_object(srv.scrambled_),
                "scrambled login");

            insert_config_record(
                stmt_insert,
                stmt_select,
                cyng::to_path(cfg::sep, OBIS_ROOT_IPT_PARAM, cyng::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx), "sk"),
                cyng::make_object(ipt::to_string(srv.sk_)),
                "scramble key");

            idx++;
        }
    }

    void read_ipt_params(cyng::db::statement_ptr stmt_insert, cyng::db::statement_ptr stmt_select, cyng::param_map_t &&pmap) {
        // std::cout << "IP-T parameter: " << pmap << std::endl;

        // cyng::make_param("enabled", true)
        insert_config_record(
            stmt_insert,
            stmt_select,
            cyng::to_path(cfg::sep, OBIS_ROOT_IPT_PARAM, "enabled"),
            cyng::make_object(cyng::find_value(pmap, std::string("enabled"), true)),
            "IP-T enabled");

        //  81 48 27 32 06 01 - minutes
        auto const tcp_wait_to_reconnect =
            cyng::numeric_cast<std::uint32_t>(cyng::find(pmap, cyng::to_string(OBIS_TCP_WAIT_TO_RECONNECT)), 1u);
        insert_config_record(
            stmt_insert,
            stmt_select,
            cyng::to_path(cfg::sep, OBIS_ROOT_IPT_PARAM, OBIS_TCP_WAIT_TO_RECONNECT),
            cyng::make_object(std::chrono::seconds(tcp_wait_to_reconnect)),
            "reconnect timer (seconds)");

        // 81 48 31 32 02 01
        auto const obis_tcp_connect_retries =
            cyng::numeric_cast<std::uint32_t>(cyng::find(pmap, cyng::to_string(OBIS_TCP_CONNECT_RETRIES)), 20u);
        insert_config_record(
            stmt_insert,
            stmt_select,
            cyng::to_path(cfg::sep, OBIS_ROOT_IPT_PARAM, OBIS_TCP_CONNECT_RETRIES),
            cyng::make_object(obis_tcp_connect_retries),
            "reconnect counter");

        // 00 80 80 00 03 01 - has SSL configuration
        auto const obis_has_ssl_config = cyng::value_cast(cyng::find(pmap, cyng::to_string(OBIS_HAS_SSL_CONFIG)), false);
        insert_config_record(
            stmt_insert,
            stmt_select,
            cyng::to_path(cfg::sep, OBIS_ROOT_IPT_PARAM, OBIS_HAS_SSL_CONFIG),
            cyng::make_object(obis_has_ssl_config),
            "SSL support");
    }

    void read_hardware(cyng::db::statement_ptr stmt_insert, cyng::db::statement_ptr stmt_select, cyng::param_map_t &&pmap) {

        for (auto const &param : pmap) {
            if (boost::algorithm::equals(param.first, "serial")) {
                auto const serial_number = cyng::numeric_cast<std::uint32_t>(param.second, 10000000u);
                insert_config_record(
                    stmt_insert,
                    stmt_select,
                    cyng::to_path(cfg::sep, "hardware", param.first),
                    cyng::make_object(serial_number),
                    "serial number " + std::to_string(serial_number));
            } else if (boost::algorithm::equals(param.first, "adapter")) {
                read_hardware_adapter(stmt_insert, stmt_select, cyng::container_cast<cyng::param_map_t>(param.second));
            } else {

                insert_config_record(
                    stmt_insert,
                    stmt_select,
                    cyng::to_path(cfg::sep, "hardware", param.first),
                    param.second,
                    "hardware: " + param.first);
            }
        }
    }

    void read_hardware_adapter(cyng::db::statement_ptr stmt_insert, cyng::db::statement_ptr stmt_select, cyng::param_map_t &&pmap) {
        for (auto const &param : pmap) {
            insert_config_record(
                stmt_insert,
                stmt_select,
                cyng::to_path(cfg::sep, "hw", "adapter", param.first),
                param.second,
                "adapter: " + param.first);
        }
    }

    void read_sml(cyng::db::statement_ptr stmt_insert, cyng::db::statement_ptr stmt_select, cyng::param_map_t &&pmap) {

        auto flag_default_profile = false;

        for (auto const &param : pmap) {
            if (boost::algorithm::equals(param.first, "address")) {

                boost::system::error_code ec;
                auto const address = boost::asio::ip::make_address(cyng::value_cast(param.second, "0.0.0.0"), ec);
                insert_config_record(
                    stmt_insert,
                    stmt_select,
                    cyng::to_path(cfg::sep, "sml", param.first),
                    cyng::make_object(address),
                    "SML bind address");

            } else if (boost::algorithm::equals(param.first, "port")) {

                auto const sml_port = cyng::numeric_cast<std::uint16_t>(param.second, 7259);
                insert_config_record(
                    stmt_insert,
                    stmt_select,
                    cyng::to_path(cfg::sep, "sml", param.first),
                    cyng::make_object(sml_port),
                    "default SML listener port (" + std::to_string(sml_port) + ")");
            } else if (boost::algorithm::equals(param.first, "discover")) {

                auto const sml_discover = cyng::numeric_cast<std::uint16_t>(param.second, 5798);
                insert_config_record(
                    stmt_insert,
                    stmt_select,
                    cyng::to_path(cfg::sep, "sml", param.first),
                    cyng::make_object(sml_discover),
                    "SML discovery port (" + std::to_string(sml_discover) + ")");
            } else if (boost::algorithm::equals(param.first, "default.profile")) {
                auto const code = cyng::numeric_cast<std::uint64_t>(param.second, CODE_PROFILE_15_MINUTE);
                auto const profile = cyng::make_obis(code);
                if (sml::is_profile(profile)) {

                    insert_config_record(
                        stmt_insert,
                        stmt_select,
                        cyng::to_path(cfg::sep, "sml", param.first),
                        cyng::make_object(profile),
                        "default profile for auto-config mode (" + obis::get_name(profile) + ")");
                    flag_default_profile = true;
                }
            } else {
                insert_config_record(
                    stmt_insert, stmt_select, cyng::to_path(cfg::sep, "sml", param.first), param.second, "SML: " + param.first);
            }
        }

        if (!flag_default_profile) {
            //
            //  profile not supported or missing
            //
            insert_config_record(
                stmt_insert,
                stmt_select,
                cyng::to_path(cfg::sep, "sml", "default.profile"),
                cyng::make_object(OBIS_PROFILE_15_MINUTE),
                "use \"15 minutes\" as default profile");
        }
    }

    void read_nms(cyng::db::statement_ptr stmt_insert, cyng::db::statement_ptr stmt_select, cyng::param_map_t &&pmap) {

        //
        //  This flag is used to check if one of the NICs is selected
        //  for the communication over a local-link address.
        //
        bool flag_nic = false;
        bool flag_nic_ipv4 = false;
        bool flag_nic_linklocal = false;
        bool flag_nic_index = false;

        auto const nic = get_nic();
        auto const ipv4_addr = get_ipv4_address(nic);
        auto const link_local = get_ipv6_linklocal(nic);

        for (auto const &param : pmap) {

            auto const key = sanitize_key(param.first);

            if (boost::algorithm::equals(key, "address")) {

                auto const address = cyng::value_cast(param.second, "0.0.0.0");
                insert_config_record(
                    stmt_insert,
                    stmt_select,
                    cyng::to_path(cfg::sep, "nms", key),
                    cyng::make_object(address),
                    "default NMS bind address");

            } else if (boost::algorithm::equals(key, "nic.ipv4")) {
                auto const s = cyng::value_cast(param.second, ipv4_addr.to_string());

                boost::system::error_code ec;
                auto const address = boost::asio::ip::make_address(s, ec);
                if (!ec) {
                    insert_config_record(
                        stmt_insert,
                        stmt_select,
                        cyng::to_path(cfg::sep, "nms", key),
                        cyng::make_object(address),
                        "IPv4 address of NMS adapter");
                } else {
                    insert_config_record(
                        stmt_insert,
                        stmt_select,
                        cyng::to_path(cfg::sep, "nms", key),
                        cyng::make_object(boost::asio::ip::make_address("169.254.0.1", ec)),
                        "IPv4 address of NMS adapter");
                }
                flag_nic_ipv4 = true;
            } else if (boost::algorithm::equals(key, "nic.linklocal")) {
                auto const s = cyng::value_cast(param.second, "fe80::");
                boost::system::error_code ec;
                auto const address = boost::asio::ip::make_address(s, ec);
                if (!ec) {
                    insert_config_record(
                        stmt_insert,
                        stmt_select,
                        cyng::to_path(cfg::sep, "nms", key),
                        cyng::make_object(address),
                        "link local address of NMS adapter");
                } else {
                    insert_config_record(
                        stmt_insert,
                        stmt_select,
                        cyng::to_path(cfg::sep, "nms", key),
                        cyng::make_object(boost::asio::ip::make_address("[fe80::]", ec)),
                        "link local address of NMS adapter");
                }
                flag_nic_linklocal = true;
            } else if (boost::algorithm::equals(key, "port")) {

                auto const nms_port = cyng::numeric_cast<std::uint16_t>(param.second, 7261);
                insert_config_record(
                    stmt_insert,
                    stmt_select,
                    cyng::to_path(cfg::sep, "nms", key),
                    cyng::make_object(nms_port),
                    "default NMS listener port (" + std::to_string(nms_port) + ")");
            } else if (boost::algorithm::equals(key, "script.path")) {

                std::filesystem::path const script_path = cyng::find_value(pmap, key, std::string(""));
                insert_config_record(
                    stmt_insert,
                    stmt_select,
                    cyng::to_path(cfg::sep, "nms", key),
                    cyng::make_object(script_path),
                    "path to update script");
            } else if (boost::algorithm::equals(key, "delay")) {
                auto const inp = cyng::value_cast(param.second, "00:00:12.000000");
                auto const delay = cyng::to_seconds(inp);
                insert_config_record(
                    stmt_insert,
                    stmt_select,
                    cyng::to_path(cfg::sep, "nms", key),
                    cyng::make_object(delay),
                    "default bind delay " + inp);

            } else if (boost::algorithm::equals(key, "nic.index")) {
                flag_nic_index = true;
                auto const nic_index = cyng::numeric_cast<std::uint32_t>(param.second, 0u);
                insert_config_record(
                    stmt_insert,
                    stmt_select,
                    cyng::to_path(cfg::sep, "nms", key),
                    cyng::make_object(nic_index),
                    "default NMS nic index (" + std::to_string(nic_index) + ")");

            } else {

                insert_config_record(stmt_insert, stmt_select, cyng::to_path(cfg::sep, "nms", key), param.second, "NMS: " + key);
                if (boost::algorithm::equals(key, "nic")) {
                    flag_nic = true;
                }
            }
        }

        if (!flag_nic) {
            //
            //  set a default NIC to communicate over a link-local address
            //
            insert_config_record(
                stmt_insert,
                stmt_select,
                cyng::to_path(cfg::sep, "nms", "nic"),
                cyng::make_object(nic),
                "designated nic for communication over a local-link address");
        }
        if (!flag_nic_ipv4) {
            insert_config_record(
                stmt_insert,
                stmt_select,
                cyng::to_path(cfg::sep, "nms", "nic.ipv4"),
                cyng::make_object(ipv4_addr),
                "IPv4 address of NMS adapter");
        }
        if (!flag_nic_linklocal) {
            insert_config_record(
                stmt_insert,
                stmt_select,
                cyng::to_path(cfg::sep, "nms", "nic.linklocal"),
                cyng::make_object(link_local.first),
                "link local address of NMS adapter");
        }
        if (!flag_nic_index) {
            insert_config_record(
                stmt_insert,
                stmt_select,
                cyng::to_path(cfg::sep, "nms", "nic.index"),
                cyng::make_object(link_local.second), //  u32
                "device index of NMS adapter");
        }
    }

    void read_lnm(cyng::db::statement_ptr stmt_insert, cyng::db::statement_ptr stmt_select, cyng::vector_t &&vec) {
        std::size_t counter{0};
        for (auto const &cfg : vec) {
            //
            //	%(
            //	("HCI":CP210x),
            //	("blocklist":%(("enabled":true),("list":[00684279,12345678]),("mode":drop),("period":30))),
            //	("broker":[%(("address":segw.ch),("port":12001))]),
            //	("broker.enabled":false),
            //	("broker.login":false),
            //	("databits":8),
            //	("enabled":false),
            //	("flow.control":none),
            //	("parity":none),
            //	("port":COM3),
            //	("speed":57600),
            //	("stopbits":one),
            //	("type":wireless))
            //
            // std::cout << "LMN: " << cfg << std::endl;

            auto const pmap = cyng::container_cast<cyng::param_map_t>(cfg);
            for (auto const &param : pmap) {

                auto const key = sanitize_key(param.first);

                if (boost::algorithm::equals(key, "databits")) {

                    auto const databits = cyng::numeric_cast<std::uint8_t>(cyng::find(cfg, key), 8);
                    insert_config_record(
                        stmt_insert,
                        stmt_select,
                        cyng::to_path(cfg::sep, "lmn", std::to_string(counter), key),
                        cyng::make_object(databits),
                        "databits (7, 8)");
                } else if (boost::algorithm::equals(key, "speed")) {

                    auto const speed = cyng::numeric_cast<std::uint32_t>(cyng::find(cfg, key), 2400);
                    insert_config_record(
                        stmt_insert,
                        stmt_select,
                        cyng::to_path(cfg::sep, "lmn", std::to_string(counter), key),
                        cyng::make_object(speed),
                        "speed in bauds (" + std::to_string(speed) + ")");
                } else if (boost::algorithm::equals(key, "broker.timeout")) {
                    auto const reconnect = cyng::numeric_cast<std::uint32_t>(cyng::find(cfg, key), 12);
                    insert_config_record(
                        stmt_insert,
                        stmt_select,
                        cyng::to_path(cfg::sep, "lmn", std::to_string(counter), key),
                        cyng::make_object(std::chrono::seconds(reconnect)),
                        " deprecated");
                } else if (boost::algorithm::equals(key, "broker")) {

                    //
                    //	multiple broker possible
                    //
                    read_broker(stmt_insert, stmt_select, counter, cyng::container_cast<cyng::vector_t>(param.second));
                } else if (boost::algorithm::equals(key, "blocklist")) {

                    //	enabled, mode, period, list[]
                    read_blocklist(stmt_insert, stmt_select, counter, cyng::container_cast<cyng::param_map_t>(param.second));

                } else if (boost::algorithm::equals(key, "listener")) {

                    //
                    //	One listener allowed
                    //
                    read_listener(stmt_insert, stmt_select, counter, cyng::container_cast<cyng::param_map_t>(param.second));

                } else if (boost::algorithm::equals(key, "cache")) {

                    //
                    //	readout cache
                    //
                    read_cache(stmt_insert, stmt_select, counter, cyng::container_cast<cyng::param_map_t>(param.second));

                } else if (boost::algorithm::equals(key, "virtual.meter")) {
                    //
                    //  virtual meter
                    //
                    read_virtual_meter(stmt_insert, stmt_select, counter, cyng::container_cast<cyng::param_map_t>(param.second));
                } else {
                    insert_config_record(
                        stmt_insert,
                        stmt_select,
                        cyng::to_path(cfg::sep, "lmn", std::to_string(counter), key),
                        cyng::find(cfg, key),
                        key);
                }
            }

            //
            //	next LMN
            //
            counter++;
        }

        insert_config_record(
            stmt_insert, stmt_select, cyng::to_path(cfg::sep, "lmn", "count"), cyng::make_object(counter), "LMN count");
    }

    void read_blocklist(
        cyng::db::statement_ptr stmt_insert,
        cyng::db::statement_ptr stmt_select,
        std::size_t counter,
        cyng::param_map_t &&pmap) {

        //
        //  reference to parent node
        //
        insert_config_record(
            stmt_insert, stmt_select, cyng::to_path(cfg::sep, "blocklist", "parent"), cyng::make_object("lmn"), "parent node");

        for (auto const &param : pmap) {
            if (boost::algorithm::equals(param.first, "list")) {

                auto const list = cyng::vector_cast<std::string>(param.second, "");
                std::size_t idx{0};
                for (auto const &meter : list) {
                    insert_config_record(
                        stmt_insert,
                        stmt_select,
                        cyng::to_path(cfg::sep, "blocklist", std::to_string(counter), "meter", idx),
                        cyng::make_object(meter),
                        "meter: " + meter);
                    idx++;
                }

                BOOST_ASSERT(idx == list.size());
                insert_config_record(
                    stmt_insert,
                    stmt_select,
                    cyng::to_path(cfg::sep, "blocklist", std::to_string(counter), "size"),
                    cyng::make_object(idx),
                    "blocklist size");

            } else if (boost::algorithm::equals(param.first, "period")) {
                //	"max-readout-frequency"
                auto const period = std::chrono::seconds(cyng::numeric_cast<std::uint32_t>(param.second, 30));
                insert_config_record(
                    stmt_insert,
                    stmt_select,
                    cyng::to_path(cfg::sep, "blocklist", std::to_string(counter), param.first),
                    cyng::make_object(period),
                    "blocklist: " + param.first);
            } else if (boost::algorithm::equals(param.first, "mode")) {
                auto mode = cyng::value_cast(param.second, "drop");
                std::transform(mode.begin(), mode.end(), mode.begin(), ::tolower);
                insert_config_record(
                    stmt_insert,
                    stmt_select,
                    cyng::to_path(cfg::sep, "blocklist", std::to_string(counter), param.first),
                    cyng::make_object(mode),
                    "mode: " + mode);
            } else {
                insert_config_record(
                    stmt_insert,
                    stmt_select,
                    cyng::to_path(cfg::sep, "blocklist", std::to_string(counter), param.first),
                    param.second,
                    "blocklist: " + param.first);
            }
        }
    }

    void read_listener(
        cyng::db::statement_ptr stmt_insert,
        cyng::db::statement_ptr stmt_select,
        std::size_t counter,
        cyng::param_map_t &&pmap) {

        //
        //  reference to parent node
        //
        insert_config_record(
            stmt_insert, stmt_select, cyng::to_path(cfg::sep, "listener", "parent"), cyng::make_object("lmn"), "parent node");

        for (auto const &listener : pmap) {
            if (boost::algorithm::equals(listener.first, "port")) {

                auto const listener_port = cyng::numeric_cast<std::uint16_t>(listener.second, 6006);
                insert_config_record(
                    stmt_insert,
                    stmt_select,
                    cyng::to_path(cfg::sep, "listener", std::to_string(counter), listener.first),
                    cyng::make_object(listener_port),
                    "default listener port " + std::to_string(listener_port));
            } else if (boost::algorithm::equals(listener.first, "address")) {
                boost::system::error_code ec;
                auto const address = boost::asio::ip::make_address(cyng::value_cast(listener.second, "0.0.0.0"), ec);
                insert_config_record(
                    stmt_insert,
                    stmt_select,
                    cyng::to_path(cfg::sep, "listener", std::to_string(counter), listener.first),
                    cyng::make_object(address),
                    "default listener bind address " + address.to_string());
            } else if (boost::algorithm::equals(listener.first, "delay")) {
                auto const inp = cyng::value_cast(listener.second, "00:00:12.000000");
                auto const delay = cyng::to_seconds(inp);
                insert_config_record(
                    stmt_insert,
                    stmt_select,
                    cyng::to_path(cfg::sep, "listener", std::to_string(counter), listener.first),
                    cyng::make_object(delay),
                    "default startup delay " + inp);

            } else if (boost::algorithm::equals(listener.first, "timeout")) {
                auto const inp = cyng::value_cast(listener.second, "00:00:10.000000");
                auto const timeout = cyng::to_seconds(inp);
                insert_config_record(
                    stmt_insert,
                    stmt_select,
                    cyng::to_path(cfg::sep, "listener", std::to_string(counter), listener.first),
                    cyng::make_object(timeout),
                    "default maximum idle time " + inp);

            } else {
                insert_config_record(
                    stmt_insert,
                    stmt_select,
                    cyng::to_path(cfg::sep, "listener", std::to_string(counter), listener.first),
                    listener.second,
                    "listener: " + listener.first);
            }
        }

        //
        //  reintroduced in v0.9.2.12
        //
        auto pos = pmap.find("timeout");
        if (pos == pmap.end()) {
            insert_config_record(
                stmt_insert,
                stmt_select,
                cyng::to_path(cfg::sep, "listener", std::to_string(counter), "timeout"),
                cyng::make_object(std::chrono::seconds(10)),
                "default maximum idle time is 10 seconds");
        }
    }

    void read_cache(
        cyng::db::statement_ptr stmt_insert,
        cyng::db::statement_ptr stmt_select,
        std::size_t counter,
        cyng::param_map_t &&pmap) {

        //
        //  reference to parent node
        //
        insert_config_record(
            stmt_insert, stmt_select, cyng::to_path(cfg::sep, "cache", "parent"), cyng::make_object("lmn"), "parent node");

        for (auto const &param : pmap) {

            auto const key = sanitize_key(param.first);

            if (boost::algorithm::equals(key, "period.minutes")) {
                auto const inp = cyng::value_cast(param.second, "00:30:00.000000");
                auto const period = cyng::to_minutes(inp);
                insert_config_record(
                    stmt_insert,
                    stmt_select,
                    cyng::to_path(cfg::sep, "cache", std::to_string(counter), key),
                    cyng::make_object(period),
                    "default period is 60 minutes");
            } else {
                insert_config_record(
                    stmt_insert,
                    stmt_select,
                    cyng::to_path(cfg::sep, "cache", std::to_string(counter), key),
                    param.second,
                    "cache: " + key);
            }
        }
    }

    void read_virtual_meter(
        cyng::db::statement_ptr stmt_insert,
        cyng::db::statement_ptr stmt_select,
        std::size_t counter,
        cyng::param_map_t &&pmap) {
        for (auto const &param : pmap) {
            //"enabled": false,
            //"server": "01-d81c-10000001-3c-02",
            //"protocol": "wM-Bus-EN13757-4",
            //"interval": 00:00:07.000000

            auto const key = sanitize_key(param.first);

            if (boost::algorithm::equals(key, "server")) {
                // cyng::hex_to_buffer(id);
                auto const srv = cyng::value_cast(param.second, "");
                auto const id = to_srv_id(srv);
                switch (detect_server_type(id)) {
                case srv_type::MBUS_WIRED: // M-Bus (long)
                    insert_config_record(
                        stmt_insert,
                        stmt_select,
                        cyng::to_path(cfg::sep, "virtual.meter", std::to_string(counter), "type"),
                        cyng::make_object("MBUS_WIRED"),
                        "server type is M-Bus (long)");
                    break;
                case srv_type::MBUS_RADIO: // M-Bus (long)
                    insert_config_record(
                        stmt_insert,
                        stmt_select,
                        cyng::to_path(cfg::sep, "virtual.meter", std::to_string(counter), "type"),
                        cyng::make_object("MBUS_RADIO"),
                        "server type is M-Bus (long)");
                    break;
                case srv_type::W_MBUS: // wireless M-Bus (short)
                    insert_config_record(
                        stmt_insert,
                        stmt_select,
                        cyng::to_path(cfg::sep, "virtual.meter", std::to_string(counter), "type"),
                        cyng::make_object("W_MBUS"),
                        "server type is wireless M-Bus (short)");
                    break;
                case srv_type::SERIAL:
                    insert_config_record(
                        stmt_insert,
                        stmt_select,
                        cyng::to_path(cfg::sep, "virtual.meter", std::to_string(counter), "type"),
                        cyng::make_object("SERIAL"),
                        "server type is serial");
                    break;
                default:
                    // insert_config_record(
                    //     stmt_insert, stmt_select,
                    //     cyng::to_path(cfg::sep, "virtual-meter", std::to_string(counter), "type"),
                    //     cyng::make_object("other"),
                    //     "server type is not supported");
                    break;
                }
                //
                //  "virtual-meter/N/server" as data type cyng::buffer
                //
                insert_config_record(
                    stmt_insert,
                    stmt_select,
                    cyng::to_path(cfg::sep, "virtual.meter", std::to_string(counter), key),
                    cyng::make_object(id),
                    "virtual meter server id: " + srv);

            } else if (boost::algorithm::equals(key, "interval")) {
                auto const inp = cyng::value_cast(param.second, "00:00:32.000000");
                auto const interval = cyng::to_seconds(inp);
                insert_config_record(
                    stmt_insert,
                    stmt_select,
                    cyng::to_path(cfg::sep, "virtual.meter", std::to_string(counter), key),
                    cyng::make_object(interval),
                    "push interval " + inp);
            } else {
                insert_config_record(
                    stmt_insert,
                    stmt_select,
                    cyng::to_path(cfg::sep, "virtual.meter", std::to_string(counter), key),
                    param.second,
                    "virtual meter: " + key);
            }
        }
    }

    void read_broker(
        cyng::db::statement_ptr stmt_insert,
        cyng::db::statement_ptr stmt_select,
        std::size_t counter,
        cyng::vector_t &&vec) {

        //
        //  reference to parent
        //
        insert_config_record(
            stmt_insert, stmt_select, cyng::to_path(cfg::sep, "broker", "parent"), cyng::make_object("lmn"), "parent node");

        std::size_t broker_index{0};
        for (auto const &obj : vec) {

            auto const pmap = cyng::container_cast<cyng::param_map_t>(obj);

            for (auto const &param : pmap) {

                auto const key = sanitize_key(param.first);

                if (boost::algorithm::equals(key, "port")) {

                    auto const broker_port =
                        cyng::numeric_cast<std::uint16_t>(param.second, static_cast<std::uint16_t>(12000 + broker_index));
                    insert_config_record(
                        stmt_insert,
                        stmt_select,
                        cyng::to_path(cfg::sep, "broker", std::to_string(counter), std::to_string(broker_index), key),
                        cyng::make_object(broker_port),
                        "broker port " + std::to_string(broker_port));
                } else if (boost::algorithm::equals(key, "write.timeout")) {
                    //  at least 1 second
                    auto const inp = cyng::value_cast(param.second, "00:00:02.000000");
                    auto const timeout = cyng::to_seconds(inp);
                    insert_config_record(
                        stmt_insert,
                        stmt_select,
                        cyng::to_path(cfg::sep, "broker", std::to_string(counter), std::to_string(broker_index), "write.timeout"),
                        cyng::make_object(timeout > std::chrono::seconds(1) ? timeout : std::chrono::seconds(2)),
                        "write timeout in seconds (for [connect-on-demand] only)");

                } else if (boost::algorithm::equals(key, "watchdog")) {
                    //  at least 5 seconds
                    auto const inp = cyng::value_cast(param.second, "00:00:12.000000");
                    auto const watchdog = cyng::to_seconds(inp);
                    insert_config_record(
                        stmt_insert,
                        stmt_select,
                        cyng::to_path(cfg::sep, "broker", std::to_string(counter), std::to_string(broker_index), "watchdog"),
                        cyng::make_object(watchdog > std::chrono::seconds(5) ? watchdog : std::chrono::seconds(5)),
                        "watchdog in seconds (for [connect-on-start] only)");

                } else {
                    insert_config_record(
                        stmt_insert,
                        stmt_select,
                        cyng::to_path(cfg::sep, "broker", std::to_string(counter), std::to_string(broker_index), key),
                        param.second,
                        "broker: " + key);
                }
            }

            //
            //  new element since v0.9.1.29
            //
            auto pos = pmap.find("connect.on.demand");
            if (pos == pmap.end()) {
                insert_config_record(
                    stmt_insert,
                    stmt_select,
                    cyng::to_path(cfg::sep, "broker", std::to_string(counter), std::to_string(broker_index), "connect.on.demand"),
                    cyng::make_object(true),
                    "connect on demand = true, otherwise connect at start");
            }
            //
            //  new element since v0.9.2.8
            //
            pos = pmap.find("write.timeout");
            if (pos == pmap.end()) {
                insert_config_record(
                    stmt_insert,
                    stmt_select,
                    cyng::to_path(cfg::sep, "broker", std::to_string(counter), std::to_string(broker_index), "write.timeout"),
                    cyng::make_object(std::chrono::seconds(1)),
                    "write timeout in seconds (for [connect-on-demand] only)");
            }
            //
            //  new element since v0.9.2.8
            //
            pos = pmap.find("watchdog");
            if (pos == pmap.end()) {
                insert_config_record(
                    stmt_insert,
                    stmt_select,
                    cyng::to_path(cfg::sep, "broker", std::to_string(counter), std::to_string(broker_index), "watchdog"),
                    cyng::make_object(std::chrono::seconds(12)),
                    "watchdog in seconds (for [connect-on-start] only)");
            }

            //
            //  next broker index
            //
            broker_index++;
        }

        insert_config_record(
            stmt_insert,
            stmt_select,
            cyng::to_path(cfg::sep, "broker", std::to_string(counter), "count"),
            cyng::make_object(broker_index),
            "broker count");
    }

    void read_gpio(cyng::db::statement_ptr stmt_insert, cyng::db::statement_ptr stmt_select, cyng::param_map_t &&pmap) {
        // std::cout << "GPIO: " << pmap << std::endl;

        insert_config_record(
            stmt_insert,
            stmt_select,
            cyng::to_path(cfg::sep, "gpio", "enabled"),
            cyng::find(pmap, std::string("enabled")),
            "GPIO enabled");

        //
        //	treat as filesystem path
        //
        std::filesystem::path p = cyng::find_value(pmap, "path", "/sys/class/gpio");
        insert_config_record(
            stmt_insert, stmt_select, cyng::to_path(cfg::sep, "gpio", "path"), cyng::make_object(std::move(p)), "virtual path");

        auto const vec = cyng::vector_cast<std::int64_t>(cyng::find(pmap, "list"), 0);
        std::size_t counter{0};
        for (auto pin : vec) {

            insert_config_record(
                stmt_insert,
                stmt_select,
                cyng::to_path(cfg::sep, "gpio", "pin", std::to_string(++counter)),
                cyng::make_object(pin),
                "GPIO pin");
        }
    }

    void write_json_config(cyng::db::session &db, cyng::object &&cfg, std::filesystem::path file_name, cyng::param_map_t const &s) {
        //
        //  generate new file name from old one
        //  segw_vx.y.json
        auto stem = std::filesystem::path("_" + file_name.stem().string() + file_name.extension().string());
        file_name.replace_filename(stem);
        std::error_code ec;
        if (std::filesystem::exists(file_name, ec)) {
            std::cerr << "**error: output file [" << file_name.string() << "] already exists" << std::endl;

        } else {
            std::ofstream ofs(file_name.string(), std::ios::trunc);
            if (ofs.is_open()) {
                write_json_config(db, ofs, s);
            } else {
                std::cerr << "**error: cannot open [" << file_name.string() << "]" << std::endl;
            }
        }
    }

    void write_json_config(cyng::db::session &db, std::ofstream &ofs, cyng::param_map_t const &s) {

        //
        //  collect data and transform it
        //
        auto cfg = transform_config(read_config(db, s));
        auto const vec = cyng::make_vector({cfg});

        //
        //  write to file
        //
        cyng::io::serialize_json_pretty(ofs, vec);
    }

    cyng::param_map_t read_config(cyng::db::session &db, cyng::param_map_t const &s) {
        BOOST_ASSERT(db.is_alive());

        auto const now = std::chrono::system_clock::now();
        cyng::param_map_t cfg = cyng::param_map_factory("generated", now)("version", SMF_VERSION_TAG)("DB", s);

        auto const ms = get_table_cfg();
        auto const sql = cyng::sql::select(db.get_dialect(), ms).all(ms, false).from().order_by("path")();
        auto stmt = db.create_statement();
        stmt->prepare(sql);

        while (auto res = stmt->get_result()) {
            auto const rec = cyng::to_record(ms, res);
            // std::cout << rec.to_tuple() << std::endl;
            auto const path = rec.value("path", "");
            auto const val = rec.value("value", "");
            auto const def = rec.value("def", "");
            auto const type = rec.value("type", static_cast<std::uint16_t>(0));
            auto const desc = rec.value("desc", "");
            try {
                auto const obj = cyng::restore(val, type);
                auto const vec = cyng::split(path, "/");
                std::cout << path << ": " << val << std::endl;
                if (!vec.empty()) {
                    if (boost::algorithm::equals(path, cyng::to_string(OBIS_SERVER_ID))) {
                        //  move this entry into the "net/" path
                        insert_value({"net", path}, &cfg, obj);
                    } else if (boost::algorithm::equals("count", vec.back())) {
                        //  skip
                        //} else if (boost::algorithm::starts_with(path, "gpio/pin")) {
                        //    insert_gpio_pin(vec, &cfg, obj);
                    } else {
                        insert_value(vec, &cfg, obj);
                    }
                }

            } catch (std::exception const &ex) {
                std::cout << path << ": ***error " << ex.what();
            }

            //
            //	complete
            //
        }

        return cfg;
    }

    void insert_value(std::vector<std::string> const &vec, cyng::param_map_t *p, cyng::object obj) {
        BOOST_ASSERT(!vec.empty());

        //
        //  start with root
        //
        for (auto pos = vec.begin(); pos != vec.end(); ++pos) {

            //  Detect type of key
            //  To distinguis from numbers at general it must be of length 1.
            //  This is a good appoximation but could lead to errors in case of
            //  unusual key names
            // auto const is_number = (pos->size() == 1) && cyng::is_dec_number(*pos);

            //  prepare key
            auto const key = sanitize_key(*pos);

            //  lookup for existing entry
            auto it = p->find(key);
            if (it != p->end()) {
                //  exists
                if (pos + 1 != vec.end()) {
                    p = const_cast<cyng::param_map_t *>(cyng::object_cast<cyng::param_map_t>(it->second));
                    BOOST_ASSERT(p != nullptr);
                } else {
                    //  eos - error
                    //  overwrite
#ifdef _DEBUG
                    std::cerr << "***error: " << key << " already exists" << std::endl;
#endif
                }
            } else {
                //  create new entry
                if (pos + 1 != vec.end()) {
                    auto r = p->emplace(sanitize_key(key), cyng::param_map_factory()());
                    p = const_cast<cyng::param_map_t *>(cyng::object_cast<cyng::param_map_t>((*r.first).second));
                    BOOST_ASSERT(p != nullptr);
                    BOOST_ASSERT(p->empty());
                } else {
                    //  eos - insert value
                    p->emplace(sanitize_key(key), obj);
                }
            }
        }
    }

    cyng::param_map_t transform_config(cyng::param_map_t &&cfg) {

        cyng::io::serialize_json_pretty(std::cout, cfg);
        cyng::transform(
            cfg, [=](cyng::object const &obj, std::vector<std::string> const &path) -> std::pair<cyng::object, cyng::action> {
                if (path.size() == 2 && boost::algorithm::equals(path.at(0), "gpio") &&
                    boost::algorithm::equals(path.at(1), "pin")) {
                    //
                    //  collect pin numbers:
                    //
                    //
                    auto vec = transform_pin_numbers(cyng::container_cast<cyng::param_map_t>(obj));
                    return {cyng::make_object(vec), cyng::action::REPLACE};
                }
                return {obj, cyng::action::NONE};
            });
        cyng::io::serialize_json_pretty(std::cout, cfg);
        return cfg;
    }

    cyng::vector_t transform_pin_numbers(cyng::param_map_t &&pmap) {
        //  "pin": {
        //  "1": 46,
        //  "2": 47,
        //  "3": 50,
        //  "4": 53
        //}
        cyng::vector_t vec;
        std::transform(
            pmap.begin(), pmap.end(), std::back_inserter(vec), [](cyng::param_map_t::value_type const &val) { return val.second; });
        return vec;
    }

} // namespace smf
