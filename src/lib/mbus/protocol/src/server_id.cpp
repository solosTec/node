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

    std::string to_string(srv_id_t id) { return srv_id_to_str(to_buffer(id)); }

    std::string srv_id_to_str(cyng::buffer_t id) {
        std::ostringstream ss;
        ss << std::hex << std::setfill('0');

        if (id.size() == 9) {
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

        // return (s.size() == 8) ? to_meter_id(s) : cyng::hex_to_buffer(s);
        return cyng::hex_to_buffer(s);
    }

    cyng::buffer_t get_id_as_buffer(srv_id_t address) {
        return {
            static_cast<char>(address.at(6)),
            static_cast<char>(address.at(5)),
            static_cast<char>(address.at(4)),
            static_cast<char>(address.at(3))};
    }

    std::string get_id(srv_id_t address) {

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

    std::uint32_t get_dev_id(srv_id_t address) {
        std::uint32_t id{0};
        std::stringstream ss(get_id(address));
        ss >> id;
        return id;
    }

    const char *get_medium_name(srv_id_t address) noexcept {
        return mbus::get_medium_name(static_cast<mbus::device_type>(get_medium(address)));
    }

    std::pair<char, char> get_manufacturer_code(srv_id_t address) {

        return {static_cast<char>(address.at(1)), static_cast<char>(address.at(2))};
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
} // namespace smf
