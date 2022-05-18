/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <storage.h>

#include <smf.h>
#include <smf/obis/db.h>
#include <smf/obis/defs.h>
#include <smf/obis/tree.hpp>
#include <smf/sml/generator.h>
#include <smf/sml/value.hpp>
#include <sml/response_engine.h>

#include <config/cfg_hardware.h>
#include <config/cfg_ipt.h>

#include <boost/predef.h>

#include <cyng/io/io_buffer.h>
#include <cyng/io/ostream.h>
#include <cyng/io/serialize.h>
#include <cyng/log/record.h>
#include <cyng/sys/info.h>
#include <cyng/sys/ntp.h>
#if defined(BOOST_OS_WINDOWS_AVAILABLE)
#include <cyng/scm/mgr.h>
#endif
#include <cyng/obj/algorithm/swap_bytes.h>

namespace smf {

    response_engine::response_engine(cyng::logger logger, cfg &config, storage &s, sml::response_generator &rg)
        : logger_(logger)
        , cfg_(config)
        , storage_(s)
        , res_gen_(rg) {}

    void response_engine::generate_get_proc_parameter_response(
        sml::messages_t &msgs,
        std::string trx,
        cyng::buffer_t server,
        std::string user,
        std::string pwd,
        cyng::obis_path_t path,
        cyng::object attr) {

        if (!path.empty()) {
            CYNG_LOG_INFO(
                logger_,
                "SML_GetProcParameter.Req - trx: " << trx << ", server: " << cyng::io::to_hex(server) << " ("
                                                   << obis::get_name(path.front()) << "), user: " << user << ", pwd: " << pwd
                                                   << ", path: " << path << ", attr: " << attr);

            //
            //  each login is followed by y query of CODE_CLASS_OP_LOG_STATUS_WORD and CODE_ROOT_DEVICE_IDENT
            //
            switch (path.front().to_uint64()) {
            case CODE_CLASS_OP_LOG_STATUS_WORD: msgs.push_back(get_proc_parameter_status_word(trx, server, path)); break;
            case CODE_ROOT_DEVICE_IDENT: msgs.push_back(get_proc_parameter_device_ident(trx, server, path)); break;
            case CODE_ROOT_DEVICE_TIME: //	0x8181C78810FF
                msgs.push_back(get_proc_parameter_device_time(trx, server, path));
                break;
            case CODE_ROOT_NTP: //	0x8181C78801FF
                msgs.push_back(get_proc_parameter_ntp(trx, server, path));
                break;
            case CODE_ROOT_ACCESS_RIGHTS: //	81 81 81 60 FF FF
                msgs.push_back(get_proc_parameter_access_rights(trx, server, path));
                break;
            case CODE_ROOT_CUSTOM_INTERFACE: //	81 02 00 07 00 FF
                msgs.push_back(get_proc_parameter_custom_interface(trx, server, path));
                break;
            case CODE_ROOT_CUSTOM_PARAM: //	0x8102000710FF
                // msgs.push_back(get_proc_parameter_custom_param(trx, server, path));
                break;
            // case CODE_ROOT_NMS:
            //    break;
            case CODE_ROOT_WAN: //	0x8104000610FF
                // msgs.push_back(get_proc_parameter_wan(trx, server, path));
                break;
            case CODE_ROOT_GSM: //	0x8104020700FF
                // msgs.push_back(get_proc_parameter_gsm(trx, server, path));
                break;
            case CODE_ROOT_GPRS_PARAM: //	0x81040D0700FF
                // msgs.push_back(get_proc_parameter_gprs(trx, server, path));
                break;
            case CODE_ROOT_IPT_STATE: //	0x81490D0600FF
                msgs.push_back(get_proc_parameter_ipt_state(trx, server, path));
                break;
            case CODE_ROOT_IPT_PARAM: //	0x81490D0700FF
                msgs.push_back(get_proc_parameter_ipt_param(trx, server, path));
                break;
            case CODE_ROOT_W_MBUS_STATUS: //	0x81060F0600FF
                msgs.push_back(get_proc_parameter_wmbus_state(trx, server, path));
                break;
            case CODE_ROOT_W_MBUS_PARAM: //	0x8106190700FF
                msgs.push_back(get_proc_parameter_wmbus_param(trx, server, path));
                break;
            case CODE_ROOT_LAN_DSL: //	0x81480D0600FF
                msgs.push_back(get_proc_parameter_lan_dsl_state(trx, server, path));
                break;
            case CODE_IF_LAN_DSL: //	0x8148170700FF
                msgs.push_back(get_proc_parameter_lan_dsl_param(trx, server, path));
                break;
            case CODE_ROOT_MEMORY_USAGE: //	0x0080800010FF
                msgs.push_back(get_proc_parameter_memory(trx, server, path));
                break;
            case CODE_ROOT_VISIBLE_DEVICES: //	81 81 10 06 FF FF
                msgs.push_back(get_proc_parameter_visisble_devices(trx, server, path));
                break;
            case CODE_ROOT_ACTIVE_DEVICES: //	81 81 11 06 FF FF
                msgs.push_back(get_proc_parameter_active_devices(trx, server, path));
                break;
            case CODE_ROOT_DEVICE_INFO: //	81 81 12 06 FF FF
                msgs.push_back(get_proc_parameter_device_info(trx, server, path));
                break;
            case CODE_ROOT_SENSOR_PARAMS: //	81 81 C7 86 00 FF
                // msgs.push_back(get_proc_parameter_sensor_param(trx, server, path));
                break;
            case CODE_ROOT_DATA_COLLECTOR: //	 0x8181C78620FF (Datenspiegel)
                msgs.push_back(get_proc_parameter_data_collector(trx, server, path));
                break;
            case CODE_IF_1107: //	0x8181C79300FF
                msgs.push_back(get_proc_parameter_1107_interface(trx, server, path));
                break;
            case CODE_STORAGE_TIME_SHIFT: //	0x0080800000FF
                msgs.push_back(get_proc_parameter_storage_timeshift(trx, server, path));
                break;
            case CODE_ROOT_PUSH_OPERATIONS: //	0x8181C78A01FF
                // msgs.push_back(get_proc_parameter_push_ops(trx, server, path));
                break;
            case CODE_LIST_SERVICES: //	0x990000000004
                msgs.push_back(get_proc_parameter_list_services(trx, server, path));
                break;
            case CODE_ACTUATORS: //	0x0080801100FF
                //  not longer supported
                msgs.push_back(get_proc_parameter_actuators(trx, server, path));
                break;
            case CODE_IF_EDL: //	0x81050D0700FF - M-Bus EDL (RJ10)
                // msgs.push_back(get_proc_parameter_edl(trx, server, path));
                break;
            case CODE_CLASS_MBUS: //	0x00B000020000
                // msgs.push_back(get_proc_parameter_mbus(trx, server, path));
                break;
            case CODE_ROOT_SECURITY: //	00 80 80 01 00 FF
                // msgs.push_back(get_proc_parameter_security(trx, server, path));
                break;
            // case CODE_ROOT_BROKER:
            //    break;
            // case CODE_ROOT_SERIAL:
            //    break;
            default: CYNG_LOG_WARNING(logger_, "SML_GetProcParameter.Req unknown OBIS code: " << cyng::io::to_hex(server)); break;
            }
        } else {
            CYNG_LOG_WARNING(
                logger_,
                "SML_GetProcParameter.Req has no path - trx: " << trx << ", server: " << cyng::io::to_hex(server)
                                                               << ", user: " << user << ", pwd: " << pwd);
        }
    }

    void response_engine::generate_set_proc_parameter_response(
        sml::messages_t &msgs,
        std::string trx,
        cyng::buffer_t server,
        std::string user,
        std::string pwd,
        cyng::obis_path_t path,
        cyng::obis code,
        cyng::attr_t attr,
        cyng::tuple_t child_list) {
        if (!path.empty()) {
            BOOST_ASSERT(path.back() == code);
            CYNG_LOG_INFO(
                logger_,
                "SML_SetProcParameter.Req - trx: " << trx << ", server: " << cyng::io::to_hex(server) << ", user: " << user
                                                   << ", pwd: " << pwd << ", path: " << path << " (" << obis::get_name(path.front())
                                                   << ")");

            //
            //  SML_SetProcParameter requests are generating an attention code as response
            //
            switch (path.front().to_uint64()) {
            case CODE_ROOT_IPT_PARAM: //	0x81490D0700FF
                msgs.push_back(set_proc_parameter_ipt_param(trx, server, path, attr));
                break;
            default: CYNG_LOG_WARNING(logger_, "SML_SetProcParameter.Req unknown OBIS code: " << cyng::io::to_hex(server)); break;
            }
        } else {
            CYNG_LOG_WARNING(
                logger_,
                "SML_SetProcParameter.Req has no path - trx: " << trx << ", server: " << cyng::io::to_hex(server)
                                                               << ", user: " << user << ", pwd: " << pwd);
        }
    }

    void response_engine::generate_get_profile_list_response(
        sml::messages_t &msgs,
        std::string trx,
        cyng::buffer_t server,                       // server id
        std::string user,                            // username
        std::string pwd,                             // password
        bool,                                        // has raw data
        std::chrono::system_clock::time_point begin, // begin
        std::chrono::system_clock::time_point end,   // end
        cyng::obis_path_t path,                      // parameter tree path
        cyng::object,                                // object list
        cyng::object                                 // das details
    ) {
        if (!path.empty()) {
            CYNG_LOG_INFO(
                logger_,
                "SML_GetProfileList.Req - trx: " << trx << ", server: " << cyng::io::to_hex(server) << ", user: " << user
                                                 << ", pwd: " << pwd << ", path: " << path << " (" << obis::get_name(path.front())
                                                 << ")");
            switch (path.front().to_uint64()) {
            case CODE_CLASS_OP_LOG: //	81 81 c7 89 e1 ff
                get_profile_list_response_oplog(msgs, trx, server, path, begin, end);
                break;
            case CODE_PROFILE_1_MINUTE:     //	 81 81 C7 86 10 FF
            case CODE_PROFILE_15_MINUTE:    // 81 81 C7 86 11 FF
                                            // profile_15_minute(trx, client_id, srv_id, start, end);
            case CODE_PROFILE_60_MINUTE:    // 81 81 C7 86 12 FF
            case CODE_PROFILE_24_HOUR:      // 81 81 C7 86 13 FF
            case CODE_PROFILE_LAST_2_HOURS: // 81 81 C7 86 14 FF
            case CODE_PROFILE_LAST_WEEK:    // 81 81 C7 86 15 FF
            case CODE_PROFILE_1_MONTH:      // 81 81 C7 86 16 FF
            case CODE_PROFILE_1_YEAR:       // 81 81 C7 86 17 FF
            case CODE_PROFILE_INITIAL:      // 81 81 C7 86 18 FF
                // return get_profile(code, trx, client_id, srv_id, start, end);
                break;
            default: CYNG_LOG_WARNING(logger_, "SML_GetProfileList.Req unknown OBIS code: " << cyng::io::to_hex(server)); break;
            }

        } else {
            CYNG_LOG_WARNING(
                logger_,
                "SML_GetProfileList.Req has no path - trx: " << trx << ", server: " << cyng::io::to_hex(server)
                                                             << ", user: " << user << ", pwd: " << pwd);
        }
    }

    cyng::tuple_t response_engine::get_proc_parameter_status_word(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path) {

        BOOST_ASSERT(!path.empty());
        return res_gen_.get_proc_parameter(
            trx, server, path, sml::tree_param(path.front(), sml::make_value(cfg_.get_status_word())));
    }

    cyng::tuple_t response_engine::get_proc_parameter_device_ident(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path) {

        // auto const srv_id = cfg_.get_srv_id();

        cfg_hardware const hw(cfg_);

        BOOST_ASSERT(!path.empty());
        //  contains the following elements as child list:
        //  81 81 C7 82 02 FF - DEVICE_CLASS
        //  81 81 C7 82 03 FF - DATA_MANUFACTURER
        //  81 81 C7 82 04 FF - SERVER_ID
        //  81 81 C7 82 06 FF - ROOT_FIRMWARE - list of firmware types and versions
        //  81 81 C7 82 09 FF - HARDWARE_FEATURES
        return res_gen_.get_proc_parameter(
            trx,
            server,
            path,
            sml::tree_child_list(
                path.at(0),
                {// obis code that descrives the device class
                 sml::tree_param(OBIS_DEVICE_CLASS, sml::make_value(hw.get_device_class())),
                 //  manufacturer
                 sml::tree_param(OBIS_DATA_MANUFACTURER, sml::make_value(hw.get_manufacturer())),
                 //  server id (05 + MAC)
                 sml::tree_param(OBIS_SERVER_ID, sml::make_value(cfg_.get_srv_id())),

                 //	firmware
                 sml::tree_child_list(
                     OBIS_ROOT_FIRMWARE,
                     {//	section 1
                      sml::tree_child_list(
                          cyng::make_obis(0x81, 0x81, 0xc7, 0x82, 0x07, 0x01),
                          {sml::tree_param(OBIS_DEVICE_KERNEL, sml::make_value("CURRENT_VERSION")),
                           sml::tree_param(OBIS_VERSION, sml::make_value(SMF_VERSION_TAG)),
                           sml::tree_param(OBIS_DEVICE_ACTIVATED, sml::make_value(true))}),
                      //	section 2
                      sml::tree_child_list(
                          cyng::make_obis(0x81, 0x81, 0xc7, 0x82, 0x07, 0x02),
                          {sml::tree_param(OBIS_DEVICE_KERNEL, sml::make_value("KERNEL")),
                           sml::tree_param(OBIS_VERSION, sml::make_value(cyng::sys::get_os_name())),
                           sml::tree_param(OBIS_DEVICE_ACTIVATED, sml::make_value(true))})}),

                 //	hardware
                 sml::tree_child_list(
                     OBIS_HARDWARE_FEATURES,
                     {
                         sml::tree_param(
                             cyng::make_obis(0x81, 0x81, 0xc7, 0x82, 0x0a, 0x01),
                             sml::make_value("VSES-1KW-221-1F0")), //  model code
                                                                   // sml::make_value("VMET-1KW-221-1F0)),
                         sml::tree_param(
                             cyng::make_obis(0x81, 0x81, 0xc7, 0x82, 0x0a, 0x02),
                             sml::make_value(hw.get_serial_number())) //  serial number
                     })}));
    }

    cyng::tuple_t response_engine::get_proc_parameter_device_time(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path) {

        //  81 81 c7 88 10 ff
        BOOST_ASSERT(!path.empty());
        BOOST_ASSERT(path.at(0) == OBIS_ROOT_DEVICE_TIME);

        auto const now = std::chrono::system_clock::now();

        return res_gen_.get_proc_parameter(
            trx,
            server,
            path,
            sml::tree_child_list(
                path.at(0),
                {sml::tree_child_list(
                    path.at(0),
                    // OBIS_ROOT_DEVICE_TIME,
                    {
                        sml::tree_param(OBIS_CURRENT_UTC, sml::make_value(now)),
                        sml::tree_param(cyng::make_obis(0x00, 0x00, 0x60, 0x08, 0x00, 0xFF), sml::make_value(0u)),  //  second index
                        sml::tree_param(cyng::make_obis(0x81, 0x00, 0x00, 0x09, 0x0B, 0x01), sml::make_value(0u)),  //  timezone
                        sml::tree_param(cyng::make_obis(0x81, 0x00, 0x00, 0x09, 0x0B, 0x02), sml::make_value(true)) //  sync active
                    })}));
    }

    cyng::tuple_t
    response_engine::get_proc_parameter_ntp(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path) {

        //  81 81 c7 88 01 ff
        BOOST_ASSERT(!path.empty());
        BOOST_ASSERT(path.at(0) == OBIS_ROOT_NTP);

        std::uint16_t const ntp_port = 123; // 123 is default
        bool const ntp_active = true;
        std::uint16_t const ntp_tz = 3600;

        //
        //	get all configured NTP servers
        //
        auto const ntp_servers = cyng::sys::get_ntp_servers();

        return res_gen_.get_proc_parameter(
            trx,
            server,
            path,
            sml::tree_child_list(
                path.at(0),
                {sml::tree_param(OBIS_NTP_PORT, sml::make_value(ntp_port)),
                 sml::tree_param(OBIS_NTP_ACTIVE, sml::make_value(ntp_active)), //  second index
                 sml::tree_param(OBIS_NTP_TZ, sml::make_value(ntp_tz)),         //  timezone
                 sml::tree_child_list(OBIS_NTP_SERVER, generate_tree_ntp(ntp_servers))}));
    }

    cyng::tuple_t response_engine::get_proc_parameter_access_rights(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path) {
        // 81 81 81 60 FF FF
        BOOST_ASSERT(!path.empty());
        BOOST_ASSERT(path.at(0) == OBIS_ROOT_ACCESS_RIGHTS);

        return res_gen_.get_proc_parameter(
            trx, server, path, sml::tree_child_list(path.at(0), {sml::tree_child_list(path.at(0), {})}));
    }

    cyng::tuple_t response_engine::get_proc_parameter_custom_interface(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path) {
        //  81 02 00 07 00 FF
        BOOST_ASSERT(!path.empty());
        BOOST_ASSERT(path.at(0) == OBIS_ROOT_CUSTOM_INTERFACE);

        return res_gen_.get_proc_parameter(
            trx, server, path, sml::tree_child_list(path.at(0), {sml::tree_child_list(path.at(0), {})}));
    }

    cyng::tuple_t response_engine::get_proc_parameter_custom_param(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path) {
        return cyng::tuple_t{};
    }

    cyng::tuple_t
    response_engine::get_proc_parameter_wan(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path) {
        return cyng::tuple_t{};
    }

    cyng::tuple_t
    response_engine::get_proc_parameter_gsm(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path) {
        return cyng::tuple_t{};
    }

    cyng::tuple_t
    response_engine::get_proc_parameter_gprs(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path) {
        return cyng::tuple_t{};
    }

    cyng::tuple_t response_engine::get_proc_parameter_ipt_state(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path) {

        //  81 49 0D 06 00 FF
        BOOST_ASSERT(!path.empty());
        BOOST_ASSERT(path.at(0) == OBIS_ROOT_IPT_STATE);

        cfg_ipt const cfg(cfg_);

        auto const lep = cfg.get_local_ep();
        auto const rep = cfg.get_remote_ep();

        //  network ordering required for IP address
        std::uint32_t const ip_address = cyng::swap_bytes(rep.address().to_v4().to_uint());
        std::uint16_t const target_port = rep.port();
        std::uint16_t const source_port = lep.port();

        return res_gen_.get_proc_parameter(
            trx,
            server,
            path,
            sml::tree_child_list(
                path.at(0),
                {sml::tree_param(OBIS_TARGET_IP_ADDRESS, sml::make_value(ip_address)),
                 sml::tree_param(OBIS_TARGET_PORT, sml::make_value(target_port)),
                 sml::tree_param(OBIS_SOURCE_PORT, sml::make_value(source_port))}));
    }

    cyng::tuple_t response_engine::get_proc_parameter_ipt_param(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path) {
        //  81 49 0D 07 00 FF
        BOOST_ASSERT(!path.empty());
        BOOST_ASSERT(path.at(0) == OBIS_ROOT_IPT_PARAM);

        cfg_ipt const cfg(cfg_);

        auto const cl = cfg.get_params_as_child_list();
        CYNG_LOG_DEBUG(logger_, cl);
        CYNG_LOG_TRACE(logger_, "child list - size " << cl.size() << ": " << cyng::io::to_typed(cl));
        return res_gen_.get_proc_parameter(trx, server, path, cl);
    }

    cyng::tuple_t response_engine::set_proc_parameter_ipt_param(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path,
        cyng::attr_t const &attr) {
        cfg_ipt cfg(cfg_);
        return (cfg.set_proc_parameter(path, attr.second))
                   ? res_gen_.get_attention(trx, server, sml::attention_type::OK, "ok")
                   : res_gen_.get_attention(trx, server, sml::attention_type::UNKNOWN_OBIS_CODE, "ip-t");
    }

    cyng::tuple_t response_engine::get_proc_parameter_wmbus_state(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path) {

        //  81 06 0F 06 00 FF
        BOOST_ASSERT(!path.empty());
        BOOST_ASSERT(path.at(0) == OBIS_ROOT_W_MBUS_STATUS);

        cfg_hardware const hw(cfg_);

        return res_gen_.get_proc_parameter(
            trx,
            server,
            path,
            sml::tree_child_list(
                path.at(0),
                {
                    sml::tree_param(OBIS_W_MBUS_ADAPTER_MANUFACTURER, sml::make_value(hw.get_adapter_manufacturer())), //  string
                    sml::tree_param(OBIS_W_MBUS_ADAPTER_ID, sml::make_value(hw.get_adapter_id())),                     //  buffer
                    sml::tree_param(OBIS_W_MBUS_FIRMWARE, sml::make_value(hw.get_adapter_firmware_version())),         //  string
                    sml::tree_param(OBIS_W_MBUS_HARDWARE, sml::make_value(hw.get_adapter_hardware_version()))          //  string
                }));
    }

    cyng::tuple_t response_engine::get_proc_parameter_wmbus_param(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path) {

        //  81 06 19 07 00 FF
        //   8106190701ff: 0
        //      8106190702ff: 1e (S2 mode)
        //      8106190703ff: 14 (T2 mode)
        //      810627320301: 00015180 (reboot)
        //      8106190704ff: 0 (power:  0 = default, 1 = low, 2 = medium, 3 = high)
        BOOST_ASSERT(!path.empty());
        BOOST_ASSERT(path.at(0) == OBIS_ROOT_W_MBUS_PARAM);
        return res_gen_.get_proc_parameter(
            trx,
            server,
            path,
            sml::tree_child_list(
                path.at(0),
                {
                    sml::tree_param(OBIS_W_MBUS_PROTOCOL, sml::make_value(0x02)),  //  protocol 0 = T, 1 = S, 2 = A, 3 = P
                    sml::tree_param(OBIS_W_MBUS_MODE_S, sml::make_value(0x1e)),    //  S2 mode
                    sml::tree_param(OBIS_W_MBUS_MODE_T, sml::make_value(0x14)),    //  T2 mode
                    sml::tree_param(OBIS_W_MBUS_REBOOT, sml::make_value(0x15180)), //  reboot
                    sml::tree_param(OBIS_W_MBUS_POWER, sml::make_value(1u)) //  power:  0 = default, 1 = low, 2 = medium, 3 = high
                }));
    }

    cyng::tuple_t response_engine::get_proc_parameter_lan_dsl_state(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path) {
        //  81 48 0D 06 00 FF
        BOOST_ASSERT(!path.empty());
        BOOST_ASSERT(path.at(0) == OBIS_ROOT_LAN_DSL);
        return res_gen_.get_proc_parameter(
            trx, server, path, sml::tree_child_list(path.at(0), {sml::tree_child_list(path.at(0), {})}));
    }

    cyng::tuple_t response_engine::get_proc_parameter_lan_dsl_param(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path) {
        //  81 48 17 07 00 FF
        BOOST_ASSERT(!path.empty());
        BOOST_ASSERT(path.at(0) == OBIS_IF_LAN_DSL);
        return res_gen_.get_proc_parameter(
            trx, server, path, sml::tree_child_list(path.at(0), {sml::tree_child_list(path.at(0), {})}));
    }

    cyng::tuple_t response_engine::get_proc_parameter_memory(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path) {

        //  00 80 80 00 10 FF
        BOOST_ASSERT(!path.empty());
        BOOST_ASSERT(path.at(0) == OBIS_ROOT_MEMORY_USAGE);

        return res_gen_.get_proc_parameter(
            trx,
            server,
            path,
            sml::tree_child_list(
                path.at(0),
                {
                    sml::tree_param(cyng::make_obis(0x00, 0x80, 0x80, 0x00, 0x11, 0xFF), sml::make_value(10u)), //  mirror
                    sml::tree_param(cyng::make_obis(0x00, 0x80, 0x80, 0x00, 0x12, 0xFF), sml::make_value(12u))  //  tmp
                }));
    }

    cyng::tuple_t response_engine::get_proc_parameter_visisble_devices(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path) {
        //  81 81 10 06 FF FF
        BOOST_ASSERT(!path.empty());
        BOOST_ASSERT(path.at(0) == OBIS_ROOT_VISIBLE_DEVICES);
        //
        //  * server ID
        //  * device class
        //  * timestamp (last seen)
        //

        cyng::tuple_t tpl_outer;
        cfg_.get_cache().access(
            [&](cyng::table const *tbl) {
                //
                //  convert data into tree child list
                //
                std::uint8_t q = 1, s = 1;
                cyng::buffer_t srv;
                auto const now = std::chrono::system_clock::now();
                cyng::tuple_t tpl_inner; //  current

                tbl->loop([&](cyng::record &&rec, std::size_t) -> bool {
                    //
                    //  use only visible metering devices
                    //
                    if (rec.value("visible", false)) {
                        srv = rec.value("serverID", srv);
                        tpl_inner.push_back(
                            cyng::make_object(generate_visible_device_entry(q, s, srv, rec.value("lastSeen", now))));

                        //
                        //  update counter and lists
                        //
                        if (s == 0xFE) {
                            BOOST_ASSERT_MSG(s != 0xFF, "visible meter overflow");
                            tpl_outer.push_back(cyng::make_object(
                                sml::tree_child_list(cyng::make_obis(0x81, 0x81, 0x10, 0x06, q, 0xff), tpl_inner)));
                            tpl_inner.clear();
                            s = 1;
                            q++;
                        } else {
                            s++;
                        }
                    }

                    return true;
                });
            },
            cyng::access::read("meterMBus"));

        return res_gen_.get_proc_parameter(trx, server, path, sml::tree_child_list(
            OBIS_ROOT_VISIBLE_DEVICES, //  81 81 10 06 ff ff
            tpl_outer));

    }

    cyng::tuple_t
    generate_active_device_entry(std::uint8_t q, std::uint8_t s, cyng::buffer_t srv, std::chrono::system_clock::time_point tp) {
        return sml::tree_child_list(
            cyng::make_obis(0x81, 0x81, 0x11, 0x06, q, s),
            cyng::make_tuple(
                sml::tree_param(
                    OBIS_SERVER_ID,
                    sml::make_attribute(srv)),                                                               // serverID
                sml::tree_param(OBIS_DEVICE_CLASS, sml::make_attribute(cyng::make_buffer({'-', '-', '-'}))), // class
                sml::tree_param(OBIS_CURRENT_UTC, sml::make_attribute(tp))                                   // lastSeen
                )                                                                                            // [1] device
        ); // record - 81 81 11 06 [q s]
    }

    cyng::tuple_t
    generate_visible_device_entry(std::uint8_t q, std::uint8_t s, cyng::buffer_t srv, std::chrono::system_clock::time_point tp) {
        return sml::tree_child_list(
            cyng::make_obis(0x81, 0x81, 0x10, 0x06, q, s),
            cyng::make_tuple(
                sml::tree_param(
                    OBIS_SERVER_ID,
                    sml::make_attribute(srv)),                                                               // serverID
                sml::tree_param(OBIS_DEVICE_CLASS, sml::make_attribute(cyng::make_buffer({'-', '-', '-'}))), // class
                sml::tree_param(OBIS_CURRENT_UTC, sml::make_attribute(tp))                                   // lastSeen
                )                                                                                            // [1] device
        ); // record - 81 81 10 06 [q s]
    }

    cyng::tuple_t response_engine::get_proc_parameter_active_devices(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path) {
        //  81 81 11 06 FF FF

        // server: 0500153b01ec46
        // path  : 81811106ffff
        // code  : 81811106ffff - ROOT_ACTIVE_DEVICES
        // attr  : (0:null)
        // index : 2
        // list  : 8181110601ff: null
        //           818111060101: null
        //             8181c78204ff: 0a000001
        //             8181c78202ff: ---
        //             010000090b00: 2022-03-23T11:49:31+0100
        //           818111060102: null
        //             8181c78204ff: 0a000002
        //             8181c78202ff: ---
        //            010000090b00: 2022-03-23T11:49:32+0100

        BOOST_ASSERT(!path.empty());
        BOOST_ASSERT(path.at(0) == OBIS_ROOT_ACTIVE_DEVICES); //  81 81 11 06 ff ff

        //
        //  read data from table "meter"
        cyng::tuple_t tpl_outer;
        cfg_.get_cache().access(
            [&](cyng::table const *tbl) {
                //
                //  convert data into tree child list
                //
                std::uint8_t q = 1, s = 1;
                cyng::buffer_t srv;
                auto const now = std::chrono::system_clock::now();
                cyng::tuple_t tpl_inner; //  current

                tbl->loop([&](cyng::record &&rec, std::size_t) -> bool {
                    srv = rec.value("serverID", srv);
                    tpl_inner.push_back(cyng::make_object(generate_active_device_entry(q, s, srv, rec.value("lastSeen", now))));

                    //
                    //  update counter and lists
                    //
                    if (s == 0xFE) {
                        BOOST_ASSERT_MSG(s != 0xFF, "active meter overflow");
                        tpl_outer.push_back(
                            cyng::make_object(sml::tree_child_list(cyng::make_obis(0x81, 0x81, 0x11, 0x06, q, 0xff), tpl_inner)));
                        tpl_inner.clear();
                        s = 1;
                        q++;
                    } else {
                        s++;
                    }

                    return true;
                });

                BOOST_ASSERT_MSG(s != 0xFF, "active meter overflow");
                tpl_outer.push_back(
                    cyng::make_object(sml::tree_child_list(cyng::make_obis(0x81, 0x81, 0x11, 0x06, q, 0xff), tpl_inner)));
                tpl_inner.clear();
            },
            cyng::access::read("meterMBus"));

        auto child_list = sml::tree_child_list(
            OBIS_ROOT_ACTIVE_DEVICES, //  81 81 11 06 ff ff
            tpl_outer);

        return res_gen_.get_proc_parameter(trx, server, path, child_list);

        //
        //  example to illustrate the structure
        //
#ifdef _SMF_DEMO_MODE
        auto child_list = sml::tree_child_list(
            OBIS_ROOT_ACTIVE_DEVICES, //  81 81 11 06 ff ff
            cyng::make_tuple(         //  outer tuple start
                sml::tree_child_list(
                    cyng::make_obis(0x81, 0x81, 0x11, 0x06, 1u, 0xff),
                    cyng::make_tuple( //  inner tuple start
                        generate_active_device_entry(
                            1u,
                            1u,
                            cyng::make_buffer({0x01, 0x24, 0x23, 0x96, 0x07, 0x20, 0x00, 0x63, 0x0e}),
                            std::chrono::system_clock::now() // [1] record - 81 81 11 06 [01 FF]
                            ),
                        generate_active_device_entry(
                            1u,
                            2u,
                            cyng::make_buffer({0x01, 0xE6, 0x1E, 0x57, 0x14, 0x06, 0x21, 0x36, 0x03}),
                            std::chrono::system_clock::now() // [2] record - 81 81 11 06 [02 FF]
                            ),
                        generate_active_device_entry(
                            1u,
                            3u,
                            cyng::make_buffer({0x01, 0xA8, 0x15, 0x74, 0x31, 0x45, 0x04, 0x01, 0x02}),
                            std::chrono::system_clock::now() // [3] record - 81 81 11 06 [03 FF]
                            )))))                            // 81 81 11 06 ff ff
            ;
        CYNG_LOG_DEBUG(logger_, "OBIS_ROOT_ACTIVE_DEVICES - meterMBus child list: " << child_list);

        return res_gen_.get_proc_parameter(trx, server, path, child_list);
#endif //_SMF_DEMO_MODE
    }

    cyng::tuple_t response_engine::get_proc_parameter_device_info(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path) {
        //  81 81 12 06 FF FF
        BOOST_ASSERT(!path.empty());
        BOOST_ASSERT(path.at(0) == OBIS_ROOT_DEVICE_INFO);
        return res_gen_.get_proc_parameter(
            trx, server, path, sml::tree_child_list(path.at(0), {sml::tree_child_list(path.at(0), {})}));
    }

    cyng::tuple_t response_engine::get_proc_parameter_sensor_param(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path) {
        return cyng::tuple_t{};
    }

    cyng::tuple_t response_engine::get_proc_parameter_data_collector(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path) {
        //  81 81 C7 86 20 FF (Datenspiegel)
        BOOST_ASSERT(!path.empty());
        BOOST_ASSERT(path.at(0) == OBIS_ROOT_DATA_COLLECTOR);
        return res_gen_.get_proc_parameter(
            trx, server, path, sml::tree_child_list(path.at(0), {sml::tree_child_list(path.at(0), {})}));
    }

    cyng::tuple_t response_engine::get_proc_parameter_1107_interface(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path) {
        //  81 81 C7 93 00 FF
        BOOST_ASSERT(!path.empty());
        BOOST_ASSERT(path.at(0) == OBIS_IF_1107);
        return res_gen_.get_proc_parameter(
            trx, server, path, sml::tree_child_list(path.at(0), {sml::tree_child_list(path.at(0), {})}));
    }

    cyng::tuple_t response_engine::get_proc_parameter_storage_timeshift(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path) {

        //  00 80 80 00 00 FF
        BOOST_ASSERT(!path.empty());
        BOOST_ASSERT(path.at(0) == OBIS_STORAGE_TIME_SHIFT);

        return res_gen_.get_proc_parameter(
            trx,
            server,
            path,
            sml::tree_child_list(
                path.at(0),
                {sml::tree_child_list(
                    path.at(0),
                    {
                        sml::tree_param(cyng::make_obis(0x00, 0x80, 0x80, 0x00, 0x01, 0xFF), sml::make_value(10u)) //  seconds
                    })}));
    }

    cyng::tuple_t response_engine::get_proc_parameter_push_ops(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path) {
        return cyng::tuple_t{};
    }

    cyng::tuple_t response_engine::get_proc_parameter_list_services(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path) {

#if defined(BOOST_OS_WINDOWS_AVAILABLE)
        scm::mgr m(false); //	no admin required
        auto r = m.get_active_services();
        std::stringstream ss;
        std::size_t counter{0};
        for (auto const &srv : r) {
            ss << srv.name_ << ':' << srv.display_name << ';';
            ++counter;
            //	problems with handling larger strings
            if (counter > 36)
                break;
        }

        auto data = ss.str();
        CYNG_LOG_INFO(logger_, "active services: " << data);

        return res_gen_.get_proc_parameter(trx, server, path, sml::tree_param(path.front(), sml::make_value(data)));

#else
        return res_gen_.get_proc_parameter(
            trx, server, path, sml::tree_param(path.front(), sml::make_value("smfService:smfConfiguration-001;")));
#endif
    }

    cyng::tuple_t response_engine::get_proc_parameter_actuators(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path) {

        //
        //   example:
        //   0080801101ff: null
        //   008080110101: null
        //   0080801110ff : 0ae00001
        //   0080801112ff : b
        //   0080801113ff : true
        //   0080801114ff : 0
        //   0080801115ff : 23
        //   0080801116ff : 45696e736368616c74656e204d616e75656c6c
        //
        return cyng::tuple_t{};
    }

    cyng::tuple_t
    response_engine::get_proc_parameter_edl(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path) {
        return cyng::tuple_t{};
    }

    cyng::tuple_t
    response_engine::get_proc_parameter_mbus(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path) {
        return cyng::tuple_t{};
    }

    cyng::tuple_t response_engine::get_proc_parameter_security(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path) {
        return cyng::tuple_t{};
    }

    void response_engine::get_profile_list_response_oplog(
        sml::messages_t &msgs,
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path,
        std::chrono::system_clock::time_point const &begin,
        std::chrono::system_clock::time_point const &end) {

        BOOST_ASSERT_MSG(begin < end, "invalid time range");

        CYNG_LOG_INFO(logger_, "op log from " << begin << " to " << end);

        //
        //  collect all log entries
        //
        //
        // std::uint8_t push_nr = 1;
        storage_.loop_oplog(begin, end, [&, this](cyng::record &&rec) {
            //  example:
            //  [ROWID: 000000000025883d][actTime: 1999-12-30T23:00:00+0100][age: 1999-12-30T23:00:00+0100][regPeriod:
            //  00022202][valTime: 1969-12-31T23:59:59+0100][status: 0000000000000000][event: 000007e6][peer: null][utc:
            //  1969-12-31T23:59:59+0100][serverId: 30][target: power return][pushNr: 0][details: null]
            CYNG_LOG_TRACE(logger_, "op record: " << rec.to_string());

            auto const now = std::chrono::system_clock::now();
            auto const age = rec.value("age", now);
            auto const act_time = rec.value("actTime", now);
            auto const val_time = rec.value<std::uint32_t>("valTime", now.time_since_epoch().count());
            auto const reg_period = rec.value<std::uint32_t>("regPeriod", 900u);
            auto const status = rec.value<std::uint64_t>("status", 0u);

            auto const event_class = rec.value<std::uint32_t>("event", 0u);
            auto const target = rec.value("target", "");
            auto const push_nr = rec.value<std::uint8_t>("pushNr", 0u);
            auto const details = rec.value("details", "");
            cyng::obis peer;
            peer = rec.value("peer", peer);

            msgs.push_back(res_gen_.get_profile_list(
                trx,
                server,
                age,
                reg_period,
                OBIS_CLASS_OP_LOG, //	path entry: 81 81 C7 89 E1 FF
                val_time,
                status,
                {//	81 81 C7 89 E2 FF - OBIS_CLASS_EVENT
                 //  62FF                                    unit: 255
                 //  5200                                    scaler: 0
                 //  64800008                                value: 8388616
                 sml::period_entry(OBIS_CLASS_EVENT, 0xFF, event_class),
                 //	81 81 00 00 00 FF - PEER_ADDRESS
                 //  62FF                                    unit: 255
                 //  5200                                    scaler: 0
                 //  078146000002FF                          value: 81 46 00 00 02 FF
                 sml::period_entry(OBIS_PEER_ADDRESS, 0xFF, peer),
                 //  0781042B070000                          objName: 81 04 2B 07 00 00
                 //  62FE                                    unit: 254
                 //  5200                                    scaler: 0
                 //  5500000000                              value: 0
                 sml::period_entry(OBIS_CLASS_OP_LOG_FIELD_STRENGTH, 0xFE, 0u),
                 //  81 04 1A 07 00 00                       objName: CLASS_OP_LOG_CELL
                 //  62FF                                    unit: 255
                 //  5200                                    scaler: 0
                 //  6200                                    value: 0
                 sml::period_entry(OBIS_CLASS_OP_LOG_CELL, 0xFF, 0u),
                 //  81 04 17 07 00 00                       objName:  CLASS_OP_LOG_AREA_CODE
                 //  62FF                                    unit: 255
                 //  5200                                    scaler: 0
                 //  6200                                    value: 0
                 sml::period_entry(OBIS_CLASS_OP_LOG_AREA_CODE, 0xFF, 0u),
                 //  81 04 0D 06 00 00                       objName: CLASS_OP_LOG_PROVIDER
                 //  62FF                                    unit: 255
                 //  5200                                    scaler: 0
                 //  6200                                    value: 0
                 sml::period_entry(OBIS_CLASS_OP_LOG_PROVIDER, 0xFF, 0u),
                 //  01 00 00 09 0B 00                       objName:  CURRENT_UTC
                 //  6207                                    unit: 7
                 //  5200                                    scaler: 0
                 //  655B8E3F04                              value: 1536048900
                 sml::period_entry(OBIS_CURRENT_UTC, 0xFF, age),
                 //  81 81 C7 82 04 FF                       objName:  SERVER_ID
                 //  62FF                                    unit: 255
                 //  5200                                    scaler: 0
                 //  0A01A815743145040102                    value: 01 A8 15 74 31 45 04 01 02
                 sml::period_entry(OBIS_SERVER_ID, 0xFF, server),
                 //  81 47 17 07 00 FF                       objName:  PUSH_TARGET
                 //  62FF                                    unit: 255
                 //  5200                                    scaler: 0
                 //  0F706F77657240736F6C6F73746563          value: power@solostec
                 sml::period_entry(OBIS_PUSH_TARGET, 0xFF, target),
                 // 81 81 C7 8A 01 FF                       objName:  ROOT_PUSH_OPERATIONS
                 // 62FF                                    unit: 255
                 // 5200                                    scaler: 0
                 // 6201                                    value: 1
                 sml::period_entry(OBIS_ROOT_PUSH_OPERATIONS, 0xFF, push_nr),
                 // 81 81 C7 81 23 FF
                 // 62FF                                    unit: 255
                 // 5200                                    scaler: 0
                 sml::period_entry(OBIS_DATA_PUSH_DETAILS, 0xFF, details)}));
            return true;
        });
    }

    cyng::tuple_t generate_tree_ntp(std::vector<std::string> const &ntp_servers) {
        cyng::tuple_t tpl;
        std::uint8_t nr = 0;
        std::transform(std::begin(ntp_servers), std::end(ntp_servers), std::back_inserter(tpl), [&nr](std::string const &name) {
            ++nr;
            return cyng::make_object(sml::tree_param(cyng::make_obis(0x81, 0x81, 0xC7, 0x88, 0x02, nr), sml::make_value(name)));
        });
        return tpl;
    }
} // namespace smf
