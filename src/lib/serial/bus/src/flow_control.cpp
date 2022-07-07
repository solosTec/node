/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <boost/algorithm/string.hpp>
#include <smf/serial/flow_control.h>

namespace smf {

    namespace serial {

        boost::asio::serial_port_base::flow_control to_flow_control(std::string s) {
            if (boost::algorithm::equals("software", s)) {
                return boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::software);
            } else if (boost::algorithm::equals("hardware", s)) {
                return boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::hardware);
            }
            //	none
            return boost::asio::serial_port_base::flow_control();
        }

        std::string to_string(boost::asio::serial_port_base::flow_control f) {
            switch (f.value()) {
            case boost::asio::serial_port_base::flow_control::software: "software";
            case boost::asio::serial_port_base::flow_control::hardware: "hardware";
            default: break;
            }
            return "none";
        }

    } // namespace serial
} // namespace smf
