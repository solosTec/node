/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_REDIRECTOR_SERVER_H
#define SMF_SEGW_REDIRECTOR_SERVER_H

#include <cfg.h>

#include <cyng/log/logger.h>
#include <cyng/task/task_fwd.h>

namespace smf {
	namespace rdr {
		class server
		{
		public:
			server(cyng::controller& ctl, cfg&, cyng::logger);

			void start(boost::asio::ip::tcp::endpoint ep);
			void stop();

		private:
			void do_accept();

		private:
			cfg& cfg_;
			cyng::logger logger_;
			boost::asio::ip::tcp::acceptor acceptor_;
		};
	}
}

#endif