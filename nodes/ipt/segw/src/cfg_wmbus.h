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
		 * OBIS_IF_wMBUS:enabled"
		 * @return "8106190700FF:enabled"
		 */
		bool is_enabled() const;

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
		 * @return HCI: W_MBUS_PROTOCOL:HCI.......................: CP210x (CP210x:s)
		 */
		std::string get_hci() const;
		bool is_CP210x() const;

		/**
		 * disable/enable collecting/storing meter data
		 */
		bool generate_profile() const;

	private:
		cache& cache_;
	};
}
#endif
