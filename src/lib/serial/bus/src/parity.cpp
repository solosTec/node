/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <boost/algorithm/string.hpp>
#include <smf/serial/parity.h>

namespace smf {

    namespace serial {

        boost::asio::serial_port_base::parity to_parity(std::string s) {
            if (boost::algorithm::equals("odd", s)) {
                return boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::odd);
            } else if (boost::algorithm::equals("even", s)) {
                return boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::even);
            }
            //	none
            return boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none);
        }

        std::string to_string(boost::asio::serial_port_base::parity p) {
            switch (p.value()) {
            case boost::asio::serial_port_base::parity::odd: return "odd";
            case boost::asio::serial_port_base::parity::even: return "even";
            default: break;
            }
            return "none";
        }

    } // namespace serial
} // namespace smf
