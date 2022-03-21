#include <smf/obis/defs.h>
#include <smf/sml/readout.h>
#include <smf/sml/value.hpp>

#include <cyng/io/io_buffer.h>
#include <cyng/io/serialize.h>
#include <cyng/obj/algorithm/swap_bytes.h>
#include <cyng/obj/container_cast.hpp>
#include <cyng/obj/container_factory.hpp>
#include <cyng/obj/util.hpp>

#ifdef _DEBUG_SML
#include <iostream>
#endif

#include <boost/assert.hpp>

namespace smf {
    namespace sml {

        cyng::object customize_value(cyng::obis code, cyng::object obj) {
            if (code.starts_with({0x81, 0x49, 0x17, 0x07, 0x00}) ||
                cyng::compare_n(code, OBIS_CUSTOM_IF_IP_CURRENT_1, 5) //	CUSTOM_IF_IP_ADDRESS...
                || OBIS_CUSTOM_IF_IP_MASK_1 == code || OBIS_CUSTOM_IF_IP_MASK_2 == code || OBIS_CUSTOM_IF_IP_ADDRESS_1 == code ||
                OBIS_CUSTOM_IF_IP_ADDRESS_2 == code || OBIS_CUSTOM_IF_DHCP_LOCAL_IP_MASK == code ||
                OBIS_CUSTOM_IF_DHCP_DEFAULT_GW == code || OBIS_CUSTOM_IF_DHCP_DNS == code ||
                OBIS_CUSTOM_IF_DHCP_START_ADDRESS == code || OBIS_CUSTOM_IF_DHCP_END_ADDRESS == code || OBIS_NMS_ADDRESS == code) {
                return cyng::make_object(ip_address_to_str(obj));
            } else if (
                cyng::compare_n(code, OBIS_TARGET_PORT, 5)    //	TARGET_PORT
                || cyng::compare_n(code, OBIS_SOURCE_PORT, 5) //	SOURCE_PORT
                || OBIS_BROKER_SERVICE == code || OBIS_NMS_PORT == code) {
                return cyng::make_object(cyng::numeric_cast<std::uint16_t>(obj, 0u));
            } else if (OBIS_W_MBUS_MODE_S == code || OBIS_W_MBUS_MODE_T == code) {
                return cyng::make_object(cyng::numeric_cast<std::uint8_t>(obj, 0u));
            } else if (OBIS_MBUS_STATE == code) {
                auto const state = cyng::numeric_cast<std::int32_t>(obj, 0);
                return cyng::make_object(state);
            } else if (OBIS_CLASS_EVENT == code) {
                auto const state = cyng::numeric_cast<std::uint32_t>(obj, 0);
                return cyng::make_object(state);
            } else if (
                code.starts_with({0x81, 0x49, 0x63, 0x3C, 0x01})      //	IP-T user name redundancy 1
                || code.starts_with({0x81, 0x49, 0x63, 0x3C, 0x02})   //	IP-T user name redundancy 2
                || cyng::compare_n(code, OBIS_NTP_SERVER, 5)          //	NTP server (81 81 c7 88 02 NN)
                || cyng::compare_n(code, OBIS_BROKER_SERVER, 5)       //	BROKER_SERVER
                || cyng::compare_n(code, OBIS_BROKER_USER, 5)         //	BROKER_USER
                || cyng::compare_n(code, OBIS_BROKER_PWD, 5)          //	BROKER_PWD
                || cyng::compare_n(code, OBIS_SERIAL_NAME, 5)         //	SERIAL_NAME
                || cyng::compare_n(code, OBIS_SERIAL_PARITY, 5)       //	SERIAL_PARITY
                || cyng::compare_n(code, OBIS_SERIAL_FLOW_CONTROL, 5) //	SERIAL_FLOW_CONTROL
                || cyng::compare_n(code, OBIS_SERIAL_STOPBITS, 5)     //	SERIAL_STOPBITS
                || OBIS_W_MBUS_ADAPTER_MANUFACTURER == code || OBIS_W_MBUS_FIRMWARE == code || OBIS_W_MBUS_HARDWARE == code ||
                OBIS_DATA_MANUFACTURER == code || OBIS_DEVICE_KERNEL == code || OBIS_VERSION == code || OBIS_FILE_NAME == code ||
                OBIS_IF_1107_METER_ID == code || OBIS_IF_1107_ADDRESS == code || OBIS_IF_1107_P1 == code ||
                OBIS_IF_1107_W5 == code || OBIS_PUSH_TARGET == code || code.starts_with({0x81, 0x81, 0xC7, 0x82, 0x0A}) ||
                OBIS_ACCESS_USER_NAME == code || OBIS_ACCESS_PASSWORD == code || OBIS_NMS_USER == code || OBIS_NMS_PWD == code ||
                OBIS_PEER_ADDRESS == code //	//	OBIS-T-Kennzahl der Ereignisquelle
                || OBIS_DATA_PUSH_DETAILS == code || OBIS_DEVICE_MODEL == code || OBIS_DEVICE_SERIAL == code ||
                OBIS_DEVICE_CLASS == code) {
                //	buffer to string
                cyng::buffer_t const buffer = cyng::to_buffer(obj);
                return (cyng::is_ascii(buffer)) ? cyng::make_object(cyng::make_string(buffer)) : cyng::make_object(buffer);

            } else if (OBIS_TARGET_IP_ADDRESS == code || code.starts_with({0x81, 0x49, 0x17, 0x07, 0x0})) {
                // 81 49 17 07 00 NN    //  primary and secondary IP-T server
                return cyng::make_object(to_ip_address_v4(obj));
            } else if (OBIS_DEVICE_CLASS == code || OBIS_DATA_AES_KEY == code) {

                if (cyng::is_null(obj)) {
                    return cyng::make_object("");
                }
            } else if (OBIS_CURRENT_UTC == code || OBIS_ACT_SENSOR_TIME == code) {
                return read_time(obj);
            } else if (OBIS_SERIAL_NR == code) {
                cyng::buffer_t const buffer = cyng::to_buffer(obj);
                auto const serial_nr = cyng::io::to_hex(buffer);
                return cyng::make_object(serial_nr);
            }
            return obj;
        }

        cyng::obis_path_t read_param_tree_path(cyng::object obj) {

            cyng::obis_path_t result;
            auto const tpl = cyng::container_cast<cyng::tuple_t>(obj);
            result.reserve(tpl.size());

            std::transform(
                tpl.begin(), tpl.end(), std::back_inserter(result), [](cyng::object obj) -> cyng::obis { return read_obis(obj); });

            return result;
        }

        cyng::obis read_obis(cyng::object obj) {

            auto const tmp = cyng::to_buffer(obj);
            BOOST_ASSERT(tmp.size() == 6);
            return cyng::make_obis(tmp);
        }

        cyng::object read_time(cyng::object obj) {

            //
            //  encoded as simple unix time stamp
            //
            if (obj.tag() == cyng::TC_UINT32) {
                return cyng::make_object(std::chrono::system_clock::from_time_t(cyng::value_cast<std::uint32_t>(obj, 0u)));
            }

            //
            //  could be unix time stamp or second index
            //
            else if (obj.tag() == cyng::TC_TUPLE) {
                auto const choice = cyng::container_cast<cyng::tuple_t>(obj);
                BOOST_ASSERT_MSG(choice.size() == 2, "TIME");
                if (choice.size() == 2) {

                    auto code = cyng::value_cast<std::uint8_t>(choice.front(), 0);
                    switch (code) {
                    case TIME_TIMESTAMP: {
                        //	unix time
                        std::time_t const tt = cyng::value_cast<std::uint32_t>(choice.back(), 0);
                        return cyng::make_object(std::chrono::system_clock::from_time_t(tt));
                    } break;
                    case TIME_SECINDEX:
                        return choice.back();
                    default:
                        break;
                    }
                }
            }
            return cyng::make_object(std::chrono::system_clock::from_time_t(0u));
            // return cyng::make_object(std::chrono::system_clock::now());
        }

        std::string to_string(cyng::object obj) {
            if (obj.tag() == cyng::TC_BUFFER) {
                cyng::buffer_t const buffer = cyng::to_buffer(obj);
                return cyng::make_string(buffer);
            }
            return "";
        }

        cyng::object read_string(cyng::object obj) { return cyng::make_object(to_string(obj)); }

        cyng::object read_value(cyng::obis code, std::int8_t scaler, cyng::object obj) {
            if (scaler != 0) {
                switch (obj.tag()) {
                case cyng::TC_INT64: {
                    std::int64_t const value = cyng::value_cast<std::int64_t>(obj, 0);
                    auto const str = scale_value(value, scaler);
                    return cyng::make_object(str);
                } break;
                case cyng::TC_INT32: {
                    std::int32_t const value = cyng::value_cast<std::int32_t>(obj, 0);
                    auto const str = scale_value(value, scaler);
                    return cyng::make_object(str);
                } break;
                case cyng::TC_UINT64: {
                    std::uint64_t const value = cyng::value_cast<std::uint64_t>(obj, 0);
                    auto const str = scale_value(value, scaler);
                    return cyng::make_object(str);
                } break;
                case cyng::TC_UINT32: {
                    std::uint32_t const value = cyng::value_cast<std::uint32_t>(obj, 0);
                    auto const str = scale_value(value, scaler);
                    return cyng::make_object(str);
                } break;
                default:
                    break;
                }
            }

            return customize_value(code, obj);
        }

        cyng::attr_t read_parameter(cyng::obis code, cyng::object obj) {
            auto const tpl = cyng::container_cast<cyng::tuple_t>(obj);
            if (tpl.size() == 2) {
                const auto type = cyng::value_cast<std::uint8_t>(tpl.front(), 0);
                switch (type) {
                case PROC_PAR_VALUE:
                    return {type, customize_value(code, tpl.back())};
                case PROC_PAR_PERIODENTRY:
                    return {type, customize_value(code, tpl.back())};
                case PROC_PAR_TUPELENTRY:
                    return {type, customize_value(code, tpl.back())};
                case PROC_PAR_TIME:
                    return {type, read_time(tpl.back())};
                default:
                    break;
                }
            }
            return {PROC_PAR_UNDEF, obj};
        }

    } // namespace sml

    std::string ip_address_to_str(cyng::object obj) {
        std::stringstream ss;
        if (obj.tag() == cyng::TC_UINT32) {

            //
            //	convert an u32 to a IPv4 address in dotted format
            //
            ss << to_ip_address_v4(obj);
        } else if (obj.tag() == cyng::TC_BUFFER) {

            //
            //	convert an octet to an string
            //
            cyng::buffer_t buffer;
            buffer = cyng::value_cast(obj, buffer);
            ss << std::string(buffer.begin(), buffer.end());
        } else {

            //
            //	everything else
            //
            cyng::io::serialize_plain(ss, obj);
        }
        return ss.str();
    }

    boost::asio::ip::address to_ip_address_v4(cyng::object obj) {
        const boost::asio::ip::address_v4::uint_type ia = cyng::numeric_cast<boost::asio::ip::address_v4::uint_type>(obj, 0u);
        return boost::asio::ip::make_address_v4(cyng::swap_bytes(ia));
    }

    std::string scale_value(std::int64_t value, std::int8_t scaler) {
        //	Working with a string operation prevents us of losing precision.
        std::string str_value = std::to_string(value);

        //	treat 0 as special value
        if (value == 0)
            return str_value;

        //	should at least contain a "0"
        BOOST_ASSERT_MSG(!str_value.empty(), "no number");
        if (str_value.empty())
            return "0.0"; //	emergency exit

        if (scaler < 0) {

            //	negative
            while ((std::size_t)std::abs(scaler - 1) > str_value.size()) {
                //	pad missing zeros
                str_value.insert(0, "0");
            }

            //	finish
            if (str_value.size() > (std::size_t)std::abs(scaler)) {
                const std::size_t pos = str_value.size() + scaler;
                str_value.insert(pos, ".");
            }
        } else {
            //	simulating: result = value * pow10(scaler)
            while (scaler-- > 0) {
                //	append missing zeros
                str_value.append("0");
            }
        }

        return str_value;
    }

} // namespace smf
