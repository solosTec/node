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
#include <cyng/intrinsics/buffer.h>

namespace node
{
	class cache;
	namespace nms
	{
		/**
		 * Purpose of this class is to read the incoming
		 * JSON and execute the specified commands.
		 */
		class reader 
		{

		public:
			reader(cyng::logging::log_ptr logger, cache&);
			void run(cyng::param_map_t&&);

		private:
			cyng::logging::log_ptr logger_;
			cache& cache_;
		};

		class session : public std::enable_shared_from_this<session>
		{
			using read_buffer_t = std::array<char, NODE::PREFERRED_BUFFER_SIZE>;

		public:
			session(boost::asio::ip::tcp::socket socket
				, cyng::logging::log_ptr
				, cache&);
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

			reader reader_;

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
