/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SERIAL_BUS_SERIAL_PARITY_H
#define SMF_SERIAL_BUS_SERIAL_PARITY_H


#include <string>
#include <boost/asio/serial_port_base.hpp>

namespace smf {

	namespace serial {

		/**
		 *	@return parity enum with the specified name.
		 *	If no name is matching function returns boost::asio::serial_port_base::parity::none
		 */
		boost::asio::serial_port_base::parity to_parity(std::string);
		std::string to_str(boost::asio::serial_port_base::parity);
	}
}	


#endif	
