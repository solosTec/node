/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_HTTP_WS_H
#define SMF_HTTP_WS_H

#include <smf.h>

#include <cyng/log/logger.h>
#include <cyng/log/record.h>

#include <string>
#include <memory>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>

namespace smf {
	namespace http {
		
        std::string select_protocol(boost::beast::string_view offered_tokens);

        // Echoes back all received WebSocket messages
        class ws : public std::enable_shared_from_this<ws>
        {
            boost::beast::websocket::stream<boost::beast::tcp_stream> ws_;
            boost::beast::flat_buffer buffer_;

        public:
            // Take ownership of the socket
            explicit ws(boost::asio::ip::tcp::socket&& socket, cyng::logger);

            // Start the asynchronous accept operation
            template<class Body, class Allocator>
            void do_accept(boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> req)
            {
                // Set suggested timeout settings for the websocket
                ws_.set_option(
                    boost::beast::websocket::stream_base::timeout::suggested(
                        boost::beast::role_type::server));

                //
                //	check subprotocols
                //
                std::string protocol = select_protocol(req[boost::beast::http::field::sec_websocket_protocol]);
                if (protocol.empty()) {

                    // none of our supported protocols were offered
                    boost::beast::http::response<boost::beast::http::string_body> res;
                    res.result(boost::beast::http::status::bad_request);
                    res.body() = "No valid sub-protocol was offered."
                        " This server implements"
                        " v9.smf,"
                        " and v8.smf";
                    boost::beast::http::write(ws_.next_layer(), res);
                    CYNG_LOG_WARNING(logger_, "ws request without subprotocol");
                }
                CYNG_LOG_DEBUG(logger_, "ws subprotocol: " << protocol);

                // Set a decorator to change the Server of the handshake
                ws_.set_option(boost::beast::websocket::stream_base::decorator(
                    [&protocol](boost::beast::websocket::response_type& res) {

                        res.set(boost::beast::http::field::server, std::string(SMF_VERSION_SUFFIX));
                        res.set(boost::beast::http::field::sec_websocket_protocol, protocol);

                     }));

                // Accept the websocket handshake
                ws_.async_accept(
                    req,
                    boost::beast::bind_front_handler(
                        &ws::on_accept,
                        shared_from_this()));
            }

        private:
            void on_accept(boost::beast::error_code ec);
            void do_read();
            void on_read(boost::beast::error_code ec,
                std::size_t bytes_transferred);
            void on_write(boost::beast::error_code ec,
                std::size_t bytes_transferred);

        private:
            cyng::logger logger_;
        };


	}
}

#endif
