/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <tasks/broker.h>

#include <cyng/log/record.h>
#include <cyng/task/controller.h>

#ifdef _DEBUG_SEGW
#include <iostream>
#endif

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/write.hpp>
#include <boost/bind.hpp>

namespace smf {

	broker::broker(cyng::channel_weak wp
		, cyng::controller& ctl
		, cyng::logger logger
		, target const& t
		, std::chrono::seconds timeout
		, bool login)
	: state_(state::START)
		, sigs_{
			std::bind(&broker::stop, this, std::placeholders::_1),
			std::bind(&broker::receive, this, std::placeholders::_1),
			std::bind(&broker::start, this)
		}	
		, channel_(wp)
		, ctl_(ctl)
		, logger_(logger)
		, target_(t)
		, timeout_(timeout)
		, login_(login)
		, endpoints_()
		, socket_(ctl.get_ctx())
		, timer_(ctl.get_ctx())
		, buffer_write_()
	{
		auto sp = channel_.lock();
		if (sp) {
			sp->set_channel_name("receive", 1);
			sp->set_channel_name("start", 2);
		}

		CYNG_LOG_INFO(logger_, "[broker] ready: " << target_);
	}

	void broker::stop(cyng::eod) {

		CYNG_LOG_INFO(logger_, "[broker] stop: " << target_);
		state_ = state::STOPPED;

	}

	void broker::reset() {
		buffer_write_.clear();
		boost::system::error_code ignored_ec;
		socket_.close(ignored_ec);
		timer_.cancel();
		state_ = state::START;
	}

	void broker::receive(cyng::buffer_t data) {
		if (is_connected()) {
			CYNG_LOG_INFO(logger_, "[broker] transmit " << data.size() << " bytes to " << target_);
			buffer_write_.emplace_back(data);
			do_write();
		}
		else {
			CYNG_LOG_WARNING(logger_, "[broker] drops " << data.size() << " bytes to " << target_);
		}
	}

	void broker::start() {
		CYNG_LOG_INFO(logger_, "[broker] start " << target_);

		state_ = state::START;
		buffer_write_.clear();

#ifdef __DEBUG
		//	test with an HTTP server
		write_buffer_.emplace_back(cyng::make_buffer("GET / HTTP/1.1"));
		write_buffer_.emplace_back(cyng::make_buffer("\r\n"));
		write_buffer_.emplace_back(cyng::make_buffer("Host: "));
		write_buffer_.emplace_back(cyng::make_buffer(target_.get_address()));
		write_buffer_.emplace_back(cyng::make_buffer("\r\n"));
		write_buffer_.emplace_back(cyng::make_buffer("\r\n\r\n"));
#endif

		boost::asio::ip::tcp::resolver r(ctl_.get_ctx());
		connect(r.resolve(target_.get_address(), std::to_string(target_.get_port())));
	}

	void broker::connect(boost::asio::ip::tcp::resolver::results_type endpoints) {

		state_ = state::WAIT;

		// Start the connect actor.
		endpoints_ = endpoints;
		start_connect(endpoints_.begin());

		// Start the deadline actor. You will note that we're not setting any
		// particular deadline here. Instead, the connect and input actors will
		// update the deadline prior to each asynchronous operation.
		timer_.async_wait(boost::bind(&broker::check_deadline, this, boost::asio::placeholders::error));
	}

	void broker::start_connect(boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter) {

		if (endpoint_iter != endpoints_.end())	{
			CYNG_LOG_TRACE(logger_, "[broker] trying " << endpoint_iter->endpoint() << "...");

			//	Set a deadline for the connect operation.
			timer_.expires_after(std::chrono::seconds(12));

			// Start the asynchronous connect operation.
			socket_.async_connect(endpoint_iter->endpoint(),
				std::bind(&broker::handle_connect, this,
					std::placeholders::_1, endpoint_iter));
		}
		else {

			//
			// There are no more endpoints to try. Shut down the client.
			//
			reset();

			//
			//	reconnect after 20 seconds
			//
			timer_.expires_after(boost::asio::chrono::seconds(20));
			timer_.async_wait(boost::bind(&broker::check_deadline, this, boost::asio::placeholders::error));

		}
	}

	void broker::check_deadline(const boost::system::error_code& ec) {
		if (is_stopped())	return;
		CYNG_LOG_TRACE(logger_, "[broker] check deadline " << ec);

		if (!ec) {
			switch (state_) {
			case state::START:
				CYNG_LOG_TRACE(logger_, "[broker] check deadline: start");
				start();
				break;
			case state::CONNECTED:
				CYNG_LOG_TRACE(logger_, "[broker] check deadline: connected");
				break;
			case state::WAIT:
				CYNG_LOG_TRACE(logger_, "[broker] check deadline: waiting");
				start();
				break;
			default:
				CYNG_LOG_TRACE(logger_, "[broker] check deadline: other");
				BOOST_ASSERT_MSG(false, "invalid state");
				break;
			}
		}
		else {
			CYNG_LOG_TRACE(logger_, "[broker] check deadline timer cancelled");
		}
	}

	void broker::handle_connect(const boost::system::error_code& ec,
		boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter) {

		if (is_stopped())	return;

		// The async_connect() function automatically opens the socket at the start
		// of the asynchronous operation. If the socket is closed at this time then
		// the timeout handler must have run first.
		if (!socket_.is_open())	{

			CYNG_LOG_WARNING(logger_, "[broker] " << target_ << " connect timed out");

			// Try the next available endpoint.
			start_connect(++endpoint_iter);
		}

		// Check if the connect operation failed before the deadline expired.
		else if (ec)
		{
			CYNG_LOG_WARNING(logger_, "[broker] " << target_ << " connect error " << ec.value() << ": " << ec.message());

			// We need to close the socket used in the previous connection attempt
			// before starting a new one.
			socket_.close();

			// Try the next available endpoint.
			start_connect(++endpoint_iter);
		}

		// Otherwise we have successfully established a connection.
		else
		{
			CYNG_LOG_INFO(logger_, "[broker] " << target_ << " connected to " << endpoint_iter->endpoint());
			state_ = state::CONNECTED;


			// Start the input actor.
			do_read();

			if (login_) {

				//
				//	set login sequence
				//
				buffer_write_.emplace_back(cyng::make_buffer(target_.get_login_sequence()));
				buffer_write_.emplace_back(cyng::make_buffer("\r\n"));

				// Start the heartbeat actor.
				do_write();
			}
		}
	}

	void broker::do_read()
	{
		//
		//	connect was successful
		//

		// Start an asynchronous operation to read a newline-delimited message.
		boost::asio::async_read_until(socket_,
			boost::asio::dynamic_buffer(input_buffer_), '\n',
			std::bind(&broker::handle_read, this,
				std::placeholders::_1, std::placeholders::_2));
	}

	void broker::handle_read(const boost::system::error_code& ec, std::size_t n)
	{
		if (is_stopped())	return;

		if (!ec)
		{
			// Extract the newline-delimited message from the buffer.
			std::string line(input_buffer_.substr(0, n - 1));
			input_buffer_.erase(0, n);

			// Empty messages are heartbeats and so ignored.
			if (!line.empty())
			{
				CYNG_LOG_DEBUG(logger_, "[broker] " << target_ << " received " << line);
			}

			do_read();
		}
		else
		{
			CYNG_LOG_ERROR(logger_, "[broker] " << target_ << " read " << ec.value() << ": " << ec.message());
			reset();

			timer_.expires_after((ec == boost::asio::error::connection_reset)
				? boost::asio::chrono::seconds(10)
				: boost::asio::chrono::seconds(20));
			timer_.async_wait(boost::bind(&broker::check_deadline, this, boost::asio::placeholders::error));
		}
	}

	void broker::do_write()
	{
		if (is_stopped())	return;

		BOOST_ASSERT(!buffer_write_.empty());

		// Start an asynchronous operation to send a heartbeat message.
		boost::asio::async_write(socket_,
			boost::asio::buffer(buffer_write_.front().data(), buffer_write_.front().size()),
			std::bind(&broker::handle_write, this, std::placeholders::_1));
	}

	void broker::handle_write(const boost::system::error_code& ec)
	{
		if (is_stopped())	return;

		if (!ec)	{

			buffer_write_.pop_front();
			if (!buffer_write_.empty()) {
				do_write();
			}
			else {

				//	Wait 10 seconds before sending the next heartbeat.
				//heartbeat_timer_.expires_after(boost::asio::chrono::seconds(10));
				//heartbeat_timer_.async_wait(std::bind(&broker::check_connection_state, this));
			}
		}
		else	{
			CYNG_LOG_ERROR(logger_, "[broker] " << target_ << " on heartbeat: " << ec.message());

			reset();
		}
	}
}