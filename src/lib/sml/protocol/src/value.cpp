#include <smf/sml/value.hpp>

#include <cyng/obj/value_cast.hpp>

#ifdef _DEBUG_SML
#include <iostream>
#endif

#include <boost/assert.hpp>

namespace smf {
    namespace sml {

        namespace detail {
            cyng::attr_t factory_policy<bool>::create(bool b) { return cyng::make_attr(PROC_PAR_VALUE, b); }

            cyng::attr_t factory_policy<std::chrono::system_clock::time_point>::create(std::chrono::system_clock::time_point &&v) {
                return cyng::make_attr(PROC_PAR_TIME, make_timestamp(std::move(v)));
            }
            cyng::attr_t
            factory_policy<std::chrono::system_clock::time_point>::create(std::chrono::system_clock::time_point const &v) {
                return cyng::make_attr(PROC_PAR_TIME, make_timestamp(v));
            }

            cyng::attr_t factory_policy<std::chrono::milliseconds>::create(std::chrono::milliseconds v) {
                return cyng::make_attr(PROC_PAR_VALUE, static_cast<std::uint32_t>(v.count()));
            }
            cyng::attr_t factory_policy<std::chrono::seconds>::create(std::chrono::seconds v) {
                return cyng::make_attr(PROC_PAR_VALUE, static_cast<std::uint32_t>(v.count()));
            }
            cyng::attr_t factory_policy<std::chrono::minutes>::create(std::chrono::minutes v) {
                return cyng::make_attr(PROC_PAR_VALUE, static_cast<std::uint32_t>(v.count()));
            }

            cyng::attr_t factory_policy<cyng::buffer_t>::create(cyng::buffer_t &&v) {
                return cyng::make_attr(PROC_PAR_VALUE, std::move(v));
            }
            cyng::attr_t factory_policy<cyng::buffer_t>::create(cyng::buffer_t const &v) {
                return cyng::make_attr(PROC_PAR_VALUE, v);
            }
            cyng::attr_t factory_policy<std::string>::create(std::string v) {
                //	convert to buffer_t
                return cyng::make_attr(PROC_PAR_VALUE, cyng::buffer_t(v.begin(), v.end()));
            }

            cyng::attr_t factory_policy<cyng::obis>::create(cyng::obis v) {
                //	convert to buffer_t
                return factory_policy<cyng::buffer_t>::create(cyng::to_buffer(v));
            }

            cyng::attr_t factory_policy<boost::asio::ip::address>::create(boost::asio::ip::address v) {
                return (v.is_v4()) ? factory_policy<boost::asio::ip::address_v4>::create(v.to_v4())
                                   : factory_policy<boost::asio::ip::address_v6>::create(v.to_v6());
            }
            cyng::attr_t factory_policy<boost::asio::ip::address_v4>::create(boost::asio::ip::address_v4 v) {
                const auto ia = v.to_bytes();
                return factory_policy<cyng::buffer_t>::create(cyng::buffer_t(ia.begin(), ia.end()));
            }
            cyng::attr_t factory_policy<boost::asio::ip::address_v6>::create(boost::asio::ip::address_v6 v) {
                const auto ia = v.to_bytes();
                return factory_policy<cyng::buffer_t>::create(cyng::buffer_t(ia.begin(), ia.end()));
            }

        } // namespace detail

        cyng::tuple_t make_local_timestamp(std::chrono::system_clock::time_point tp) {
            //	1. UNIX timestamp
            std::time_t ut = std::chrono::system_clock::to_time_t(tp);
            //	2. local offset
            //	3. summertime offset

            //	convert to SML_Time type 3
            //	SML_Time is a tuple with 2 elements
            //	(1) choice := 2 (timestamp)
            //	(2) value := UNIX time
            //	(3) value := UNIX time + offset data
            return cyng::make_tuple(
                static_cast<std::uint8_t>(PROC_PAR_TUPELENTRY),
                cyng::make_tuple(
                    static_cast<std::uint32_t>(ut),
                    static_cast<std::uint8_t>(0),   //	local offset
                    static_cast<std::uint8_t>(0))); //	summertime offset
        }
        cyng::tuple_t make_local_timestamp() { return make_local_timestamp(std::chrono::system_clock::now()); }

        cyng::tuple_t make_timestamp(std::chrono::system_clock::time_point tp) {
            //	1. UNIX timestamp - Y2K38 problem.
            // std::time_t ut = std::chrono::system_clock::to_time_t(tp);
            // return cyng::make_tuple(static_cast<std::uint8_t>(TIME_TIMESTAMP), static_cast<std::uint32_t>(ut));
            auto attr = make_timestamp_attr(tp);
            return cyng::make_tuple(static_cast<std::uint8_t>(attr.first), std::move(attr.second));
        }
        cyng::tuple_t make_timestamp() { return make_timestamp(std::chrono::system_clock::now()); }

        cyng::tuple_t make_sec_index(std::chrono::system_clock::time_point tp) {
            //	1. UNIX timestamp - Y2K38 problem.
            std::time_t ut = std::chrono::system_clock::to_time_t(tp);
            return cyng::make_tuple(static_cast<std::uint8_t>(TIME_SECINDEX), static_cast<std::uint32_t>(ut));
        }

        cyng::attr_t make_timestamp_attr(std::chrono::system_clock::time_point tp) {
            //	1. UNIX timestamp - Y2K38 problem.
            std::time_t ut = std::chrono::system_clock::to_time_t(tp);
            return cyng::make_attr(TIME_TIMESTAMP, static_cast<std::uint32_t>(ut));
        }
        cyng::attr_t make_timestamp_attr() { return make_timestamp_attr(std::chrono::system_clock::now()); }

        cyng::tuple_t make_value() { return cyng::make_tuple(static_cast<std::uint8_t>(PROC_PAR_VALUE), cyng::null()); }
        cyng::attr_t make_attribute() { return cyng::attr_t(1, cyng::make_object()); }

        cyng::tuple_t to_value(cyng::object obj) { return to_value(to_attribute(obj)); }

        cyng::tuple_t to_value(cyng::attr_t attr) { return cyng::make_tuple(static_cast<std::uint8_t>(attr.first), attr.second); }

        cyng::attr_t to_attribute(cyng::object obj) {
            switch (obj.tag()) {
            case cyng::TC_BOOL:
            case cyng::TC_UINT8:
            case cyng::TC_UINT16:
            case cyng::TC_UINT32:
            case cyng::TC_UINT64:
            case cyng::TC_INT8:
            case cyng::TC_INT16:
            case cyng::TC_INT32:
            case cyng::TC_INT64:
            case cyng::TC_BUFFER:
            case cyng::TC_STRING:
                return cyng::attr_t(PROC_PAR_VALUE, obj);
            case cyng::TC_TIME_POINT:
                return cyng::attr_t(PROC_PAR_TIME, obj);
            case cyng::TC_SECOND: //	convert to [uint]
                return detail::factory_policy<std::uint32_t>::create(cyng::value_cast(obj, std::chrono::seconds(900)).count());
            default:
                break;
            }
            //  empty attribute
            return make_attribute();
        }

    } // namespace sml
} // namespace smf
