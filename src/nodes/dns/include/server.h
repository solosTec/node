/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Sylko Olzscher
 *
 */
#ifndef SMF_DNS_SERVER_H
#define SMF_DNS_SERVER_H

#include <cyng/log/logger.h>
#include <cyng/task/controller.h>
// #include <cyng/net/server_proxy.h>
// #include <cyng/obj/intrinsics/eod.h>
// #include <cyng/task/task_fwd.h>
// #include <cyng/vm/mesh.h>
//
// #include <array>
// #include <functional>
// #include <memory>
// #include <string>
// #include <tuple>
//
// #include <boost/uuid/uuid.hpp>
#include <boost/asio.hpp>

namespace smf {
    namespace dns {
        class server {

          public:
            server(cyng::controller &ctl, cyng::logger, boost::asio::ip::udp::endpoint);
            ~server() = default;

            void start();
            void stop();

          private:
            void handle_receive(const boost::system::error_code &error, std::size_t bytes_transferred);

          private:
            cyng::controller &ctl_;
            cyng::logger logger_;

            /**
             * DNS server
             */
            boost::asio::ip::udp::socket socket_;
            boost::asio::ip::udp::endpoint remote_endpoint_;
            std::array<char, 1024> recv_buffer_;
        };

    } // namespace dns
} // namespace smf

#endif
