
#include <smf/mbus/field_definitions.h>

namespace smf {
    namespace mbus {
        const char *field_name(ci_field ci) {
            switch (ci) {
            case FIELD_CI_RESET: //!<		Reset(EN 13757-3)
                return "RESET";
            case FIELD_CI_DATA_SEND: //!<	Data send - mode 1 (EN 13757-3)
                return "DATA_SEND";
            case FIELD_CI_SLAVE_SELECT: //!<	Slave select - mode 1 (EN 13757-3)
                return "SLAVE_SELECT";
            case FIELD_CI_APPLICATION_RESET: //!<	Application reset or select (EN 13757-3).
                return "APPLICATION_RESET";
            case FIELD_CI_SYNC: //!< syncronize action
                return "SYNC";
            case FIELD_CI_CMD_TO_DEVICE_SHORT: //!<	CMD to device with short header (OMS Vol.2 Issue 2.0.0/2009-07-20)
                return "CMD_TO_DEVICE_SHORT";
            case FIELD_CI_CMD_TO_DEVICE_LONG: //!<	CMD to device with long header (OMS Vol.2 Issue 2.0.0/2009-07-20)
                return "CMD_TO_DEVICE_LONG";
            case FIELD_CI_CMD_TLS_HANDSHAKE: //!<	Security Management (TLS-Handshake)
                return "CMD_TLS_HANDSHAKE";
            case FIELD_CI_CMD_DLMS_LONG: return "CMD_DLMS_LONG";
            case FIELD_CI_CMD_DLMS_SHORT: return "CMD_DLMS_SHORT";
            case FIELD_CI_CMD_SML_LONG: return "CMD_SML_LONG";
            case FIELD_CI_CMD_SML_SHORT: return "CMD_SML_SHORT";
            case FIELD_CI_TIME_SYNC_1: //!< Time synchronization (OMS Vol.2 Issue 3.0.0xxxx-xx-xx)
                return "TIME_SYNC_1";
            case FIELD_CI_TIME_SYNC_2: //!<	Time synchronization (OMS Vol.2 Issue 3.0.0xxxx-xx-xx)
                return "TIME_SYNC_2";
            case FIELD_CI_APL_ERROR_SHORT: //!<	Error from device with short header (OMS Vol.2 Issue 3.0.0xxxx-xx-xx)
                return "APL_ALARM_SHORT";
            case FIELD_CI_APL_ERROR_LONG: //!<	Error from device with long header (OMS Vol.2 Issue 3.0.0xxxx-xx-xx)
                return "APL_ALARM_LONG";
            case FIELD_CI_APL_ERROR: //!<		Report of general application errors (EN 13757-3)
                return "APL_ALARM";
            case FIELD_CI_ALARM: //!<		report of alarms (EN 13757-3)
                return "ALARM";
            case FIELD_CI_HEADER_LONG: //!<	12 byte header followed by variable format data (EN 13757-3)
                return "HEADER_LONG";
            case FIELD_CI_APL_ALARM_SHORT: //!	<Alarm from device with short header (OMS Vol.2 Issue 3.0.0xxxx-xx-xx)
                return "APL_ALARM_SHORT";
            case FIELD_CI_APL_ALARM_LONG: //!<	Alarm from device with long header (OMS Vol.2 Issue 3.0.0xxxx-xx-xx)
                return "APL_ALARM_LONG";
            case FIELD_CI_HEADER_NO: //!<	Variable data format respond without header (EN 13757-3) - no encryption
                return "HEADER_NO";
            case FIELD_CI_HEADER_SHORT: //!<	4 byte header followed by variable data format respond (EN 13757-3)
                return "HEADER_SHORT";
            case FIELD_CI_RES_LONG_DLMS: return "RES_LONG_DLMS";
            case FIELD_CI_RES_SHORT_DLSM: //!<	short header
                return "RES_SHORT_DLMS";
            case FIELD_CI_RES_LONG_SML: return "RES_LONG_SML";
            case FIELD_CI_RES_SHORT_SML: return "RES_SHORT_SML"; //!<	short header
            case FIELD_CI_LINK_TO_DEVICE_LONG:    //!<	Link extension to device with long header (OMS Vol.2 Issue 2.0.02009-07-20)
            case FIELD_CI_LINK_FROM_DEVICE_SHORT: //!<	Link extension from device with short header
            case FIELD_CI_LINK_FROM_DEVICE_LONG:  //!<	Link extension from device with long header(OMS Vol.2
                                                  //!< Issue 2.0.0/2009-07-20)

                //#if !OMS_V4_ENABLED

            case FIELD_CI_EXT_DLL_I:  //!<	Additional Link Layer may be applied for Radio messages with or without Application
                                      //!< Layer.
            case FIELD_CI_EXT_DLL_II: //!<	Additional Link Layer may be applied for Radio messages with or without Application
                                      //!< Layer (ELL)
                                      //#endif // OMS_V4_ENABLED

            case FIELD_CI_EXT_DLL_III: //!<	Lower Layer Service (10 Byte)

            case FIELD_CI_AUTH_LAYER: //!<	Authentication and Fragmentation Layer (Lower Layer Service)
                break;

            case FIELD_CI_RES_TLS_SHORT: //!<	Security Management (TLS-Handshake)
                return "RES_TLS_SHORT";
            case FIELD_CI_RES_TLS_LONG: //!<	Security Management (TLS-Handshake)
                return "RES_TLS_LONG";

            case FIELD_CI_MANU_SPEC:     //!<	Manufacture specific CI-Field
            case FIELD_CI_MANU_NO:       //!<	Manufacture specific CI-Field with no header
            case FIELD_CI_MANU_SHORT:    //!<	Manufacture specific CI-Field with short header
            case FIELD_CI_MANU_LONG:     //!<	Manufacture specific CI-Field with long header
            case FIELD_CI_MANU_SHORT_RF: //!<	Manufacture specific CI-Field with short header (for RF-Test)
                break;
            case FIELD_CU_BAUDRATE_300: return "BAUDRATE_300";
            case FIELD_CU_BAUDRATE_600: return "BAUDRATE_600";
            case FIELD_CU_BAUDRATE_1200: return "BAUDRATE_1200";
            case FIELD_CU_BAUDRATE_2400: return "BAUDRATE_2400";
            case FIELD_CU_BAUDRATE_4800: return "BAUDRATE_4800";
            case FIELD_CU_BAUDRATE_9600: return "BAUDRATE_9600";
            case FIELD_CU_BAUDRATE_19200: return "BAUDRATE_19200";
            case FIELD_CU_BAUDRATE_38400: return "BAUDRATE_38400";

            case FIELD_CI_NULL: //!<	No CI-field transmitted.
                return "NULL";
            default: break;
            }
            return "unknown ci_field";
        }

    } // namespace mbus
} // namespace smf
