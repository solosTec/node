/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_REDIRECTOR_SESSION_H
#define NODE_SEGW_REDIRECTOR_SESSION_H

#include <cfg_rs485.h>
#include <NODE_project_info.h>

#include <cyng/log.h>
#include <cyng/intrinsics/buffer.h>
#include <cyng/async/task_fwd.h>

namespace node
{
	class cache;
	namespace redirector
	{

		class session : public std::enable_shared_from_this<session>
		{
			using read_buffer_t = std::array<char, NODE::PREFERRED_BUFFER_SIZE>;

		public:
			session(boost::asio::ip::tcp::socket socket
				, cyng::async::mux&
				, cyng::logging::log_ptr
				, cache&);
			virtual ~session();

			session(session const&) = delete;
			session& operator=(session const&) = delete;

			void start();

		private:
			void do_read();
			void process_data(cyng::buffer_t&&);
			void send_response(cyng::param_map_t&&);

		private:
			boost::asio::ip::tcp::socket socket_;
			cyng::async::mux& mux_;
			cyng::logging::log_ptr logger_;

			/**
			 * Buffer for incoming data.
			 */
			read_buffer_t buffer_;

			/**
			 * authorization state
			 */
			bool authorized_;

			/**
			 * temporary data buffer
			 */
			cyng::buffer_t data_;

			/**
			 * in/out byte counter
			 */
			std::uint64_t rx_, sx_;

			/**
			 * read/write RS 485 configuration
			 */
			cfg_rs485 rs485_;

			std::size_t tsk_lmn_;
			std::size_t tsk_reflux_;
		};
	}
}

#endif
