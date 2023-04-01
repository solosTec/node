/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/mbus/server_id.h>

#include <smf/mbus/medium.h>

#include <cyng/obj/buffer_cast.hpp>
#include <cyng/parse/buffer.h>
#include <cyng/parse/hex.h>

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>

#include <boost/assert.hpp>

namespace smf {
    cyng::buffer_t to_meter_id(std::uint32_t id) {
        //	Example: 0x3105c = > 96072000

        std::stringstream ss;
        ss.fill('0');
        ss << std::setw(8) << std::setbase(10) << id;

        auto const s = ss.str();
        BOOST_ASSERT(s.size() == 8);

        ss >> std::setbase(16) >> id;

        return cyng::to_buffer_be(id);
    }

    cyng::buffer_t to_meter_id(std::string id) {

        //	Example: "10320047" => [10, 32, 00, 47]dec == [0A, 20, 00, 2F]hex
        BOOST_ASSERT(id.size() == 8);

        cyng::buffer_t r;
        std::fill_n(std::back_inserter(r), 4, 0);
        if (id.size() == 8) {
            for (std::size_t idx = 0; idx < id.size(); idx += 2) {
                r.at(idx / 2) = static_cast<cyng::buffer_t::value_type>(std::stoul(id.substr(idx, 2)));
            }
        }
        return r;
    }

    cyng::buffer_t to_buffer(srv_id_t const &id) { return cyng::buffer_t(id.begin(), id.end()); }

    std::string to_string(srv_id_t id) { return srv_id_to_str(to_buffer(id), true); }

    std::string srv_id_to_str(cyng::buffer_t id, bool formatted) {
        std::ostringstream ss;
        ss << std::hex << std::setfill('0');

        if (id.size() == 9) {
            if (formatted) {
                std::size_t counter{0};
                for (auto c : id) {
                    ss << std::setw(2) << (+c & 0xFF);
                    counter++;
                    switch (counter) {
                    case 1:
                    case 3:
                    case 7:
                    case 8: ss << '-'; break;
                    default: break;
                    }
                }
            } else {
                for (auto c : id) {
                    ss << std::setw(2) << (+c & 0xFF);
                }
            }
        } else if (id.size() == 4) {
            //  meter id
            //	read this value as a hex value
            //
            std::stringstream ss;
            ss.fill('0');
            for (auto c : id) {
                ss << std::setw(2) << std::setbase(16) << (+c & 0xFF);
            }
            return ss.str();

        } else if (id.size() == 8 && std::all_of(id.begin(), id.end(), [](cyng::buffer_t::value_type c) {
                       return c >= '0' && c <= '9';
                   })) {
            //  example: 3034393137373731 => 04917771
            return {id.begin(), id.end()};
        } else {
            //	all other cases
            for (auto c : id) {
                ss << std::setw(2) << (+c & 0xFF);
            }
        }
        return ss.str();
    }

    cyng::buffer_t to_srv_id(std::string s) {
        //  tt-mmmm-nnnnnnnn-vv-uu
        if (s.size() == 22 && s.at(0) == '0' && ((s.at(1) == '1') || (s.at(1) == '2')) && s.at(2) == '-' && s.at(7) == '-' &&
            s.at(16) == '-' && s.at(19) == '-') {

            //
            //  convert to an hex string
            //
            std::string id;
            id.reserve(18);
            std::copy_if(std::begin(s), std::end(s), std::back_inserter(id), [](char c) { return c != '-'; });

            //
            //  convert hex string to buffer
            //
            return cyng::hex_to_buffer(id);
        }

        return cyng::hex_to_buffer(s);
    }

    cyng::buffer_t get_id_as_buffer(srv_id_t address) {
        return {
            static_cast<char>(address.at(6)),
            static_cast<char>(address.at(5)),
            static_cast<char>(address.at(4)),
            static_cast<char>(address.at(3))};
    }

    std::string get_meter_id(srv_id_t address) {

        std::uint32_t id{0};

        //
        //	get the device ID as u32 value
        //
        std::memcpy(&id, &address.at(3), sizeof(id));

        //
        //	read this value as a hex value
        //
        std::stringstream ss;
        ss.fill('0');
        ss << std::setw(8) << std::setbase(16) << id;

        return ss.str();
    }

    std::string get_meter_id(std::string s) {
        // "tt-mmmm-nnnnnnnn-vv-uu" - 22 bytes
        // "ttmmmmnnnnnnnnvvuu" - 18 bytes
        // "nnnnnnnn" 8 bytes
        if (s.size() == 22 && s.at(0) == '0' && ((s.at(1) == '1') || (s.at(1) == '2')) && s.at(2) == '-' && s.at(7) == '-' &&
            // "01-a815-54787404-01-02" => 04747854
            s.at(16) == '-' && s.at(19) == '-') {
            std::string id;
            for (std::size_t idx = 14; idx >= 8; idx -= 2) {
                id.push_back(s.at(idx));
                id.push_back(s.at(idx + 1));
            }
            return id;
        } else if (s.size() == 18) {
            // auto id = s.substr(6, 8);
            std::string id;
            for (std::size_t idx = 12; idx >= 6; idx -= 2) {
                id.push_back(s.at(idx));
                id.push_back(s.at(idx + 1));
            }
            return id;
        } else if (s.size() == 8) {
            return s;
        } else if (s.size() == 16 && std::all_of(s.begin(), s.end(), [](char c) { return c >= '0' && c <= '9'; })) {
            //  "3034393137373235" => "04917725"
            std::string id;
            for (std::size_t idx = 0; idx < s.size(); idx += 2) {
                id.push_back(cyng::hex_to_u8(s.at(idx), s.at(idx + 1)));
            }
            return id;
        }
        //  error
        return s;
    }

    std::uint32_t get_dev_id(srv_id_t address) {
        std::uint32_t id{0};
        std::stringstream ss(get_meter_id(address));
        ss >> id;
        return id;
    }

    const char *get_medium_name(srv_id_t address) noexcept {
        return mbus::get_medium_name(static_cast<mbus::device_type>(get_medium(address)));
    }

    std::pair<char, char> get_manufacturer_code(srv_id_t id) { return {static_cast<char>(id.at(1)), static_cast<char>(id.at(2))}; }

    std::uint16_t get_manufacturer_flag(srv_id_t id) {
        auto const [c1, c2] = get_manufacturer_code(id);
        return static_cast<unsigned char>(c1) + (static_cast<unsigned char>(c2) << 8);
    }

    std::string gen_metering_code(std::string const &country_code, boost::uuids::uuid tag) {

        BOOST_ASSERT_MSG(country_code.size() == 2, "invalid country code");

        std::stringstream ss;
        ss << country_code;
        for (auto const c : tag.data) {
            ss << +c;
        }
        return ss.str();
    }

    srv_id_t to_srv_id(cyng::buffer_t const &buffer) {
        srv_id_t srv_id{0};
        if (buffer.size() == std::tuple_size<srv_id_t>::value) {
            std::copy(buffer.begin(), buffer.end(), srv_id.begin());
        } else if (buffer.size() == 4) {
            srv_id.at(0) = 1; // wireless
            //  [3-6] 4 bytes serial number (reversed)
            srv_id.at(3) = buffer.at(3);
            srv_id.at(4) = buffer.at(2);
            srv_id.at(5) = buffer.at(1);
            srv_id.at(6) = buffer.at(0);
        } else if (buffer.size() == 8) {
            BOOST_ASSERT_MSG(false, "not implemented yet");
            srv_id.at(0) = 1; // wireless
        }
        return srv_id;
    }

    srv_type detect_server_type(cyng::buffer_t const &buffer) {
        if (buffer.size() == 8 && (buffer.at(6) >= 0x00) && (buffer.at(6) <= 0x0F))
            // tt-mmmm-nnnnnnnn-vv-uu
            return srv_type::W_MBUS;
        else if ((buffer.size() == 9) && (buffer.at(0) == 2))
            return srv_type::MBUS_WIRED;
        else if ((buffer.size() == 9) && (buffer.at(0) == 1))
            return srv_type::MBUS_RADIO;
        else if ((buffer.size() == 7) && (buffer.at(0) == 5))
            return srv_type::GW;
        else if (buffer.size() == 10 && buffer.at(1) == '3')
            return srv_type::BCD;
        else if ((buffer.size() == 10) && (buffer.at(0) == '0') && (buffer.at(1) == '6'))
            return srv_type::DKE_1;
        else if ((buffer.size() == 10) && (buffer.at(0) == '0') && (buffer.at(1) == '9'))
            return srv_type::DKE_2;
        else if ((buffer.size() == 4) && (buffer.at(0) == 10))
            return srv_type::SWITCH;
        else if (std::all_of(buffer.begin(), buffer.end(), [](char c) {
                     return ((c >= '0') && (c <= '9')) || ((c >= 'A') && (c <= 'Z'));
                 })) {
            return srv_type::SERIAL;
        } else if (buffer.size() == 8 && buffer.at(2) == '4') {
            return srv_type::EON;
        }

        return srv_type::OTHER;
    }

    bool is_null(srv_id_t const &id) {
        //  test if all elements are '0'
        return std::all_of(id.begin(), id.end(), [](std::uint8_t v) { return v == 0; });
    }

} // namespace smf
