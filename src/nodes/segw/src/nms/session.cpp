/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <nms/session.h>

#include <cyng/obj/container_cast.hpp>
#include <cyng/log/record.h>
#include <cyng/io/ostream.h>
#include <cyng/io/serialize.h>

namespace smf {
	namespace nms {

		session::session(boost::asio::ip::tcp::socket socket, cfg& c, cyng::logger logger, std::function<void(boost::asio::ip::tcp::endpoint ep)> rebind)
			: socket_(std::move(socket))
			, logger_(logger)
			, buffer_{ 0 }
			, reader_(c, logger)
			, parser_([this, rebind](cyng::object&& obj) {

				CYNG_LOG_TRACE(logger_, "[NMS] session [" << socket_.remote_endpoint() << "] parsed " << obj);
				send_response(reader_.run(cyng::container_cast<cyng::param_map_t>(std::move(obj)), rebind));

			})
		{}

		void session::start() {
			do_read();
		}

		void session::do_read() {

			auto self = shared_from_this();

			socket_.async_read_some(boost::asio::buffer(buffer_.data(), buffer_.size()),
				[this, self](boost::system::error_code ec, std::size_t bytes_transferred) {

					if (!ec) {
						CYNG_LOG_DEBUG(logger_, "[NMS] session [" << socket_ .remote_endpoint() << "] received " << bytes_transferred << " bytes");

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
						CYNG_LOG_WARNING(logger_, "[NMS] session stopped: " << ec.message());
					}

			});
		}

		void session::send_response(cyng::param_map_t&& pmap) {
			auto str = cyng::io::to_json(pmap);
			boost::system::error_code ec;
			boost::asio::write(socket_, boost::asio::buffer(str.data(), str.size()), ec);
		}

	}
}