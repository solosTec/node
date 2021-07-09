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
                break;
            case CODE_ROOT_ACCESS_RIGHTS: //	81 81 81 60 FF FF
                break;
            case CODE_ROOT_CUSTOM_INTERFACE: //	81 02 00 07 00 FF
                break;
            case CODE_ROOT_CUSTOM_PARAM: //	0x8102000710FF
                break;
            case CODE_ROOT_NMS:
                break;
            case CODE_ROOT_WAN: //	0x8104000610FF
                break;
            case CODE_ROOT_GSM: //	0x8104020700FF
                break;
            case CODE_ROOT_GPRS_PARAM: //	0x81040D0700FF
                break;
            case CODE_ROOT_IPT_STATE: //	0x81490D0600FF
                break;
            case CODE_ROOT_IPT_PARAM: //	0x81490D0700FF
                break;
            case CODE_ROOT_W_MBUS_STATUS: //	0x81060F0600FF
                break;
            case CODE_IF_wMBUS: //	0x8106190700FF
                break;
            case CODE_ROOT_LAN_DSL: //	0x81480D0600FF
                break;
            case CODE_IF_LAN_DSL: //	0x8148170700FF
                break;
            case CODE_ROOT_MEMORY_USAGE: //	0x0080800010FF
                break;
            case CODE_ROOT_VISIBLE_DEVICES: //	81 81 10 06 FF FF
                break;
            case CODE_ROOT_ACTIVE_DEVICES: //	81 81 11 06 FF FF
                break;
            case CODE_ROOT_DEVICE_INFO: //	81 81 12 06 FF FF
                break;
            case CODE_ROOT_SENSOR_PARAMS: //	81 81 C7 86 00 FF
                break;
            case CODE_ROOT_DATA_COLLECTOR: //	 0x8181C78620FF (Datenspiegel)
                break;
            case CODE_IF_1107: //	0x8181C79300FF
                break;
            case CODE_STORAGE_TIME_SHIFT: //	0x0080800000FF
                break;
            case CODE_ROOT_PUSH_OPERATIONS: //	0x8181C78A01FF
                break;
            case CODE_LIST_SERVICES: //	0x990000000004
                break;
            case CODE_ACTUATORS: //	0x0080801100FF
                break;
            case CODE_IF_EDL: //	0x81050D0700FF - M-Bus EDL (RJ10)
                break;
            case CODE_CLASS_MBUS: //	0x00B000020000
                break;
            case CODE_ROOT_SECURITY: //	00 80 80 01 00 FF
                break;
            case CODE_ROOT_BROKER:
                break;
            case CODE_ROOT_SERIAL:
                break;
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

        auto const now = std::chrono::system_clock::now();

        return res_gen_.get_proc_parameter(
            trx,
            server,
            path,
            sml::tree_child_list(
                path.at(0),
                {sml::tree_child_list(
                    OBIS_ROOT_DEVICE_TIME,
                    {
                        sml::tree_param(OBIS_CURRENT_UTC, sml::make_value(now)),
                        sml::tree_param(cyng::make_obis(0x00, 0x00, 0x60, 0x08, 0x00, 0xFF), sml::make_value(0u)),  //  second index
                        sml::tree_param(cyng::make_obis(0x81, 0x00, 0x00, 0x09, 0x0B, 0x01), sml::make_value(0u)),  //  timezone
                        sml::tree_param(cyng::make_obis(0x81, 0x00, 0x00, 0x09, 0x0B, 0x02), sml::make_value(true)) //  sync active
                    })}));
    }
} // namespace smf
