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
			, master_config_t const& cfg
			, std::size_t packet_size_min
			, std::size_t packet_size_max
			, std::chrono::milliseconds delay)
		: base_(*btp)
			, bus_(bus_factory(btp->mux_, logger, boost::uuids::random_generator()(), scramble_key::default_scramble_key_, btp->get_id(), "ipt:stress:sender"))
			, logger_(logger)
			, config_(cfg)
			, packet_size_min_(packet_size_min)
			, packet_size_max_(packet_size_max)
			, delay_(delay)
			, task_state_(TASK_STATE_INITIAL_)
			, rnd_device_()
			, mersenne_engine_(rnd_device_())
		{
			CYNG_LOG_INFO(logger_, "initialize task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> "
				<< config_.get().account_);

			//
			//	request handler
			//
			bus_->vm_.register_function("bus.reconfigure", 1, std::bind(&sender::reconfigure, this, std::placeholders::_1));

		}

		cyng::continuation sender::run()
		{
			if (bus_->is_online())
			{
				//
				//	send watchdog response - without request
				//
				if (bus_->has_watchdog())
				{
					bus_->vm_.async_run(cyng::generate_invoke("res.watchdog", static_cast<std::uint8_t>(0)));
					base_.suspend(std::chrono::minutes(bus_->get_watchdog()));
					CYNG_LOG_INFO(logger_, "task #"
						<< base_.get_id()
						<< " <"
						<< base_.get_class_name()
						<< "> has watchdog with "
						<< bus_->get_watchdog()
						<< " minute(s)");
				}
				else
				{
					base_.suspend(config_.get().monitor_);
					CYNG_LOG_INFO(logger_, "task #"
						<< base_.get_id()
						<< " <"
						<< base_.get_class_name()
						<< "> has monitor with "
						<< config_.get().monitor_.count()
						<< " seconds(s)");
				}

				if (task_state_ == TASK_STATE_CONNECTED_)
				{
					std::uniform_int_distribution<int> dist_buffer_size(512, 1024);
					cyng::buffer_t buffer(dist_buffer_size(rnd_device_));

					CYNG_LOG_TRACE(logger_, "task #"
						<< base_.get_id()
						<< " <"
						<< base_.get_class_name()
						<< "> buffer size "
						<< buffer.size());

					//
					//	fill buffer
					//
					std::uniform_int_distribution<int> dist(std::numeric_limits<char>::min(), std::numeric_limits<char>::max());
					auto gen = std::bind(dist, mersenne_engine_);

					std::generate(begin(buffer), end(buffer), gen);
					bus_->vm_.async_run(cyng::generate_invoke("ipt.transfer.data", buffer));
				}
				else
				{
					CYNG_LOG_WARNING(logger_, "task #"
						<< base_.get_id()
						<< " <"
						<< base_.get_class_name()
						<< "> "
						<< config_.get().account_
						<< " is not connected");

				}

				bus_->vm_.async_run(cyng::generate_invoke("stream.flush"));
			}
			else
			{
				//
				//	reset parser and serializer
				//
				bus_->vm_.async_run(cyng::generate_invoke("ipt.reset.parser", config_.get().sk_));
				bus_->vm_.async_run(cyng::generate_invoke("ipt.reset.serializer", config_.get().sk_));

				//
				//	login request
				//
				if (config_.get().scrambled_)
				{
					bus_->vm_.async_run(ipt_req_login_scrambled());
				}
				else
				{
					bus_->vm_.async_run(ipt_req_login_public());
				}
			}

			return cyng::continuation::TASK_CONTINUE;
		}

		void sender::stop()
		{
			bus_->stop();
			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> is stopped");
		}

		//	slot [0] 0x4001/0x4002: response login
		cyng::continuation sender::process(std::uint16_t watchdog, std::string redirect)
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

			if (watchdog != 0)
			{
				CYNG_LOG_INFO(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> has watchdog with "
					<< watchdog
					<< " minute(s)");
				base_.suspend(std::chrono::minutes(watchdog));
			}
			else
			{
				CYNG_LOG_INFO(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> has monitor with "
					<< config_.get().monitor_.count()
					<< " seconds(s)");
			}

			//
			//	authorized
			//
			BOOST_ASSERT_MSG(task_state_ == TASK_STATE_INITIAL_, "wrong task state");
			task_state_ = TASK_STATE_AUTHORIZED_;

			return cyng::continuation::TASK_CONTINUE;
		}

		//	slot 1
		cyng::continuation sender::process()
		{
			//
			//	switch to other configuration
			//
			reconfigure_impl();

			//
			//	continue task
			//
			return cyng::continuation::TASK_CONTINUE;
		}

		//	slot [2] 0x4005: push target registered response
		cyng::continuation sender::process(sequence_type seq, bool success, std::uint32_t channel)
		{
			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> "
				<< config_.get().account_
				<< " push target registered #"
				<< channel);

			return cyng::continuation::TASK_CONTINUE;
		}

		//	slot [3] 0x4006: push target deregistered response
		cyng::continuation sender::process(sequence_type, bool, std::string const&)
		{
			return cyng::continuation::TASK_CONTINUE;
		}

		//	slot [4] 0x1000: push channel open response
		cyng::continuation sender::process(sequence_type seq
			, bool res
			, std::uint32_t channel
			, std::uint32_t source
			, std::uint16_t status
			, std::size_t count)
		{
			return cyng::continuation::TASK_CONTINUE;
		}

		//	slot [5] 0x1001: push channel close response
		cyng::continuation sender::process(sequence_type seq
			, bool success
			, std::uint32_t channel
			, std::string res)
		{
			return cyng::continuation::TASK_CONTINUE;
		}

		//	slot [6] 0x9003: connection open request 
		cyng::continuation sender::process(sequence_type seq, std::string const& number)
		{
			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> "
				<< config_.get().account_
				<< " connection open request from "
				<< number);

			return cyng::continuation::TASK_CONTINUE;
		}

		//	slot [7] 0x1003: connection open response
		cyng::continuation sender::process(sequence_type seq, bool success)
		{
			if (success) {
				CYNG_LOG_INFO(logger_, "connection established - send data");
				send_data();
			}
			else {
				CYNG_LOG_WARNING(logger_, "open connection failed");

			}
			return cyng::continuation::TASK_CONTINUE;
		}

		//	slot [8] 0x9004: connection close request
		cyng::continuation sender::process(sequence_type, bool req, std::size_t origin)
		{
			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> "
				<< config_.get().account_
				<< " received connection close "
				<< (req ? "request" : "response")
				<< " #"
				<< origin);

			return cyng::continuation::TASK_CONTINUE;
		}

		//	slot [9] 0x9002: push data transfer request
		cyng::continuation sender::process(sequence_type, std::uint32_t channel, std::uint32_t source, cyng::buffer_t const& data)
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

			return cyng::continuation::TASK_CONTINUE;
		}

		//	slot [10] transmit data(if connected)
		cyng::continuation sender::process(cyng::buffer_t const& data)
		{
			CYNG_LOG_TRACE(logger_, config_.get().account_
				<< " received "
				<< data.size()
				<< " bytes");

			std::this_thread::sleep_for(delay_);
			send_data();
			return cyng::continuation::TASK_CONTINUE;
		}


		//	slot 11
		cyng::continuation sender::process(std::string number)
		{
			//
			//	open connection to receiver
			//
			CYNG_LOG_INFO(logger_, "*** open connection to: " << number);
			bus_->req_connection_open(number, std::chrono::seconds(12));
			return cyng::continuation::TASK_CONTINUE;
		}

		void sender::reconfigure(cyng::context& ctx)
		{
			reconfigure_impl();
		}

		void sender::reconfigure_impl()
		{
			task_state_ = TASK_STATE_INITIAL_;

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

		cyng::vector_t sender::ipt_req_login_public() const
		{
			CYNG_LOG_INFO(logger_, "send public login request "
				<< config_.get().account_
				<< '@'
				<< config_.get().host_
				<< ':'
				<< config_.get().service_);

			return gen::ipt_req_login_public(config_.get());
		}

		cyng::vector_t sender::ipt_req_login_scrambled() const
		{
			CYNG_LOG_INFO(logger_, "send scrambled login request "
				<< config_.get().account_
				<< '@'
				<< config_.get().host_
				<< ':'
				<< config_.get().service_);

			return gen::ipt_req_login_scrambled(config_.get());
		}

		void sender::send_data()
		{
			//
			//	buffer size
			//
			std::uniform_int_distribution<int> dist_buffer_size(packet_size_min_, packet_size_max_);
			cyng::buffer_t buffer(dist_buffer_size(rnd_device_));

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

			bus_->vm_.async_run(cyng::generate_invoke("ipt.transfer.data", buffer));
			bus_->vm_.async_run(cyng::generate_invoke("stream.flush"));

		}

	}
}
