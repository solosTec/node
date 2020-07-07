/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */
#ifndef NODE_LMN_H
#define NODE_LMN_H

#include "decoder.h"
#include <cyng/async/mux.h>
#include <cyng/vm/controller.h>

#include <boost/uuid/uuid.hpp>

namespace node
{
	//
	//	forward declaration
	//
	class cache;
	class cfg_wmbus;

	/**
	 * Read configuration and starts all required
	 * tasks to read data from serial ports and connects
	 * this data stream to a parser.
	 */
	class lmn
	{
	public:
		lmn(cyng::async::mux&
			, cyng::logging::log_ptr logger
			, cache&
			, boost::uuids::uuid tag);

		/**
		 * Open serial and wireless communication ports
		 */
		void start();

	private:
		void start_lmn_wired();
		void start_lmn_wireless();

		/**
		 * @param init initialization message
		 */
		std::pair<std::size_t, bool> start_lmn_port_wireless(std::size_t, std::size_t, cyng::buffer_t&& init);
		std::pair<std::size_t, bool> start_lmn_port_wired(std::size_t);

		/**
		 * Start RS485 interface manager
		 */
		std::pair<std::size_t, bool> start_rs485_mgr(std::size_t, std::chrono::seconds);

		/**
		 * Starts the wireless mBus reciver. This could be the wireless mBus parse
		 * or a server connected via TCP/IP.
		 */
		std::pair<std::size_t, bool> start_mbus_receiver(bool transparent);

		void wmbus_push_frame(cyng::context& ctx);
		void sml_msg(cyng::context& ctx);
		void sml_eom(cyng::context& ctx);
		void sml_get_list_response(cyng::context& ctx);

		void mbus_ack(cyng::context& ctx);
		void mbus_frame_short(cyng::context& ctx);
		void mbus_frame_ctrl(cyng::context& ctx);
		void mbus_frame_long(cyng::context& ctx);

	private:
		cyng::logging::log_ptr logger_;
		cyng::async::mux& mux_;
		cyng::controller vm_;
		cache& cache_;
		decoder_wireless_mbus decoder_wmbus_;

		std::size_t serial_parser_;	//	wired mbus
		std::size_t serial_mgr_;
		std::size_t serial_port_;	//	incoming data

		std::size_t radio_parser_;	//	wireless mbus
		std::size_t radio_port_;	//	incoming data
	};

}
#endif