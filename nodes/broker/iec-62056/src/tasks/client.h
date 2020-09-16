/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_IEC_62056_TASK_CLIENT_H
#define NODE_IEC_62056_TASK_CLIENT_H

#include <NODE_project_info.h>

#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/policy.h>
#include <cyng/store/db.h>

namespace node
{

	class client
	{
		using read_buffer_t = std::array<char, NODE::PREFERRED_BUFFER_SIZE>;

	public:
		//using msg_0 = std::tuple<cyng::version>;
		using msg_1 = std::tuple<>;
		using signatures_t = std::tuple<msg_1>;

	public:
		client(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cyng::store::db&
			, boost::asio::ip::tcp::endpoint
			, std::chrono::seconds );
		cyng::continuation run();
		void stop(bool shutdown);

		/**
		 * @brief slot [0]
		 *
		 * sucessful cluster login
		 */
		//cyng::continuation process(cyng::version const&);

		/**
		 * @brief slot [1]
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

	private:
		/**
		 * communication bus to master
		 */
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;

		/**
		 * global data cache
		 */
		cyng::store::db& cache_;

		boost::asio::ip::tcp::endpoint const ep_;
		std::chrono::seconds const monitor_;

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
