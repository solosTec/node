/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <ipt_session.h>
#include <ipt_server.h>

#include <smf/ipt/codes.h>

#include <cyng/log/record.h>

#include <iostream>

#ifdef _DEBUG_IPT
#include <iostream>
#include <sstream>
#include <cyng/io/hex_dump.hpp>
#endif

namespace smf {

	ipt_session::ipt_session(boost::asio::ip::tcp::socket socket, ipt_server* srv, cyng::logger logger)
	: socket_(std::move(socket))
		, logger_(logger)
		, cluster_bus_(srv->cluster_bus_)
		, buffer_()
		, buffer_write_()
		, parser_(srv->sk_
			, std::bind(&ipt_session::ipt_cmd, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&ipt_session::ipt_stream, this, std::placeholders::_1)
		)
	{}

	ipt_session::~ipt_session()
	{
#ifdef _DEBUG_IPT
		std::cout << "session(~)" << std::endl;
#endif
	}


	void ipt_session::stop()
	{
		//	https://www.boost.org/doc/libs/1_75_0/doc/html/boost_asio/reference/basic_stream_socket/close/overload2.html
		CYNG_LOG_WARNING(logger_, "stop session");
		boost::system::error_code ec;
		socket_.shutdown(boost::asio::socket_base::shutdown_both, ec);
		socket_.close(ec);
	}

	void ipt_session::start()
	{
		do_read();
	}

	void ipt_session::do_read() {
		auto self = shared_from_this();

		socket_.async_read_some(boost::asio::buffer(buffer_.data(), buffer_.size()),
			[this, self](boost::system::error_code ec, std::size_t bytes_transferred) {

				if (!ec) {
					CYNG_LOG_DEBUG(logger_, "[session] received " << bytes_transferred << " bytes from [" << socket_.remote_endpoint() << "]");

#ifdef _DEBUG_IPT
					{
						std::stringstream ss;
						cyng::io::hex_dump<8> hd;
						hd(ss, buffer_.data(), buffer_.data() + bytes_transferred);
						CYNG_LOG_DEBUG(logger_, "[" << socket_.remote_endpoint() << "] " << bytes_transferred << " bytes:\n" << ss.str());
					}
#endif

					//
					//	let parse it
					//
					parser_.read(buffer_.data(), buffer_.data() + bytes_transferred);

					//
					//	continue reading
					//
					do_read();
				}
				else {
					CYNG_LOG_WARNING(logger_, "[session] read: " << ec.message());
				}

		});
	}

	void ipt_session::do_write()
	{
		//if (is_stopped())	return;

		// Start an asynchronous operation to send a heartbeat message.
		boost::asio::async_write(socket_,
			boost::asio::buffer(buffer_write_.front().data(), buffer_write_.front().size()),
			std::bind(&ipt_session::handle_write, this, std::placeholders::_1));
	}

	void ipt_session::handle_write(const boost::system::error_code& ec)
	{
		//if (is_stopped())	return;

		if (!ec) {

			buffer_write_.pop_front();
			if (!buffer_write_.empty()) {
				do_write();
			}
			else {

				// Wait 10 seconds before sending the next heartbeat.
				//heartbeat_timer_.expires_after(boost::asio::chrono::seconds(10));
				//heartbeat_timer_.async_wait(std::bind(&bus::do_write, this));
			}
		}
		else {
			CYNG_LOG_ERROR(logger_, "[session] write: " << ec.message());

			//reset();
		}
	}

	void ipt_session::ipt_cmd(ipt::header const& h, cyng::buffer_t&& body) {

		CYNG_LOG_TRACE(logger_, "ipt cmd " << ipt::command_name(h.command_));
		switch (ipt::to_code(h.command_)) {
		case ipt::code::CTRL_REQ_LOGIN_PUBLIC:
			cluster_bus_.pty_login();
			break;
		case ipt::code::CTRL_REQ_LOGIN_SCRAMBLED:
			cluster_bus_.pty_login();
			break;
		default:
			break;
		}

	}
	void ipt_session::ipt_stream(cyng::buffer_t&& data) {
		CYNG_LOG_TRACE(logger_, "ipt stream " << data.size() << " byte");

	}

}


