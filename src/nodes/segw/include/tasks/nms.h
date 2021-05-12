/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_NMS_SERVER_H
#define SMF_SEGW_NMS_SERVER_H

#include <cfg.h>

#include <cyng/log/logger.h>
#include <cyng/task/task_fwd.h>

namespace smf {
    namespace nms {

        /**
         * Listen to CM module and performs changes to the configuration or responds to
         * queries about the configuration.
         */
        class server {

            template <typename T> friend class cyng::task;

            using signatures_t = std::tuple<
                std::function<void(boost::asio::ip::tcp::endpoint ep)>, //	start
                std::function<void(cyng::eod)>>;

          public:
            server(cyng::channel_weak wp, cyng::controller &ctl, cfg &, cyng::logger);

          private:
            void start(boost::asio::ip::tcp::endpoint ep);
            void stop(cyng::eod);

            /**
             * start accepting incoming IP connects
             */
            void do_accept();

            /**
             * Allows a running session to modify the listener port and address
             */
            void rebind(boost::asio::ip::tcp::endpoint ep);

          private:
            signatures_t sigs_;
            cyng::channel_weak channel_;
            cfg &cfg_;
            cyng::logger logger_;
            boost::asio::ip::tcp::acceptor acceptor_;
            std::uint64_t session_counter_;
        };
    } // namespace nms
} // namespace smf

#endif
