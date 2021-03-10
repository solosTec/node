/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_REDIRECTOR_SESSION_H
#define SMF_SEGW_REDIRECTOR_SESSION_H


#include <cyng/log/logger.h>

#include <memory>
#include <array>

namespace smf {
	namespace rdr {

		class session : public std::enable_shared_from_this<session>
		{
		public:
			session(boost::asio::ip::tcp::socket socket, cyng::logger);

			void start();

		private:
			void do_read();

		private:
			boost::asio::ip::tcp::socket socket_;
			cyng::logger logger_;

			/**
			 * Buffer for incoming data.
			 */
			std::array<char, 2048>	buffer_;
		};
	}
}

#endif