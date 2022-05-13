/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_SML_SERVER_H
#define SMF_SEGW_SML_SERVER_H

#include <cfg.h>

#include <cyng/log/logger.h>
#include <cyng/task/task_fwd.h>

namespace smf {
    namespace sml {
        class server {
          public:
            server(cyng::logger, cyng::controller &ctl, cfg &);

            void start(boost::asio::ip::tcp::endpoint ep);
            void stop();

          private:
            void do_accept();

          private:
            cyng::logger logger_;
            cfg &cfg_;
            boost::asio::ip::tcp::acceptor acceptor_;
        };
    } // namespace sml
} // namespace smf

#endif
