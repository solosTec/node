/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2020 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_WMBUS_SEGW_CONFIG_H
#define NODE_WMBUS_SEGW_CONFIG_H

#include <string>
#include <chrono>
#include <cstdint>
#include <boost/asio/serial_port_base.hpp>

namespace node
{

	class cache;
	/**
	 * wireless mbus configuration
	 */
	class cfg_wmbus
	{
	public:
		cfg_wmbus(cache&);

		/**
		 * OBIS_IF_wMBUS:... ("wireless-LMN")
		 * @return "8106190700FF:port"
		 */
		std::string get_port() const;

		/**
		 * @return "8106190700FF:monitor"
		 */
		std::chrono::seconds get_monitor() const;

		/**
		 * @return "8106190700FF:speed"
		 */
		boost::asio::serial_port_base::baud_rate get_baud_rate() const;

		/**
		 * @return "8106190700FF:parity"
		 */
		boost::asio::serial_port_base::parity get_parity() const;

		/**
		 * @return "8106190700FF:flow-control"
		 */
		boost::asio::serial_port_base::flow_control get_flow_control() const;

		/**
		 * @return "8106190700FF:stopbits"
		 */
		boost::asio::serial_port_base::stop_bits get_stopbits() const;

		/**
		 * @return "8106190700FF:databits"
		 */
		boost::asio::serial_port_base::character_size get_databits() const;

		/**
		 * @return W_MBUS_PROTOCOL:broker-mode..........: [true/false] (true:bool)
		 */
		bool is_broker_mode() const;

		/**
		 * W_MBUS_PROTOCOL:broker-port.......... : 12001 (12001:i16)
		 * 
		 * @return return address of TCP/IP server
		 */
		std::uint16_t get_broker_port() const;

		/**
		 * W_MBUS_PROTOCOL:broker-address....... : segw.ch (segw.ch:s)
		 *
		 * @return return address of TCP/IP server
		 */
		std::string get_broker_address() const;

		/**
		 * @return HCI: W_MBUS_PROTOCOL:HCI.......................: CP210x (CP210x:s)
		 */
		std::string get_hci() const;

	private:
		cache& cache_;
	};
}
#endif
