/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/mbus/radio/header.h>

#include <cstring>
#include <iomanip>
#include <sstream>

#include <boost/assert.hpp>

namespace smf {
    namespace mbus {
        namespace radio {
            header::header()
                : data_{0} {}

            void header::reset() { data_.fill(0); }

            cyng::buffer_t header::get_server_id_as_buffer() const noexcept {
                return {
                    1, //	wireless M-Bus

                    //	[1-2] 2 bytes manufacturer ID
                    static_cast<char>(data_.at(2)),
                    static_cast<char>(data_.at(3)),

                    //	[3-6] 4 bytes serial number (reversed)
                    static_cast<char>(data_.at(4)),
                    static_cast<char>(data_.at(5)),
                    static_cast<char>(data_.at(6)),
                    static_cast<char>(data_.at(7)),

                    static_cast<char>(data_.at(8)),
                    static_cast<char>(data_.at(9))};
            }
            cyng::buffer_t restore_data(header const &h, tpl const &t, cyng::buffer_t const &payload) {
                cyng::buffer_t res;
                res.reserve(h.total_size() + 1);
                res.insert(res.end(), h.data_.begin(), h.data_.end()); //	header
                switch (get_tpl_type(h.get_frame_type())) {
                case tpl_type::SHORT:
                    //	skip secondary address
                    res.insert(res.end(), t.data_.begin() + 8, t.data_.end()); //	tpl short
                    break;
                case tpl_type::LONG:
                    res.insert(res.end(), t.data_.begin(), t.data_.end()); //	tpl long
                    break;
                default:
                    //	no tpl
                    break;
                }
                res.insert(res.end(), payload.begin(), payload.end()); //	payload
                return res;
            }

            tpl::tpl()
                : data_{0} {}

            void tpl::reset() { data_.fill(0); }

        } // namespace radio

        std::string dev_id_to_str(std::uint32_t id) {
            std::stringstream ss;
            ss.fill('0');
            ss << std::setw(8) << id;
            return ss.str();
        }

        std::string to_str(radio::header const &h) {
            std::stringstream ss;
            ss << h;
            return ss.str();
        }

    } // namespace mbus
} // namespace smf
