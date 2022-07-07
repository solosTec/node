/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SERIAL_BUS_FLOW_CONTROL_H
#define SMF_SERIAL_BUS_FLOW_CONTROL_H

#include <boost/asio/serial_port_base.hpp>
#include <string>

namespace smf {

    namespace serial {

        /**
         *	@return parity enum with the specified name.
         *	If no name is matching function returns boost::asio::serial_port_base::flow_control::none
         */
        boost::asio::serial_port_base::flow_control to_flow_control(std::string);
        std::string to_string(boost::asio::serial_port_base::flow_control);
    } // namespace serial
} // namespace smf

#endif //	SMF_SERIAL_BUS_FLOW_CONTROL_H
