#include <smf/sml/value.hpp>

#ifdef _DEBUG_SML
#include <iostream>
#endif

#include <boost/assert.hpp>

namespace smf {
    namespace sml {

        namespace detail {
            cyng::tuple_t factory_policy<bool>::create(bool b) {
                return cyng::make_tuple(static_cast<std::uint8_t>(PROC_PAR_VALUE), b);
            }
            cyng::tuple_t factory_policy<std::uint8_t>::create(std::uint8_t v) {
                return cyng::make_tuple(static_cast<std::uint8_t>(PROC_PAR_VALUE), v);
            }
            cyng::tuple_t factory_policy<std::uint16_t>::create(std::uint16_t v) {
                return cyng::make_tuple(static_cast<std::uint8_t>(PROC_PAR_VALUE), v);
            }
            cyng::tuple_t factory_policy<std::uint32_t>::create(std::uint32_t v) {
                return cyng::make_tuple(static_cast<std::uint8_t>(PROC_PAR_VALUE), v);
            }
            cyng::tuple_t factory_policy<std::uint64_t>::create(std::uint64_t v) {
                return cyng::make_tuple(static_cast<std::uint8_t>(PROC_PAR_VALUE), v);
            }
            cyng::tuple_t factory_policy<std::int8_t>::create(std::int8_t v) {
                return cyng::make_tuple(static_cast<std::uint8_t>(PROC_PAR_VALUE), v);
            }
            cyng::tuple_t factory_policy<std::int16_t>::create(std::int16_t v) {
                return cyng::make_tuple(static_cast<std::uint8_t>(PROC_PAR_VALUE), v);
            }
            cyng::tuple_t factory_policy<std::int32_t>::create(std::int32_t v) {
                return cyng::make_tuple(static_cast<std::uint8_t>(PROC_PAR_VALUE), v);
            }
            cyng::tuple_t factory_policy<std::int64_t>::create(std::int64_t v) {
                return cyng::make_tuple(static_cast<std::uint8_t>(PROC_PAR_VALUE), v);
            }

            cyng::tuple_t factory_policy<std::chrono::system_clock::time_point>::create(std::chrono::system_clock::time_point v) {
                return cyng::make_tuple(static_cast<std::uint8_t>(PROC_PAR_TIME), make_timestamp(v));
            }
            cyng::tuple_t factory_policy<std::chrono::milliseconds>::create(std::chrono::milliseconds v) {
                return cyng::make_tuple(static_cast<std::uint8_t>(PROC_PAR_VALUE), static_cast<std::uint32_t>(v.count()));
            }
            cyng::tuple_t factory_policy<std::chrono::seconds>::create(std::chrono::seconds v) {
                return cyng::make_tuple(static_cast<std::uint8_t>(PROC_PAR_VALUE), static_cast<std::uint32_t>(v.count()));
            }
            cyng::tuple_t factory_policy<std::chrono::minutes>::create(std::chrono::minutes v) {
                return cyng::make_tuple(static_cast<std::uint8_t>(PROC_PAR_VALUE), static_cast<std::uint32_t>(v.count()));
            }

            cyng::tuple_t factory_policy<cyng::buffer_t>::create(cyng::buffer_t &&v) {
                return cyng::make_tuple(static_cast<std::uint8_t>(PROC_PAR_VALUE), std::move(v));
            }
            cyng::tuple_t factory_policy<cyng::buffer_t const &>::create(cyng::buffer_t const &v) {
                return cyng::make_tuple(static_cast<std::uint8_t>(PROC_PAR_VALUE), v);
            }
            cyng::tuple_t factory_policy<std::string>::create(std::string v) {
                //	convert to buffer_t
                return cyng::make_tuple(static_cast<std::uint8_t>(PROC_PAR_VALUE), cyng::buffer_t(v.begin(), v.end()));
            }

            cyng::tuple_t factory_policy<cyng::obis>::create(cyng::obis v) {
                //	convert to buffer_t
                return factory_policy<cyng::buffer_t>::create(cyng::to_buffer(v));
            }

            cyng::tuple_t factory_policy<boost::asio::ip::address>::create(boost::asio::ip::address v) {
                return (v.is_v4()) ? factory_policy<boost::asio::ip::address_v4>::create(v.to_v4())
                                   : factory_policy<boost::asio::ip::address_v6>::create(v.to_v6());
            }
            cyng::tuple_t factory_policy<boost::asio::ip::address_v4>::create(boost::asio::ip::address_v4 v) {
                const auto ia = v.to_bytes();
                return factory_policy<cyng::buffer_t>::create(cyng::buffer_t(ia.begin(), ia.end()));
            }
            cyng::tuple_t factory_policy<boost::asio::ip::address_v6>::create(boost::asio::ip::address_v6 v) {
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
                    static_cast<std::uint8_t>(0) //	local offset
                    ,
                    static_cast<std::uint8_t>(0))); //	summertime offset
        }

        cyng::tuple_t make_timestamp(std::chrono::system_clock::time_point tp) {
            //	1. UNIX timestamp - Y2K38 problem.
            std::time_t ut = std::chrono::system_clock::to_time_t(tp);
            return cyng::make_tuple(static_cast<std::uint8_t>(TIME_TIMESTAMP), static_cast<std::uint32_t>(ut));
        }
        cyng::tuple_t make_sec_index(std::chrono::system_clock::time_point tp) {
            //	1. UNIX timestamp - Y2K38 problem.
            std::time_t ut = std::chrono::system_clock::to_time_t(tp);
            return cyng::make_tuple(static_cast<std::uint8_t>(TIME_SECINDEX), static_cast<std::uint32_t>(ut));
        }
        cyng::tuple_t make_value() { return cyng::make_tuple(static_cast<std::uint8_t>(PROC_PAR_VALUE), cyng::null()); }

    } // namespace sml
} // namespace smf
