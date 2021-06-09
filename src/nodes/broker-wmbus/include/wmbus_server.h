/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_WMBUS_SERVER_H
#define SMF_WMBUS_SERVER_H

#include <db.h>

#include <cyng/log/logger.h>
#include <cyng/obj/intrinsics/eod.h>
#include <cyng/task/controller.h>

#include <functional>
#include <memory>
#include <string>
#include <tuple>

#include <boost/uuid/uuid.hpp>

namespace smf {

    class bus;
    class wmbus_server {
        using blocklist_type = std::vector<boost::asio::ip::address>;

      public:
        wmbus_server(
            cyng::controller &ctl,
            cyng::logger,
            bus &,
            std::shared_ptr<db>,
            std::chrono::seconds client_timeout,
            std::filesystem::path client_out);
        ~wmbus_server();

        void stop(cyng::eod);
        void listen(boost::asio::ip::tcp::endpoint);

      private:
        /**
         * incoming connection
         */
        void do_accept();

      private:
        cyng::controller &ctl_;
        cyng::logger logger_;
        bus &bus_;
        std::shared_ptr<db> db_;
        std::chrono::seconds const client_timeout_;
        std::filesystem::path const client_out_;
        boost::asio::ip::tcp::acceptor acceptor_;
        std::uint64_t session_counter_;
    };

} // namespace smf

#endif
