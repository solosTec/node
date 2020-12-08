/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <tasks/network.h>
#include <tasks/push.h>
#include <cache.h>
#include <storage.h>
#include <bridge.h>
#include <segw.h>

#include <smf/serial/baudrate.h>
#include <smf/ipt/response.hpp>
#include <smf/ipt/generator.h>
#include <smf/sml/protocol/serializer.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/event.h>
#include <smf/sml/srv_id_io.h>

#include <cyng/factory/set_factory.h>
#include <cyng/io/serializer.h>
#include <cyng/io/serializer.h>
#include <cyng/vm/generator.h>
#include <cyng/vm/domain/task_domain.h>
#include <cyng/tuple_cast.hpp>
#include <cyng/numeric_cast.hpp>
#include <cyng/buffer_cast.h>
#include <cyng/async/task/task_builder.hpp>

//#ifdef SMF_IO_DEBUG
#include <cyng/io/hex_dump.hpp>
//#endif

namespace node
{
	namespace ipt
	{
		network::network(cyng::async::base_task* btp
			, cyng::logging::log_ptr logger
			, bridge& b
			, std::string account
			, std::string pwd
			, bool accept_all
			, boost::uuids::uuid tag)
		: bus(logger
				, btp->mux_
				, tag	
				, "SMF-GW:ETH"
				, 1u)
			, base_(*btp)
			, logger_(logger)
			, bridge_(b)
			, cache_(b.cache_)
			, cfg_(cache_)
			, storage_(b.storage_)
			, parser_([this](cyng::vector_t&& prg) {
				//CYNG_LOG_INFO(logger_, prg.size() << " SML instructions received (client)");
				//CYNG_LOG_TRACE(logger_, cyng::io::to_str(prg));
				vm_.async_run(std::move(prg));
			}, false, false, false)
			, router_(logger_
				, false	//	client mode
				, this->bus::vm_
				, b.cache_
				, b.storage_
				, account
				, pwd
				, accept_all)
			, task_gpio_(cyng::async::NO_TASK)
			, retries_(cfg_.get_ipt_tcp_connect_retries())
		{

			if (cfg_.is_ipt_enabled()) {

				CYNG_LOG_INFO(logger_, "initialize task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> as "
					<< account
					<< ':'
					<< pwd);
			}
			else {

				CYNG_LOG_INFO(logger_, "initialize task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> without IP-T");
			}

			//
			//	register the task manager on this VM
			//
			cyng::register_task(btp->mux_, vm_);

			//
			//	request handler
			//
			vm_.register_function("bus.reconfigure", 1, std::bind(&network::reconfigure, this, std::placeholders::_1));

			vm_.register_function("update.status.ip", 2, [&](cyng::context& ctx) {

				//	 [192.168.1.200:59536,192.168.1.100:26862]
				auto const frame = ctx.get_frame();
				CYNG_LOG_INFO(logger_, ctx.get_name()
					<< " - "
					<< cyng::io::to_str(frame));

				auto const tpl = cyng::tuple_cast<
					boost::asio::ip::tcp::endpoint,		//	[0] remote
					boost::asio::ip::tcp::endpoint		//	[1] local
				>(frame);

				//	with IP-T prefix
				this->cache_.set_cfg(build_cfg_key({ sml::OBIS_ROOT_IPT_PARAM }, "ep.remote"), std::get<0>(tpl));
				this->cache_.set_cfg(build_cfg_key({ sml::OBIS_ROOT_IPT_PARAM }, "ep.local"), std::get<1>(tpl));

			});

			vm_.register_function("net.start.tsk.push", 8, [&](cyng::context& ctx) {

				//
				//	start <push> task
				//
				auto const frame = ctx.get_frame();
				CYNG_LOG_INFO(logger_, ctx.get_name()
					<< " - "
					<< cyng::io::to_str(frame));

				auto const tpl = cyng::tuple_cast<
					cyng::buffer_t,		//	[0] srv_id
					std::uint8_t,		//	[1] nr
					cyng::buffer_t,		//	[2] profile
					std::uint32_t,		//	[3] interval
					std::uint32_t,		//	[4] delay
					cyng::buffer_t,		//	[5] source
					std::string,		//	[6] target
					cyng::buffer_t		//	[7] service
				>(frame);

				//	OBIS_PUSH_INTERVAL - cyng::TC_UINT32
				//	OBIS_PUSH_DELAY - cyng::TC_UINT32
				//	OBIS_PUSH_SOURCE - cyng::TC_BUFFER
				//	OBIS_PUSH_TARGET - cyng::TC_STRING
				//	OBIS_PUSH_SERVICE - cyng::TC_BUFFER

				//cache_.read_table("_DataCollector");

				auto const tsk = start_task_push(std::get<0>(tpl)	//	srv_id
					, std::get<1>(tpl)	//	nr
					, std::get<2>(tpl)	//	profile/service
					, std::get<3>(tpl)	//	interval
					, std::get<4>(tpl)	//	delay
					, std::get<6>(tpl)	//	target
					);

				cache_.write_table("_PushOps", [&](cyng::store::table* tbl) {

					auto const key = cyng::table::key_generator(std::get<0>(tpl), std::get<1>(tpl));

					tbl->insert(key
						, cyng::table::data_generator(std::get<3>(tpl)	//	interval
							, std::get<4>(tpl)	//	delay
							, std::get<5>(tpl)	//	source
							, std::get<6>(tpl)	//	target
							, std::get<7>(tpl)	//	service
							, static_cast<std::uint64_t>(0)	//	lowerBound
							, static_cast<std::uint64_t>(tsk))
						, 0u
						, ctx.tag());
				});
			});

			//
			//	statistics
			//
			vm_.async_run(cyng::generate_invoke("log.msg.debug", cyng::invoke("lib.size"), " callbacks registered"));

			//
			//	load and start push tasks
			//
			load_push_ops();

			//
			//	connect cache and db
			//
			bridge_.finalize();

		}

		cyng::continuation network::run()
		{
			if (is_online())
			{
				//
				//	re/start monitor
				//
				base_.suspend(cfg_.get_ipt_tcp_wait_to_reconnect());
			}
			else if (cfg_.is_ipt_enabled()) 
			{
				//
				//	reset parser and serializer
				//
				auto const sk = cfg_.get_ipt_sk();
				vm_.async_run({ cyng::generate_invoke("ipt.reset.parser", sk)
					, cyng::generate_invoke("ipt.reset.serializer", sk) });

				//
				//	login request
				//
				req_login(cfg_.get_ipt_master());
			}

			return cyng::continuation::TASK_CONTINUE;
		}

		void network::stop(bool shutdown)
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
			//	update status word
			//
			cache_.set_status_word(sml::STATUS_BIT_NOT_AUTHORIZED_IPT, false);

			//
			//	op log entry
			//
			auto const sw = cache_.get_status_word();
			auto const srv = cache_.get_srv_id();

			storage_.generate_op_log(sw
				, sml::LOG_CODE_61	//	0x4970000A - IP-T - Zugang erfolgt
				, sml::OBIS_PEER_ADDRESS_WANGSM	//	source is WANGSM (or LOG_SOURCE_ETH == 81, 04, 00, 00, 00, FF)
				, srv	//	server ID
				, ""	//	target
				, 0		//	push nr
				, "IP-T login");	//	description


			//
			//	signal LED
			//
			if (cyng::async::NO_TASK != task_gpio_) {
				base_.mux_.post(task_gpio_, 0, cyng::tuple_factory(true));
			}

			//
			//	update IP address
			//
			vm_.async_run(cyng::generate_invoke("update.status.ip", cyng::invoke("ip.tcp.socket.ep.local"), cyng::invoke("ip.tcp.socket.ep.remote")));

			//
			//	reset retry counter for attempts to reconnect
			//
			retries_ = cfg_.get_ipt_tcp_connect_retries();
		}

		//	slot [1] - connection lost / reconnect
		void network::on_logout()
		{
			//
			//	update status word
			//
			cache_.set_status_word(sml::STATUS_BIT_NOT_AUTHORIZED_IPT, true);

			//
			//	op log entry
			//
			auto const sw = cache_.get_status_word();
			auto const srv = cache_.get_srv_id();

			auto const rec = cfg_.get_ipt_master();


			//LOG_CODE_65 = 0x4970000E,	//	IP-T - Zugang verloren, Verbindung unerwartet abgebrochen
			storage_.generate_op_log(sw
				, sml::LOG_CODE_65	//	0x4970000A - IP-T - Zugang erfolgt
				, sml::OBIS_PEER_ADDRESS_WANGSM	//	source is WANGSM (or LOG_SOURCE_ETH == 81, 04, 00, 00, 00, FF)
				, srv	//	server ID
				, rec.host_	//	target
				, 0		//	nr
				, "IP-T login");	//	description

			//
			//	signal LED
			//
			if (cyng::async::NO_TASK != task_gpio_) {
				base_.mux_.post(task_gpio_, 0, cyng::tuple_factory(false));
			}

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
		{	//	no implementation
			BOOST_ASSERT_MSG(false, "[register target response] not implemented");
		}

		//	@brief slot [3] - 0x4006: push target deregistered response
		void network::on_res_deregister_target(sequence_type, bool, std::string const&)
		{	//	no implementation
			CYNG_LOG_WARNING(logger_, "push target deregistered response not implemented");
		}

		//	slot [4] - 0x1000: push channel open response
		void network::on_res_open_push_channel(sequence_type seq
			, bool success
			, std::uint32_t channel
			, std::uint32_t source
			, std::uint16_t status
			, std::size_t count)
		{	//	use implementation in base class
		}

		//	slot [5] - 0x1001: push channel close response
		void network::on_res_close_push_channel(sequence_type seq
			, bool success
			, std::uint32_t channel)
		{
			CYNG_LOG_INFO(logger_, "push channel "
				<< channel
				<< " close response received");
		}

		//	slot [6] 0x9003: connection open request 
		bool network::on_req_open_connection(sequence_type seq, std::string const& number)
		{
			CYNG_LOG_TRACE(logger_, "incoming call " << +seq << ':' << number);

			BOOST_ASSERT(is_online());

			//
			//	accept incoming calls
			//
			return true;
		}

		//	slot [7] - 0x1003: connection open response
		cyng::buffer_t network::on_res_open_connection(sequence_type seq, bool success)
		{	//	no implementation
			CYNG_LOG_WARNING(logger_, "connection open response not implemented");
			return cyng::buffer_t();	//	no data
		}

		//	slot [8] - 0x9004/0x1004: connection close request/response
		void network::on_req_close_connection(sequence_type seq)
		{	
			CYNG_LOG_INFO(logger_, "connection closed");
		}
		void network::on_res_close_connection(sequence_type)
		{	//	no implementation
			CYNG_LOG_WARNING(logger_, "connection close response not implemented");
		}

		//	slot [9] - 0x9002: push data transfer request
		void network::on_req_transfer_push_data(sequence_type seq
			, std::uint32_t channel
			, std::uint32_t source
			, cyng::buffer_t const& data)
		{
			//CYNG_LOG_INFO(logger_, "task #"
			//	<< base_.get_id()
			//	<< " <"
			//	<< base_.get_class_name()
			//	<< "> "
			//	<< config_.get().account_
			//	<< " received "
			//	<< data.size()
			//	<< " bytes push data from "
			//	<< channel
			//	<< '.'
			//	<< source);
		}

		//	slot [10] - transmit data (if connected)
		cyng::buffer_t network::on_transmit_data(cyng::buffer_t const& data)
		{
			CYNG_LOG_TRACE(logger_, "incoming IPT/SML data " << data.size() << " bytes");
			if (data.size() == 6
				&& data.at(0) == 'h'
				&& data.at(1) == 'e'
				&& data.at(2) == 'l'
				&& data.at(3) == 'l'
				&& data.at(4) == 'o'
				&& data.at(5) == '!')
			{
				CYNG_LOG_DEBUG(logger_, "answer with hey!");
				return cyng::make_buffer({ 'h', 'e', 'y', '!' });
			}

#ifdef _DEBUG
			std::stringstream ss;
			cyng::io::hex_dump hd;
			hd(ss, data.begin(), data.end());
			CYNG_LOG_DEBUG(logger_, ss.str());
#endif

			//
			//	parse incoming data
			//
			parser_.read(data.begin(), data.end());
			return cyng::buffer_t();
		}

		void network::reconfigure(cyng::context& ctx)
		{
			auto const frame = ctx.get_frame();
			CYNG_LOG_WARNING(logger_, ctx.get_name() << " - " << cyng::io::to_type(frame));
			for (auto const& obj : frame) {
				CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_type(obj));
			}
			reconfigure_impl();
		}

		void network::reconfigure_impl()
		{
			if (retries_ != 0u) {

				//
				//	wait a shorter time
				//
				base_.suspend(cfg_.get_ipt_tcp_wait_to_reconnect()/retries_);

				CYNG_LOG_INFO(logger_, "reconnect to network in "
					<< cyng::to_str(cfg_.get_ipt_tcp_wait_to_reconnect() / retries_));

				--retries_;

			}
			else {
				//
				//	switch to other master
				//
				auto const rec = cfg_.switch_ipt_redundancy();

				CYNG_LOG_INFO(logger_, "switch to redundancy ["
					<< rec.host_
					<< ':'
					<< rec.service_
					<< "]");

				//
				//	trigger reconnect 
				//
				CYNG_LOG_INFO(logger_, "reconnect to network in "
					<< cyng::to_str(rec.monitor_));

				base_.suspend(cfg_.get_ipt_tcp_wait_to_reconnect());
			}
		}

		void network::load_push_ops()
		{
			std::size_t counter{ 0 };
			cache_.write_tables("_PushOps", "_DataCollector", [&](cyng::store::table* tbl_po, cyng::store::table* tbl_dc) {

				//
				//	PushOps to remove, because there is no corresponding data collector.
				//
				cyng::table::key_list_t to_remove;

				storage_.loop("TPushOps", [&](cyng::table::record const& rec)->bool {

					//
					//	add PushOps task
					//
					cyng::table::record r(tbl_po->meta_ptr());
					r.read_data(rec);

					//
					//	get profile type
					//
					auto const rec_dc = tbl_dc->lookup(rec.key());	//	same key
					if (rec_dc.empty()) {

						CYNG_LOG_WARNING(logger_, "PushOp "
							<< cyng::io::to_str(rec.convert())
							<< " without data collector");

						//
						//	remove this configuration
						//
						to_remove.push_back(rec.key());

						return true;	//	continue
					}
					auto const profile = cyng::to_buffer(rec_dc["profile"]);
					auto const tsk = start_task_push(rec, profile);
					if (tsk != cyng::async::NO_TASK) {
						
						//
						//	store task ID as [u64] in cache table "_PushOps"
						//
						r.set("tsk", cyng::make_object(static_cast<std::uint64_t>(tsk)));

						if (tbl_po->insert(rec.key(), r.data(), rec.get_generation(), cache_.get_tag())) {

							cyng::buffer_t const srv = cyng::to_buffer(rec.key().at(0));
							CYNG_LOG_TRACE(logger_, "load push op "
								<< node::sml::from_server_id(srv));

							++counter;
						}
						else {

							//
							//	Insert failed - stop task
							//
							CYNG_LOG_ERROR(logger_, "transfer TPushOps config to _PushOp failed for: "
								<< cyng::io::to_str(rec.convert())
								<< " - stop task #"
								<< tsk);

							this->base_.mux_.stop(tsk);
						}
					}
					else {

						CYNG_LOG_ERROR(logger_, "insert into table TPushOps failed - key: "
							<< cyng::io::to_str(rec.key())
							<< ", body: "
							<< cyng::io::to_str(rec.data()));

					}

					return true;	//	continue
					});

				//
				//	remove dangling push ops
				//
				for (auto const& key : to_remove) {
					tbl_po->erase(key, cache_.get_tag());
				}

			});

			CYNG_LOG_INFO(logger_, counter
				<< " PushOps started");

		}

		std::size_t network::start_task_push(cyng::table::record const& rec, cyng::buffer_t profile)
		{
			if (!rec.empty()) {

				auto const srv_id = cyng::to_buffer(rec["serverID"]);
				auto const target = cyng::value_cast(rec["target"], "");
				auto const interval = cyng::value_cast<std::uint32_t>(rec["interval"], 0);
				auto const delay = cyng::value_cast<std::uint32_t>(rec["delay"], 0);

				return start_task_push(srv_id
					, cyng::value_cast<std::uint8_t>(rec["nr"], 0)
					, profile
					, interval
					, delay
					, target);
			}
			return cyng::async::NO_TASK;
		}

		std::size_t network::start_task_push(cyng::buffer_t srv_id
			, std::uint8_t nr
			, cyng::buffer_t profile
			, std::uint32_t interval
			, std::uint32_t delay
			, std::string target)
		{
			BOOST_ASSERT(srv_id.size() == 9);
			if (profile.size() != 6) {
				CYNG_LOG_ERROR(logger_, "try to start push task for target "
					<< target
					<< " with invalid profile");
				return cyng::async::NO_TASK;
			}

			return cyng::async::start_task_detached<push>(this->base_.mux_
				, logger_
				, cache_
				, storage_
				, srv_id
				, nr
				, profile
				, std::chrono::seconds(interval)
				, std::chrono::seconds(delay)
				, target
				, this);
		}
	}
}
