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
			, master_config_t const& cfg)
		: base_(*btp)
			, bus_(bus_factory(btp->mux_, logger, boost::uuids::random_generator()(), scramble_key::default_scramble_key_, btp->get_id(), "ipt:stress:sender"))
			, logger_(logger)
			, config_(cfg)
			, master_(0)
			, task_state_(TASK_STATE_INITIAL_)
		{
			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> is running");

			//
			//	request handler
			//
			bus_->vm_.async_run(cyng::register_function("network.task.resume", 4, std::bind(&sender::task_resume, this, std::placeholders::_1)));
			bus_->vm_.async_run(cyng::register_function("bus.reconfigure", 1, std::bind(&sender::reconfigure, this, std::placeholders::_1)));
			bus_->vm_.async_run(cyng::register_function("ipt.res.open.connection", 3, std::bind(&sender::res_open_connection, this, std::placeholders::_1)));

		}

		void sender::run()
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
				}
				else
				{
					base_.suspend(config_[master_].monitor_);
				}

				if (task_state_ == TASK_STATE_CONNECTED_)
				{
					bus_->vm_.async_run(cyng::generate_invoke("ipt.transfer.data", cyng::buffer_t({ 'h', 'e', 'l', 'l', 'o' })));
				}

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

		void sender::stop()
		{
			CYNG_LOG_INFO(logger_, "network is stopped");
		}

		//	slot 0
		cyng::continuation sender::process(std::uint16_t watchdog, std::string redirect)
		{
			if (watchdog != 0)
			{
				CYNG_LOG_INFO(logger_, "start watchdog: " << watchdog << " minutes");
				base_.suspend(std::chrono::minutes(watchdog));
			}

			//
			//	authorized
			//
			BOOST_ASSERT_MSG(task_state_ == TASK_STATE_INITIAL_, "wrong task state");
			task_state_ = TASK_STATE_AUTHORIZED_;

			return cyng::continuation::TASK_CONTINUE;
		}

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

		cyng::continuation sender::process(sequence_type, std::string const& number)
		{
			return cyng::continuation::TASK_CONTINUE;
		}

		cyng::continuation sender::process(sequence_type, std::uint32_t, std::uint32_t, cyng::buffer_t const&)
		{
			return cyng::continuation::TASK_CONTINUE;
		}

		cyng::continuation sender::process(sequence_type, bool, std::uint32_t)
		{
			return cyng::continuation::TASK_CONTINUE;
		}

		cyng::continuation sender::process(cyng::buffer_t const&)
		{
			return cyng::continuation::TASK_CONTINUE;
		}

		cyng::continuation sender::process(std::string const& number)
		{
			//
			//	open connection to receiver
			//
			CYNG_LOG_INFO(logger_, "open connection to: " << number);
			bus_->vm_.async_run(cyng::generate_invoke("req.open.connection", number));
			bus_->vm_.async_run(cyng::generate_invoke("stream.flush"));
			return cyng::continuation::TASK_CONTINUE;
		}

		void sender::task_resume(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			//	[1,0,TDevice,3]
			CYNG_LOG_TRACE(logger_, "resume task - " << cyng::io::to_str(frame));
			std::size_t tsk = cyng::value_cast<std::size_t>(frame.at(0), 0);
			std::size_t slot = cyng::value_cast<std::size_t>(frame.at(1), 0);
			base_.mux_.send(tsk, slot, cyng::tuple_t{ frame.at(2), frame.at(3) });
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

		cyng::vector_t sender::ipt_req_login_public() const
		{
			CYNG_LOG_INFO(logger_, "send public login request [ "
				<< master_
				<< " ] "
				<< config_[master_].host_
				<< ':'
				<< config_[master_].service_);

			return gen::ipt_req_login_public(config_, master_);
		}

		cyng::vector_t sender::ipt_req_login_scrambled() const
		{
			CYNG_LOG_INFO(logger_, "send scrambled login request [ "
				<< master_
				<< " ] "
				<< config_[master_].host_
				<< ':'
				<< config_[master_].service_);

			return gen::ipt_req_login_scrambled(config_, master_);
		}

		void sender::res_open_connection(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			//	[fb05dc46-eb0f-4a01-bab7-5ae0c64c5e8e,1,1]
			//
			//	* session tag
			//	* ipt seq
			//	* response
			//CYNG_LOG_TRACE(logger_, "ipt.res.open.connection - " << cyng::io::to_str(frame));

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,		//	[0] session tag
				sequence_type,			//	[1] ipt seq
				response_type			//	[2] response
			>(frame);

			//
			//	read response type
			//
			auto res = make_tp_res_open_connection(std::get<1>(tpl), std::get<2>(tpl));
			CYNG_LOG_TRACE(logger_, "ipt.res.open.connection - " << res.get_response_name());

			if (res.is_success())
			{
				BOOST_ASSERT_MSG(task_state_ == TASK_STATE_AUTHORIZED_, "wrong task state");
				task_state_ = TASK_STATE_CONNECTED_;

				bus_->vm_.async_run(cyng::generate_invoke("ipt.transfer.data", cyng::buffer_t({'w', 'e', 'l', 'c', 'o', 'm', 'e' })));
				bus_->vm_.async_run(cyng::generate_invoke("stream.flush"));

			}
			else
			{
				CYNG_LOG_WARNING(logger_, "ipt.res.open.connection failed");

			}

		}

	}
}
