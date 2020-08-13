/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2020 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_RS485_SEGW_CONFIG_H
#define NODE_RS485_SEGW_CONFIG_H

#include <string>
#include <chrono>
#include <cstdint>
#include <boost/asio/serial_port_base.hpp>

namespace node
{

	class cache;

	/**
	 * RS 485 configuration
	 */
	class cfg_rs485
	{
	public:
		cfg_rs485(cache&);

		/**
		 * @return "rs485:port"
		 */
		std::string get_port() const;

		/**
		 * @return "rs485:monitor"
		 */
		std::chrono::seconds get_monitor() const;

		/**
		 * @return "rs485:speed"
		 */
		boost::asio::serial_port_base::baud_rate get_baud_rate() const;

		/**
		 * @return "rs485:parity"
		 */
		boost::asio::serial_port_base::parity get_parity() const;

		/**
		 * @return "rs485:flow_control"
		 */
		boost::asio::serial_port_base::flow_control get_flow_control() const;

		/**
		 * @return "rs485:stopbits"
		 */
		boost::asio::serial_port_base::stop_bits get_stopbits() const;

		/**
		 * @return "rs485:databits"
		 */
		boost::asio::serial_port_base::character_size get_databits() const;

	private:
		cache& cache_;
	};
}
#endif
