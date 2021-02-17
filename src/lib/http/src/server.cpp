/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/http/server.h>
#include <smf/http/session.h>

#include <cyng/log/record.h>

namespace smf
{
	namespace http
	{
		server::server(boost::asio::io_context& ioc, cyng::logger logger, std::string doc_root)
		: ioc_(ioc)
			, acceptor_(boost::asio::make_strand(ioc))
            , logger_(logger)
			, doc_root_(doc_root)
            , is_running_(false)
		{}

		bool server::start(boost::asio::ip::tcp::endpoint ep) {

            boost::beast::error_code ec;

            // Open the acceptor
            acceptor_.open(ep.protocol(), ec);
            if (ec) {
                CYNG_LOG_ERROR(logger_, "open: " << ec);
                return false;
            }

            // Allow address reuse
            acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
            if (ec) {
                CYNG_LOG_ERROR(logger_, "set_option: " << ec);
                return false;
            }

            // Bind to the server address
            acceptor_.bind(ep, ec);
            if (ec) {
                CYNG_LOG_ERROR(logger_, "bind: " << ec);
                return false;
            }

            // Start listening for connections
            acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
            if (ec) {
                CYNG_LOG_ERROR(logger_, "listen: " << ec);
                return false;
            }

            if (!is_running_.exchange(true)) {
                CYNG_LOG_INFO(logger_, "run: " << ep);
                run();
                return true;
            }

            CYNG_LOG_ERROR(logger_, "already running");
            return false;
		}

        void server::run() {


            // We need to be executing within a strand to perform async operations
            // on the I/O objects in this session. Although not strictly necessary
            // for single-threaded contexts, this example code is written to be
            // thread-safe by default.

            boost::asio::dispatch(
                acceptor_.get_executor(),
                boost::beast::bind_front_handler(
                    &server::do_accept, this));
        }

        void server::do_accept() {

            // The new connection gets its own strand
            acceptor_.async_accept(
                boost::asio::make_strand(ioc_),
                boost::beast::bind_front_handler(
                    &server::on_accept, this));
        }

        void server::on_accept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket) {

            if (ec) {
                CYNG_LOG_ERROR(logger_, "accept: " << ec);

                //
                //	remove running flag
                //
                is_running_.exchange(false);
            }
            else
            {
                // Create the http session and run it
                std::make_shared<session>(
                    std::move(socket),
                    logger_,
                    doc_root_)->run();
            }

            // Accept another connection
            do_accept();
        }

        void server::close() {
            if (is_running_) {
                boost::system::error_code ec;
                acceptor_.cancel(ec);
                acceptor_.close(ec);
            }
        }

	}	//	http
}



