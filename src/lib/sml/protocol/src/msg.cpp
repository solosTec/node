#include <smf/sml/msg.h>

#include <smf/sml/crc16.h>
#include <smf/sml/serializer.h>

#include <cyng/obj/util.hpp>
#ifdef _DEBUG
#include <cyng/obj/container_cast.hpp>
#endif

#include <algorithm>
#ifdef _DEBUG_SML
#include <iostream>
#endif

#include <boost/assert.hpp>

namespace smf {
    namespace sml {

        cyng::buffer_t to_sml(sml::messages_t const &reqs) {
            std::vector<cyng::buffer_t> msg;
            for (auto const &req : reqs) {
                //
                //	linearize and set CRC16
                //	append to current SML message
                //
                BOOST_ASSERT_MSG(req.size() == 6, "invalid SML message size");
                msg.push_back(set_crc16(serialize(req)));
            }

            return boxing(msg);
        }

        cyng::buffer_t set_crc16(cyng::buffer_t &&buffer) {
            BOOST_ASSERT_MSG(buffer.size() > 4, "message buffer to small");

            crc_16_data crc;
            crc.crc_ = crc16_calculate(buffer);

            if (buffer.size() > 4) {
                //	patch
                buffer[buffer.size() - 3] = crc.data_[1];
                buffer[buffer.size() - 2] = crc.data_[0];
                BOOST_ASSERT_MSG(buffer[buffer.size() - 1] == 0, "invalid EOD");
            }
            return buffer;
        }

        cyng::buffer_t boxing(std::vector<cyng::buffer_t> const &inp) {
            //
            //	trailer v1.0
            //
            cyng::buffer_t buf{0x1b, 0x1b, 0x1b, 0x1b, 0x01, 0x01, 0x01, 0x01};

            //
            //	append messages
            //
            for (auto const &msg : inp) {
                buf.insert(buf.end(), msg.begin(), msg.end());
            }

            //
            //	padding bytes
            //
            const char pad = ((buf.size() % 4) == 0) ? 0 : (4 - (buf.size() % 4));
            for (std::size_t pos = 0; pos < pad; ++pos) {
                buf.push_back(0x0);
            }

            //
            //	tail v1.0
            //
            buf.insert(buf.end(), {0x1b, 0x1b, 0x1b, 0x1b, 0x1a, pad});

            //
            //	CRC calculation over complete buffer
            //
            crc_16_data crc;
            BOOST_ASSERT(buf.size() < std::numeric_limits<int>::max());
            crc.crc_ = crc16_calculate(reinterpret_cast<const unsigned char *>(buf.data()), buf.size());

            //	network order
            buf.push_back(crc.data_[1]);
            buf.push_back(crc.data_[0]);

            return buf;
        }

        cyng::buffer_t boxing(cyng::buffer_t &&msg) {
            //
            //	trailer v1.0
            //
            msg.insert(msg.begin(), {0x1b, 0x1b, 0x1b, 0x1b, 0x01, 0x01, 0x01, 0x01});

            //
            //	padding bytes
            //
            const char pad = ((msg.size() % 4) == 0) ? 0 : (4 - (msg.size() % 4));
            for (std::size_t pos = 0; pos < pad; ++pos) {
                msg.push_back(0x0);
            }

            //
            //	tail v1.0
            //
            msg.insert(msg.end(), {0x1b, 0x1b, 0x1b, 0x1b, 0x1a, pad});

            //
            //	CRC calculation over complete buffer
            //
            crc_16_data crc;
            BOOST_ASSERT(msg.size() < std::numeric_limits<int>::max());
            crc.crc_ = crc16_calculate(reinterpret_cast<const unsigned char *>(msg.data()), msg.size());

            //	network order
            msg.push_back(crc.data_[1]);
            msg.push_back(crc.data_[0]);

            return msg;
        }

        cyng::tuple_t make_message(
            std::string trx,
            std::uint8_t group_no,
            std::uint8_t abort_code,
            msg_type type,
            cyng::tuple_t body,
            std::uint16_t crc) {

            return cyng::make_tuple(
                trx, group_no, abort_code, cyng::make_tuple(static_cast<std::uint16_t>(type), body), crc, cyng::eod());
        }

        cyng::tuple_t make_empty_tree(cyng::obis code) { return cyng::make_tuple(code, cyng::null{}, cyng::null{}); }

        cyng::tuple_t tree_param(cyng::obis code, cyng::tuple_t param) { return cyng::make_tuple(code, param, cyng::null{}); }
        cyng::tuple_t tree_param(cyng::obis code, cyng::attr_t attr) { return cyng::make_tuple(code, attr, cyng::null{}); }

        cyng::tuple_t tree_child_list(cyng::obis code, cyng::tuple_t child_list) {
#ifdef _DEBUG
            for (auto const &obj : child_list) {
                auto const tpl = cyng::container_cast<cyng::tuple_t>(obj);
                BOOST_ASSERT(tpl.size() == 3 || tpl.empty());
            }
#endif
            return cyng::make_tuple(code, cyng::null{}, child_list);
        }

        cyng::tuple_t tree_child_list(cyng::obis code, std::initializer_list<cyng::tuple_t> list) {
#ifdef _DEBUG
            for (auto &tpl : list) {
                BOOST_ASSERT(tpl.size() == 3 || tpl.empty());
            }
#endif
            return cyng::make_tuple(code, cyng::null{}, cyng::make_tuple(list));
        }

        cyng::tuple_t make_tree(cyng::obis code, cyng::tuple_t param, cyng::tuple_t list) {
#ifdef _DEBUG
            for (auto const &obj : list) {
                auto const tpl = cyng::container_cast<cyng::tuple_t>(obj);
                BOOST_ASSERT(tpl.size() == 3 || tpl.empty());
            }
#endif
            return cyng::make_tuple(code, param, list);
        }

        cyng::tuple_t list_entry(cyng::obis code, std::uint32_t status, std::uint8_t unit, std::int8_t scaler, cyng::object value) {
            return cyng::make_tuple(
                code,         // object name
                status,       // status
                cyng::null{}, // valTime (optional)
                unit,         // unit code
                scaler,       // scale factor
                value,        // readout value
                cyng::null{}  // signature
            );
        }

        cyng::tuple_t period_entry(cyng::obis code, std::uint8_t unit, std::int8_t scaler, cyng::object value) {
            return cyng::make_tuple(
                code,        // object name
                unit,        // unit code
                scaler,      // scale factor
                value,       // value
                cyng::null{} // signature
            );
        }

    } // namespace sml
} // namespace smf
