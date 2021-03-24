/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/http/ws.h>

namespace smf {

    namespace http {

        ws::ws(boost::asio::ip::tcp::socket&& socket
            , boost::asio::io_context& ctx
            , cyng::logger logger
            , std::function<void(std::string)> on_msg)
        : ws_(std::move(socket))
            , strand_(ctx)
            , logger_(logger)
            , on_msg_(on_msg)
            , buffer_write_()
        {}

        void ws::on_accept(boost::beast::error_code ec)
        {
            if (ec) {
                CYNG_LOG_WARNING(logger_, "ws accept" << ec);
                return;
            }

            // Read a message
            do_read();
        }

        void ws::do_read()
        {
            // Read a message into our buffer
            ws_.async_read(
                buffer_,
                boost::beast::bind_front_handler(
                    &ws::on_read,
                    shared_from_this()));
        }

        void ws::on_read(boost::beast::error_code ec, std::size_t bytes_transferred)
        {
            boost::ignore_unused(bytes_transferred);

            // This indicates that the websocket_session was closed
            if (ec == boost::beast::websocket::error::closed) {
                CYNG_LOG_TRACE(logger_, "ws closed");
                on_msg_("{\"cmd\": \"exit\", \"reason\": \"ws closed\"}");
                return;
            }

            if (ec) {
                CYNG_LOG_WARNING(logger_, "ws read " << ec << ": " << ec.message());
                on_msg_("{\"cmd\": \"exit\", \"reason\": \"" + ec.message() + "\"}");
                return;
            }

            CYNG_LOG_DEBUG(logger_, "ws read [" 
                << ws_.next_layer().socket().remote_endpoint() 
                << "] "
                << boost::beast::buffer_bytes(buffer_.data())
                << " bytes");

            //
            //  extract JSON/text
            //
            auto const str = boost::beast::buffers_to_string(buffer_.data());

            CYNG_LOG_TRACE(logger_, "ws read ["
                << ws_.next_layer().socket().remote_endpoint()
                << "] "
                << str);

            //
            //  callback
            //
            on_msg_(str);

            // Echo the message
            //ws_.text(ws_.got_text());
            //ws_.async_write(
            //    buffer_.data(),
            //    boost::beast::bind_front_handler(
            //        &ws::on_write,
            //        shared_from_this()));

            // Clear buffer
            //ws_.text();
            //ws_.text(ws_.got_text());
            buffer_.consume(buffer_.size());

            //
            //	continue to read incoming data
            //
            do_read();

        }

        void ws::on_write(boost::beast::error_code ec,
            std::size_t bytes_transferred)
        {
            boost::ignore_unused(bytes_transferred);

            if (ec) {
                CYNG_LOG_WARNING(logger_, "ws write " << ec << ": " << ec.message());
                on_msg_("{\"cmd\": \"exit\", \"reason\": \"" + ec.message() + "\"}");
                return;
            }

            BOOST_ASSERT(!buffer_write_.empty());
            buffer_write_.pop_front();

            if (!buffer_write_.empty()) {
                do_write();
            }
        }

        void ws::push_msg(std::string msg) {

            boost::asio::post(strand_, [this, msg]() {

                bool const b = buffer_write_.empty();
                buffer_write_.push_back(msg);
                if (b)	do_write();

                });
        }

        void ws::do_write() {

            BOOST_ASSERT(!buffer_write_.empty());

            //
            //	write actually data to socket
            //
            ws_.async_write(
                boost::asio::buffer(buffer_write_.front(), buffer_write_.front().size()),
                boost::asio::bind_executor(strand_, std::bind(&ws::on_write, shared_from_this(),
                    std::placeholders::_1, std::placeholders::_2)));
        }

        // a function to select the most preferred protocol from a comma-separated list
        std::string select_protocol(boost::beast::string_view offered_tokens)
        {
            // tokenize the Sec-Websocket-Protocol header offered by the client
            boost::beast::http::token_list offered(offered_tokens);

            // an array of protocols supported by this server
            // in descending order of preference
            static const std::array<boost::beast::string_view, 3>
                supported = {
                "v9.smf",
                "v8.smf",
                "SMF"
            };

            std::string result;

            for (auto proto : supported)
            {
                auto iter = std::find(offered.begin(), offered.end(), proto);
                if (iter != offered.end())
                {
                    // we found a supported protocol in the list offered by the client
                    result.assign(proto.begin(), proto.end());
                    break;
                }
            }

            return result;
        };
    }
}
