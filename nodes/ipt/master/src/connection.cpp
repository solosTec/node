/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#include "connection.h"
#include <smf/cluster/generator.h>
#include <cyng/vm/domain/asio_domain.h>
#ifdef SMF_IO_DEBUG
#include <cyng/io/hex_dump.hpp>
#endif
#include <boost/uuid/uuid_io.hpp>

namespace node 
{
	namespace ipt
	{
		connection::connection(boost::asio::ip::tcp::socket&& socket
			, cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, bus::shared_type bus
			, boost::uuids::uuid tag
			, scramble_key const& sk
			, std::uint16_t watchdog
			, std::chrono::seconds const& timeout)
		: socket_(std::move(socket))
			, logger_(logger)
			, buffer_()
			, session_(mux, logger, bus, tag, sk, watchdog, timeout)
			, serializer_(socket_, session_.vm_, sk)
		{
			//
			//	register socket operations to session VM
			//
			cyng::register_socket(socket_, session_.vm_);
		}

        connection::~connection()
        {
			//CYNG_LOG_DEBUG(logger_, "deconstruct connection(" << tag_ << ')');
			BOOST_ASSERT_MSG(session_.vm_.is_halted(), "session not in HALT state");
        }

		void connection::start()
		{
			do_read();
		}

		void connection::close()
		{
			if (socket_.is_open()) {

				//
				//  close socket
				//
				CYNG_LOG_DEBUG(logger_, session_.vm_.tag() 
					<< " close socket " 
					<< socket_.remote_endpoint());

				boost::system::error_code ec;
				socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
				socket_.close(ec);
			}
		}

		void connection::do_read()
		{
			socket_.async_read_some(boost::asio::buffer(buffer_),
				[this](boost::system::error_code ec, std::size_t bytes_transferred)
			{
				if (!ec)
				{
					session_.vm_.async_run(cyng::generate_invoke("log.msg.trace", "ipt connection received", bytes_transferred, "bytes"));

					//
					//	buffer contains the unscrambled input
					//
					const auto buffer = session_.parser_.read(buffer_.data(), buffer_.data() + bytes_transferred);
#ifdef SMF_IO_DEBUG
					cyng::io::hex_dump hd;
					std::stringstream ss;
					hd(ss, buffer.begin(), buffer.end());
					CYNG_LOG_TRACE(logger_, "ipt input dump \n" << ss.str());
#endif

					do_read();
				}


				else if (ec != boost::asio::error::operation_aborted)
				{
					//
					//	device/party closed connection or network shutdown
					//
					CYNG_LOG_WARNING(logger_, "ipt connection "
						<< session_.vm_.tag()
						<< " closed: " 
						<< ec 
						<< ':' 
						<< ec.value() 
						<< " [" 
						<< ec.message() 
						<< ']');

					session_.stop_req(ec);
				}
				else
				{
					//
					//	The session was closed intentionally.
					//	At this point nothing more is to do. Service is going down and all session have to be stopped fast.
					//
					CYNG_LOG_WARNING(logger_, "ipt connection "
						<< session_.vm_.tag()
						<< " closed intentionally: "
						<< ec
						<< ':'
						<< ec.value()
						<< " ["
						<< ec.message()
						<< ']');
					session_.stop_res(ec);

				}
			});
		}

		std::size_t connection::hash() const noexcept
		{
			return session_.vm_.hash();
		}

		cyng::object make_client(boost::asio::ip::tcp::socket&& socket
			, cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, bus::shared_type bus
			, boost::uuids::uuid tag
			, scramble_key const& sk
			, std::uint16_t watchdog
			, std::chrono::seconds const& timeout)
		{
			return cyng::make_object<connection>(std::move(socket)
				, mux
				, logger 
				, bus
				, tag 
				, sk 
				, watchdog 
				, timeout);

		}

	}
}

namespace cyng
{
	namespace traits
	{

#if defined(CYNG_LEGACY_MODE_ON)
		const char type_tag<node::ipt::connection>::name[] = "ipt::connection";
#endif
	}	// traits	
}




