#include <server.h>

#include <cyng/log/record.h>
// #include <cyng/obj/util.hpp>
// #include <cyng/task/channel.h>
//
// #include <iostream>
//
#include <boost/bind/bind.hpp>

#ifdef _DEBUG_DNS
#include <cyng/io/hex_dump.hpp>
#include <iostream>
#include <sstream>
#endif

namespace smf {
    namespace dns {
        server::server(cyng::controller &ctl, cyng::logger logger, boost::asio::ip::udp::endpoint ep)
            : ctl_(ctl)
            , logger_(logger)
            , socket_(ctl_.get_ctx(), ep) {}

        void server::start() {
            //
            //  create the DNS server (udp)
            //
            socket_.async_receive_from(
                boost::asio::buffer(recv_buffer_),
                remote_endpoint_,
                boost::bind(
                    &server::handle_receive, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
        }

        void server::handle_receive(const boost::system::error_code &error, std::size_t bytes_transferred) {
            CYNG_LOG_TRACE(logger_, "[dns] received " << bytes_transferred << " bytes");

#ifdef _DEBUG_DNS
            {
                std::stringstream ss;
                cyng::io::hex_dump<8> hd;
                hd(ss, recv_buffer_.data(), recv_buffer_.data() + bytes_transferred);
                auto const dmp = ss.str();
                CYNG_LOG_DEBUG(logger_, "[" << remote_endpoint_ << "] " << bytes_transferred << " bytes DNS data:\n" << dmp);
                //  example:
                // [0000]  f3 94 01 20 00 01 00 00  00 00 00 01 08 6e 6f 72  ... .... .....nor
                // [0010]  6d 61 6e 64 79 03 63 64  6e 07 6d 6f 7a 69 6c 6c  mandy.cd n.mozill
                // [0020]  61 03 6e 65 74 00 00 01  00 01 00 00 29 04 b0 00  a.net... ....)...
                // [0030]  00 00 00 00 00                                    .....
            }
#endif
            //
            //  continue reading
            //
            start();
        }

        void server::stop() {
            CYNG_LOG_WARNING(logger_, "stop dns server");
            boost::system::error_code ec;
            socket_.close(ec);
        }

    } // namespace dns
} // namespace smf
