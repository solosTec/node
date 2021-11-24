/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_MODEM_SERVER_H
#define SMF_MODEM_SERVER_H

#include <smf/cluster/bus.h>

#include <cyng/log/logger.h>
#include <cyng/obj/intrinsics/eod.h>
#include <cyng/vm/mesh.h>

#include <functional>
#include <memory>
#include <string>
#include <tuple>

#include <boost/uuid/uuid.hpp>

namespace smf {

    class modem_session;
    class modem_server {
        friend class modem_session;
        using blocklist_type = std::vector<boost::asio::ip::address>;

      public:
        modem_server(
            boost::asio::io_context &,
            cyng::logger,
            bool auto_answer,
            std::chrono::milliseconds guard,
            std::chrono::seconds timeout, //  gatekeeper
            bus &,
            cyng::mesh &fabric);
        ~modem_server();

        // void stop(cyng::eod);
        void listen(boost::asio::ip::tcp::endpoint);

      private:
        /**
         * incoming connection
         */
        void do_accept();

      private:
        cyng::logger logger_;

        bool const auto_answer_;
        std::chrono::milliseconds const guard_;
        std::chrono::seconds const timeout_; //  gatekeeper

        bus &cluster_bus_;
        cyng::mesh &fabric_;

        boost::asio::ip::tcp::acceptor acceptor_;
        std::uint64_t session_counter_;
    };

} // namespace smf

#endif
