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
#include <cyng/io/io_chrono.hpp>
#include <boost/uuid/random_generator.hpp>

namespace node
{
	namespace ipt
	{

		network::network(cyng::async::base_task* btp
			, cyng::logging::log_ptr logger
			, master_config_t const& cfg
			, std::map<std::string, std::string> const& targets)
		: base_(*btp)
			, bus_(bus_factory(btp->mux_, logger, boost::uuids::random_generator()(), scramble_key::default_scramble_key_, btp->get_id(), "ipt:store"))
			, logger_(logger)
			, config_(cfg)
			, targets_(targets)
			, seq_target_map_()
			, channel_protocol_map_()
			, sml_lines_()
			, iec_lines_()
			, rng_()
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
			bus_->vm_.register_function("net.insert.rel", 3, std::bind(&network::insert_rel, this, std::placeholders::_1));
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
					bus_->vm_.async_run(ipt_req_login_scrambled());
				}
				else
				{
					bus_->vm_.async_run(ipt_req_login_public());
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

		cyng::continuation network::process(sequence_type seq
			, std::uint32_t channel
			, std::uint32_t source
			, cyng::buffer_t const& data)
		{
			//
			//	check secondary data to get protocol type and start parser
			//	if required
			//
			auto pos = channel_protocol_map_.find(channel);
			if (pos != channel_protocol_map_.end()) {

				CYNG_LOG_TRACE(logger_, "channel "
					<< channel
					<< ':'
					<< pos->second.second
					<< " received "
					<< data.size()
					<< " bytes of protocol "
					<< pos->second.first);

				distribute(channel, source, pos->second.first, pos->second.second, data);
			}
			else {

				CYNG_LOG_ERROR(logger_, "channel "
					<< channel
					<< " has no protocol entry (channel_protocol_map) - "
					<< data.size()
					<< " bytes get lost");
			}

			//
			//	continue task
			//
			return cyng::continuation::TASK_CONTINUE;
		}

		void network::distribute(std::uint32_t channel
			, std::uint32_t source
			, std::string const& protocol
			, std::string const& target
			, cyng::buffer_t const& data)
		{

			if (boost::algorithm::equals(protocol, "SML")) {

				//
				//	distribute raw SML data
				//
				distribute_raw(channel, source, "SML:RAW", target, data);

				//
				//	manage and distribute to SML parser instances
				//
				distribute_sml(channel, source, target, data);
			}
			else if (boost::algorithm::equals(protocol, "IEC")) {

				//
				//	distribute raw IEC data
				//
				distribute_raw(channel, source, "IEC:RAW", target, data);

				//
				//	manage and distribute to IEC parser instances
				//
				distribute_iec(channel, source, target, data);
			}
			else {

				CYNG_LOG_ERROR(logger_, "unknown protocol "
					<< protocol
					<< " from channel "
					<< channel
					<< ':'
					<< target);
			}

			//
			//	send raw data
			//
			distribute_raw_all(channel, source, protocol, target, data);
		}

		std::size_t network::distribute_sml(std::uint32_t channel
			, std::uint32_t source
			, std::string const& target
			, cyng::buffer_t const& data)
		{
			auto line = build_line(channel, source);

			auto pos = sml_lines_.find(line);
			if (pos == sml_lines_.end()) {

				//
				//	get all SML consumer
				//	
				auto consumers = get_consumer("SML:");

				//
				//	create a new SML processor
				//
				auto tag = rng_();

				CYNG_LOG_TRACE(logger_, "create SML processor " 
					<< tag 
					<< " for line " 
					<< channel
					<< ':'
					<< source 
					<< " with "
					<< consumers.size()
					<< " consumer(s)");

				auto res = sml_lines_.emplace(std::piecewise_construct,
					std::forward_as_tuple(line),
					std::forward_as_tuple(base_.mux_
						, logger_
						, tag
						, channel
						, source
						, target
						, base_.get_id()
						, consumers));
				BOOST_ASSERT(res.second);
				if (res.second)	res.first->second.parse(data);
			}
			else {

				//
				//	use existing SML processor
				//
				pos->second.parse(data);
			}
			return 0;
		}

		std::size_t network::distribute_iec(std::uint32_t channel
			, std::uint32_t source
			, std::string const& target
			, cyng::buffer_t const& data)
		{
			auto line = build_line(channel, source);

			auto pos = iec_lines_.find(line);
			if (pos == iec_lines_.end()) {

				//
				//	get all IEC consumer
				//	
				auto consumers = get_consumer("IEC:");

				//
				//	create a new IEC processor
				//
				auto tag = rng_();

				CYNG_LOG_TRACE(logger_, "create IEC processor "
					<< tag
					<< " for line "
					<< channel
					<< ':'
					<< source
					<< " with "
					<< consumers.size()
					<< " consumer(s)");

				auto res = iec_lines_.emplace(std::piecewise_construct,
					std::forward_as_tuple(line),
					std::forward_as_tuple(base_.mux_
						, logger_
						, tag
						, channel
						, source
						, target
						, base_.get_id()
						, consumers));
				BOOST_ASSERT(res.second);
				if (res.second)	res.first->second.parse(data);
			}
			else {

				//
				//	use existing IEC processor
				//
				pos->second.parse(data);
			}
			return 0;
		}

		std::size_t network::distribute_raw(std::uint32_t channel
			, std::uint32_t source
			, std::string protocol
			, std::string const& target
			, cyng::buffer_t const& data)
		{
			auto range = consumers_.equal_range(protocol);
			for (auto pos = range.first; pos != range.second; ++pos) {

				CYNG_LOG_TRACE(logger_, "send "
					<< protocol
					<< " data to task #"
					<< pos->second);

				base_.mux_.post(pos->second, 0, cyng::tuple_factory(channel, source, target, data));
			}
			return std::distance(range.first, range.second);
		}

		std::size_t network::distribute_raw_all(std::uint32_t channel
			, std::uint32_t source
			, std::string const& protocol
			, std::string const& target
			, cyng::buffer_t const& data)
		{
			auto range = consumers_.equal_range("ALL:RAW");
			for (auto pos = range.first; pos != range.second; ++pos) {

				CYNG_LOG_TRACE(logger_, "send raw data of protocol "
					<< protocol
					<< " to task #"
					<< pos->second);

				base_.mux_.post(pos->second, 0, cyng::tuple_factory(channel, source, protocol, target, data));
			}
			return std::distance(range.first, range.second);
		}

		//	slot [4] - register target response
		cyng::continuation network::process(sequence_type seq, bool success, std::uint32_t channel)
		{
			if (success)
			{
				auto pos = seq_target_map_.find(seq);
				if (pos != seq_target_map_.end())
				{
					auto pos_target = targets_.find(pos->second);
					if (pos_target != targets_.end())
					{
						BOOST_ASSERT(pos->second == pos_target->first);

						CYNG_LOG_INFO(logger_, "channel "
							<< channel
							<< " => "
							<< pos->second
							<< ':'
							<< pos_target->second);

						//
						//	channel => [protocol, target]
						//
						channel_protocol_map_.emplace(channel, std::make_pair(pos_target->second, pos->second));
					}
					seq_target_map_.erase(pos);

					if (seq_target_map_.empty()) {
						CYNG_LOG_INFO(logger_, "push target registration is complete");
					}
					else {
						CYNG_LOG_TRACE(logger_, seq_target_map_.size()
							<< " push target register request(s) are pending");
					}
				}
			}

			//
			//	continue task
			//
			return cyng::continuation::TASK_CONTINUE;
		}

		cyng::continuation network::process(cyng::buffer_t const&)
		{	//	no implementation
			return cyng::continuation::TASK_CONTINUE;
		}

		cyng::continuation network::process(sequence_type, bool, std::string const&)
		{	//	no implementation
			return cyng::continuation::TASK_CONTINUE;
		}

		cyng::continuation network::process(sequence_type)
		{	//	no implementation
			return cyng::continuation::TASK_CONTINUE;
		}

		cyng::continuation network::process(sequence_type, response_type, std::uint32_t, std::uint32_t, std::uint16_t, std::size_t)
		{	//	no implementation
			return cyng::continuation::TASK_CONTINUE;
		}

		cyng::continuation network::process(sequence_type seq
			, response_type res
			, std::uint32_t channel)
		{	//	no implementation
			return cyng::continuation::TASK_CONTINUE;
		}

		cyng::continuation network::process(std::string type, std::size_t tid)
		{
			consumers_.emplace(type, tid);
			CYNG_LOG_INFO(logger_, "consumer task #"
				<< tid
				<< " registered as "
				<< type);

			//
			//	continue task
			//
			return cyng::continuation::TASK_CONTINUE;
		}

		cyng::continuation network::process(std::string protocol, std::uint64_t line, boost::uuids::uuid tag)
		{

			CYNG_LOG_INFO(logger_, "remove "
				<< protocol
				<< " line "
				<< line
				<< "@"
				<< tag);

			if (boost::algorithm::equals("SML", protocol)) {

				auto size = sml_lines_.erase(line);
				CYNG_LOG_TRACE(logger_, sml_lines_.size()
					<< " active SML line(s)");

			}
			else if (boost::algorithm::equals("IEC", protocol)) {

				auto size = iec_lines_.erase(line);
				CYNG_LOG_TRACE(logger_, sml_lines_.size()
					<< " active IEC line(s)");
			}
			else {
				CYNG_LOG_ERROR(logger_, "unknown protocol "
					<< protocol
					<< " line "
					<< line
					<< "@"
					<< tag);
			}

			//
			//	continue task
			//
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

		void network::register_targets()
		{
			if (targets_.empty())
			{
				CYNG_LOG_WARNING(logger_, "no push targets defined");
			}
			else
			{
				seq_target_map_.clear();
				seq_target_map_.clear();
				for (auto target : targets_)
				{
					CYNG_LOG_INFO(logger_, "register target " << target.first << ':' << target.second);
					cyng::vector_t prg;
					prg
						<< cyng::generate_invoke_unwinded("req.register.push.target", target.first, static_cast<std::uint16_t>(0xffff), static_cast<std::uint8_t>(1))
						<< cyng::generate_invoke_unwinded("net.insert.rel", cyng::invoke("ipt.push.seq"),  target.first, target.second)
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
			//	* protocol layer
			const cyng::vector_t frame = ctx.get_frame();

			auto const tpl = cyng::tuple_cast<
				sequence_type,		//	[0] ipt seq
				std::string,		//	[1] target
				std::string			//	[2] protocol
			>(frame);

			CYNG_LOG_TRACE(logger_, "ipt sequence " 
				<< +std::get<0>(tpl)
				<< " ==> "
				<< std::get<1>(tpl)
				<< ':'
				<< std::get<2>(tpl));

			seq_target_map_.emplace(std::get<0>(tpl), std::get<1>(tpl));
		}

		std::vector<std::size_t> network::get_consumer(std::string protocol)
		{
			std::vector<std::size_t> consumer;

			for (auto const& c : consumers_) {
				if (boost::algorithm::starts_with(c.first, protocol)) {
					consumer.push_back(c.second);
				}
			}

			return consumer;
		}
	}
}
