/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "sender.h"
#include <smf/ipt/response.hpp>
#include <smf/ipt/generator.h>
#include <cyng/async/task/task_builder.hpp>
#include <cyng/io/serializer.h>
#include <cyng/vm/generator.h>
#include <cyng/tuple_cast.hpp>
#include <boost/uuid/random_generator.hpp>

namespace node
{
	namespace ipt
	{

		sender::sender(cyng::async::base_task* btp
			, cyng::logging::log_ptr logger
			, redundancy const& cfg
			, std::size_t packet_size_min
			, std::size_t packet_size_max
			, std::chrono::milliseconds delay
			, std::size_t retries)
		: base_(*btp)
			, bus(logger
				, btp->mux_
				, boost::uuids::random_generator()()
				, cfg.get().sk_
				, "ipt:stress:sender"
				, retries)
			, logger_(logger)
			, config_(cfg)
			, packet_size_min_(packet_size_min)
			, packet_size_max_(packet_size_max)
			, delay_(delay)
			, rnd_device_()
			, mersenne_engine_(rnd_device_())
		{
			CYNG_LOG_INFO(logger_, "initialize task #"
				<< base_.get_id()
				<< ':'
				<< vm_.tag()
				<< " <"
				<< base_.get_class_name()
				<< "> "
				<< config_.get().account_);

			//
			//	request handler
			//
			vm_.register_function("bus.reconfigure", 1, std::bind(&sender::reconfigure, this, std::placeholders::_1));

			//
			//	statistics
			//
			vm_.async_run(cyng::generate_invoke("log.msg.info", cyng::invoke("lib.size"), "callbacks registered"));
		}

		cyng::continuation sender::run()
		{
			if (is_online())
			{
				base_.suspend(config_.get().monitor_);

				CYNG_LOG_INFO(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> "
					<< get_state()
					<< " has monitor with "
					<< config_.get().monitor_.count()
					<< " seconds(s)");

				if (this->is_connected())
				{
					//std::uniform_int_distribution<int> dist_buffer_size(512, 1024);
					//cyng::buffer_t buffer(dist_buffer_size(rnd_device_));

					//CYNG_LOG_TRACE(logger_, "task #"
					//	<< base_.get_id()
					//	<< " <"
					//	<< base_.get_class_name()
					//	<< "> buffer size "
					//	<< buffer.size());

					////
					////	fill buffer
					////
					//std::uniform_int_distribution<int> dist(std::numeric_limits<char>::min(), std::numeric_limits<char>::max());
					//auto gen = std::bind(dist, mersenne_engine_);

					//std::generate(begin(buffer), end(buffer), gen);
					vm_.async_run({cyng::generate_invoke("ipt.transfer.data", send_data()), cyng::generate_invoke("stream.flush")});
				}
				else
				{
					CYNG_LOG_WARNING(logger_, "task #"
						<< base_.get_id()
						<< " <"
						<< base_.get_class_name()
						<< "> "
						<< config_.get().account_
						<< " is online but not connected");
				}
			}
			else
			{
				//
				//	reset parser and serializer
				//
				vm_.async_run({ cyng::generate_invoke("ipt.reset.parser", config_.get().sk_)
					, cyng::generate_invoke("ipt.reset.serializer", config_.get().sk_) });

				//
				//	login request
				//
				req_login(config_.get());
			}

			return cyng::continuation::TASK_CONTINUE;
		}

		void sender::stop(bool shutdown)
		{
			//
			//	call base class
			//
			bus::stop();
			while (!vm_.is_halted()) {
				CYNG_LOG_INFO(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> continue gracefull shutdown");
				std::this_thread::sleep_for(delay_);
			}
			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> is stopped");
		}

		//	slot [0] 0x4001/0x4002: response login
		void sender::on_login_response(std::uint16_t watchdog, std::string redirect)
		{
			//
			//	authorization successful
			//
			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> "
				<< config_.get().account_
				<< " successful authorized");

			//
			//	re/start monitor
			//
			base_.suspend(config_.get().monitor_);

			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> has monitor with "
				<< config_.get().monitor_.count()
				<< " seconds(s)");

			//
			//	authorized
			//
			vm_.async_run({ cyng::generate_invoke("ipt.transfer.data", send_data()), cyng::generate_invoke("stream.flush") });

		}

		//	slot [1] - connection lost / reconnect
		void sender::on_logout()
		{
			BOOST_ASSERT_MSG(!this->is_connected(), "state error");

			//
			//	switch to other configuration
			//
			reconfigure_impl();
		}

		//	slot [2] - 0x4005: push target registered response
		void sender::on_res_register_target(sequence_type seq, bool success, std::uint32_t channel, std::string)
		{
			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> "
				<< config_.get().account_
				<< " push target registered #"
				<< channel);
		}

		//	@brief slot [3] - 0x4006: push target deregistered response
		void sender::on_res_deregister_target(sequence_type, bool, std::string const&)
		{	//	no implementation
			CYNG_LOG_WARNING(logger_, "push target deregistered response not implemented");
		}

		//	slot [4] 0x1000: push channel open response
		void sender::on_res_open_push_channel(sequence_type seq
			, bool success
			, std::uint32_t channel
			, std::uint32_t source
			, std::uint16_t status
			, std::size_t count)
		{	//	no implementation
			CYNG_LOG_WARNING(logger_, "push channel open response not implemented");
		}

		//	slot [5] - 0x1001: push channel close response
		void sender::on_res_close_push_channel(sequence_type seq
			, bool success
			, std::uint32_t channel)
		{	//	no implementation
			CYNG_LOG_WARNING(logger_, "push channel close response not implemented");
		}

		//	slot [6] 0x9003: connection open request 
		bool sender::on_req_open_connection(sequence_type seq, std::string const& number)
		{
			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> "
				<< config_.get().account_
				<< " connection open request from "
				<< number);

			//
			//	don't accept incoming calls
			//
			return false;
		}

		//	slot [7] 0x1003: connection open response
		cyng::buffer_t sender::on_res_open_connection(sequence_type seq, bool success)
		{
			if (success) {
				CYNG_LOG_INFO(logger_, "connection established - send data");
				return send_data();
			}

			CYNG_LOG_WARNING(logger_, "open connection failed");
			return cyng::buffer_t();
		}

		//	slot [8] 0x9004: connection close request
		void sender::on_req_close_connection(sequence_type)
		{
			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> "
				<< config_.get().account_
				<< " received connection close request");
		}

		void sender::on_res_close_connection(sequence_type)
		{
			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> "
				<< config_.get().account_
				<< " received connection close response")
		}


		//	slot [9] 0x9002: push data transfer request
		void sender::on_req_transfer_push_data(sequence_type, std::uint32_t channel, std::uint32_t source, cyng::buffer_t const& data)
		{
			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> "
				<< config_.get().account_
				<< " received "
				<< data.size()
				<< " bytes push data from "
				<< channel
				<< '.'
				<< source);
		}

		//	slot [10] transmit data(if connected)
		cyng::buffer_t sender::on_transmit_data(cyng::buffer_t const& data)
		{
			CYNG_LOG_TRACE(logger_, config_.get().account_
				<< " received "
				<< data.size()
				<< " bytes");

			std::this_thread::sleep_for(delay_);
			return send_data();
		}


		//	slot 11
		cyng::continuation sender::process(std::string number)
		{
			//
			//	open connection to receiver
			//
			if (!req_connection_open(number, std::chrono::seconds(12))) {
				CYNG_LOG_WARNING(logger_, "cannot connect to: " << number);
			}
			else {
				CYNG_LOG_INFO(logger_, "*** open connection to: " << number);
			}

			return cyng::continuation::TASK_CONTINUE;
		}

		void sender::reconfigure(cyng::context& ctx)
		{
			reconfigure_impl();
		}

		void sender::reconfigure_impl()
		{
			//
			//	switch to other master
			//
			if (config_.next())
			{
				CYNG_LOG_INFO(logger_, "switch to redundancy "
					<< config_.get().host_
					<< ':'
					<< config_.get().service_);

			}
			else
			{
				CYNG_LOG_ERROR(logger_, "network login failed - no other redundancy available");
			}

			//
			//	trigger reconnect 
			//
			CYNG_LOG_INFO(logger_, "reconnect to network in "
				<< cyng::to_str(config_.get().monitor_));

			base_.suspend(config_.get().monitor_);
		}

		cyng::buffer_t sender::send_data()
		{
			//
			//	buffer size
			//
			std::uniform_int_distribution<int> dist_buffer_size(packet_size_min_, packet_size_max_);
			cyng::buffer_t buffer(dist_buffer_size(rnd_device_));
			BOOST_ASSERT(!buffer.empty());

			CYNG_LOG_INFO(logger_, "initialize task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> is sending "
				<< buffer.size()
				<< " bytes");

			//
			//	fill buffer
			//
			std::uniform_int_distribution<int> dist(std::numeric_limits<char>::min(), std::numeric_limits<char>::max());
			auto gen = std::bind(dist, mersenne_engine_);
			std::generate(begin(buffer), end(buffer), gen);
#ifdef _DEBUG
			buffer.at(0) = 0x00;
#endif

			return buffer;

		}

	}
}
