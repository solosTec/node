/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf.h>
#include <smf/obis/db.h>
#include <smf/obis/defs.h>
#include <smf/sml/generator.h>
#include <smf/sml/value.hpp>
#include <sml/response_engine.h>

#include <config/cfg_hardware.h>

#include <cyng/io/io_buffer.h>
#include <cyng/io/ostream.h>
#include <cyng/log/record.h>
#include <cyng/sys/info.h>
#include <cyng/sys/ntp.h>

namespace smf {

    response_engine::response_engine(cfg &config, cyng::logger logger, sml::response_generator &rg)
        : cfg_(config)
        , logger_(logger)
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
            case CODE_CLASS_OP_LOG_STATUS_WORD:
                msgs.push_back(get_proc_parameter_status_word(trx, server, path));
                break;
            case CODE_ROOT_DEVICE_IDENT:
                msgs.push_back(get_proc_parameter_device_ident(trx, server, path));
                break;
            case CODE_ROOT_DEVICE_TIME: //	0x8181C78810FF
                msgs.push_back(get_proc_parameter_device_time(trx, server, path));
                break;
            case CODE_ROOT_NTP: //	0x8181C78801FF
                msgs.push_back(get_proc_parameter_ntp(trx, server, path));
                break;
            case CODE_ROOT_ACCESS_RIGHTS: //	81 81 81 60 FF FF
                // msgs.push_back(get_proc_parameter_access_rights(trx, server, path));
                break;
            case CODE_ROOT_CUSTOM_INTERFACE: //	81 02 00 07 00 FF
                // msgs.push_back(get_proc_parameter_custom_interface(trx, server, path));
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
                // msgs.push_back(get_proc_parameter_ipt_state(trx, server, path));
                break;
            case CODE_ROOT_IPT_PARAM: //	0x81490D0700FF
                // msgs.push_back(get_proc_parameter_ipt_param(trx, server, path));
                break;
            case CODE_ROOT_W_MBUS_STATUS: //	0x81060F0600FF
                msgs.push_back(get_proc_parameter_wmbus_state(trx, server, path));
                break;
            case CODE_IF_wMBUS: //	0x8106190700FF
                // msgs.push_back(get_proc_parameter_wmbus_param(trx, server, path));
                break;
            case CODE_ROOT_LAN_DSL: //	0x81480D0600FF
                // msgs.push_back(get_proc_parameter_lan_dsl_state(trx, server, path));
                break;
            case CODE_IF_LAN_DSL: //	0x8148170700FF
                // msgs.push_back(get_proc_parameter_lan_dsl_param(trx, server, path));
                break;
            case CODE_ROOT_MEMORY_USAGE: //	0x0080800010FF
                // msgs.push_back(get_proc_parameter_memory(trx, server, path));
                break;
            case CODE_ROOT_VISIBLE_DEVICES: //	81 81 10 06 FF FF
                // msgs.push_back(get_proc_parameter_visisble_devices(trx, server, path));
                break;
            case CODE_ROOT_ACTIVE_DEVICES: //	81 81 11 06 FF FF
                // msgs.push_back(get_proc_parameter_active_devices(trx, server, path));
                break;
            case CODE_ROOT_DEVICE_INFO: //	81 81 12 06 FF FF
                // msgs.push_back(get_proc_parameter_device_info(trx, server, path));
                break;
            case CODE_ROOT_SENSOR_PARAMS: //	81 81 C7 86 00 FF
                // msgs.push_back(get_proc_parameter_sensor_param(trx, server, path));
                break;
            case CODE_ROOT_DATA_COLLECTOR: //	 0x8181C78620FF (Datenspiegel)
                // msgs.push_back(get_proc_parameter_data_collector(trx, server, path));
                break;
            case CODE_IF_1107: //	0x8181C79300FF
                // msgs.push_back(get_proc_parameter_1107_interface(trx, server, path));
                break;
            case CODE_STORAGE_TIME_SHIFT: //	0x0080800000FF
                // msgs.push_back(get_proc_parameter_storage_timeshift(trx, server, path));
                break;
            case CODE_ROOT_PUSH_OPERATIONS: //	0x8181C78A01FF
                // msgs.push_back(get_proc_parameter_push_ops(trx, server, path));
                break;
            case CODE_LIST_SERVICES: //	0x990000000004
                // msgs.push_back(get_proc_parameter_list_services(trx, server, path));
                break;
            case CODE_ACTUATORS: //	0x0080801100FF
                // msgs.push_back(get_proc_parameter_actuators(trx, server, path));
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
            default:
                CYNG_LOG_WARNING(logger_, "SML_GetProcParameter.Req unknown OBIS code: " << cyng::io::to_hex(server));
                break;
            }
        } else {
            CYNG_LOG_WARNING(
                logger_,
                "SML_GetProcParameter.Req has no path - trx: " << trx << ", server: " << cyng::io::to_hex(server)
                                                               << ", user: " << user << ", pwd: " << pwd);
        }
    }

    void response_engine::generate_set_proc_parameter_response(
        sml::messages_t &,
        std::string trx,
        cyng::buffer_t server,
        std::string user,
        std::string pwd,
        cyng::obis_path_t path) {
        if (!path.empty()) {
            CYNG_LOG_INFO(
                logger_,
                "SML_SetProcParameter.Req - trx: " << trx << ", server: " << cyng::io::to_hex(server) << ", user: " << user
                                                   << ", pwd: " << pwd << ", path: " << path << " ("
                                                   << obis::get_name(path.front()));
        } else {
            CYNG_LOG_WARNING(
                logger_,
                "SML_SetProcParameter.Req has no path - trx: " << trx << ", server: " << cyng::io::to_hex(server)
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
                    // path.at(0),
                    OBIS_ROOT_DEVICE_TIME,
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
                {sml::tree_child_list(
                    OBIS_ROOT_NTP,
                    {sml::tree_param(OBIS_CODE_NTP_PORT, sml::make_value(ntp_port)),
                     sml::tree_param(OBIS_CODE_NTP_ACTIVE, sml::make_value(ntp_active)), //  second index
                     sml::tree_param(OBIS_CODE_NTP_TZ, sml::make_value(ntp_tz)),         //  timezone
                     sml::tree_child_list(OBIS_ROOT_NTP, generate_tree_ntp(ntp_servers))})}));

        return cyng::tuple_t{};
    }

    cyng::tuple_t response_engine::get_proc_parameter_access_rights(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path) {
        return cyng::tuple_t{};
    }

    cyng::tuple_t response_engine::get_proc_parameter_custom_interface(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path) {
        return cyng::tuple_t{};
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
        return cyng::tuple_t{};
    }

    cyng::tuple_t response_engine::get_proc_parameter_ipt_param(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path) {
        return cyng::tuple_t{};
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
                {sml::tree_child_list(
                    // path.at(0),
                    OBIS_ROOT_W_MBUS_STATUS,
                    {
                        sml::tree_param(
                            OBIS_W_MBUS_ADAPTER_MANUFACTURER, sml::make_value(hw.get_adapter_manufacturer())),     //  string
                        sml::tree_param(OBIS_W_MBUS_ADAPTER_ID, sml::make_value(hw.get_adapter_id())),             //  buffer
                        sml::tree_param(OBIS_W_MBUS_FIRMWARE, sml::make_value(hw.get_adapter_firmware_version())), //  string
                        sml::tree_param(OBIS_W_MBUS_HARDWARE, sml::make_value(hw.get_adapter_hardware_version()))  //  string
                    })}));
    }

    cyng::tuple_t response_engine::get_proc_parameter_wmbus_param(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path) {
        return cyng::tuple_t{};
    }

    cyng::tuple_t response_engine::get_proc_parameter_lan_dsl_state(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path) {
        return cyng::tuple_t{};
    }

    cyng::tuple_t response_engine::get_proc_parameter_lan_dsl_param(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path) {
        return cyng::tuple_t{};
    }

    cyng::tuple_t response_engine::get_proc_parameter_memory(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path) {
        return cyng::tuple_t{};
    }

    cyng::tuple_t response_engine::get_proc_parameter_visisble_devices(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path) {
        return cyng::tuple_t{};
    }

    cyng::tuple_t response_engine::get_proc_parameter_active_devices(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path) {
        return cyng::tuple_t{};
    }

    cyng::tuple_t response_engine::get_proc_parameter_device_info(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path) {
        return cyng::tuple_t{};
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
        return cyng::tuple_t{};
    }

    cyng::tuple_t response_engine::get_proc_parameter_1107_interface(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path) {
        return cyng::tuple_t{};
    }

    cyng::tuple_t response_engine::get_proc_parameter_storage_timeshift(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path) {
        return cyng::tuple_t{};
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
        return cyng::tuple_t{};
    }

    cyng::tuple_t response_engine::get_proc_parameter_actuators(
        std::string const &trx,
        cyng::buffer_t const &server,
        cyng::obis_path_t const &path) {
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
