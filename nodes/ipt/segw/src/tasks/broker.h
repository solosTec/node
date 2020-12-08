/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_TASK_BROKER_WMBUS_H
#define NODE_SEGW_TASK_BROKER_WMBUS_H

#include <NODE_project_info.h>
#include "../cfg_wmbus.h"

#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/table/table_fwd.h>

#include <cyng/compatibility/file_system.hpp>

namespace cyng
{
	class controller;
}

namespace node
{
	/**
	 * receiver and parser for wireless mbus data
	 */
	class cache;
	class broker
	{
		using read_buffer_t = std::array<char, NODE::PREFERRED_BUFFER_SIZE>;

	public:
		//	[0] receive data
		using msg_0 = std::tuple<cyng::buffer_t, std::size_t>;

		using signatures_t = std::tuple<msg_0>;

	public:
		broker(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cyng::controller&
			, cache& cfg
			, std::string account
			, std::string pwd
			, std::string address
			, std::uint16_t port);

		cyng::continuation run();
		void stop(bool shutdown);

		/**
		 * @brief slot [0] - receive data
		 *
		 */
		cyng::continuation process(cyng::buffer_t, std::size_t);


	private:
		/**
		 * connect to server
		 */
		void do_connect(const boost::asio::ip::tcp::resolver::results_type& endpoints);
		void do_read();
		void do_write();
		void reset_write_buffer();

		void sig_ins(cyng::store::table const* tbl
			, cyng::table::key_type const& key
			, cyng::table::data_type const& data
			, std::uint64_t gen
			, boost::uuids::uuid source);

	private:
		cyng::async::base_task& base_;

		/**
		 * global logger
		 */
		cyng::logging::log_ptr logger_;

		/**
		 * configuration management
		 */
		cfg_wmbus cfg_;

		/**
		 * credentials & IP address
		 */
		std::string const account_;
		std::string const pwd_;
		std::string const host_;
		std::uint16_t const port_;

		/**
		 * connection socket
		 */
		boost::asio::ip::tcp::socket socket_;

		/**
		 * Buffer for incoming data.
		 */
		read_buffer_t buffer_read_;
		std::deque<cyng::buffer_t>	buffer_write_;

	};

}

#endif