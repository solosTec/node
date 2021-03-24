/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf.h>
#include <smf/ipt/bus.h>
#include <smf/ipt/codes.h>
#include <smf/ipt/transpiler.h>
#include <smf/ipt/response.hpp>

#include <cyng/vm/mesh.h>
#include <cyng/log/record.h>
#include <cyng/vm/vm.h>

#include <functional>

#include <boost/bind.hpp>

namespace smf
{
	namespace ipt
	{
		bus::bus(boost::asio::io_context& ctx
			, cyng::logger logger
			, toggle::server_vec_t&& tgl
			, std::string model
			, parser::command_cb cb_cmd
			, parser::data_cb cb_stream)
		: state_(state::START)
			, logger_(logger)
			, tgl_(std::move(tgl))
			, model_(model)
			, cb_cmd_(cb_cmd)
			, endpoints_()
			, socket_(ctx)
			, timer_(ctx)
			, serializer_(tgl_.get().sk_)
			, parser_(tgl_.get().sk_
				, std::bind(&bus::cmd_complete, this, std::placeholders::_1, std::placeholders::_2), cb_stream)
			, buffer_write_()
			, input_buffer_()
		{}

		void bus::start() {
			BOOST_ASSERT(state_ == state::START);

			CYNG_LOG_INFO(logger_, "start IP-T client [" << tgl_.get() << "]");

			//
			//	connect to IP-T server
			//
			boost::asio::ip::tcp::resolver r(socket_.get_executor());
			connect(r.resolve(tgl_.get().host_, tgl_.get().service_));

		}

		void bus::stop() {
			CYNG_LOG_INFO(logger_, "ipt " << tgl_.get() << " stop");
			state_ = state::STOPPED;
		}

		void bus::reset() {
			buffer_write_.clear();
			boost::system::error_code ignored_ec;
			socket_.close(ignored_ec);
			timer_.cancel();
			state_ = state::START;
		}


		void bus::connect(boost::asio::ip::tcp::resolver::results_type endpoints) {

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
				//CYNG_LOG_TRACE(logger_, "broker [" << target_ << "] trying " << endpoint_iter->endpoint() << "...");

				// Set a deadline for the connect operation.
				timer_.expires_after(std::chrono::seconds(60));

				// Start the asynchronous connect operation.
				socket_.async_connect(endpoint_iter->endpoint(),
					std::bind(&bus::handle_connect, this,
						std::placeholders::_1, endpoint_iter));
			}
			else {
				// There are no more endpoints to try. Shut down the client.
				reset();
				
				//
				//	switch rdundancy
				//
				tgl_.changeover();
				CYNG_LOG_WARNING(logger_, "[ipt] connect failed - switch to " << tgl_.get());

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

				CYNG_LOG_WARNING(logger_, "ipt [" << tgl_.get() << "] connect timed out");

				// Try the next available endpoint.
				start_connect(++endpoint_iter);
			}

			// Check if the connect operation failed before the deadline expired.
			else if (ec)
			{
				CYNG_LOG_WARNING(logger_, "ipt [" << tgl_.get() << "] connect error: " << ec.message());

				// We need to close the socket used in the previous connection attempt
				// before starting a new one.
				socket_.close();

				// Try the next available endpoint.
				start_connect(++endpoint_iter);
			}

			// Otherwise we have successfully established a connection.
			else
			{
				CYNG_LOG_INFO(logger_, "ipt [" << tgl_.get() << "] connected to " << endpoint_iter->endpoint());
				state_ = state::CONNECTED;

				//
				//	send login sequence
				//
				if (tgl_.get().scrambled_) {

					buffer_write_.push_back(serializer_.req_login_scrambled(
						tgl_.get().account_,
						tgl_.get().pwd_,
						tgl_.get().sk_));
				}
				else {
					buffer_write_.push_back(serializer_.req_login_public(
						tgl_.get().account_,
						tgl_.get().pwd_));

				}

				// Start the input actor.
				do_read();

				// Start the heartbeat actor.
				do_write();
			}

		}

		void bus::check_deadline(boost::system::error_code const& ec)
		{
			if (is_stopped())	return;
			CYNG_LOG_TRACE(logger_, "[ipt] check deadline " << ec);
			if (!ec) {
				switch (state_) {
				case state::START:
					CYNG_LOG_TRACE(logger_, "[ipt] check deadline: start");
					start();
					break;
				case state::CONNECTED:
					CYNG_LOG_TRACE(logger_, "[ipt] check deadline: connected");
					break;
				case state::AUTHORIZED:
					CYNG_LOG_TRACE(logger_, "[ipt] check deadline: authorized");
					break;
				case state::LINKED:
					CYNG_LOG_TRACE(logger_, "[ipt] check deadline: linked");
					break;
				case state::STOPPED:
					CYNG_LOG_TRACE(logger_, "[ipt] check deadline: stopped");
					break;
				default:
					break;
				}
			}
			else {
				CYNG_LOG_TRACE(logger_, "[ipt] check deadline timer cancelled");
			}
		}

		void bus::do_read()
		{
			// Set a deadline for the read operation.
			timer_.expires_after(std::chrono::minutes(20));

			// Start an asynchronous operation to read a newline-delimited message.
			socket_.async_read_some(boost::asio::buffer(input_buffer_), std::bind(&bus::handle_read, this,
				std::placeholders::_1, std::placeholders::_2));
		}

		void bus::do_write()
		{
			if (is_stopped())	return;

			CYNG_LOG_TRACE(logger_, "ipt [" << tgl_.get() << "] write: " << buffer_write_.front().size() << " bytes");

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
				CYNG_LOG_DEBUG(logger_, "ipt [" << tgl_.get() << "] received " << n << " bytes");

				parser_.read(input_buffer_.begin(), input_buffer_.begin() + n);

				//
				//	continue reading
				//
				do_read();
			}
			else
			{
				CYNG_LOG_ERROR(logger_, "ipt [" << tgl_.get() << "] on receive: " << ec.message());

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
				CYNG_LOG_ERROR(logger_, "ipt [" << tgl_.get() << "] on heartbeat: " << ec.message());

				reset();
			}
		}

		void bus::cmd_complete(header const& h, cyng::buffer_t&& body) {
			CYNG_LOG_TRACE(logger_, "ipt [" << tgl_.get() << "] cmd " << command_name(h.command_));	

			//auto instructions = gen_instructions(h, std::move(body));

			switch (to_code(h.command_)) {
			case code::TP_RES_OPEN_PUSH_CHANNEL:
				cb_cmd_(h, std::move(body));
				break;
			case code::TP_RES_CLOSE_PUSH_CHANNEL:
				cb_cmd_(h, std::move(body));
				break;
			case code::TP_RES_PUSHDATA_TRANSFER:
				cb_cmd_(h, std::move(body));
				break;
			case code::TP_REQ_OPEN_CONNECTION:
				cb_cmd_(h, std::move(body));
				break;
			case code::TP_RES_OPEN_CONNECTION:
				cb_cmd_(h, std::move(body));
				break;
			case code::TP_REQ_CLOSE_CONNECTION:
				cb_cmd_(h, std::move(body));
				break;
			case code::TP_RES_CLOSE_CONNECTION:
				cb_cmd_(h, std::move(body));
				break;
			case code::APP_REQ_PROTOCOL_VERSION:
				buffer_write_.push_back(serializer_.res_protocol_version(h.sequence_, 1));
				break;
			case code::APP_REQ_SOFTWARE_VERSION:
				buffer_write_.push_back(serializer_.res_software_version(h.sequence_, SMF_VERSION_NAME));
				break;
			case code::APP_REQ_DEVICE_IDENTIFIER:
				buffer_write_.push_back(serializer_.res_device_id(h.sequence_, model_));
				break;
			case code::APP_REQ_DEVICE_AUTHENTIFICATION:
				buffer_write_.push_back(serializer_.res_device_auth(h.sequence_
					, tgl_.get().account_
					, tgl_.get().pwd_
					, tgl_.get().account_	//	number
					, model_));
				break;
			case code::APP_REQ_DEVICE_TIME:
				buffer_write_.push_back(serializer_.res_device_time(h.sequence_));
				break;
			case code::CTRL_RES_LOGIN_PUBLIC:
				res_login(std::move(body));
				break;
			case code::CTRL_RES_LOGIN_SCRAMBLED:
				res_login(std::move(body));
				break;

			//case code::CTRL_REQ_LOGOUT:
			//	break;
			//case code::CTRL_RES_LOGOUT:
			//	break;

			case code::CTRL_REQ_REGISTER_TARGET:
				buffer_write_.push_back(serializer_.res_unknown_command(h.sequence_, h.command_));
				break;
			case code::CTRL_RES_REGISTER_TARGET:
				cb_cmd_(h, std::move(body));
				break;
			case code::CTRL_REQ_DEREGISTER_TARGET:
				buffer_write_.push_back(serializer_.res_unknown_command(h.sequence_, h.command_));
				break;
			case code::CTRL_RES_DEREGISTER_TARGET:
				cb_cmd_(h, std::move(body));
				break;

			case code::UNKNOWN:
			default:
				buffer_write_.push_back(serializer_.res_unknown_command(h.sequence_, h.command_));
				break;
			}

			if (!buffer_write_.empty())	do_write();
		}

		void bus::res_login(cyng::buffer_t&& data) {
			auto const [res, watchdog, redirect] = ctrl_res_login(std::move(data));
			auto r = make_login_response(res);
			if (r.is_success()) {

				CYNG_LOG_INFO(logger_, "ipt [" << tgl_.get() << "] " << r.get_response_name());

				//
				//	update state
				//
				state_ = state::AUTHORIZED;

				//
				//	ToDo: set watchdog
				//
			}
			else {
				CYNG_LOG_WARNING(logger_, "ipt [" << tgl_.get() << "] login failed: " << r.get_response_name());
			}
		}


	}	//	ipt
}



