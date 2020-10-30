/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2020 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_RS485_SEGW_CONFIG_H
#define NODE_RS485_SEGW_CONFIG_H

#include <segw.h>

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
		enum class protocol
		{
			RAW,
			IEC,
			MBUS,
			SML
		};

	public:
		cfg_rs485(cache&);

		/**
		 * NN = 2
		 * @return "ROOT_SERIAL:9100000001NN"
		 */
		std::string get_port() const;

		/**
		 * @return "rs485:monitor"
		 */
		std::chrono::seconds get_monitor() const;

		/**
		 * @return "ROOT_SERIAL:monitor" capped
		 */
		std::chrono::seconds get_delay(std::chrono::seconds cap) const;

		/**
		 * NN = 2
		 * @return "ROOT_SERIAL:9100000006NN"
		 */
		boost::asio::serial_port_base::baud_rate get_baud_rate() const;
		bool set_baud_rate(cyng::object);

		/**
		 * NN = 2
		 * @return "ROOT_SERIAL:9100000003NN"
		 */
		boost::asio::serial_port_base::parity get_parity() const;
		bool set_parity(cyng::object) const;

		/**
		 * NN = 2
		 * @return "ROOT_SERIAL:9100000004NN"
		 */
		boost::asio::serial_port_base::flow_control get_flow_control() const;
		bool set_flow_control(cyng::object obj) const;

		/**
		 * NN = 2
		 * @return "ROOT_SERIAL:9100000005NN"
		 */
		boost::asio::serial_port_base::stop_bits get_stopbits() const;
		bool set_stopbits(cyng::object obj) const;

		/**
		 * NN = 2
		 * @return "ROOT_SERIAL:9100000002NN"
		 */
		boost::asio::serial_port_base::character_size get_databits() const;
		bool set_databits(cyng::object obj) const;

		/**
		 * NN = 2
		 * @return "ROOT_SERIAL:SERIAL_TASK"
		 */
		std::size_t get_lmn_task() const;
		bool set_lmn_task(std::size_t) const;

		/**
		 * @return "rs485:protocol"
		 */
		protocol get_protocol() const;
		std::string get_protocol_by_name() const;
		bool set_protocol(cyng::object obj) const;

		/**
		 * Control to start the LMN port task or not.
		 * 
		 * @return "rs485:enabled"
		 */
		bool is_enabled() const;
		bool set_enabled(cyng::object obj) const;

		/**
		 * internal port index
		 */
		static constexpr std::uint8_t port_idx = static_cast<std::uint8_t>(source::WIRED_LMN);

	private:
		cache& cache_;
	};
}
#endif
