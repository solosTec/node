/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SERIAL_BUS_SERIAL_STOPBITS_H
#define SMF_SERIAL_BUS_SERIAL_STOPBITS_H

#include <boost/asio/serial_port_base.hpp>
#include <string>

namespace smf {

    namespace serial {

        /**
         *	@return stop_bits enum with the specified name.
         *	If no name is matching function returns boost::asio::serial_port_base::stop_bits::one
         */
        boost::asio::serial_port_base::stop_bits to_stopbits(std::string);
        std::string to_string(boost::asio::serial_port_base::stop_bits);
    } // namespace serial
} // namespace smf

#endif
