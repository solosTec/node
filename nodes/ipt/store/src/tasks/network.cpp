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
			, std::vector<std::size_t> const& tsks
			, master_config_t const& cfg
			, std::map<std::string, std::string> const& targets)
		: base_(*btp)
			, bus_(bus_factory(btp->mux_, logger, boost::uuids::random_generator()(), scramble_key::default_scramble_key_, btp->get_id(), "ipt:store"))
			, logger_(logger)
			, tasks_(tsks)
			, config_(cfg)
			, targets_(targets)
			, master_(0)
			, seq_target_map_()
			, channel_target_map_()
		{
			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> is running");

			//
			//	request handler
			//
			bus_->vm_.register_function("network.task.resume", 4, std::bind(&network::task_resume, this, std::placeholders::_1));
			bus_->vm_.register_function("bus.reconfigure", 1, std::bind(&network::reconfigure, this, std::placeholders::_1));

			bus_->vm_.register_function("net.insert.rel", 1, std::bind(&network::insert_rel, this, std::placeholders::_1));
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
					bus_->vm_.async_run(ipt_req_login_scrambled());
				}
				else
				{
					bus_->vm_.async_run(ipt_req_login_public());
				}
			}
		}

		void network::stop()
		{
			bus_->stop();
			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> is stopped");
		}

		//	slot 0
		cyng::continuation network::process(std::uint16_t watchdog, std::string redirect)
		{
			if (watchdog != 0)
			{
				CYNG_LOG_INFO(logger_, "start watchdog: " << watchdog << " minutes");
				base_.suspend(std::chrono::minutes(watchdog));
			}

			//
			//	register targets
			//
			register_targets();

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

		cyng::continuation network::process(sequence_type seq, std::string const& number)
		{
			CYNG_LOG_TRACE(logger_, "incoming call " << +seq << ':' << number);

			//
			//	don't accept incoming calls
			//
			bus_->vm_.async_run(cyng::generate_invoke("res.open.connection", seq, static_cast<response_type>(ipt::tp_res_open_connection_policy::BUSY)));
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

			auto pos = channel_target_map_.find(channel);
			if (pos != channel_target_map_.end())
			{
				CYNG_LOG_TRACE(logger_, "distribute "
					<< data.size()
					<< " bytes from ["
					<< pos->second
					<< "] to "
					<< tasks_.size()
					<< " task(s)");

				for (auto tsk : tasks_)
				{
					CYNG_LOG_DEBUG(logger_, "send "
						<< data.size()
						<< " bytes to task "
						<< tsk);

					//
					//	distribute async
					//
					base_.mux_.post(tsk, 0, cyng::tuple_factory(channel, source, pos->second, data));
				}
			}
			else
			{
				CYNG_LOG_ERROR(logger_, "channel "
					<< channel
					<< " has no target "
					<< data.size()
					<< " bytes get lost");

			}

			//
			//	continue task
			//
			return cyng::continuation::TASK_CONTINUE;
		}

		//	slot [4] - register target response
		cyng::continuation network::process(sequence_type seq, bool success, std::uint32_t channel)
		{
			if (success)
			{
				auto pos = seq_target_map_.find(seq);
				if (pos != seq_target_map_.end())
				{

					//
					//	get data layer
					//
					auto target = targets_.find(pos->second);
					if (target != targets_.end())
					{
						CYNG_LOG_INFO(logger_, "channel "
							<< channel
							<< " ==> "
							<< pos->second
							<< ':'
							<< target->second);

						//
						//	insert into channel => target map
						//
						channel_target_map_.emplace(channel, pos->second);

					}
					else
					{
						CYNG_LOG_ERROR(logger_, "target "
							<< pos->second
							<< " not found in target list");
					}

					//
					//	remove from sequence => target map
					//
					seq_target_map_.erase(pos);
				}
				else
				{
					CYNG_LOG_ERROR(logger_, "no entry for sequence "
						<< seq
						<< " found");
				}
			}

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
			base_.mux_.post(tsk, slot, cyng::tuple_t{ frame.at(2), frame.at(3) });
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

		cyng::vector_t network::ipt_req_login_public() const
		{
			CYNG_LOG_INFO(logger_, "send public login request [ "
				<< master_
				<< " ] "
				<< config_[master_].host_
				<< ':'
				<< config_[master_].service_);

			return gen::ipt_req_login_public(config_, master_);
		}

		cyng::vector_t network::ipt_req_login_scrambled() const
		{
			CYNG_LOG_INFO(logger_, "send scrambled login request [ "
				<< master_
				<< " ] "
				<< config_[master_].host_
				<< ':'
				<< config_[master_].service_);

			return gen::ipt_req_login_scrambled(config_, master_);

		}

		void network::register_targets()
		{
			if (targets_.empty())
			{
				CYNG_LOG_WARNING(logger_, "no push targets defined");
			}
			else
			{
				seq_target_map_.clear();
				channel_target_map_.clear();
				for (auto target : targets_)
				{
					CYNG_LOG_INFO(logger_, "register target " << target.first << ':' << target.second);
					cyng::vector_t prg;
					prg
						<< cyng::generate_invoke_unwinded("req.register.push.target", target.first, static_cast<std::uint16_t>(0xffff), static_cast<std::uint8_t>(1))
						<< cyng::generate_invoke_unwinded("net.insert.rel", cyng::invoke("ipt.push.seq"),  target.first)
						<< cyng::generate_invoke_unwinded("stream.flush")
						;

					bus_->vm_.async_run(std::move(prg));
				}
			}
		}

		void network::insert_rel(cyng::context& ctx)
		{
			//	[5,power@solostec]
			//
			//	* ipt sequence
			//	* target name
			const cyng::vector_t frame = ctx.get_frame();

			auto const tpl = cyng::tuple_cast<
				sequence_type,		//	[0] ipt seq
				std::string			//	[1] target
			>(frame);

			CYNG_LOG_TRACE(logger_, "ipt sequence " 
				<< +std::get<0>(tpl)
				<< " ==> "
				<< std::get<1>(tpl));

			seq_target_map_.emplace(std::get<0>(tpl), std::get<1>(tpl));
		}
	}
}
