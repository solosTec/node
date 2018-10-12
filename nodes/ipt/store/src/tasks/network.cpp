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
			, redundancy const& cfg
			, std::map<std::string, std::string> const& targets)
		: base_(*btp)
			, bus(logger
				, btp->mux_
				, boost::uuids::random_generator()()
				, cfg.get().sk_
				, "ipt:store"
				, 1u)
			, logger_(logger)
			, config_(cfg)
			, targets_(targets)
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
			vm_.register_function("bus.reconfigure", 1, std::bind(&network::reconfigure, this, std::placeholders::_1));

			//
			//	statistics
			//
			vm_.async_run(cyng::generate_invoke("log.msg.info", cyng::invoke("lib.size"), "callbacks registered"));
		}

		cyng::continuation network::run()
		{
			if (is_online())	{

				//
				//	re/start monitor
				//
				base_.suspend(config_.get().monitor_);

				CYNG_LOG_INFO(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> has monitor with "
					<< config_.get().monitor_.count()
					<< " seconds(s)");
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

		void network::stop()
		{
			//
			//	call base class
			//
			bus::stop();
			while (!vm_.is_halted()) {
				CYNG_LOG_INFO(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> continue gracefull shutdown");
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> is stopped");
		}

		//	slot [0] 0x4001/0x4002: response login
		void network::on_login_response(std::uint16_t watchdog, std::string redirect)
		{
			//
			//	register targets
			//
			register_targets();
		}

		//	slot [1] - connection lost / reconnect
		void network::on_logout()
		{
			//
			//	switch to other configuration
			//
			reconfigure_impl();
		}

		//	slot [2] - 0x4005: push target registered response
		void network::on_res_register_target(sequence_type seq
			, bool success
			, std::uint32_t channel
			, std::string target)
		{
			if (success)
			{
				auto pos = targets_.find(target);
				if (pos != targets_.end()) {

					CYNG_LOG_INFO(logger_, "channel "
						<< channel
						<< " => "
						<< target
						<< ':'
						<< pos->second);

					//
					//	channel => [protocol, target]
					//
					BOOST_ASSERT(!target.empty());
					channel_protocol_map_.emplace(channel, std::make_pair(pos->second, target));

				}
			}
		}

		//	@brief slot [3] - 0x4006: push target deregistered response
		void network::on_res_deregister_target(sequence_type, bool, std::string const&)
		{	//	no implementation
			CYNG_LOG_WARNING(logger_, "push target deregistered response not implemented");
		}

		//	slot [4] - 0x1000: push channel open response
		void network::on_res_open_push_channel(sequence_type
			, bool
			, std::uint32_t
			, std::uint32_t
			, std::uint16_t
			, std::size_t)
		{	//	no implementation
			CYNG_LOG_WARNING(logger_, "push channel open response not implemented");
		}

		//	slot [5] - 0x1001: push channel close response
		void network::on_res_close_push_channel(sequence_type seq
			, bool success
			, std::uint32_t channel)
		{	//	no implementation
			CYNG_LOG_WARNING(logger_, "push channel close response not implemented");
		}

		//	slot [6] - 0x9003: connection open request 
		bool network::on_req_open_connection(sequence_type seq, std::string const& number)
		{
			CYNG_LOG_TRACE(logger_, "incoming call " << +seq << ':' << number);

			//
			//	don't accept incoming calls
			//
			return false;
		}

		//	slot [7] - 0x1003: connection open response
		cyng::buffer_t network::on_res_open_connection(sequence_type seq, bool success)
		{	//	no implementation
			CYNG_LOG_WARNING(logger_, "connection open response not implemented");
			return cyng::buffer_t();	//	no data
		}

		//	slot [8] - 0x9004/0x1004: connection close request/response
		void network::on_req_close_connection(sequence_type)
		{	//	no implementation
			CYNG_LOG_WARNING(logger_, "connection close request/response not implemented");
		}
		void network::on_res_close_connection(sequence_type)
		{	//	no implementation
			CYNG_LOG_WARNING(logger_, "connection close request/response not implemented");
		}

		//	slot [9] - 0x9002: push data transfer request
		void network::on_req_transfer_push_data(sequence_type seq
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
		}

		//	slot [10] - transmit data (if connected)
		cyng::buffer_t network::on_transmit_data(cyng::buffer_t const& data)
		{	//	no implementation
			CYNG_LOG_WARNING(logger_, "transmit data not implemented - "
				<< data.size()
				<< " bytes received");
			return cyng::buffer_t();
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

		//	slot [12] - remove line
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
					<< "@"
					<< tag);
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

				//
				//	slot [0] ????
				//
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

		void network::register_targets()
		{
			if (targets_.empty())
			{
				CYNG_LOG_WARNING(logger_, "no push targets defined");
			}
			else
			{
				for (auto target : targets_)
				{
					CYNG_LOG_INFO(logger_, "register target " << target.first << ':' << target.second);

					//
					//	safe to call since no VM calls involved
					//
					req_register_push_target(target.first
						, std::numeric_limits<std::uint16_t>::max()	//	0xffff
						, static_cast<std::uint8_t>(1)
						, std::chrono::seconds(12));
				}
			}
		}

		//void network::insert_rel(cyng::context& ctx)
		//{
		//	//	[5,power@solostec]
		//	//
		//	//	* ipt sequence
		//	//	* target name
		//	//	* protocol layer
		//	const cyng::vector_t frame = ctx.get_frame();

		//	auto const tpl = cyng::tuple_cast<
		//		sequence_type,		//	[0] ipt seq
		//		std::string,		//	[1] target
		//		std::string			//	[2] protocol
		//	>(frame);

		//	CYNG_LOG_TRACE(logger_, "ipt sequence " 
		//		<< +std::get<0>(tpl)
		//		<< " ==> "
		//		<< std::get<1>(tpl)
		//		<< ':'
		//		<< std::get<2>(tpl));

		//	seq_target_map_.emplace(std::get<0>(tpl), std::get<1>(tpl));
		//}

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


