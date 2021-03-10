/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <redirector/session.h>

#include <cyng/log/record.h>
#include <cyng/task/registry.h>

namespace smf {
	namespace rdr {

		session::session(boost::asio::ip::tcp::socket socket, cyng::registry& reg, cfg& config, cyng::logger logger, lmn_type type)
			: socket_(std::move(socket))
			, registry_(reg)
			, logger_(logger)
			, cfg_(config, type)
			, buffer_{ 0 }
		{}

		void session::start() {
			do_read();
		}

		void session::do_read() {

			auto self = shared_from_this();

			socket_.async_read_some(boost::asio::buffer(buffer_.data(), buffer_.size()),
				[this, self](boost::system::error_code ec, std::size_t bytes_transferred) {

					//
					//	memoize port name
					//
					auto port_name = cfg_.get_port_name();
					if (!ec) {
						CYNG_LOG_TRACE(logger_, "[RDR] session [" << socket_.remote_endpoint() << "] received " << bytes_transferred << " bytes => "  << port_name);

						//
						//	send to port
						// 
						auto const count = registry_.dispatch(port_name, "write", cyng::make_tuple(cyng::buffer_t(buffer_.begin(), buffer_.begin() + bytes_transferred)));
						CYNG_LOG_TRACE(logger_, "[RDR] " << port_name << " found " << count << " receiver");

						//
						//	continue reading
						//
						do_read();
					}
					else {
						CYNG_LOG_WARNING(logger_, "[RDR] session " << port_name << " stopped: " << ec.message());
					}

				});
		}

	}
}