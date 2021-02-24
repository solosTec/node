/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <sml/session.h>

#include <cyng/log/record.h>

namespace smf {
	namespace sml {

		session::session(boost::asio::ip::tcp::socket socket, cyng::logger logger)
			: socket_(std::move(socket))
			, logger_(logger)
			, buffer_{ 0 }
		{}

		void session::start() {
			do_read();
		}

		void session::do_read() {

			auto self = shared_from_this();

			socket_.async_read_some(boost::asio::buffer(buffer_.data(), buffer_.size()),
				[this, self](boost::system::error_code ec, std::size_t bytes_transferred) {

					if (!ec) {
						CYNG_LOG_TRACE(logger_, "[SML] session [" << socket_.remote_endpoint() << "] received " << bytes_transferred << " bytes");


						//
						//	continue reading
						//
						do_read();
					}
					else {
						CYNG_LOG_WARNING(logger_, "[SML] session stopped: " << ec.message());
					}

				});
		}

	}
}