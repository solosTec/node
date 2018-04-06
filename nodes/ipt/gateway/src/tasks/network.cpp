/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "network.h"
#include <smf/ipt/response.hpp>
#include <smf/ipt/generator.h>
#include <cyng/factory/set_factory.h>
#include <cyng/async/task/task_builder.hpp>
#include <cyng/io/serializer.h>
#include <cyng/vm/generator.h>
#include <cyng/io/serializer.h>
#include <cyng/tuple_cast.hpp>
#include <boost/uuid/random_generator.hpp>

namespace node
{
	namespace ipt
	{

		network::network(cyng::async::base_task* btp
			, cyng::logging::log_ptr logger
			, master_config_t const& cfg)
		: base_(*btp)
			, bus_(bus_factory(btp->mux_, logger, boost::uuids::random_generator()(), scramble_key::default_scramble_key_, btp->get_id(), "ipt:gateway"))
			, logger_(logger)
			, config_(cfg)
			, master_(0)
			, parser_([this](cyng::vector_t&& prg) {
				CYNG_LOG_INFO(logger_, prg.size() << " instructions received");
				CYNG_LOG_TRACE(logger_, cyng::io::to_str(prg));
				//vm_.async_run(std::move(prg));
			}, false)
			, reader_()
		{
			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> is running");

			//
			//	request handler
			//
			bus_->vm_.async_run(cyng::register_function("network.task.resume", 4, std::bind(&network::task_resume, this, std::placeholders::_1)));
			bus_->vm_.async_run(cyng::register_function("bus.reconfigure", 1, std::bind(&network::reconfigure, this, std::placeholders::_1)));

		}

		void network::run()
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
					bus_->vm_.async_run(gen::ipt_req_login_scrambled(config_, master_));
				}
				else
				{
					bus_->vm_.async_run(gen::ipt_req_login_public(config_, master_));
				}
			}
		}

		void network::stop()
		{
			CYNG_LOG_INFO(logger_, "network is stopped");
		}

		//	slot 0
		cyng::continuation network::process(std::uint16_t watchdog, std::string redirect)
		{
			if (watchdog != 0)
			{
				CYNG_LOG_INFO(logger_, "start watchdog: " << watchdog << " minutes");
				base_.suspend(std::chrono::minutes(watchdog));
			}

			return cyng::continuation::TASK_CONTINUE;
		}

		cyng::continuation network::process()
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

		//	slot [2]
		cyng::continuation network::process(sequence_type seq, std::string const& number)
		{
			CYNG_LOG_TRACE(logger_, "incoming call " << +seq << ':' << number);

			BOOST_ASSERT(bus_->is_online());

			//
			//	accept incoming calls
			//
			bus_->vm_.async_run(cyng::generate_invoke("res.open.connection", seq, static_cast<response_type>(ipt::tp_res_open_connection_policy::DIALUP_SUCCESS)));
			bus_->vm_.async_run(cyng::generate_invoke("stream.flush"));

			//
			//	continue task
			//
			return cyng::continuation::TASK_CONTINUE;
		}

		cyng::continuation network::process(sequence_type seq, std::uint32_t channel, std::uint32_t source, cyng::buffer_t const& data)
		{
			//
			//	distribute to output tasks
			//


			//
			//	continue task
			//
			return cyng::continuation::TASK_CONTINUE;
		}

		//	slot [4]
		cyng::continuation network::process(sequence_type seq, bool success, std::uint32_t channel)
		{

			//
			//	continue task
			//
			return cyng::continuation::TASK_CONTINUE;
		}

		//	slot [5]
		cyng::continuation network::process(cyng::buffer_t const& data)
		{
			CYNG_LOG_TRACE(logger_, "incoming SML data " << data.size() << " bytes");

			//
			//	parse incoming data
			//
			parser_.read(data.begin(), data.end());

			//
			//	continue task
			//
			return cyng::continuation::TASK_CONTINUE;
		}

		void network::task_resume(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			//	[1,0,TDevice,3]
			CYNG_LOG_TRACE(logger_, "resume task - " << cyng::io::to_str(frame));
			std::size_t tsk = cyng::value_cast<std::size_t>(frame.at(0), 0);
			std::size_t slot = cyng::value_cast<std::size_t>(frame.at(1), 0);
			base_.mux_.send(tsk, slot, cyng::tuple_t{ frame.at(2), frame.at(3) });
		}

		void network::reconfigure(cyng::context& ctx)
		{
			reconfigure_impl();
		}

		void network::reconfigure_impl()
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

	}
}
