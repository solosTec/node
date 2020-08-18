/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "session.h"

#include <cyng/io/io_bytes.hpp>
#include <cyng/io/hex_dump.hpp>
#include <cyng/vm/controller.h>

namespace node
{
	session::session(boost::asio::ip::tcp::socket socket
		, cyng::logging::log_ptr logger
		, cyng::controller& vm)
	: socket_(std::move(socket))
		, logger_(logger)
		, buffer_()
		, vm_(vm)
	{
		CYNG_LOG_INFO(logger_, "session at  " << socket_.remote_endpoint());
	}

	session::~session()
	{
		//
		//	remove from session list
		//
	}

	void session::start()
	{
		do_read();
	}

	void session::do_read()
	{
		auto self(shared_from_this());
		socket_.async_read_some(boost::asio::buffer(buffer_.data(), buffer_.size()),
			[this, self](boost::system::error_code ec, std::size_t bytes_transferred)
			{
				if (!ec)
				{
					CYNG_LOG_TRACE(logger_, "session received "
						<< cyng::bytes_to_str(bytes_transferred));

#ifdef _DEBUG
					cyng::io::hex_dump hd;
					std::stringstream ss;
					hd(ss, buffer_.begin(), buffer_.begin() + bytes_transferred);
					CYNG_LOG_TRACE(logger_, "\n" << ss.str());
#endif
					do_read();
				}
				else
				{
					//	leave
					CYNG_LOG_WARNING(logger_, "session closed: "
						<< ec.message());
				}
			});
	}

}
