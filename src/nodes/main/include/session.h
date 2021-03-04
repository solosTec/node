/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_MAIN_SESSION_H
#define SMF_MAIN_SESSION_H

#include <cyng/log/logger.h>
#include <cyng/io/parser/parser.h>

#include <memory>
#include <array>

namespace smf {

	class session : public std::enable_shared_from_this<session>
	{
	public:
		session(boost::asio::ip::tcp::socket socket, cyng::logger);
		~session();

		void start();
		void stop();

	private:
		void do_read();

	private:
		boost::asio::ip::tcp::socket socket_;
		cyng::logger logger_;

		/**
		 * Buffer for incoming data.
		 */
		std::array<char, 2048>	buffer_;

		cyng::io::parser parser_;
	};

}

#endif
