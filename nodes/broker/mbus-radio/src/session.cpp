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
		, vm_(vm)
		, buffer_()
		, authorized_(false)
		, data_()
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
					if (authorized_) {
						//
						//	raw data
						//
						process_data(cyng::buffer_t{ buffer_.begin(), buffer_.begin() + bytes_transferred });
					}
					else {

						//
						//	login data
						//
						process_login(cyng::buffer_t{ buffer_.begin(), buffer_.begin() + bytes_transferred });
					}

					//
					//	continue reading
					//
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

	void session::process_data(cyng::buffer_t&& data)
	{

	}

	void session::process_login(cyng::buffer_t&& data)
	{
		data_.insert(data_.end(), data.begin(), data.end());
		if (data.back() == '\n') {

			CYNG_LOG_INFO(logger_, "session authorized: "
				<< socket_.remote_endpoint());

			//
			//	login complete
			//
			authorized_ = true;
		}
	}


}
