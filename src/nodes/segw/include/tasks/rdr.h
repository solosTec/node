/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_RDR_SERVER_H
#define SMF_SEGW_RDR_SERVER_H

#include <cfg.h>
#include <config/cfg_lmn.h>

#include <cyng/log/logger.h>
#include <cyng/task/task_fwd.h>

namespace smf {
    namespace rdr {
        class server {

            template <typename T> friend class cyng::task;

            using signatures_t = std::tuple<
                std::function<void(std::chrono::seconds)>,           //	start
                std::function<void()>,                               //  pause
                std::function<void(boost::asio::ip::tcp::endpoint)>, //  rebind
                std::function<void(cyng::eod)>>;

          public:
            enum class type : std::uint8_t { ipv4, ipv6 } const srv_type_;

          public:
            server(
                cyng::channel_weak wp,
                cyng::controller &ctl,
                cfg &,
                cyng::logger,
                lmn_type,
                type,
                boost::asio::ip::tcp::endpoint ep);

#ifdef _DEBUG
            virtual ~server();
#endif

          private:
            void start(std::chrono::seconds);
            void stop(cyng::eod);
            void pause();
            void do_accept();
            void rebind(boost::asio::ip::tcp::endpoint ep);

          private:
            signatures_t sigs_;
            cyng::channel_weak channel_;
            cyng::controller &ctl_;
            cfg &cfg_;
            cyng::logger logger_;
            lmn_type const type_;
            boost::asio::ip::tcp::endpoint ep_;
            cyng::registry &registry_;
            boost::asio::ip::tcp::acceptor acceptor_;
            std::uint64_t session_counter_;
        };
    } // namespace rdr
} // namespace smf

#endif
