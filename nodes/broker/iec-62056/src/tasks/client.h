/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_IEC_62056_TASK_CLIENT_H
#define NODE_IEC_62056_TASK_CLIENT_H

#include <NODE_project_info.h>
#include <smf/iec/parser.h>

#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/policy.h>
#include <cyng/store/db.h>
#include <cyng/vm/controller_fwd.h>

namespace node
{

	class client
	{
		using read_buffer_t = std::array<char, NODE::PREFERRED_BUFFER_SIZE>;

	public:
		using msg_1 = std::tuple<>;
		using signatures_t = std::tuple<msg_1>;

	public:
		client(cyng::async::base_task* bt
			, cyng::controller& cluster
			, cyng::controller& vm
			, cyng::logging::log_ptr
			, cyng::store::db&
			, boost::asio::ip::tcp::endpoint
			, std::chrono::seconds 
			, std::string const& meter
			, bool client_login
			, bool verbose);
		cyng::continuation run();
		void stop(bool shutdown);

		/**
		 * @brief slot [0]
		 *
		 * reconnect
		 */
		cyng::continuation process();

	private:
		/**
		 * connect to server
		 */
		void do_connect(const boost::asio::ip::tcp::resolver::results_type& endpoints);
		void do_read();
		void do_write();
		void reset_write_buffer();

		void data_start(cyng::context& ctx);
		void data_line(cyng::context& ctx);
		void data_bcc(cyng::context& ctx);
		void data_eof(cyng::context& ctx);

	private:
		cyng::async::base_task& base_;
		/**
		 * communication bus to master
		 */
		cyng::controller& cluster_;
		cyng::controller& vm_;
		cyng::logging::log_ptr logger_;

		/**
		 * global data cache
		 */
		cyng::store::db& cache_;

		boost::asio::ip::tcp::endpoint const ep_;
		std::chrono::seconds const monitor_;

		std::string const meter_;

		bool const client_login_;

		/**
		 * connection socket
		 */
		boost::asio::ip::tcp::socket socket_;

		/**
		 * IEC parser
		 */
		iec::parser parser_;

		/**
		 * Buffer for incoming data.
		 */
		read_buffer_t buffer_read_;
		std::deque<cyng::buffer_t>	buffer_write_;


	};
	
}

#endif
