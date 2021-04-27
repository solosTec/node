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
			, parser::data_cb cb_stream
			, auth_cb cb_auth)
		: state_(state::START)
			, logger_(logger)
			, tgl_(std::move(tgl))
			, model_(model)
			, cb_cmd_(cb_cmd)
			, cb_auth_(cb_auth)
			, endpoints_()
			, socket_(ctx)
			, timer_(ctx)
			, dispatcher_(ctx)
			, serializer_(tgl_.get().sk_)
			, parser_(tgl_.get().sk_
				, std::bind(&bus::cmd_complete, this, std::placeholders::_1, std::placeholders::_2), cb_stream)
			, buffer_write_()
			, input_buffer_()
			, registrant_()
			, targets_()
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

			if (state_ == state::AUTHORIZED) {
				//
				//	signal changed authorization state
				//
				cb_auth_(false);
			}

			state_ = state::START;
		}


		void bus::connect(boost::asio::ip::tcp::resolver::results_type endpoints) {

			// Start the connect actor.
			endpoints_ = endpoints;
			start_connect(endpoints_.begin());

			// Start the deadline actor. You will note that we're not setting any
			// particular deadline here. Instead, the connect and input actors will
			// update the deadline prior to each asynchronous operation.
			timer_.async_wait(boost::asio::bind_executor(dispatcher_, boost::bind(&bus::check_deadline, this, boost::asio::placeholders::error)));

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
				//	switch redundancy
				//
				tgl_.changeover();
				CYNG_LOG_WARNING(logger_, "[ipt] connect failed - switch to " << tgl_.get());

				//
				//	reconnect after 20 seconds
				//
				timer_.expires_after(boost::asio::chrono::seconds(20));
				timer_.async_wait(boost::asio::bind_executor(dispatcher_, boost::bind(&bus::check_deadline, this, boost::asio::placeholders::error)));

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
				auto const srv = tgl_.get();
				if (srv.scrambled_) {

					//
					//	use a random key
					//
					auto const sk = gen_random_sk();

					send(serializer_.req_login_scrambled(
						srv.account_,
						srv.pwd_,
						sk));
					parser_.set_sk(sk);
				}
				else {
					send(serializer_.req_login_public(
						srv.account_,
						srv.pwd_));
				}

				// Start the input actor.
				do_read();

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
					//CYNG_LOG_DEBUG(logger_, "[ipt] check deadline: connected");
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
				dispatcher_.wrap(std::bind(&bus::handle_write, this, std::placeholders::_1)));
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
				timer_.async_wait(boost::asio::bind_executor(dispatcher_, boost::bind(&bus::check_deadline, this, boost::asio::placeholders::error)));
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

		void bus::send(cyng::buffer_t data) {
			boost::asio::post(dispatcher_, [this, data]() {
				bool const b = buffer_write_.empty();
				buffer_write_.push_back(data);
				if (b)	do_write();
				});
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
			case code::TP_REQ_PUSHDATA_TRANSFER:
				pushdata_transfer(h, std::move(body));
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
				send(serializer_.res_protocol_version(h.sequence_, 1));
				break;
			case code::APP_REQ_SOFTWARE_VERSION:
				send(serializer_.res_software_version(h.sequence_, SMF_VERSION_NAME));
				break;
			case code::APP_REQ_DEVICE_IDENTIFIER:
				send(serializer_.res_device_id(h.sequence_, model_));
				break;
			case code::APP_REQ_DEVICE_AUTHENTIFICATION:
				send(serializer_.res_device_auth(h.sequence_
					, tgl_.get().account_
					, tgl_.get().pwd_
					, tgl_.get().account_	//	number
					, model_));
				break;
			case code::APP_REQ_DEVICE_TIME:
				send(serializer_.res_device_time(h.sequence_));
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
				send(serializer_.res_unknown_command(h.sequence_, h.command_));
				break;
			case code::CTRL_RES_REGISTER_TARGET:
				res_register_target(h, std::move(body));
				break;
			case code::CTRL_REQ_DEREGISTER_TARGET:
				send(serializer_.res_unknown_command(h.sequence_, h.command_));
				break;
			case code::CTRL_RES_DEREGISTER_TARGET:
				cb_cmd_(h, std::move(body));
				break;
			case code::CTRL_REQ_WATCHDOG:
				req_watchdog(h, std::move(body));
				break;

			case code::UNKNOWN:
			default:
				send(serializer_.res_unknown_command(h.sequence_, h.command_));
				break;
			}
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
				//	set watchdog
				//
				if (watchdog != 0) {
					CYNG_LOG_INFO(logger_, "ipt [" << tgl_.get() << "] set watchdog: " << watchdog << " minutes");

					//
					//	ToDo:
					//
				}

				//
				//	signal changed authorization state
				//
				cb_auth_(true);
			}
			else {
				CYNG_LOG_WARNING(logger_, "ipt [" << tgl_.get() << "] login failed: " << r.get_response_name());
			}
		}

		void bus::register_target(std::string name, cyng::channel_weak wp) {

			boost::asio::post(dispatcher_, [this, name, wp]() {

				bool const b = buffer_write_.empty();
				auto r = serializer_.req_register_push_target(name
					, std::numeric_limits<std::uint16_t>::max()	//	0xffff
					, 1);

				//
				//	send register command to ip-t server
				//
				registrant_.emplace(r.second, wp);
				buffer_write_.push_back(r.first);

				//
				//	send request
				//
				if (b)	do_write();
				});

		}

		void bus::res_register_target(header const& h, cyng::buffer_t&& body) {
			auto const pos = registrant_.find(h.sequence_);
			if (pos != registrant_.end()) {

				auto const [res, channel] = ctrl_res_register_target(std::move(body));
				CYNG_LOG_INFO(logger_, "[ipt] cmd " 
					<< ipt::command_name(h.command_) 
					<< ": " 
					<< ctrl_res_register_target_policy::get_response_name(res)
					<< ", channel: "
					<< channel);

				auto sp = pos->second.lock();
				if (sp) {
					if (ctrl_res_register_target_policy::is_success(res)) {
						registrant_.emplace(channel, std::move(pos->second));
					}
					else {
						//
						//	stop task
						//
						sp->stop();
					}
				}

				//
				//	cleanup list
				//
				registrant_.erase(pos);
			}
			else {
				CYNG_LOG_ERROR(logger_, "[ipt] cmd " << ipt::command_name(h.command_) << ": missing registrant");
				cb_cmd_(h, std::move(body));
			}
		}

		void bus::pushdata_transfer(header const& h, cyng::buffer_t&& body) {

			/**
			 * @return channel, source, status, block and data
			 */
			auto [channel, source, status, block, data] = tp_req_pushdata_transfer(std::move(body));

			auto const pos = targets_.find(channel);
			if (pos != targets_.end()) {
				CYNG_LOG_TRACE(logger_, "[ipt] cmd " << ipt::command_name(h.command_) << " " << channel << ':' << source << " - " << data.size() << " bytes");
				auto sp = pos->second.lock();
				if (sp) {
					sp->dispatch("receive", cyng::make_tuple(channel, source, data));
				}
				else {
					//
					//	task removed - clean up
					//
					targets_.erase(pos);
				}
			}
			else {
				CYNG_LOG_WARNING(logger_, "[ipt] cmd " << ipt::command_name(h.command_) << " " << channel << ':' << source << " dropped");
			}
		}

		void bus::req_watchdog(header const& h, cyng::buffer_t&& body) {
			BOOST_ASSERT(body.empty());
			CYNG_LOG_TRACE(logger_, "[ipt] cmd " << ipt::command_name(h.command_) << " " << +h.sequence_);
			send(serializer_.res_watchdog(h.sequence_));
		}


	}	//	ipt
}



