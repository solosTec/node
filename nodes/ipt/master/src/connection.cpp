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
//#include <cyng/vm/generator.h>

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
			, tag_(tag)
			, buffer_()
			, session_(mux, logger, bus, tag, sk, watchdog, timeout)
			, serializer_(socket_, session_.vm_, sk)
		{
			//
			//	register socket operations to session VM
			//
			cyng::register_socket(socket_, session_.vm_);
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
			//auto self(shared_from_this());
			socket_.async_read_some(boost::asio::buffer(buffer_),
				[this/*, self*/](boost::system::error_code ec, std::size_t bytes_transferred)
			{
				if (!ec)
				{
					//CYNG_LOG_TRACE(logger_, "ipt connection received " << bytes_transferred << " bytes");
					session_.vm_.async_run(cyng::generate_invoke("log.msg.info", "ipt connection received", bytes_transferred, "bytes"));

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
				else //if (ec != boost::asio::error::operation_aborted)
				{
					CYNG_LOG_WARNING(logger_, "read <" << ec << ':' << ec.value() << ':' << ec.message() << '>');
					session_.bus_->vm_.async_run(client_req_close(tag_, ec.value()));
					session_.bus_->vm_.async_run(cyng::generate_invoke("server.close.connection", tag_, ec));
				}
			});
		}

		std::size_t connection::hash() const noexcept
		{
			return session_.vm_.hash();
		}

		cyng::object make_connection(boost::asio::ip::tcp::socket&& socket
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




