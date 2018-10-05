/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "receiver.h"
#include <smf/ipt/response.hpp>
#include <smf/ipt/generator.h>
#include <cyng/async/task/task_builder.hpp>
#include <cyng/io/serializer.h>
#include <cyng/vm/generator.h>
#include <boost/uuid/random_generator.hpp>

namespace node
{
	namespace ipt
	{
		receiver::receiver(cyng::async::base_task* btp
			, cyng::logging::log_ptr logger
			, master_config_t const& cfg
			, std::size_t tsk_sender
			, std::size_t rec_limit
			, std::size_t packet_size_min
			, std::size_t packet_size_max
			, std::chrono::milliseconds delay
			, std::size_t retries)
		: base_(*btp)
			, bus_(bus_factory(btp->mux_, logger, boost::uuids::random_generator()(), scramble_key::default_scramble_key_, btp->get_id(), "ipt:stress:receiver", retries))
			, logger_(logger)
			, config_(cfg)
			, tsk_sender_(tsk_sender)
			, rec_limit_(rec_limit)
			, packet_size_min_(packet_size_min)
			, packet_size_max_(packet_size_max)
			, delay_(delay)
			, rnd_device_()
			, mersenne_engine_(rnd_device_())
			, total_bytes_received_(0)
		{
			CYNG_LOG_INFO(logger_, "initialize task #"
				<< base_.get_id()
				<< ':'
				<< bus_->vm_.tag()
				<< " <"
				<< base_.get_class_name()
				<< "> "
				<< config_.get().account_);

			//
			//	request handler
			//
			bus_->vm_.register_function("bus.reconfigure", 1, std::bind(&receiver::reconfigure, this, std::placeholders::_1));

		}

		cyng::continuation receiver::run()
		{
			if (bus_->is_online())
			{
				//
				//	send watchdog response - without request
				//
				bus_->vm_.async_run(cyng::generate_invoke("res.watchdog", static_cast<std::uint8_t>(0)));
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
				bus_->req_login(config_.get());
			}

			return cyng::continuation::TASK_CONTINUE;
		}

		void receiver::stop()
		{
			bus_->stop();
			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> is stopped");
		}

		//	slot [0] 0x4001/0x4002: response login
		cyng::continuation receiver::process(std::uint16_t watchdog, std::string redirect)
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
				CYNG_LOG_INFO(logger_, "start watchdog: " << watchdog << " minutes");
				base_.suspend(std::chrono::minutes(watchdog));
			}

			//
			//	waiting for connection open request
			//	trigger sender (account is equal to number)
			//
			CYNG_LOG_INFO(logger_, "trigger #: " << tsk_sender_ << " to open connection");
			base_.mux_.post(tsk_sender_, 11u, cyng::tuple_factory(config_.get().account_));

			return cyng::continuation::TASK_CONTINUE;
		}

		//	slot 1
		cyng::continuation receiver::process()
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
		cyng::continuation receiver::process(sequence_type seq
			, bool success
			, std::uint32_t channel)
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
		cyng::continuation receiver::process(sequence_type, bool, std::string const&)
		{
			return cyng::continuation::TASK_CONTINUE;

		}

		//	slot [4] 0x1000: push channel open response
		cyng::continuation receiver::process(sequence_type seq
			, bool success
			, std::uint32_t channel
			, std::uint32_t source
			, std::uint16_t status
			, std::size_t count)
		{

			return cyng::continuation::TASK_CONTINUE;
		}

		//	slot [5] 0x1001: push channel close response
		cyng::continuation receiver::process(sequence_type, bool, std::uint32_t, std::string res)
		{
			return cyng::continuation::TASK_CONTINUE;
		}

		//	slot [6] 0x9003: connection open request 
		cyng::continuation receiver::process(sequence_type seq, std::string const& number)
		{
			CYNG_LOG_TRACE(logger_, config_.get().account_
				<< " incoming call " << +seq << ':' << number);

			//
			//	accept incoming calls
			//
			bus_->res_connection_open(seq, true);

			return cyng::continuation::TASK_CONTINUE;
		}

		//	slot [7] 0x1003: connection open response
		cyng::continuation receiver::process(sequence_type seq, bool success)
		{
			return cyng::continuation::TASK_CONTINUE;
		}

		//	slot [8] 0x9004: connection close request
		cyng::continuation receiver::process(sequence_type, bool req, std::size_t origin)
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

			//
			//	trigger new open connection
			//
			if (!req) {
				CYNG_LOG_INFO(logger_, "trigger #: " << tsk_sender_ << " to re-open connection");
				base_.mux_.post(tsk_sender_, 11u, cyng::tuple_factory(config_.get().account_));
			}

			return cyng::continuation::TASK_CONTINUE;
		}

		//	slot [9] 0x9002: push data transfer request
		cyng::continuation receiver::process(sequence_type, std::uint32_t channel, std::uint32_t source, cyng::buffer_t const& data)
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
		cyng::continuation receiver::process(cyng::buffer_t const& data)
		{
			total_bytes_received_ += data.size();

			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> "
				<< config_.get().account_
				<< " received "
				<< data.size()
				<< " ("
				<< ((total_bytes_received_ * 100) / rec_limit_)
				<< "%)");

			std::this_thread::sleep_for(delay_);

			if (total_bytes_received_ > rec_limit_) {

				CYNG_LOG_INFO(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> "
					<< config_.get().account_
					<< " is closing connection");

				//
				//	close connection
				//
				bus_->req_connection_close(std::chrono::seconds(12));
				total_bytes_received_ = 0u;

			}
			else {
				//
				//	buffer size
				//
				std::uniform_int_distribution<int> dist_buffer_size(packet_size_min_, packet_size_max_);
				cyng::buffer_t buffer(dist_buffer_size(rnd_device_));

				//
				//	fill buffer
				//
				std::uniform_int_distribution<int> dist(std::numeric_limits<char>::min(), std::numeric_limits<char>::max());
				auto gen = std::bind(dist, mersenne_engine_);
				std::generate(begin(buffer), end(buffer), gen);

				bus_->vm_.async_run(cyng::generate_invoke("ipt.transfer.data", buffer));
				bus_->vm_.async_run(cyng::generate_invoke("stream.flush"));
			}

			return cyng::continuation::TASK_CONTINUE;
		}

		void receiver::reconfigure(cyng::context& ctx)
		{
			reconfigure_impl();
		}

		void receiver::reconfigure_impl()
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

	}
}
