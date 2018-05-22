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
				bus_->vm_.async_run(std::move(prg));
				//vm_.async_run(std::move(prg));
			}, false)
			, reader_()
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
			bus_->vm_.async_run(cyng::register_function("network.task.resume", 4, std::bind(&network::task_resume, this, std::placeholders::_1)));
			bus_->vm_.async_run(cyng::register_function("bus.reconfigure", 1, std::bind(&network::reconfigure, this, std::placeholders::_1)));
			bus_->vm_.register_function("net.insert.rel", 1, std::bind(&network::insert_rel, this, std::placeholders::_1));

			//
			//	SML transport
			//
			bus_->vm_.register_function("sml.msg", 2, std::bind(&network::sml_msg, this, std::placeholders::_1));
			bus_->vm_.register_function("sml.eom", 2, std::bind(&network::sml_eom, this, std::placeholders::_1));

			//
			//	SML data
			//
			bus_->vm_.register_function("sml.public.open.request", 8, std::bind(&network::sml_public_open_request, this, std::placeholders::_1));
			bus_->vm_.register_function("sml.public.close.request", 3, std::bind(&network::sml_public_close_request, this, std::placeholders::_1));
			bus_->vm_.register_function("sml.get.proc.parameter.request", 8, std::bind(&network::sml_get_proc_parameter_request, this, std::placeholders::_1));

		}

		void network::run()
		{
			if (bus_->is_online())
			{
				//
				//	deregister target
				//
				//bus_->vm_.async_run(cyng::generate_invoke("req.deregister.push.target", "DEMO"));

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
				//base_.suspend(std::chrono::minutes(watchdog));
				base_.suspend(std::chrono::seconds(watchdog));
			}

			//
			//	register targets
			//
			//register_targets();

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

		//	slot [4] - register target response
		cyng::continuation network::process(sequence_type seq, bool success, std::uint32_t channel)
		{
			if (success)
			{
				auto pos = seq_target_map_.find(seq);
				if (pos != seq_target_map_.end())
				{
					CYNG_LOG_INFO(logger_, "channel "
						<< channel
						<< " ==> "
						<< pos->second);

					channel_target_map_.emplace(channel, pos->second);
					seq_target_map_.erase(pos);

					//cyng::vector_t prg;
					//prg
					//	<< cyng::generate_invoke_unwinded("req.deregister.push.target", "DEMO")
					//	//<< cyng::generate_invoke_unwinded("net.remove.rel", cyng::invoke("ipt.push.seq"), "DEMO")
					//	<< cyng::generate_invoke_unwinded("stream.flush")
					//	;

					//bus_->vm_.async_run(std::move(prg));

				}
			}

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

		cyng::continuation network::process(sequence_type seq, bool success, std::string const& target)
		{
			CYNG_LOG_INFO(logger_, "target "
				<< target
				<< " deregistered");

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

		void network::register_targets()
		{
			seq_target_map_.clear();
			channel_target_map_.clear();
			CYNG_LOG_INFO(logger_, "register target DEMO" );
			cyng::vector_t prg;
			prg
				<< cyng::generate_invoke_unwinded("req.register.push.target", "DEMO", static_cast<std::uint16_t>(0xffff), static_cast<std::uint8_t>(1))
				<< cyng::generate_invoke_unwinded("net.insert.rel", cyng::invoke("ipt.push.seq"), "DEMO")
				<< cyng::generate_invoke_unwinded("stream.flush")
				;

			bus_->vm_.async_run(std::move(prg));

		}

		void network::sml_msg(cyng::context& ctx)
		{
			//	[{31383035323232323535353231383235322D31,0,0,{0100,{null,005056C00008,3230313830353232323235353532,0500153B02297E,6F70657261746F72,6F70657261746F72,null}},9c91},0]
			//	[{31383035323232323535353231383235322D32,1,0,{0500,{0500153B02297E,6F70657261746F72,6F70657261746F72,{810060050000},null}},f8b5},1]
			//	[{31383035323232323535353231383235322D33,2,0,{0500,{0500153B02297E,6F70657261746F72,6F70657261746F72,{8181C78201FF},null}},4d37},2]
			//	[{31383035323232323535353231383235322D34,0,0,{0200,{null}},e6e8},3]
			//
			const cyng::vector_t frame = ctx.get_frame();
			const std::size_t idx = cyng::value_cast<std::size_t>(frame.at(1), 0);
			//CYNG_LOG_TRACE(logger_, "sml.msg - " << cyng::io::to_str(frame));
			CYNG_LOG_INFO(logger_, "sml.msg #" << idx);

			//
			//	get message body
			//
			cyng::tuple_t msg;
			msg = cyng::value_cast(frame.at(0), msg);
			CYNG_LOG_INFO(logger_, "sml.msg " << cyng::io::to_str(frame));

			reader_.read(ctx, msg, idx);

		}

		void network::sml_eom(cyng::context& ctx)
		{
			//[4aa5,4]
			//
			//	* CRC
			//	* message counter
			//
			const cyng::vector_t frame = ctx.get_frame();
			const std::size_t idx = cyng::value_cast<std::size_t>(frame.at(1), 0);
			//CYNG_LOG_TRACE(logger_, "sml.eom - " << cyng::io::to_str(frame));
			CYNG_LOG_INFO(logger_, "sml.eom #" << idx);
			reader_.reset();

		}

		void network::sml_public_open_request(cyng::context& ctx)
		{
			//	[34481794-6866-4776-8789-6f914b4e34e7,180301091943374505-1,0,005056c00008,00:15:3b:02:23:b3,20180301092332,operator,operator]
			//
			//	* pk
			//	* transaction id
			//	* SML message id
			//	* client ID - MAC from client
			//	* server ID - target server/gateway
			//	* request file id
			//	* username
			//	* password
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_TRACE(logger_, "sml.public.open.request " << cyng::io::to_str(frame));

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,	//	[0] pk
				std::string,		//	[1] trx
				std::uint64_t,		//	[2] SML message id
				std::string,		//	[3] client ID
				std::string,		//	[4] server ID
				std::string,		//	[5] request file id
				std::string,		//	[6] username
				std::string			//	[7] password
			>(frame);
			CYNG_LOG_INFO(logger_, "sml.public.open.request - trx: "
				<< std::get<1>(tpl)
				<< ", msg id: "
				<< std::get<2>(tpl)
				<< ", client id: "
				<< std::get<3>(tpl)
				<< ", server id: "
				<< std::get<4>(tpl)
				<< ", file id: "
				<< std::get<5>(tpl)
				<< ", user: "
				<< std::get<6>(tpl)
				<< ", pwd: "
				<< std::get<7>(tpl))
				;

			//
			//	ToDo: generate public open response
			//
		}

		void network::sml_public_close_request(cyng::context& ctx)
		{
			//	
			//
			//	* pk
			//	* transaction id
			//	* SML message id
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.public.close.request " << cyng::io::to_str(frame));

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,	//	[0] pk
				std::string,		//	[1] trx
				std::string			//	[2] msg id
			>(frame);
			CYNG_LOG_INFO(logger_, "sml.public.close.request - trx: "
				<< std::get<1>(tpl)
				<< ", msg id: "
				<< std::get<2>(tpl))
				;
		}

		void network::sml_get_proc_parameter_request(cyng::context& ctx)
		{
			//	[b5cfc8a0-0bf4-4afe-9598-ded99f71469c,180301094328243436-3,2,05:00:15:3b:02:23:b3,operator,operator,81 81 c7 82 01 ff,null]
			//	[50cfab74-eef0-4c92-89d4-46b28ab9da20,180522233943303816-2,1,00:15:3b:02:29:7e,operator,operator,81 00 60 05 00 00,null]
			//
			//	* pk
			//	* transaction id
			//	* SML message id
			//	* server ID
			//	* username
			//	* password
			//	* OBIS (requested parameter)
			//	* attribute (should be null)
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.get.proc.parameter.request " << cyng::io::to_str(frame));

			//CODE_ROOT_DEVICE_IDENT

		}

	}
}
