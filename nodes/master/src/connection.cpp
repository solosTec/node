/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#include "connection.h"
#include <cyng/vm/domain/asio_domain.h>
#ifdef SMF_IO_DEBUG
#include <cyng/io/hex_dump.hpp>
#endif
#include <cyng/vm/generator.h>
#include <cyng/object_cast.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace node 
{
	connection::connection(boost::asio::ip::tcp::socket&& socket
		, cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, cyng::store::db& db
		, std::string const& account
		, std::string const& pwd
		, std::chrono::seconds const& monitor)
	: socket_(std::move(socket))
	, logger_(logger)
	, buffer_()
	, session_(make_session(mux, logger, db, account, pwd, monitor))
	, serializer_(socket_, this->get_session()->vm_)
	{
		//
		//	register socket operations to session VM
		//
		cyng::register_socket(socket_, get_session()->vm_);

		get_session()->vm_.async_run(cyng::register_function("push.session", 0, std::bind(&connection::push_session, this, std::placeholders::_1)));

	}
		
	void connection::start()
	{
		do_read();
	}

	void connection::stop()
	{
		socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
		socket_.close();
	}
	
	void connection::do_read()
	{
		auto self(shared_from_this());
		socket_.async_read_some(boost::asio::buffer(buffer_),
			[this, self](boost::system::error_code ec, std::size_t bytes_transferred)
			{
				if (!ec)
				{
					//CYNG_LOG_TRACE(logger_, bytes_transferred << " bytes read");
					get_session()->vm_.async_run(cyng::generate_invoke("log.msg.trace", "cluster connection received", bytes_transferred, "bytes"));

#ifdef SMF_IO_DEBUG
					cyng::io::hex_dump hd;
					std::stringstream ss;
					hd(ss, buffer_.data(), buffer_.data() + bytes_transferred);
					CYNG_LOG_TRACE(logger_, "cluster connection received " << ss.str());
#endif

					get_session()->parser_.read(buffer_.data(), buffer_.data() + bytes_transferred);
					do_read();
				}
				else //if (ec != boost::asio::error::operation_aborted)
				{
					CYNG_LOG_WARNING(logger_, "read <" << ec << ':' << ec.value() << ':' << ec.message() << '>');

					//
					//	session cleanup - hold a reference of the session
					//
					get_session()->vm_.run(cyng::generate_invoke("session.cleanup", cyng::invoke("push.session"), ec));
				}
			});
	}

	session* connection::get_session()
	{
		return const_cast<session*>(cyng::object_cast<session>(session_));
	}
	session const* connection::get_session() const
	{
		return cyng::object_cast<session>(session_);
	}

	void connection::push_session(cyng::context& ctx)
	{
		CYNG_LOG_TRACE(logger_, "push session " 
			<< get_session()->hash()
			<< " on stack");
		ctx.push(session_);
	}

}




