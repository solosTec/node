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
		, boost::uuids::uuid mtag // master tag
		, cyng::store::db& db
		, std::string const& account
		, std::string const& pwd
		, boost::uuids::uuid stag
		, std::chrono::seconds monitor //	cluster watchdog
		, std::chrono::seconds connection_open_timeout
		, std::chrono::seconds connection_close_timeout
		, bool connection_auto_login
		, bool connection_auto_enabled
		, bool connection_superseed)
	: socket_(std::move(socket))
		, logger_(logger)
		, buffer_()
		, session_(make_session(mux
			, logger
			, mtag
			, db
			, account
			, pwd
			, stag
			, monitor
			, connection_open_timeout
			, connection_close_timeout
			, connection_auto_login
			, connection_auto_enabled
			, connection_superseed))
		, serializer_(socket_, this->get_session()->vm_)
	{
		//
		//	register socket operations to session VM
		//
		cyng::register_socket(socket_, get_session()->vm_);

        get_session()->vm_.register_function("push.session", 0, std::bind(&connection::push_session, this, std::placeholders::_1));

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
		CYNG_LOG_TRACE(logger_, "DO READ");
		auto self(shared_from_this());
		socket_.async_read_some(boost::asio::buffer(buffer_),
			[this, self](boost::system::error_code ec, std::size_t bytes_transferred)
			{
				CYNG_LOG_TRACE(logger_, "READ SOME");
				if (!ec)
				{
#ifdef SMF_IO_DEBUG
					cyng::io::hex_dump hd;
					std::stringstream ss;
					hd(ss, buffer_.data(), buffer_.data() + bytes_transferred);
					CYNG_LOG_TRACE(logger_, "cluster connection received " << ss.str());
#endif

					const std::size_t count = get_session()->parser_.read(buffer_.data(), buffer_.data() + bytes_transferred);
					CYNG_LOG_TRACE(logger_, "cluster connection "
						<< get_session()->vm_.tag()
						<< " received "
						<< bytes_transferred
						<< " bytes -> "
						<< count
						<< " ops");
					do_read();
				}
				else if (ec != boost::asio::error::operation_aborted)
				{
					CYNG_LOG_WARNING(logger_, "read <" << ec << ':' << ec.value() << ':' << ec.message() << '>');

					//
					//	session cleanup - hold a reference of the session
					//
					//get_session()->vm_.async_run(cyng::generate_invoke("session.cleanup", cyng::invoke("push.session"), ec));
					get_session()->vm_.access([&ec, self](cyng::vm& vm) {
						vm.run(cyng::generate_invoke("session.cleanup", cyng::invoke("push.session"), ec));
					});
				}
				else
				{
					//
					//	The session was closed intentionally.
					//	At this point nothing more is to do. Service is going down and all session have to be stopped fast.
					//
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
		CYNG_LOG_DEBUG(logger_, "push session " 
			<< get_session()->vm_.tag()
			<< " on stack");
		ctx.push(session_);
	}

}




