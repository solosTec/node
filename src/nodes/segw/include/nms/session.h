/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_NMS_SESSION_H
#define SMF_SEGW_NMS_SESSION_H

#include <cfg.h>
#include <nms/reader.h>

#include <cyng/log/logger.h>
#include <cyng/parse/json/json_parser.h>

#include <memory>
#include <array>

namespace smf {
	namespace nms {

		class session : public std::enable_shared_from_this<session>
		{
		public:
			session(boost::asio::ip::tcp::socket socket, cfg&, cyng::logger, std::function<void(boost::asio::ip::tcp::endpoint ep)>);

			void start();

		private:
			void do_read();
			void send_response(cyng::param_map_t&&);

		private:
			boost::asio::ip::tcp::socket socket_;
			cyng::logger logger_;

			/**
			 * Buffer for incoming data.
			 */
			std::array<char, 2048>	buffer_;

			/**
			 * convert incoming commands into updates or queries
			 * into the configuration.
			 */
			reader	reader_;

			/**
			 * JSON parser
			 */
			cyng::json::parser	parser_;

		};
	}
}

#endif