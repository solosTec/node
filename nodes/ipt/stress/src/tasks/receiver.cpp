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
			, std::size_t tsk_sender)
		: base_(*btp)
			, bus_(bus_factory(btp->mux_, logger, boost::uuids::random_generator()(), scramble_key::default_scramble_key_, btp->get_id(), "ipt:stress:receiver"))
			, logger_(logger)
			, config_(cfg)
			, tsk_sender_(tsk_sender)
			, master_(0)
		{
			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> is running");

			//
			//	request handler
			//
			bus_->vm_.register_function("network.task.resume", 4, std::bind(&receiver::task_resume, this, std::placeholders::_1));
			bus_->vm_.register_function("bus.reconfigure", 1, std::bind(&receiver::reconfigure, this, std::placeholders::_1));

		}

		void receiver::run()
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
				bus_->vm_.async_run(cyng::generate_invoke("ipt.reset.parser", config_[master_].sk_));
				bus_->vm_.async_run(cyng::generate_invoke("ipt.reset.serializer", config_[master_].sk_));

				//
				//	login request
				//
				if (config_[master_].scrambled_)
				{
					bus_->vm_.async_run(ipt_req_login_scrambled());
				}
				else
				{
					bus_->vm_.async_run(ipt_req_login_public());
				}
			}
		}

		void receiver::stop()
		{
			CYNG_LOG_INFO(logger_, "network is stopped");
		}

		//	slot 0
		cyng::continuation receiver::process(std::uint16_t watchdog, std::string redirect)
		{
			if (watchdog != 0)
			{
				CYNG_LOG_INFO(logger_, "start watchdog: " << watchdog << " minutes");
				base_.suspend(std::chrono::minutes(watchdog));
			}

			//
			//	waiting for connection open request
			//	trigger sender (account is equal to number)
			//
			base_.mux_.post(tsk_sender_, 6, cyng::tuple_factory(config_[master_].account_));

			return cyng::continuation::TASK_CONTINUE;
		}

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

		cyng::continuation receiver::process(sequence_type seq, std::string const& number)
		{
			CYNG_LOG_TRACE(logger_, config_[master_].account_
				<< " incoming call " << +seq << ':' << number);

			//
			//	accept incoming calls
			//
			bus_->vm_.async_run(cyng::generate_invoke("res.open.connection", seq, static_cast<response_type>(ipt::tp_res_open_connection_policy::DIALUP_SUCCESS)));
			bus_->vm_.async_run(cyng::generate_invoke("stream.flush"));

			return cyng::continuation::TASK_CONTINUE;
		}

		cyng::continuation receiver::process(sequence_type, std::uint32_t, std::uint32_t, cyng::buffer_t const&)
		{
			return cyng::continuation::TASK_CONTINUE;
		}

		cyng::continuation receiver::process(sequence_type, bool, std::uint32_t)
		{
			return cyng::continuation::TASK_CONTINUE;
		}

		cyng::continuation receiver::process(cyng::buffer_t const& data)
		{
			CYNG_LOG_TRACE(logger_, config_[master_].account_
				<< " received " 
				<< std::string(data.begin(), data.end()))
				;

			//std::this_thread::sleep_for(std::chrono::milliseconds(100));
			bus_->vm_.async_run(cyng::generate_invoke("ipt.transfer.data", cyng::buffer_t({ 'p', 'i', 'n', 'g' })));
			bus_->vm_.async_run(cyng::generate_invoke("stream.flush"));

			return cyng::continuation::TASK_CONTINUE;
		}

		void receiver::task_resume(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			//	[1,0,TDevice,3]
			CYNG_LOG_TRACE(logger_, "resume task - " << cyng::io::to_str(frame));
			std::size_t tsk = cyng::value_cast<std::size_t>(frame.at(0), 0);
			std::size_t slot = cyng::value_cast<std::size_t>(frame.at(1), 0);
			base_.mux_.post(tsk, slot, cyng::tuple_t{ frame.at(2), frame.at(3) });
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
			if (config_.size() > 1)
			{
				master_++;
				if (master_ == config_.size())
				{
					master_ = 0;
				}
				CYNG_LOG_INFO(logger_, "switch to redundancy "
					<< config_[master_].host_
					<< ':'
					<< config_[master_].service_);

			}
			else
			{
				CYNG_LOG_ERROR(logger_, "network login failed - no other redundancy available");
			}

			//
			//	trigger reconnect 
			//
			CYNG_LOG_INFO(logger_, "reconnect to network in "
				<< config_[master_].monitor_.count()
				<< " seconds");
			base_.suspend(config_[master_].monitor_);

		}

		cyng::vector_t receiver::ipt_req_login_public() const
		{
			CYNG_LOG_INFO(logger_, "send public login request [ "
				<< master_
				<< " ] "
				<< config_[master_].account_
				<< '@'
				<< config_[master_].host_
				<< ':'
				<< config_[master_].service_);

			return gen::ipt_req_login_public(config_, master_);
		}

		cyng::vector_t receiver::ipt_req_login_scrambled() const
		{
			CYNG_LOG_INFO(logger_, "send scrambled login request [ "
				<< master_
				<< " ] "
				<< config_[master_].account_
				<< '@'
				<< config_[master_].host_
				<< ':'
				<< config_[master_].service_);

			return gen::ipt_req_login_scrambled(config_, master_);
		}

	}
}
