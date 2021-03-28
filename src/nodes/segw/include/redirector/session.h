/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_REDIRECTOR_SESSION_H
#define SMF_SEGW_REDIRECTOR_SESSION_H

#include <cfg.h>
#include <config/cfg_lmn.h>
#include <config/cfg_listener.h>

#include <cyng/log/logger.h>
#include <cyng/task/task_fwd.h>

#include <memory>
#include <array>

namespace smf {
	namespace rdr {

		class session : public std::enable_shared_from_this<session>
		{
		public:
			session(boost::asio::ip::tcp::socket socket, cyng::registry&, cfg&, cyng::logger, lmn_type);

			void start(cyng::controller&);

		private:
			void do_read();
			void do_write(cyng::buffer_t);
			//void handle_write(const boost::system::error_code& ec);

		private:
			boost::asio::ip::tcp::socket socket_;
			cyng::registry& registry_;
			cyng::logger logger_;
			cfg_listener cfg_;

			/**
			 * Buffer for incoming data.
			 */
			std::array<char, 2048>	buffer_;
			/**
			 * Buffer for outgoing data.
			 */
			//std::deque<cyng::buffer_t>	buffer_write_;

			cyng::channel_ptr channel_;
		};
	}
}

#endif