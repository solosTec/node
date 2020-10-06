/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_NMS_SESSION_H
#define NODE_SEGW_NMS_SESSION_H

#include <NODE_project_info.h>
#include <cyng/json/json_inc_parser.h>

#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/vm/controller_fwd.h>

namespace node
{
	namespace nms
	{
		class session : public std::enable_shared_from_this<session>
		{
			using read_buffer_t = std::array<char, NODE::PREFERRED_BUFFER_SIZE>;

		public:
			session(boost::asio::ip::tcp::socket socket
				, cyng::logging::log_ptr);
				//, cyng::controller&
				//, cyng::controller&);
			virtual ~session();

			session(session const&) = delete;
			session& operator=(session const&) = delete;

			void start();

		private:
			void do_read();
			void process_data(cyng::buffer_t&&);

		private:
			boost::asio::ip::tcp::socket socket_;
			cyng::logging::log_ptr logger_;
			//cyng::controller& cluster_;	//!< cluster bus VM
			//cyng::controller& vm_;	//!< session VM

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
			 * JSON parser
			 */
			cyng::json::parser parser_;

		};
	}
}

#endif
