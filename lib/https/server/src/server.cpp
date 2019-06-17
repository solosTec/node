/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/https/srv/server.h>
#include <smf/https/srv/detector.h>
#include <smf/cluster/generator.h>
#include <cyng/vm/controller.h>

namespace node
{
	namespace https
	{
		server::server(cyng::logging::log_ptr logger
			, boost::asio::io_context& ioc
			, boost::asio::ssl::context& ctx
			, boost::asio::ip::tcp::endpoint endpoint
			, std::size_t timeout
			, std::string const& doc_root
			, auth_dirs const& ad
			, std::set<boost::asio::ip::address> const& blacklist
			, std::map<std::string, std::string> const& redirects
			, cyng::controller& vm)
		: std::enable_shared_from_this<server>()
			, logger_(logger)
			, ctx_(ctx)
			, acceptor_(ioc)
#if (BOOST_BEAST_VERSION < 248)
			, socket_(ioc)
#else
			, ioc_(ioc)
#endif
			, connection_manager_(logger, vm, doc_root, ad, redirects, timeout)
			, blacklist_(blacklist)
			, is_listening_(false)
			, shutdown_complete_()
			, mutex_()
		{
			boost::system::error_code ec;

			// Open the acceptor
			acceptor_.open(endpoint.protocol(), ec);
			if (ec)	{
				CYNG_LOG_FATAL(logger_, "open: " << ec.message());
				return;
			}
			CYNG_LOG_TRACE(logger_, "open: " << endpoint.address().to_string() << ':' << endpoint.port());

			// Allow address reuse
			acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
			if (ec)	{
				CYNG_LOG_FATAL(logger_, "allow address reuse: " << ec.message());
				return;
			}

			// Bind to the server address
			if (!bind(endpoint, 16))	return;
			CYNG_LOG_TRACE(logger_, "bind: " << endpoint.address().to_string() << ':' << endpoint.port());

			// Start listening for connections
			acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
			if (ec)	{
				CYNG_LOG_FATAL(logger_, "listen: " << ec.message());
				return;
			}
		}

		connection_manager_interface& server::get_cm()
		{
			return connection_manager_;
		}

        bool server::bind(boost::asio::ip::tcp::endpoint ep, std::size_t retries)
		{
			boost::system::error_code ec;

			do
            {
                acceptor_.bind(ep, ec);
                if (ec)
                {
                    CYNG_LOG_FATAL(logger_, "bind: " << ec.message());
                    std::this_thread::sleep_for(std::chrono::seconds(10));
                }
            } while(ec && (retries-- > 0u));
			
            return !ec;
		}
		
		// Start accepting incoming connections
		bool server::run()
		{
			//
			//	set listener flag immediately to prevent
			//	a concurrent call of this same
			//	method
			//
			if (acceptor_.is_open() && !is_listening_.exchange(true))
			{
				do_accept();
			}
			return is_listening_;
		}

		void server::do_accept()
		{
#if (BOOST_BEAST_VERSION < 248)
			acceptor_.async_accept(socket_, std::bind(&server::on_accept, this, std::placeholders::_1));
#else
			acceptor_.async_accept(
				boost::asio::make_strand(ioc_),
				boost::beast::bind_front_handler(
					&server::on_accept,
					shared_from_this()));
#endif
		}

#if (BOOST_BEAST_VERSION < 248)
		void server::on_accept(boost::system::error_code ec)
#else
		void server::on_accept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket)
#endif
		{
			if (ec)	{
				CYNG_LOG_ERROR(logger_, "accept: " << ec.message());

				//
				//	remove listener flag
				//
				is_listening_.exchange(false);

				//
				//	notify
				//
				cyng::async::lock_guard<cyng::async::mutex> lk(mutex_);
				shutdown_complete_.notify_all();
			}
			else
			{
				//	since C++20 use contains()
#if (BOOST_BEAST_VERSION < 248)
				auto pos = blacklist_.find(socket_.remote_endpoint().address());
				if (pos != blacklist_.end()) {

					CYNG_LOG_WARNING(logger_, "address "
						<< socket_.remote_endpoint()
						<< " is blacklisted");
					socket_.close();
#else
				auto pos = blacklist_.find(socket.remote_endpoint().address());
				if (pos != blacklist_.end()) {

					CYNG_LOG_WARNING(logger_, "address "
						<< socket.remote_endpoint()
						<< " is blacklisted");
					socket.close();
#endif
					std::stringstream ss;
					ss
					 << "access from blacklisted address: "
					 << *pos
					 ;
					connection_manager_.vm().async_run(cyng::generate_invoke("log.msg.warning", ss.str()));

				}
				else {
#if (BOOST_BEAST_VERSION < 248)
					CYNG_LOG_TRACE(logger_, "accept "
						<< socket_.remote_endpoint());

					//	Create the session and run it
					connection_manager_.create_session(std::move(socket_), ctx_);
#else
					CYNG_LOG_TRACE(logger_, "accept "
						<< socket.remote_endpoint());

					//	Create the session and run it
					connection_manager_.create_session(std::move(socket), ctx_);
#endif
				}

				// Accept another connection
				do_accept();

			}
		}

		void server::close()
		{
			if (is_listening_)
			{
				cyng::async::unique_lock<cyng::async::mutex> lock(mutex_);

				acceptor_.cancel();
				acceptor_.close();

				CYNG_LOG_TRACE(logger_, "listener is waiting for cancellation");
				//	wait for cancellation of accept operation
				shutdown_complete_.wait(lock, [this] {
					return !is_listening_.load();
				});
				CYNG_LOG_TRACE(logger_, "listener cancellation complete");
			}
			else
			{
				CYNG_LOG_WARNING(logger_, "listener already closed");
			}

			//
			//	stop all connections
			//
			CYNG_LOG_INFO(logger_, "stop all connections");
			connection_manager_.stop_all();
		}
	}
}
