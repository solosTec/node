/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#include <smf/http/srv/server.h>
#include <smf/http/srv/session.h>

namespace node
{
	namespace http
	{

		server::server(cyng::logging::log_ptr logger
			, boost::asio::io_context& ioc
			, boost::asio::ip::tcp::endpoint endpoint
			, std::string const& doc_root
			, node::bus::shared_type bus
			, cyng::store::db& cache)
		: acceptor_(ioc)
			, socket_(ioc)
			, logger_(logger)
			, doc_root_(doc_root)
			, bus_(bus)
			, cache_(cache)
			, connection_manager_(logger, bus)
			, is_listening_(false)
			, shutdown_complete_()
			, mutex_()
			, rgn_()
		{
			boost::system::error_code ec;

			// Open the acceptor
			acceptor_.open(endpoint.protocol(), ec);
			if (ec)
			{
				CYNG_LOG_FATAL(logger_, "open: " << ec.message());
				return;
			}
			CYNG_LOG_TRACE(logger_, "open: " << endpoint.address().to_string() << ':' << endpoint.port());

			// Allow address reuse
			acceptor_.set_option(boost::asio::socket_base::reuse_address(true));
			if (ec)
			{
				CYNG_LOG_FATAL(logger_, "allow address reuse: " << ec.message());
				//BOOST_ASSERT_MSG(!ec, "SET_OPTION");
				return;
			}

			// Bind to the server address
			if (!bind(endpoint, 16))	return;
			CYNG_LOG_TRACE(logger_, "bind: " << endpoint.address().to_string() << ':' << endpoint.port());

			// Start listening for connections
			acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
			if (ec)
			{
				CYNG_LOG_FATAL(logger_, "listen: " << ec.message());
				return;
			}
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
			else
			{
				CYNG_LOG_WARNING(logger_, "acceptor closed");
			}
			return is_listening_;
		}

		void server::do_accept()
		{
			acceptor_.async_accept(
				socket_,
				std::bind(
					&server::on_accept,
					this,
					std::placeholders::_1));
		}

		void server::on_accept(boost::system::error_code ec)
		{
			if (ec)
			{
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

				return;
			}
			else
			{
				CYNG_LOG_TRACE(logger_, "accept: " << socket_.remote_endpoint());

				// Create the http_session and run it
				connection_manager_.start(make_http_session(logger_
					, connection_manager_
					, std::move(socket_)
					, doc_root_
					, bus_
					, rgn_()));


				// Accept another connection
				do_accept();

			}
		}

		void server::close()
		{
			if (is_listening_)
			{
				acceptor_.cancel();
				acceptor_.close();

				cyng::async::unique_lock<cyng::async::mutex> lock(mutex_);

				CYNG_LOG_TRACE(logger_, "listener is waiting for cancellation");
				//	wait for cancellation of accept operation
				shutdown_complete_.wait(lock, [this] {
					return !is_listening_.load();
				});
				CYNG_LOG_TRACE(logger_, "listener cancellation complete");

				//
				//	stop all connections
				//
				CYNG_LOG_INFO(logger_, "stop all connections");
				connection_manager_.stop_all();

			}
		}

		bool server::send_msg(boost::uuids::uuid tag, std::string const& msg)
		{
			return connection_manager_.send_msg(tag, msg);
		}

		void server::add_channel(boost::uuids::uuid tag, std::string const& channel)
		{
			connection_manager_.add_channel(tag, channel);
		}

		void server::process_event(std::string const& channel, std::string const& msg)
		{
			connection_manager_.process_event(channel, msg);
		}

		void server::send_moved(boost::uuids::uuid tag, std::string const& location)
		{
			connection_manager_.send_moved(tag, location);
		}

		void server::trigger_download(boost::uuids::uuid tag, std::string const& filename, std::string const& attachment)
		{
			connection_manager_.trigger_download(tag, filename, attachment);
		}

	}
}
