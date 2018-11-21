/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#include <smf/cluster/session_stub.h>
#include <cyng/vm/domain/log_domain.h>
#include <cyng/vm/domain/asio_domain.h>
#ifdef SMF_IO_DEBUG
#include <cyng/io/hex_dump.hpp>
#endif

namespace node 
{
	session_stub::session_stub(boost::asio::ip::tcp::socket&& socket
		, cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, bus::shared_type bus
		, boost::uuids::uuid tag
		, std::chrono::seconds const& timeout)
	: socket_(std::move(socket))
		, buffer_()
		, pending_(false)
		, mux_(mux)
		, logger_(logger)
		, bus_(bus)
		, vm_(mux.get_io_service(), tag)
	{
		//
		//	register logger domain
		//
		cyng::register_logger(logger_, vm_);
		vm_.async_run(cyng::generate_invoke("log.msg.info", "log domain is running"));

		//
		//	register socket operations to session VM
		//
		cyng::register_socket(socket_, vm_);

		vm_.register_function("session.state.pending", 0, [&](cyng::context&) {
			//
			//	set session state => PENDING
			//
			BOOST_ASSERT(!pending_);
			pending_ = true;
			CYNG_LOG_DEBUG(logger_, "session.state.pending ON");
		});
	}

	session_stub::~session_stub()
	{
		//CYNG_LOG_DEBUG(logger_, "deconstruct connection(" << tag_ << ')');
		BOOST_ASSERT_MSG(vm_.is_halted(), "session not in HALT state");
	}

	cyng::controller& session_stub::vm()
	{
		return vm_;
	}

	cyng::controller const& session_stub::vm() const
	{
		return vm_;
	}

	std::size_t session_stub::hash() const noexcept
	{
		return vm_.hash();
	}

	void session_stub::start()
	{
		do_read();
	}

	void session_stub::close()
	{
		if (socket_.is_open()) {

			//
			//  close socket
			//
			CYNG_LOG_DEBUG(logger_, vm_.tag()
				<< " close socket "
				<< socket_.remote_endpoint());

			boost::system::error_code ec;
			socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
			socket_.close(ec);
		}
	}

	void session_stub::stop(boost::system::error_code ec)
	{
		//
		//	stop all tasks and halt VM
		//
		shutdown();

		//
		//	wait for pending operations
		//
		if (vm_.wait(12, std::chrono::milliseconds(100)))
		{
			if (pending_) {
				//
				//	tell master to close *this* client
				//
				bus_->vm_.async_run(client_req_close(vm_.tag(), ec.value()));
			}
			else {
				//
				//	remove from connection map - call destructor
				//	server::remove_client();
				//
				bus_->vm_.async_run(cyng::generate_invoke("server.remove.client", vm_.tag()));
			}
		}
		else {
			CYNG_LOG_FATAL(logger_, "shutdown (res) failed " << vm_.tag());
		}
	}

	void session_stub::shutdown()
	{
		if (socket_.is_open()) {
			CYNG_LOG_WARNING(logger_, "socket still open on session " << vm_.tag());
		}

		//
		//	gracefull shutdown
		//	device/party closed connection or network shutdown
		//
		CYNG_LOG_TRACE(logger_, vm_.tag() << " halt VM");
		vm_.halt();
	}

	void session_stub::do_read()
	{
		socket_.async_read_some(boost::asio::buffer(buffer_),
			[this](boost::system::error_code ec, std::size_t bytes_transferred)
		{
			if (!ec)
			{
				//vm_.async_run(cyng::generate_invoke("log.msg.trace", "ipt connection received", bytes_transferred, "bytes"));
				CYNG_LOG_TRACE(logger_, "session " 
					<< vm_.tag() 
					<< " received "
					<< bytes_transferred
					<< " bytes");

				//
				//	buffer contains the unscrambled input
				//
				const auto buf = parse(buffer_.cbegin(), buffer_.cbegin() + bytes_transferred);

#ifdef SMF_IO_DEBUG
				//cyng::io::hex_dump hd;
				//std::stringstream ss;
				//hd(ss, buf.cbegin(), buf.cbegin());
				//CYNG_LOG_TRACE(logger_, "session input dump \n" << ss.str());
#endif

				do_read();
			}


			else if (ec != boost::asio::error::operation_aborted)
			{
				//
				//	device/party closed connection or network shutdown
				//
				CYNG_LOG_WARNING(logger_, "session "
					<< vm_.tag()
					<< " closed: "
					<< ec
					<< ':'
					<< ec.value()
					<< " ["
					<< ec.message()
					<< ']');

				//
				//	master has to acknowledge that session is closed
				//
				pending_ = true;	
				socket_.close(ec);
				stop(ec);
			}
			else
			{
				//
				//	The session was closed intentionally.
				//	At this point nothing more is to do. Service is going down and all session have to be stopped fast.
				//
				CYNG_LOG_WARNING(logger_, "session "
					<< vm_.tag()
					<< " closed intentionally: "
					<< ec
					<< ':'
					<< ec.value()
					<< " ["
					<< ec.message()
					<< ']');

				stop(ec);

			}
		});
	};

}

