/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "network.h"
#include <smf/ipt/response.hpp>
#include <smf/ipt/generator.h>
#include <smf/sml/protocol/serializer.h>
#include <cyng/factory/set_factory.h>
#include <cyng/async/task/task_builder.hpp>
#include <cyng/io/serializer.h>
#include <cyng/vm/generator.h>
#include <cyng/io/serializer.h>
#include <cyng/tuple_cast.hpp>

#include <boost/uuid/random_generator.hpp>
#ifdef SMF_IO_DEBUG
#include <cyng/io/hex_dump.hpp>
#endif

namespace node
{
	namespace ipt
	{
		network::network(cyng::async::base_task* btp
			, cyng::logging::log_ptr logger
			, node::sml::status& status_word
			, cyng::store::db& config_db
			, boost::uuids::uuid tag
			, master_config_t const& cfg
			, std::string account
			, std::string pwd
			, std::string manufacturer
			, std::string model
			, cyng::mac48 mac)
		: base_(*btp)
			, bus_(bus_factory(btp->mux_, logger, boost::uuids::random_generator()(), scramble_key::default_scramble_key_, btp->get_id(), "ipt:gateway"))
			, logger_(logger)
			, config_(cfg)
			, parser_([this](cyng::vector_t&& prg) {
				CYNG_LOG_INFO(logger_, prg.size() << " instructions received");
#ifdef _DEBUG
				CYNG_LOG_TRACE(logger_, cyng::io::to_str(prg));
#endif
				bus_->vm_.async_run(std::move(prg));
			}, false, false)
			, core_(logger_, bus_->vm_, status_word, config_db, false, account, pwd, manufacturer, model, mac)
			, exec_(logger, btp->mux_, config_db, bus_, tag, mac)
			, seq_open_channel_map_()
		{
			CYNG_LOG_INFO(logger_, "initialize task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< ">");

			//
			//	request handler
			//
			bus_->vm_.register_function("bus.reconfigure", 1, std::bind(&network::reconfigure, this, std::placeholders::_1));
			bus_->vm_.register_function("bus.store.rel.channel.open", 3, std::bind(&network::insert_seq_open_channel_rel, this, std::placeholders::_1));

			bus_->vm_.async_run(cyng::generate_invoke("log.msg.info", cyng::invoke("lib.size"), "callbacks registered"));


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
				//	reset parser and serializer
				//
				bus_->vm_.async_run(cyng::generate_invoke("ipt.reset.parser", config_.get().sk_));
				bus_->vm_.async_run(cyng::generate_invoke("ipt.reset.serializer", config_.get().sk_));

				//
				//	login request
				//
				if (config_.get().scrambled_)
				{
					bus_->vm_.async_run(gen::ipt_req_login_scrambled(config_.get()));
				}
				else
				{
					bus_->vm_.async_run(gen::ipt_req_login_public(config_.get()));
				}
			}

			return cyng::continuation::TASK_CONTINUE;
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
				//base_.suspend(std::chrono::seconds(watchdog));
			}

			//
			//	update status word
			//
			core_.status_word_.set_authorized(true);

			return cyng::continuation::TASK_CONTINUE;
		}

		cyng::continuation network::process()
		{
			//
			//	update status word
			//
			core_.status_word_.set_authorized(false);

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
		{	//	no implementation
			BOOST_ASSERT_MSG(false, "[register target response] not implemented");
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

		cyng::continuation network::process(sequence_type)
		{	//	no implementation
			return cyng::continuation::TASK_CONTINUE;
		}

		cyng::continuation network::process(sequence_type seq
			, response_type res
			, std::uint32_t channel
			, std::uint32_t source
			, std::uint16_t status
			, std::size_t count)
		{	
			auto pos = seq_open_channel_map_.find(seq);
			if (pos != seq_open_channel_map_.end()) {


				auto r = make_tp_res_open_push_channel(seq, res);
				CYNG_LOG_INFO(logger_, "open push channel response "
					<< channel
					<< ':'
					<< source
					<< ':'
					<< r.get_response_name());

				base_.mux_.post(pos->second.first, 0, cyng::tuple_factory(r.is_success(), channel, source, status, count, pos->second.second));
			}
			else {
				CYNG_LOG_ERROR(logger_, "open push channel response "
					<< channel
					<< ':'
					<< source
					<< '@'
					<< seq
					<< " not found");
			}

			return cyng::continuation::TASK_CONTINUE;
		}

		cyng::continuation network::process(sequence_type seq
			, response_type res
			, std::uint32_t channel)
		{	//	no implementation
			return cyng::continuation::TASK_CONTINUE;
		}

		//void network::task_resume(cyng::context& ctx)
		//{
		//	const cyng::vector_t frame = ctx.get_frame();
		//	//	[1,0,TDevice,3]
		//	CYNG_LOG_TRACE(logger_, "resume task - " << cyng::io::to_str(frame));
		//	std::size_t tsk = cyng::value_cast<std::size_t>(frame.at(0), 0);
		//	std::size_t slot = cyng::value_cast<std::size_t>(frame.at(1), 0);
		//	base_.mux_.post(tsk, slot, cyng::tuple_t{ frame.at(2), frame.at(3) });
		//}

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

		//void network::insert_seq_register_target_rel(cyng::context& ctx)
		//{
		//	//	[5,power@solostec]
		//	//
		//	//	* ipt sequence
		//	//	* target name
		//	const cyng::vector_t frame = ctx.get_frame();

		//	auto const tpl = cyng::tuple_cast<
		//		sequence_type,		//	[0] ipt seq
		//		std::string			//	[1] target
		//	>(frame);

		//	CYNG_LOG_TRACE(logger_, "ipt sequence "
		//		<< +std::get<0>(tpl)
		//		<< " ==> "
		//		<< std::get<1>(tpl));

		//	seq_target_map_.emplace(std::get<0>(tpl), std::get<1>(tpl));
		//}

		void network::insert_seq_open_channel_rel(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();

			auto const tpl = cyng::tuple_cast<
				sequence_type,		//	[0] ipt seq
				std::size_t,		//	[1] task id
				std::string			//	[2] target name
			>(frame);

			CYNG_LOG_TRACE(logger_, "ipt sequence "
				<< +std::get<0>(tpl)
				<< " ==> "
				<< std::get<1>(tpl)
				<< ':'
				<< std::get<2>(tpl));

			seq_open_channel_map_.emplace(std::piecewise_construct
				, std::forward_as_tuple(std::get<0>(tpl))
				, std::forward_as_tuple(std::get<1>(tpl), std::get<2>(tpl)))
				;
		}

	}
}
