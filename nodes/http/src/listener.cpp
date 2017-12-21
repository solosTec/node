/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#include "listener.h"
#include "session.h"
#include <stdexcept>
#include <boost/exception/all.hpp>

namespace node
{
	listener::listener(cyng::logging::log_ptr logger
		, boost::asio::io_context& ioc
		, boost::asio::ip::tcp::endpoint endpoint
		, std::string const& doc_root
		, mail_config const& mc
		, auth_dirs const& ad)
	: logger_(logger)
		, acceptor_(ioc)
		, socket_(ioc)
		, doc_root_(doc_root)
		, mail_config_(mc)
		, auth_(ad)
		, connection_manager_(logger)
		, is_listening_(false)
		, shutdown_complete_()
		, mutex_()
	{
		boost::system::error_code ec;

		// Open the acceptor
		acceptor_.open(endpoint.protocol(), ec);
		if(ec)
		{
			CYNG_LOG_FATAL(logger_, "open: " << ec);
			return;
		}
		CYNG_LOG_INFO(logger_, "open: " << endpoint);	
        acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));			

		// Bind to the server address
        //	could throw boost::system::system_error: permission denied
		acceptor_.bind(endpoint, ec);
		if(ec)
		{
//  			std::cerr << endpoint << " -- " << ec.message() << std::endl;
			CYNG_LOG_FATAL(logger_, "bind: " 
			<< endpoint
			<< " -- "
			<< ec
			<< " -- "
			<< ec.message());

			return;
		}
		CYNG_LOG_INFO(logger_, "bind: " << endpoint);	

		// Start listening for connections
		acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
		if(ec)
		{
			CYNG_LOG_FATAL(logger_, "listen: " << ec);
			return;
		}
				
		CYNG_LOG_INFO(logger_, "listen: " << endpoint);	
	}

	// Start accepting incoming connections
	bool listener::run()
	{
		//
		//	set listener flag immediately to prevent
		//	a concurrent call of this same
		//	method
		//
		if(acceptor_.is_open() && !is_listening_.exchange(true))
		{
			do_accept();
		}
		else 
		{
			CYNG_LOG_WARNING(logger_, "acceptor closed");
		}
		return is_listening_;
	}
	
	void listener::close()
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

	void listener::do_accept()
	{
		acceptor_.async_accept(
			socket_,
			std::bind(
				&listener::on_accept,
				shared_from_this(),
				std::placeholders::_1));
	}

	void listener::on_accept(boost::system::error_code ec)
	{
		if(ec)
		{
//  			std::cerr << ec.message() << std::endl;
			CYNG_LOG_WARNING(logger_, "accept: " 
			<< ec
			<< " - "
			<< ec.message());
			
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
			connection_manager_.start(std::make_shared<http_session>(
				logger_,
				connection_manager_,
				std::move(socket_),
				doc_root_,
				mail_config_,
				auth_));
		}

		// Accept another connection
		do_accept();
	}
}

