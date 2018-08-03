/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#include "network.h"
//#include "sync.h"
#include <smf/ipt/generator.h>
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
			, master_config_t const& cfg)
		: base_(*btp)
			, bus_(bus_factory(btp->mux_, logger, boost::uuids::random_generator()(), scramble_key::default_scramble_key_, btp->get_id(), "ipt:collector"))
			, logger_(logger)
			, storage_tsk_(0)	//	ToDo
			, config_(cfg)
		{
			CYNG_LOG_INFO(logger_, "initialize task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< ">");

			//
			//	request handler
			//
			bus_->vm_.register_function("network.task.resume", 4, std::bind(&network::task_resume, this, std::placeholders::_1));
			bus_->vm_.register_function("bus.reconfigure", 1, std::bind(&network::reconfigure, this, std::placeholders::_1));

		}

		cyng::continuation network::run()
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

		cyng::continuation network::process(sequence_type, std::string const& number)
		{	//	slot [2] - no implementation
			return cyng::continuation::TASK_CONTINUE;
		}

		cyng::continuation network::process(sequence_type, std::uint32_t, std::uint32_t, cyng::buffer_t const&)
		{	//	slot [3] - no implementation
			return cyng::continuation::TASK_CONTINUE;
		}

		cyng::continuation network::process(sequence_type, bool, std::uint32_t)
		{	//	slot [4] - no implementation
			return cyng::continuation::TASK_CONTINUE;
		}

		cyng::continuation network::process(cyng::buffer_t const&)
		{	//	slot [5] - no implementation
			return cyng::continuation::TASK_CONTINUE;
		}

		cyng::continuation network::process(sequence_type, bool, std::string const&)
		{	//	slot [6] - no implementation
			return cyng::continuation::TASK_CONTINUE;
		}

		cyng::continuation network::process(sequence_type)
		{	//	slot [7] - no implementation
			return cyng::continuation::TASK_CONTINUE;
		}

		cyng::continuation network::process(sequence_type seq
			, response_type res
			, std::uint32_t channel
			, std::uint32_t source
			, std::uint16_t status
			, std::size_t count)
		{	//	slot [8] - no implementation
			return cyng::continuation::TASK_CONTINUE;
		}

		cyng::continuation network::process(sequence_type seq
			, response_type res
			, std::uint32_t channel)
		{	//	slot [9] no implementation
			return cyng::continuation::TASK_CONTINUE;
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
			//std::cout << "simple::slot-0($" << base_.get_id() << ", v" << v.major() << "." << v.minor() << ")" << std::endl;
			//if (v < cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR))
			//{
			//	CYNG_LOG_WARNING(logger_, "insufficient network protocol version: " << v);
			//}

			//
			//	start data synchronisation
			//
			//cyng::async::start_task_delayed<sync>(base_.mux_
			//	, std::chrono::seconds(1)
			//	, logger_
			//	, bus_
			//	, storage_tsk_
			//	, "TDevice"
			//	, cache_);

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

		cyng::vector_t network::ipt_req_login_public() const
		{
			CYNG_LOG_INFO(logger_, "send public login request "
				<< config_.get().host_
				<< ':'
				<< config_.get().service_);

			return gen::ipt_req_login_public(config_.get());
		}

		cyng::vector_t network::ipt_req_login_scrambled() const
		{
			CYNG_LOG_INFO(logger_, "send scrambled login request "
				<< config_.get().host_
				<< ':'
				<< config_.get().service_);

			return gen::ipt_req_login_scrambled(config_.get());

		}

	}
}
