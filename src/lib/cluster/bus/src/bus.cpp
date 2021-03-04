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

namespace smf {

	bus::bus(cyng::mesh& mesh, cyng::logger logger, toggle::server_vec_t&& tgl)
	: fabric_(mesh)
		, logger_(logger)
		, tgl_(std::move(tgl))
		, stopped_(false)
		, endpoints_()
		, socket_(mesh.get_ctl().get_ctx())
		, deadline_(mesh.get_ctl().get_ctx())
		, buffer_write_()
		, input_buffer_()
	{}

	void bus::start() {
		auto const srv = tgl_.get();
		CYNG_LOG_INFO(logger_, "[cluster] connect(" << srv << ")");

		//
		//	connect to cluster
		//
		boost::asio::ip::tcp::resolver r(fabric_.get_ctl().get_ctx());
		connect(r.resolve(srv.host_, srv.service_));

	}

	void bus::stop() {
		CYNG_LOG_INFO(logger_, "[cluster] " << tgl_.get() << " stop");

		//buffer_write_.clear();
		stopped_ = true;
		boost::system::error_code ignored_ec;
		socket_.close(ignored_ec);
		//deadline_.cancel();
		//heartbeat_timer_.cancel();

	}

	void bus::connect(boost::asio::ip::tcp::resolver::results_type endpoints) {

		// Start the connect actor.
		endpoints_ = endpoints;
		start_connect(endpoints_.begin());

		// Start the deadline actor. You will note that we're not setting any
		// particular deadline here. Instead, the connect and input actors will
		// update the deadline prior to each asynchronous operation.
		//deadline_.async_wait(std::bind(&bus::check_deadline, this));

	}

	void bus::start_connect(boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter) {
		if (endpoint_iter != endpoints_.end()) {
			CYNG_LOG_TRACE(logger_, "[cluster] trying " << endpoint_iter->endpoint() << "...");

			//	Set a deadline for the connect operation.
			deadline_.expires_after(std::chrono::seconds(5));

			// Start the asynchronous connect operation.
			socket_.async_connect(endpoint_iter->endpoint(),
				std::bind(&bus::handle_connect, this,
					std::placeholders::_1, endpoint_iter));
		}
		else {
			// There are no more endpoints to try. Shut down the client.
			stop();
			//auto sp = channel_.lock();
			//if (sp)	sp->suspend(std::chrono::seconds(12), "start", cyng::make_tuple());

		}
	}

	void bus::handle_connect(const boost::system::error_code& ec,
		boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter) {

		if (stopped_)	return;

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
			CYNG_LOG_WARNING(logger_, "[cluster] " << tgl_.get() << " connect error: " << ec.message());

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

			//
			//	send login sequence
			//
			auto cfg = tgl_.get();
			//buffer_write_ = cyng::convert(cfg.pwd_, cfg.account_, 2, "cluster.login", cyng::op::INVOKE);
			buffer_write_ = cyng::serialize_invoke("cluster.login", cfg.pwd_, cfg.account_);

			// Start the input actor.
			do_read();

			// Start the heartbeat actor.
			do_write();
		}

	}

	void bus::check_deadline()
	{
		if (stopped_)	return;

		// Check whether the deadline has passed. We compare the deadline against
		// the current time since a new asynchronous operation may have moved the
		// deadline before this actor had a chance to run.
		if (deadline_.expiry() <= boost::asio::steady_timer::clock_type::now())
		{
			CYNG_LOG_INFO(logger_, "[cluster] " << tgl_.get() << " deadline expired");

			// The deadline has passed. The socket is closed so that any outstanding
			// asynchronous operations are cancelled.
			socket_.close();

			// There is no longer an active deadline. The expiry is set to the
			// maximum time point so that the actor takes no action until a new
			// deadline is set.
			deadline_.expires_at(boost::asio::steady_timer::time_point::max());
		}

		// Put the actor back to sleep.
		deadline_.async_wait(std::bind(&bus::check_deadline, this));
	}

	void bus::do_read()
	{
		// Set a deadline for the read operation.
		deadline_.expires_after(std::chrono::minutes(20));

		// Start an asynchronous operation to read a newline-delimited message.
		socket_.async_read_some(boost::asio::buffer(input_buffer_), std::bind(&bus::handle_read, this,
			std::placeholders::_1, std::placeholders::_2));
	}

	void bus::do_write()
	{
		if (stopped_)	return;

		// Start an asynchronous operation to send a heartbeat message.
		boost::asio::async_write(socket_,
			boost::asio::buffer(buffer_write_.front().data(), buffer_write_.front().size()),
			std::bind(&bus::handle_write, this, std::placeholders::_1));
	}

	void bus::handle_read(const boost::system::error_code& ec, std::size_t n)
	{
		if (stopped_)	return;

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
			CYNG_LOG_ERROR(logger_, "[cluster] " << tgl_.get() << " on receive: " << ec.message());

			stop();
			//auto sp = channel_.lock();
			//if (sp)	sp->suspend(std::chrono::seconds(12), "start", cyng::make_tuple());
		}
	}

	void bus::handle_write(const boost::system::error_code& ec)
	{
		if (stopped_)	return;

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

			stop();
			//auto sp = channel_.lock();
			//if (sp)	sp->suspend(std::chrono::seconds(12), "start", cyng::make_tuple());
		}
	}

}

