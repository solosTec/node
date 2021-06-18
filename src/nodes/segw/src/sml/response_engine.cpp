/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/obis/db.h>
#include <smf/obis/defs.h>
#include <sml/response_engine.h>

#include <cyng/io/io_buffer.h>
#include <cyng/io/ostream.h>
#include <cyng/log/record.h>

namespace smf {

    response_engine::response_engine(cfg &config, cyng::logger logger, sml::response_generator &rg)
        : cfg_(config)
        , logger_(logger)
        , res_gen_(rg) {}

    void response_engine::generate_get_proc_parameter_response(
        sml::messages_t &,
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
                break;
            case CODE_ROOT_DEVICE_IDENT:
                break;
            case CODE_ROOT_DEVICE_TIME: //	0x8181C78810FF
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

} // namespace smf
