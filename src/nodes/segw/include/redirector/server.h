/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_REDIRECTOR_SERVER_H
#define SMF_SEGW_REDIRECTOR_SERVER_H

#include <cfg.h>
#include <config/cfg_lmn.h>

#include <cyng/log/logger.h>
#include <cyng/task/task_fwd.h>

namespace smf {
	namespace rdr {
		class server
		{
		public:
			server(cyng::controller& ctl, cfg&, cyng::logger, lmn_type);

			void start(boost::asio::ip::tcp::endpoint ep);
			void stop();

		private:
			void do_accept();

		private:
			cfg& cfg_;
			cyng::logger logger_;
			lmn_type const type_;
			cyng::registry& registry_;
			boost::asio::ip::tcp::acceptor acceptor_;
			std::uint64_t session_counter_;
		};
	}
}

#endif