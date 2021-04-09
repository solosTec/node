/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/client.h>

#include <cyng/task/channel.h>
#include <cyng/log/record.h>
#include <cyng/task/controller.h>
#include <cyng/obj/buffer_cast.hpp>
#include <cyng/obj/container_factory.hpp>

#include <iostream>

#include <boost/uuid/nil_generator.hpp>

namespace smf {

	client::client(cyng::channel_weak wp
		, cyng::controller& ctl
		, bus& cluster_bus
		, std::shared_ptr<db> db
		, cyng::logger logger
		, cyng::key_t key
		, std::string meter)
	: state_(state::START) 
		, sigs_{
			std::bind(&client::start, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
			std::bind(&client::stop, this, std::placeholders::_1),
		}
		, channel_(wp)
		, ctl_(ctl)
		, bus_(cluster_bus)
		, db_(db)
		, logger_(logger)
		, key_(key)
		, meters_{ meter }
		, endpoints_()
		, socket_(ctl.get_ctx())
		, dispatcher_(ctl.get_ctx())
		, buffer_write_()
		, input_buffer_()
	{
		auto sp = channel_.lock();
		if (sp) {
			std::size_t idx{ 0 };
			sp->set_channel_name("start", idx++);
			CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] started");
		}
	}

	void client::stop(cyng::eod)
	{
		state_ = state::STOPPED;
	}

	void client::start(std::string address, std::string service, std::chrono::seconds interval) {
		CYNG_LOG_INFO(logger_, "start client: " << address << ':' << service);
		state_ = state::START;	//	update client state
		boost::asio::ip::tcp::resolver r(ctl_.get_ctx());
		connect(r.resolve(address, service));
		auto sp = channel_.lock();
		if (sp)	sp->suspend(interval, "start", cyng::make_tuple(address, service, interval));
	}

	void client::connect(boost::asio::ip::tcp::resolver::results_type endpoints) {

		state_ = state::WAIT;

		// Start the connect actor.
		endpoints_ = endpoints;
		start_connect(endpoints_.begin());

		// Start the deadline actor. You will note that we're not setting any
		// particular deadline here. Instead, the connect and input actors will
		// update the deadline prior to each asynchronous operation.
		//timer_.async_wait(boost::bind(&client::check_deadline, this, boost::asio::placeholders::error));
	}

	void client::start_connect(boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter) {

		if (endpoint_iter != endpoints_.end()) {
			CYNG_LOG_TRACE(logger_, "[client] trying " << endpoint_iter->endpoint() << "...");

			//	Set a deadline for the connect operation.
			//timer_.expires_after(std::chrono::seconds(12));

			// Start the asynchronous connect operation.
			socket_.async_connect(endpoint_iter->endpoint(),
				std::bind(&client::handle_connect, this,
					std::placeholders::_1, endpoint_iter));
		}
		else {

			//
			// There are no more endpoints to try. Shut down the client.
			//
			//reset();

			//
			//	reconnect after 20 seconds
			//
			//timer_.expires_after(boost::asio::chrono::seconds(20));
			//timer_.async_wait(boost::bind(&broker::check_deadline, this, boost::asio::placeholders::error));

		}
	}

	void client::handle_connect(const boost::system::error_code& ec,
		boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter) {

		if (is_stopped())	return;

		// The async_connect() function automatically opens the socket at the start
		// of the asynchronous operation. If the socket is closed at this time then
		// the timeout handler must have run first.
		if (!socket_.is_open()) {

			CYNG_LOG_WARNING(logger_, "[client] connect timed out");

			// Try the next available endpoint.
			start_connect(++endpoint_iter);
		}

		// Check if the connect operation failed before the deadline expired.
		else if (ec)
		{
			CYNG_LOG_WARNING(logger_, "[client] connect error " << ec.value() << ": " << ec.message());

			// We need to close the socket used in the previous connection attempt
			// before starting a new one.
			socket_.close();

			// Try the next available endpoint.
			start_connect(++endpoint_iter);
		}

		// Otherwise we have successfully established a connection.
		else
		{
			CYNG_LOG_INFO(logger_, "[client] connected to " << endpoint_iter->endpoint());
			state_ = state::CONNECTED;

			//
			//	update configuration
			//
			bus_.req_db_update("meterIEC"
				, key_
				, cyng::param_map_factory()("lastSeen", std::chrono::system_clock::now()));

			// Start the input actor.
			do_read();

			//
			//	send query
			//
			boost::asio::post(dispatcher_, [this]() {

				buffer_write_.clear();

				for (auto const& name : meters_) {
					CYNG_LOG_INFO(logger_, "[client] query " << name);
					buffer_write_.push_back(generate_query(name));
				}

				if (!buffer_write_.empty()) do_write();

				});
		}
	}

	void client::do_read()
	{
		//
		//	connect was successful
		//

		// Start an asynchronous operation to read
		socket_.async_read_some(boost::asio::buffer(input_buffer_), std::bind(&client::handle_read, this,
			std::placeholders::_1, std::placeholders::_2));

	}

	void client::handle_read(const boost::system::error_code& ec, std::size_t bytes_transferred)
	{
		if (is_stopped())	return;

		if (!ec)
		{
			CYNG_LOG_DEBUG(logger_, "[client] received " << bytes_transferred << " bytes");

			//
			//	parse message
			//
			std::stringstream ss;
			ss << "[client] received " << bytes_transferred << " bytes";

			bus_.req_db_insert_auto("iecUplink", cyng::data_generator(
				std::chrono::system_clock::now(),
				ss.str(),
				socket_.remote_endpoint(),
				boost::uuids::nil_uuid()
			));

			do_read();
		}
		else
		{
			CYNG_LOG_ERROR(logger_, "[client] read " << ec.value() << ": " << ec.message());
			//reset();

			//timer_.expires_after((ec == boost::asio::error::connection_reset)
			//	? boost::asio::chrono::seconds(10)
			//	: boost::asio::chrono::seconds(20));
			//timer_.async_wait(boost::bind(&client::check_deadline, this, boost::asio::placeholders::error));
		}
	}

	void client::do_write()
	{
		if (is_stopped())	return;

		BOOST_ASSERT(!buffer_write_.empty());

		// Start an asynchronous operation to send a heartbeat message.
		boost::asio::async_write(socket_,
			boost::asio::buffer(buffer_write_.front().data(), buffer_write_.front().size()),
			dispatcher_.wrap(std::bind(&client::handle_write, this, std::placeholders::_1)));
	}

	void client::handle_write(const boost::system::error_code& ec)
	{
		if (is_stopped())	return;

		if (!ec) {

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
		else {
			CYNG_LOG_ERROR(logger_, "[client]  on heartbeat: " << ec.message());

			//reset();
		}
	}

	cyng::buffer_t generate_query(std::string meter) {
		std::stringstream ss;
		ss << "/?" << meter << "!\r\n";
		auto const query = ss.str();
		return cyng::to_buffer(query);
	}

}


