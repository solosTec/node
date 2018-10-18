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
			, std::string const& doc_root
			, std::set<boost::asio::ip::address> const& blacklist
			, cyng::controller& vm)
		: logger_(logger)
			, ctx_(ctx)
			, acceptor_(ioc)
			, socket_(ioc)
			, connection_manager_(logger, vm, doc_root)
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

			// Allow address reuse
			acceptor_.set_option(boost::asio::socket_base::reuse_address(true));
			if (ec)	{
				CYNG_LOG_FATAL(logger_, "allow address reuse: " << ec.message());
				return;
			}

			// Bind to the server address
			acceptor_.bind(endpoint, ec);
			if (ec)	{
				CYNG_LOG_FATAL(logger_, "bind: " << ec.message());
				return;
			}

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
			acceptor_.async_accept(socket_, std::bind(&server::on_accept, this, std::placeholders::_1));
		}

		void server::on_accept(boost::system::error_code ec)
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
				auto pos = blacklist_.find(socket_.remote_endpoint().address());
				if (pos != blacklist_.end()) {

					CYNG_LOG_WARNING(logger_, "address "
						<< socket_.remote_endpoint()
						<< " is blacklisted");
					socket_.close();
					
					std::stringstream ss;
					ss
					 << "access from blacklisted address: "
					 << *pos
					 ;
					connection_manager_.vm().async_run(bus_insert_msg(cyng::logging::severity::LEVEL_WARNING, ss.str()));
					
				}
				else {
					CYNG_LOG_TRACE(logger_, "accept "
						<< socket_.remote_endpoint());

					//	Create the session and run it

					connection_manager_.create_session(std::move(socket_), ctx_);
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
			//CYNG_LOG_INFO(logger_, "stop all connections");
			//connection_manager_.stop_all();

		}


	}
}
