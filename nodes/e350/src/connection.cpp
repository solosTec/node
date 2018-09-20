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
#include <boost/core/ignore_unused.hpp>

namespace node 
{
	namespace imega
	{
		connection::connection(boost::asio::ip::tcp::socket&& socket
			, cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, bus::shared_type bus
			, boost::uuids::uuid tag
			, std::chrono::seconds const& timeout
			, bool use_global_pwd
			, std::string const& global_pwd)
		: socket_(std::move(socket))
			, logger_(logger)
			, tag_(tag)
			, buffer_()
			, session_(mux, logger, bus, tag, timeout, use_global_pwd, global_pwd)
			, serializer_(socket_, session_.vm_)
            , shutdown_(false)
		{
			//
			//	register socket operations to session VM
			//
			cyng::register_socket(socket_, session_.vm_);
		}

        connection::~connection()
        {
            //std::cerr << "connection::~connection(" << tag_ << ")" << std::endl;
            CYNG_LOG_DEBUG(logger_, "deconstruct connection(" << tag_ << ')');
        }

		void connection::start()
		{
			do_read();
		}

		void connection::stop()
		{
            CYNG_LOG_DEBUG(logger_, "shutdown connection(" << tag_ << ')');
            shutdown_ = true;

			//
			//  close socket
			//
			boost::system::error_code ec;
			socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
			socket_.close(ec);
			
			//
            //  no more callbacks
            //
            session_.stop(ec);

		}

		void connection::do_read()
		{
            //
            //  check system shutdown
            //
            if (shutdown_)  return;

			socket_.async_read_some(boost::asio::buffer(buffer_),
				[this](boost::system::error_code ec, std::size_t bytes_transferred)
			{
				if (!ec)
				{
					//CYNG_LOG_TRACE(logger_, "imega connection received " << bytes_transferred << " bytes");
					session_.vm_.async_run(cyng::generate_invoke("log.msg.trace", "imega connection received", bytes_transferred, "bytes"));

					//
					//	size == parsed bytes
					//
					const auto size = session_.parser_.read(buffer_.data(), buffer_.data() + bytes_transferred);
					BOOST_ASSERT(size == bytes_transferred);
					boost::ignore_unused(size);	//	release version

#ifdef SMF_IO_DEBUG
					cyng::io::hex_dump hd;
					std::stringstream ss;
					hd(ss, buffer_.data(), buffer_.data() + bytes_transferred);
					CYNG_LOG_TRACE(logger_, "imega input dump \n" << ss.str());
#endif

					do_read();
				}
				else
				{
                    CYNG_LOG_WARNING(logger_, "imega connection closed <" << ec << ':' << ec.value() << ':' << ec.message() << '>');
                    session_.stop(ec);
                    //session_.bus_->vm_.async_run(cyng::generate_invoke("server.close.connection", tag_, cyng::invoke("push.connection"), ec));
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
			, std::chrono::seconds const& timeout
			, bool use_global_pwd
			, std::string const& global_pwd)
		{
			return cyng::make_object<connection>(std::move(socket)
				, mux
				, logger 
				, bus
				, tag 
				, timeout
				, use_global_pwd
				, global_pwd);

		}

	}
}

namespace cyng
{
	namespace traits
	{

#if defined(CYNG_LEGACY_MODE_ON)
		const char type_tag<node::imega::connection>::name[] = "imega::connection";
#endif
	}	// traits	
}




