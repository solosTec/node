/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <session.h>
#include <cyng/log/record.h>
#include <cyng/io/serialize.h>

#include <iostream>

namespace smf {

	session::session(boost::asio::ip::tcp::socket socket, cyng::logger logger)
	: socket_(std::move(socket))
		, logger_(logger)
		, parser_([this](cyng::object&& obj) {
			CYNG_LOG_DEBUG(logger_, "parser: " << cyng::io::to_typed(obj));
		})
	{}

	session::~session()
	{
#ifdef _DEBUG_MAIN
		std::cout << "session(~)" << std::endl;
#endif
	}


	void session::stop()
	{
		//	https://www.boost.org/doc/libs/1_75_0/doc/html/boost_asio/reference/basic_stream_socket/close/overload2.html
		CYNG_LOG_WARNING(logger_, "stop session");
		boost::system::error_code ec;
		socket_.shutdown(boost::asio::socket_base::shutdown_both, ec);
		socket_.close(ec);
	}

	void session::start()
	{
		do_read();
	}

	void session::do_read() {
		auto self = shared_from_this();

		socket_.async_read_some(boost::asio::buffer(buffer_.data(), buffer_.size()),
			[this, self](boost::system::error_code ec, std::size_t bytes_transferred) {

				if (!ec) {
					CYNG_LOG_DEBUG(logger_, "session [" << socket_.remote_endpoint() << "] received " << bytes_transferred << " bytes");

					//
					//	let parse it
					//
					parser_.read(buffer_.begin(), buffer_.begin() + bytes_transferred);

					//
					//	continue reading
					//
					do_read();
				}
				else {
					CYNG_LOG_WARNING(logger_, "session stopped: " << ec.message());
				}

		});
	}
}


