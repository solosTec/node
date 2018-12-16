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
			, redundancy const& cfg
			, std::vector<std::size_t> const& tsk_vec
			, std::size_t tsk_sender
			, std::size_t rec_limit
			, std::size_t packet_size_min
			, std::size_t packet_size_max
			, std::chrono::milliseconds delay
			, std::size_t retries)
		: base_(*btp)
			, bus(logger
				, btp->mux_
				, boost::uuids::random_generator()()
				, cfg.get().sk_
				, "ipt:stress:receiver"
				, retries)
			, logger_(logger)
			, config_(cfg)
			, tsk_vec_(tsk_vec)
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
				<< vm_.tag()
				<< " <"
				<< base_.get_class_name()
				<< "> "
				<< config_.get().account_);

			//
			//	request handler
			//
			vm_.register_function("bus.reconfigure", 1, std::bind(&receiver::reconfigure, this, std::placeholders::_1));

			//
			//	statistics
			//
			vm_.async_run(cyng::generate_invoke("log.msg.info", cyng::invoke("lib.size"), "callbacks registered"));
		}

		cyng::continuation receiver::run()
		{
			if (is_online())
			{
				if (!is_connected()) {

					//
					//	trigger new open connection
					//
					CYNG_LOG_INFO(logger_, "trigger #" << tsk_vec_[tsk_sender_] << " to re-open connection");
					base_.mux_.post(tsk_vec_[tsk_sender_], 0u, cyng::tuple_factory(config_.get().account_));

					//
					//	calculate next target
					//
					tsk_sender_ = next_tsk();

				}

				//
				//	re/start monitor
				//
				base_.suspend(config_.get().monitor_);
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

		void receiver::stop()
		{
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
		void receiver::on_login_response(std::uint16_t watchdog, std::string redirect)
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

			//
			//	calculate next target
			//
			tsk_sender_ = next_tsk();

			//
			//	waiting for connection open request
			//	trigger sender (account is equal to number)
			//
			CYNG_LOG_INFO(logger_, "trigger #: " << tsk_vec_[tsk_sender_] << " to open connection");
			base_.mux_.post(tsk_vec_[tsk_sender_], 0u, cyng::tuple_factory(config_.get().account_));

		}

		//	slot [1] - connection lost / reconnect
		void receiver::on_logout()
		{
			//
			//	switch to other configuration
			//
			reconfigure_impl();

		}


		//	slot [2] 0x4005: push target registered response
		void receiver::on_res_register_target(sequence_type seq
			, bool success
			, std::uint32_t channel
			, std::string target)
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

		//	slot [3] 0x4006: push target deregistered response
		void receiver::on_res_deregister_target(sequence_type, bool, std::string const&)
		{	//	no implementation
			CYNG_LOG_WARNING(logger_, "push target deregistered response not implemented");
		}

		//	slot [4] 0x1000: push channel open response
		void receiver::on_res_open_push_channel(sequence_type seq
			, bool success
			, std::uint32_t channel
			, std::uint32_t source
			, std::uint16_t status
			, std::size_t count)
		{	//	no implementation
			CYNG_LOG_WARNING(logger_, "push channel open response not implemented");
		}

		//	slot [5] 0x1001: push channel close response
		void receiver::on_res_close_push_channel(sequence_type, bool, std::uint32_t)
		{	//	no implementation
			CYNG_LOG_WARNING(logger_, "push channel close response not implemented");
		}

		//	slot [6] 0x9003: connection open request 
		bool receiver::on_req_open_connection(sequence_type seq, std::string const& number)
		{
			CYNG_LOG_TRACE(logger_, config_.get().account_
				<< " incoming call " << +seq << ':' << number);

			//
			//	accept incoming call
			//
			return true;
		}

		//	slot [7] 0x1003: connection open response
		cyng::buffer_t receiver::on_res_open_connection(sequence_type seq, bool success)
		{	//	no implementation
			CYNG_LOG_WARNING(logger_, "connection open response not implemented");
			return cyng::buffer_t();	//	no data
		}

		//	slot [8] 0x9004: connection close request
		void receiver::on_req_close_connection(sequence_type)
		{
			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> "
				<< config_.get().account_
				<< " received connection close request");
		}

		void receiver::on_res_close_connection(sequence_type)
		{
			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> "
				<< config_.get().account_
				<< " received connection close response");

			//
			//	trigger new open connection
			//
			CYNG_LOG_INFO(logger_, "trigger #: " << tsk_vec_[tsk_sender_] << " to re-open connection");
			base_.mux_.post(tsk_vec_[tsk_sender_], 0u, cyng::tuple_factory(config_.get().account_));

			//
			//	calculate next target
			//
			tsk_sender_ = next_tsk();

		}

		//	slot [9] 0x9002: push data transfer request
		void receiver::on_req_transfer_push_data(sequence_type, std::uint32_t channel, std::uint32_t source, cyng::buffer_t const& data)
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
		cyng::buffer_t receiver::on_transmit_data(cyng::buffer_t const& data)
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
				req_connection_close(std::chrono::seconds(21));
				total_bytes_received_ = 0u;
				return cyng::buffer_t();
			}

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

			return buffer;
		}

		std::size_t receiver::next_tsk()
		{
			std::uniform_int_distribution<int> dist(0, tsk_vec_.size() - 1);
			const std::size_t tsk = dist(rnd_device_);
			if (tsk == tsk_sender_) {
				CYNG_LOG_WARNING(logger_, "same task #"	<< tsk);

			}
			BOOST_ASSERT(tsk < tsk_vec_.size());
			return tsk;
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
