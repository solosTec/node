/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#include <boost/asio.hpp>
#include "show_ports.h"
#include <cyng/sys/port.h>

namespace node 
{
	int show_ports(std::ostream& os)
	{

		auto ports = cyng::sys::get_ports();
		for (auto const& name : ports) {
			os
				<< name
				<< std::endl
				;
		}

		boost::asio::io_service io_service;
		for (auto const& name : ports) {

			boost::asio::serial_port port(io_service);
			boost::system::error_code ec;
			port.open(name, ec);
			if (port.is_open()) {

				boost::asio::serial_port_base::character_size databits;
				port.get_option(databits, ec);
				os
					<< "port "
					<< name
					<< " databits: "
					<< databits.value()
					<< std::endl;

				boost::asio::serial_port_base::baud_rate baud_rate;
				port.get_option(baud_rate, ec);
				os
					<< "port "
					<< name
					<< " baudrate: "
					<< baud_rate.value()
					<< std::endl;

				boost::asio::serial_port_base::stop_bits stopbits;
				port.get_option(stopbits, ec);
				os
					<< "port "
					<< name
					<< " stopbits: "
					<< stopbits.value()
					<< std::endl;

				boost::asio::serial_port_base::parity parity;
				port.get_option(parity, ec);
				os
					<< "port "
					<< name
					<< " parity  : "
					;
				switch (parity.value()) {
				case boost::asio::serial_port_base::parity::odd:	os << "odd"; break;
				case boost::asio::serial_port_base::parity::even:	os << "even"; break;
				default:
					os << "none";
					break;
				}

				os
					<< std::endl;

			}
			else {
				os
					<< "cannot open "
					<< name
					<< std::endl
					;
			}

			//std::
			//os
			//	<< port
			//	<< std::endl
			//	;
		}

		return EXIT_SUCCESS;
	}
}

