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

namespace smf {

	broker::broker(cyng::channel_weak wp
		, cyng::controller& ctl
		, cyng::logger logger
		, target const& t
		, std::chrono::seconds timeout
		, bool login)
	: sigs_{
		std::bind(&broker::stop, this, std::placeholders::_1),
		std::bind(&broker::receive, this, std::placeholders::_1),
		std::bind(&broker::start, this),
		std::bind(&broker::receive, this, std::placeholders::_1)
	}	, channel_(wp)
		, ctl_(ctl)
		, logger_(logger)
		, target_(t)
		, timeout_(timeout)
		, login_(login)
		, stopped_(false)
		, endpoints_()
		, socket_(ctl.get_ctx())
		, deadline_(ctl.get_ctx())
		, heartbeat_timer_(ctl.get_ctx())
		, buffer_write_()
	{
		auto sp = channel_.lock();
		if (sp) {
			sp->set_channel_name("receive", 1);
			sp->set_channel_name("start", 2);
		}

		CYNG_LOG_INFO(logger_, "broker " << target_ << " ready");
	}

	void broker::stop(cyng::eod) {

		CYNG_LOG_INFO(logger_, "broker " << target_ << " stop");

		buffer_write_.clear();
		stopped_ = true;
		boost::system::error_code ignored_ec;
		socket_.close(ignored_ec);
		deadline_.cancel();
		heartbeat_timer_.cancel();
	}

	void broker::receive(cyng::buffer_t data) {
		CYNG_LOG_INFO(logger_, "broker: transmit " << data.size() << " bytes to " << target_);
		buffer_write_.emplace_back(data);
		do_write();
	}

	void broker::start() {
		CYNG_LOG_INFO(logger_, "start broker [" << target_ << "]");

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

		stopped_ = false;
		boost::asio::ip::tcp::resolver r(ctl_.get_ctx());
		connect(r.resolve(target_.get_address(), std::to_string(target_.get_port())));
	}

	void broker::connect(boost::asio::ip::tcp::resolver::results_type endpoints) {

		// Start the connect actor.
		endpoints_ = endpoints;
		start_connect(endpoints_.begin());

		// Start the deadline actor. You will note that we're not setting any
		// particular deadline here. Instead, the connect and input actors will
		// update the deadline prior to each asynchronous operation.
		deadline_.async_wait(std::bind(&broker::check_deadline, this));
	}

	void broker::start_connect(boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter) {

		if (endpoint_iter != endpoints_.end())	{
			CYNG_LOG_TRACE(logger_, "broker [" << target_ << "] trying " << endpoint_iter->endpoint() << "...");

			//	Set a deadline for the connect operation.
			deadline_.expires_after(timeout_);

			// Start the asynchronous connect operation.
			socket_.async_connect(endpoint_iter->endpoint(),
				std::bind(&broker::handle_connect, this,
					std::placeholders::_1, endpoint_iter));
		}
		else {
			// There are no more endpoints to try. Shut down the client.
			stop(cyng::eod());
			auto sp = channel_.lock();
			if (sp)	sp->suspend(std::chrono::seconds(12), "start", cyng::make_tuple());

		}
	}

	void broker::check_deadline()
	{
		if (stopped_)	return;

		// Check whether the deadline has passed. We compare the deadline against
		// the current time since a new asynchronous operation may have moved the
		// deadline before this actor had a chance to run.
		if (deadline_.expiry() <= boost::asio::steady_timer::clock_type::now())
		{
			CYNG_LOG_INFO(logger_, "broker [" << target_ << "] deadline expired");

			// The deadline has passed. The socket is closed so that any outstanding
			// asynchronous operations are cancelled.
			socket_.close();

			// There is no longer an active deadline. The expiry is set to the
			// maximum time point so that the actor takes no action until a new
			// deadline is set.
			deadline_.expires_at(boost::asio::steady_timer::time_point::max());
		}

		// Put the actor back to sleep.
		deadline_.async_wait(std::bind(&broker::check_deadline, this));
	}

	void broker::handle_connect(const boost::system::error_code& ec,
		boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter) {

		if (stopped_)	return;

		// The async_connect() function automatically opens the socket at the start
		// of the asynchronous operation. If the socket is closed at this time then
		// the timeout handler must have run first.
		if (!socket_.is_open())	{

			CYNG_LOG_WARNING(logger_, "broker [" << target_ << "] connect timed out");

			// Try the next available endpoint.
			start_connect(++endpoint_iter);
		}

		// Check if the connect operation failed before the deadline expired.
		else if (ec)
		{
			CYNG_LOG_WARNING(logger_, "broker [" << target_ << "] connect error: " << ec.message());

			// We need to close the socket used in the previous connection attempt
			// before starting a new one.
			socket_.close();

			// Try the next available endpoint.
			start_connect(++endpoint_iter);
		}

		// Otherwise we have successfully established a connection.
		else
		{
			CYNG_LOG_INFO(logger_, "broker [" << target_ << "] connected to " << endpoint_iter->endpoint());

			if (login_) {

				//
				//	set login sequence
				//
				buffer_write_.emplace_back(cyng::make_buffer(target_.get_login_sequence()));
				buffer_write_.emplace_back(cyng::make_buffer("\r\n"));
			}

			// Start the input actor.
			do_read();

			// Start the heartbeat actor.
			do_write();
		}
	}

	void broker::do_read()
	{
		// Set a deadline for the read operation.
		deadline_.expires_after(timeout_);

		// Start an asynchronous operation to read a newline-delimited message.
		boost::asio::async_read_until(socket_,
			boost::asio::dynamic_buffer(input_buffer_), '\n',
			std::bind(&broker::handle_read, this,
				std::placeholders::_1, std::placeholders::_2));
	}

	void broker::handle_read(const boost::system::error_code& ec, std::size_t n)
	{
		if (stopped_)	return;

		if (!ec)
		{
			// Extract the newline-delimited message from the buffer.
			std::string line(input_buffer_.substr(0, n - 1));
			input_buffer_.erase(0, n);

			// Empty messages are heartbeats and so ignored.
			if (!line.empty())
			{
				CYNG_LOG_DEBUG(logger_, "broker [" << target_ << "] received " << line);
			}

			do_read();
		}
		else
		{
			CYNG_LOG_ERROR(logger_, "broker [" << target_ << "] on receive: " << ec.message());

			stop(cyng::eod());
			auto sp = channel_.lock();
			if (sp)	sp->suspend(std::chrono::seconds(12), "start", cyng::make_tuple());
		}
	}

	void broker::do_write()
	{
		if (stopped_)	return;

		// Start an asynchronous operation to send a heartbeat message.
		boost::asio::async_write(socket_,
			boost::asio::buffer(buffer_write_.front().data(), buffer_write_.front().size()),
			std::bind(&broker::handle_write, this, std::placeholders::_1));
	}

	void broker::handle_write(const boost::system::error_code& ec)
	{
		if (stopped_)	return;

		if (!ec)	{

			buffer_write_.pop_front();
			if (!buffer_write_.empty()) {
				do_write();
			}
			else {

				// Wait 10 seconds before sending the next heartbeat.
				heartbeat_timer_.expires_after(boost::asio::chrono::seconds(10));
				heartbeat_timer_.async_wait(std::bind(&broker::do_write, this));
			}
		}
		else	{
			CYNG_LOG_ERROR(logger_, "broker [" << target_ << "] on heartbeat: " << ec.message());

			stop(cyng::eod());
			auto sp = channel_.lock();
			if (sp)	sp->suspend(std::chrono::seconds(12), "start", cyng::make_tuple());
		}
	}
}