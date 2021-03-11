/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/cluster/bus.h>

#include <cyng/log/record.h>
#include <cyng/vm/linearize.hpp>
#include <cyng/vm/generator.hpp>
#include <cyng/sys/process.h>

#include <boost/bind.hpp>

namespace smf {

	bus::bus(boost::asio::io_context& ctx
		, cyng::logger logger
		, toggle::server_vec_t&& tgl
		, std::string const& node_name)
	: state_(state::START)
		, ctx_(ctx)
		, logger_(logger)
		, tgl_(std::move(tgl))
		, node_name_(node_name)
		, endpoints_()
		, socket_(ctx_)
		, timer_(ctx_)
		, buffer_write_()
		, input_buffer_()
	{}

	void bus::start() {

		auto const srv = tgl_.get();
		CYNG_LOG_INFO(logger_, "[cluster] connect(" << srv << ")");
		state_ = state::START;

		//
		//	connect to cluster
		//
		boost::asio::ip::tcp::resolver r(ctx_);
		connect(r.resolve(srv.host_, srv.service_));

	}

	void bus::stop() {
		CYNG_LOG_INFO(logger_, "[cluster] " << tgl_.get() << " stop");

		reset();
		state_ = state::STOPPED;
	}

	void bus::reset() {

		boost::system::error_code ignored_ec;
		socket_.close(ignored_ec);
		state_ = state::START;
		timer_.cancel();
	}

	void bus::check_deadline(const boost::system::error_code& ec) {
		if (is_stopped())	return;
		CYNG_LOG_TRACE(logger_, "[cluster] check deadline " << ec);

		if (!ec) {
			switch (state_) {
			case state::START:
				CYNG_LOG_TRACE(logger_, "[cluster] check deadline: start");
				start();
				break;
			case state::CONNECTED:
				CYNG_LOG_TRACE(logger_, "[cluster] check deadline: connected");
				break;
			case state::WAIT:
				CYNG_LOG_TRACE(logger_, "[cluster] check deadline: waiting");
				start();
				break;
			default:
				CYNG_LOG_TRACE(logger_, "[cluster] check deadline: other");
				BOOST_ASSERT_MSG(false, "invalid state");
				break;
			}
		}
		else {
			CYNG_LOG_TRACE(logger_, "[cluster] check deadline timer cancelled");
		}
	}

	void bus::connect(boost::asio::ip::tcp::resolver::results_type endpoints) {

		state_ = state::WAIT;

		// Start the connect actor.
		endpoints_ = endpoints;
		start_connect(endpoints_.begin());

		// Start the deadline actor. You will note that we're not setting any
		// particular deadline here. Instead, the connect and input actors will
		// update the deadline prior to each asynchronous operation.
		timer_.async_wait(boost::bind(&bus::check_deadline, this, boost::asio::placeholders::error));

	}

	void bus::start_connect(boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter) {
		if (endpoint_iter != endpoints_.end()) {
			CYNG_LOG_TRACE(logger_, "[cluster] trying " << endpoint_iter->endpoint() << "...");

			// Set a deadline for the connect operation.
			timer_.expires_after(std::chrono::seconds(60));

			// Start the asynchronous connect operation.
			socket_.async_connect(endpoint_iter->endpoint(),
				std::bind(&bus::handle_connect, this,
					std::placeholders::_1, endpoint_iter));
		}
		else {

			//
			// There are no more endpoints to try. Shut down the client.
			//
			reset();

			//
			//	alter connection endpoint
			//
			tgl_.changeover();
			CYNG_LOG_WARNING(logger_, "[cluster] connect failed - switch to " << tgl_.get());

			//
			//	reconnect after 20 seconds
			//
			timer_.expires_after(boost::asio::chrono::seconds(20));
			timer_.async_wait(boost::bind(&bus::check_deadline, this, boost::asio::placeholders::error));

		}
	}

	void bus::handle_connect(const boost::system::error_code& ec,
		boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter) {

		if (is_stopped())	return;

		// The async_connect() function automatically opens the socket at the start
		// of the asynchronous operation. If the socket is closed at this time then
		// the timeout handler must have run first.
		if (!socket_.is_open()) {

			CYNG_LOG_WARNING(logger_, "[cluster] " << tgl_.get() << " connect timed out");

			// Try the next available endpoint.
			start_connect(++endpoint_iter);
		}

		// Check if the connect operation failed before the deadline expired.
		else if (ec)
		{
			CYNG_LOG_WARNING(logger_, "[cluster] " << tgl_.get() << " connect error " << ec.value() << ": " << ec.message());

			// We need to close the socket used in the previous connection attempt
			// before starting a new one.
			socket_.close();

			// Try the next available endpoint.
			start_connect(++endpoint_iter);
		}

		// Otherwise we have successfully established a connection.
		else
		{
			CYNG_LOG_INFO(logger_, "[cluster] " << tgl_.get() << " connected to " << endpoint_iter->endpoint());
			state_ = state::CONNECTED;

			//
			//	send login sequence
			//
			auto cfg = tgl_.get();
			buffer_write_ = cyng::serialize_invoke("cluster.login"
				, cfg.pwd_
				, cfg.account_
				, cyng::sys::get_process_id()
				, node_name_);

			// Start the input actor.
			do_read();

			// Start the heartbeat actor.
			do_write();
		}

	}

	void bus::do_read()
	{
		//
		//	connect was successful
		//

		// Start an asynchronous operation to read a newline-delimited message.
		socket_.async_read_some(boost::asio::buffer(input_buffer_), std::bind(&bus::handle_read, this,
			std::placeholders::_1, std::placeholders::_2));
	}

	void bus::do_write()
	{
		if (is_stopped())	return;

		// Start an asynchronous operation to send a heartbeat message.
		boost::asio::async_write(socket_,
			boost::asio::buffer(buffer_write_.front().data(), buffer_write_.front().size()),
			std::bind(&bus::handle_write, this, std::placeholders::_1));
	}

	void bus::handle_read(const boost::system::error_code& ec, std::size_t n)
	{
		if (is_stopped())	return;

		if (!ec)
		{
			CYNG_LOG_DEBUG(logger_, "[cluster] " << tgl_.get() << " received " << n << " bytes");

			//parser_.read(input_buffer_.begin(), input_buffer_.begin() + n);

			//
			//	continue reading
			//
			do_read();
		}
		else
		{
			CYNG_LOG_WARNING(logger_, "[cluster] " << tgl_.get() << " read " << ec.value() << ": " << ec.message());
			reset();

			//
			//	reconnect after 10/20 seconds
			//
			timer_.expires_after((ec == boost::asio::error::connection_reset)
				? boost::asio::chrono::seconds(10)
				: boost::asio::chrono::seconds(20));
			timer_.async_wait(boost::bind(&bus::check_deadline, this, boost::asio::placeholders::error));

		}
	}

	void bus::handle_write(const boost::system::error_code& ec)
	{
		if (is_stopped())	return;

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
			CYNG_LOG_ERROR(logger_, "[cluster] " << tgl_.get() << " on heartbeat: " << ec.message());

			reset();
			//auto sp = channel_.lock();
			//if (sp)	sp->suspend(std::chrono::seconds(12), "start", cyng::make_tuple());
		}
	}

}

