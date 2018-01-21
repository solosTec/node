/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#include "network.h"
#include <cyng/async/task/task_builder.hpp>
#include <cyng/io/serializer.h>
#include <cyng/vm/generator.h>
#include <boost/uuid/random_generator.hpp>

namespace node
{
	namespace ipt
	{

		network::network(cyng::async::base_task* btp
			, cyng::logging::log_ptr logger
			//, std::vector<std::size_t> const& tsks
			, master_config_t const& cfg)
		: base_(*btp)
			, bus_(bus_factory(btp->mux_, logger, boost::uuids::random_generator()(), scramble_key::default_scramble_key_, btp->get_id()))
			, logger_(logger)
			, storage_tsk_(0)	//	ToDo
			, config_(cfg)
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
				//	set default sk
				//
				//bus_->vm_.async_run(cyng::generate_invoke("ipt.set.sk.def", config_[master_].sk_));

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
			//bus_->vm_.async_run(bus_shutdown());
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

		cyng::vector_t network::ipt_req_login_public() const
		{
			CYNG_LOG_INFO(logger_, "send public login request [ "
				<< master_
				<< " ] "
				<< config_[master_].host_
				<< ':'
				<< config_[master_].service_);

			cyng::vector_t prg;
			return prg
				<< cyng::generate_invoke("ip.tcp.socket.resolve", config_[master_].host_, config_[master_].service_)
				<< ":SEND-LOGIN-REQUEST"			//	label
				<< cyng::code::JNE					//	jump if no error
				<< cyng::generate_invoke("bus.reconfigure", cyng::code::LERR)
				<< cyng::generate_invoke("log.msg.error", cyng::code::LERR)	// load error register
				<< ":STOP"							//	label
				<< cyng::code::JA					//	jump always
				<< cyng::label(":SEND-LOGIN-REQUEST")
				<< cyng::generate_invoke("ipt.start")		//	start reading ipt network
				<< cyng::generate_invoke("req.login.public", config_[master_].account_, config_[master_].pwd_)
				<< cyng::generate_invoke("stream.flush")
				<< cyng::label(":STOP")
				<< cyng::code::NOOP
				<< cyng::reloc()
				;
		}

		cyng::vector_t network::ipt_req_login_scrambled() const
		{
			CYNG_LOG_INFO(logger_, "send scrambled login request [ "
				<< master_
				<< " ] "
				<< config_[master_].host_
				<< ':'
				<< config_[master_].service_);

			scramble_key sk = gen_random_sk();

			cyng::vector_t prg;
			return prg
				<< cyng::generate_invoke("ip.tcp.socket.resolve", config_[master_].host_, config_[master_].service_)
				<< ":SEND-LOGIN-REQUEST"			//	label
				<< cyng::code::JNE					//	jump if no error
				<< cyng::generate_invoke("bus.reconfigure", cyng::code::LERR)
				<< cyng::generate_invoke("log.msg.error", cyng::code::LERR)	// load error register
				<< ":STOP"							//	label
				<< cyng::code::JA					//	jump always
				<< cyng::label(":SEND-LOGIN-REQUEST")
				<< cyng::generate_invoke("ipt.start")		//	start reading ipt network
				<< cyng::generate_invoke("ipt.set.sk", sk)	//	set new scramble key for parser
				<< cyng::generate_invoke("req.login.scrambled", config_[master_].account_, config_[master_].pwd_, sk)
				<< cyng::generate_invoke("stream.flush")
				<< cyng::label(":STOP")
				<< cyng::code::NOOP
				<< cyng::reloc()
				;

		}

	}
}
