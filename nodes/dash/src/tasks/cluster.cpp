/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "cluster.h"
#include "system.h"
#include <smf/cluster/generator.h>

#include <cyng/async/task/task_builder.hpp>
#include <cyng/io/serializer.h>
#include <cyng/vm/generator.h>
#include <cyng/table/meta.hpp>
#include <cyng/tuple_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/dom/reader.h>
//#include <cyng/sys/cpu.h>
#include <cyng/sys/memory.h>
#include <cyng/json.h>

#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/algorithm/string/predicate.hpp>

namespace node
{
	cluster::cluster(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cluster_config_t const& cfg_cls
		, boost::asio::ip::tcp::endpoint ep
		, std::string const& doc_root
		, uint16_t watchdog
		, int timeout)
	: base_(*btp)
		, rgn_()
		, bus_(bus_factory(btp->mux_, logger, rgn_(), btp->get_id()))
		, logger_(logger)
		, config_(cfg_cls)
		, cache_()
		, server_(logger, btp->mux_.get_io_service(), ep, doc_root, bus_, cache_)
		, master_(0)
		, sys_tsk_(cyng::async::start_task_detached<system>(btp->mux_, logger_, cache_, bus_->vm_.tag()))
	{
		CYNG_LOG_INFO(logger_, "task #"
		<< base_.get_id()
		<< " <"
		<< base_.get_class_name()
		<< "> is running");

		//
		//	init cache
		//
		create_cache();
		subscribe_cache();

		//
		//	implement request handler
		//
		bus_->vm_.run(cyng::register_function("bus.reconfigure", 1, std::bind(&cluster::reconfigure, this, std::placeholders::_1)));

		//
		//	data handling
		//
		bus_->vm_.run(cyng::register_function("db.trx.start", 0, [this](cyng::context& ctx) {
			CYNG_LOG_TRACE(logger_, "db.trx.start");
		}));
		bus_->vm_.run(cyng::register_function("db.trx.commit", 0, [this](cyng::context& ctx) {
			CYNG_LOG_TRACE(logger_, "db.trx.commit");
		}));
		bus_->vm_.run(cyng::register_function("bus.res.subscribe", 6, std::bind(&cluster::res_subscribe, this, std::placeholders::_1)));
		bus_->vm_.run(cyng::register_function("db.res.insert", 4, std::bind(&cluster::db_res_insert, this, std::placeholders::_1)));
		bus_->vm_.run(cyng::register_function("db.res.remove", 2, std::bind(&cluster::db_res_remove, this, std::placeholders::_1)));
		bus_->vm_.run(cyng::register_function("db.res.modify.by.attr", 3, std::bind(&cluster::db_res_modify_by_attr, this, std::placeholders::_1)));
		bus_->vm_.run(cyng::register_function("db.res.modify.by.param", 3, std::bind(&cluster::db_res_modify_by_param, this, std::placeholders::_1)));
		bus_->vm_.run(cyng::register_function("db.req.insert", 4, std::bind(&cluster::db_req_insert, this, std::placeholders::_1)));
		bus_->vm_.run(cyng::register_function("db.req.remove", 3, std::bind(&cluster::db_req_remove, this, std::placeholders::_1)));
		bus_->vm_.run(cyng::register_function("db.req.modify.by.param", 5, std::bind(&cluster::db_req_modify_by_param, this, std::placeholders::_1)));

		//
		//	input from HTTP session / ws
		//
		bus_->vm_.run(cyng::register_function("ws.read", 3, std::bind(&cluster::ws_read, this, std::placeholders::_1)));


	}

	void cluster::run()
	{	
		if (!bus_->is_online())
		{
			connect();
		}
	}

	void cluster::stop()
	{
		//
		//	stop server
		//
		server_.close();

		//
		//	sign off from cloud
		//
        bus_->stop();
		CYNG_LOG_INFO(logger_, "cluster is stopped");
	}

	//	slot 0
	cyng::continuation cluster::process(cyng::version const& v)
	{
		if (v < cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR))
		{
			CYNG_LOG_WARNING(logger_, "insufficient cluster protocol version: "	<< v);
		}

		//
		//	start http server
		//
		server_.run();

		//
		//	sync tables
		//
		sync_table("TDevice");
		sync_table("TGateway");
		sync_table("TMeter");
		sync_table("*Session");
		sync_table("*Target");
		sync_table("*Connection");
		sync_table("*Cluster");
		sync_table("*Config");
		cache_.clear("*SysMsg", bus_->vm_.tag());
		sync_table("*SysMsg");

		return cyng::continuation::TASK_CONTINUE;
	}

	void cluster::sync_table(std::string const& name)
	{
		CYNG_LOG_INFO(logger_, "sync table " << name);

		//
		//	manage table state
		//
		cache_.set_state(name, 0);

		//
		//	Get existing records from master. This could be setup data
		//	from another redundancy or data collected during a line disruption.
		//
		bus_->vm_.async_run(bus_req_subscribe(name, base_.get_id()));

	}

	cyng::continuation cluster::process()
	{
		//
		//	Connection to master lost
		//

		//
		//	tell server to discard all data
		//
		cache_.clear("TDevice", bus_->vm_.tag());
		cache_.clear("TGateway", bus_->vm_.tag());
		cache_.clear("TMeter", bus_->vm_.tag());
		cache_.clear("*Session", bus_->vm_.tag());
		cache_.clear("*Target", bus_->vm_.tag());
		cache_.clear("*Connection", bus_->vm_.tag());
		cache_.clear("*Cluster", bus_->vm_.tag());
		cache_.clear("*Config", bus_->vm_.tag());
		//cache_.clear("*SysMsg", bus_->vm_.tag());

		//
		//	switch to other configuration
		//
		reconfigure_impl();

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}

	void cluster::connect()
	{

		bus_->vm_.async_run(bus_req_login(config_[master_].host_
			, config_[master_].service_
			, config_[master_].account_
			, config_[master_].pwd_
			, config_[master_].auto_config_
			, config_[master_].group_
			, "dash"));

		CYNG_LOG_INFO(logger_, "cluster login request is sent");

	}

	void cluster::reconfigure(cyng::context& ctx)
	{
		reconfigure_impl();
	}


	void cluster::reconfigure_impl()
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
			CYNG_LOG_ERROR(logger_, "cluster login failed - no other redundancy available");
		}

		//
		//	trigger reconnect 
		//
		CYNG_LOG_INFO(logger_, "reconnect to cluster in "
			<< config_[master_].monitor_.count()
			<< " seconds");
		base_.suspend(config_[master_].monitor_);

	}

	void cluster::res_subscribe(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		//	[TDevice,[763ae055-449c-4783-b383-8fc8cd52f44f],[2018-01-23 13:02:05.66872930,true,vFirmware,id,descr,number,name],72,aa7dc32f-91ff-4b08-89fc-53afb244a6a9,3]
		//	[*Session,[762b6635-5d37-45bd-b287-a34eeff87bdb],[00000000-0000-0000-0000-000000000000,2018-03-04 16:59:35.54259310,d091bb5c,LSMTest5,5fabcce3-a019-4264-a0f8-14d791a61eb9,b3aa75c9-0d50-44c5-b9dc-3e41d36daa6a,null,session],1,00f30cd8-786e-4616-8b9f-00c0b6ef272a,1]
		//
		//	* table name
		//	* record key
		//	* record data
		//	* generation
		//	* origin session id
		//	* optional task id
		//	
		CYNG_LOG_TRACE(logger_, "res.subscribe - " << cyng::io::to_str(frame));

		auto tpl = cyng::tuple_cast<
			std::string,			//	[0] table name
			cyng::table::key_type,	//	[1] table key
			cyng::table::data_type,	//	[2] record
			std::uint64_t,			//	[3] generation
			boost::uuids::uuid,		//	[4] origin session id
			std::size_t				//	[5] origin task id
		>(frame);

		CYNG_LOG_TRACE(logger_, "res.subscribe " 
			<< std::get<0>(tpl)		// table name
			<< " - "
			<< cyng::io::to_str(std::get<1>(tpl)));

		//	 [TDevice,[911fc4a1-8d9b-4d18-97f7-84a1cd576139],[00000006,2018-02-04 15:31:37.00000000,true,v88,ID,comment #88,1088,secret,device-88],88,dfa6b9a1-4170-41bd-8945-80b936059231,1]
		std::reverse(std::get<1>(tpl).begin(), std::get<1>(tpl).end());
		std::reverse(std::get<2>(tpl).begin(), std::get<2>(tpl).end());
		if (!cache_.insert(std::get<0>(tpl)
			, std::get<1>(tpl)
			, std::get<2>(tpl)
			, std::get<3>(tpl)
			, std::get<4>(tpl)))
		{
			CYNG_LOG_WARNING(logger_, "res.subscribe failed "
				<< std::get<0>(tpl)		// table name
				<< " - "
				<< cyng::io::to_str(std::get<1>(tpl)));
		}
	}

	void cluster::db_res_insert(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		//
		//	[TDevice,[32f1a373-83c9-4f24-8fac-b13103bc7466],[00000006,2018-03-11 18:35:33.61302590,true,,,comment #10,1010000,secret,device-10000],0]
		//
		//	* table name
		//	* record key
		//	* record data
		//	* generation
		//	
		CYNG_LOG_TRACE(logger_, "db.res.insert - " << cyng::io::to_str(frame));

		auto tpl = cyng::tuple_cast<
			std::string,			//	[0] table name
			cyng::table::key_type,	//	[1] table key
			cyng::table::data_type,	//	[2] record
			std::uint64_t			//	[3] generation
		>(frame);

		//
		//	assemble a record
		//
		std::reverse(std::get<1>(tpl).begin(), std::get<1>(tpl).end());
		std::reverse(std::get<2>(tpl).begin(), std::get<2>(tpl).end());
		cyng::table::record rec(cache_.meta(std::get<0>(tpl)), std::get<1>(tpl), std::get<2>(tpl), std::get<3>(tpl));

		if (!cache_.insert(std::get<0>(tpl)
			, std::get<1>(tpl)
			, std::get<2>(tpl)
			, std::get<3>(tpl)
			, ctx.tag()))	//	self
		{
			CYNG_LOG_WARNING(logger_, "db.res.insert failed "
				<< std::get<0>(tpl)		// table name
				<< " - "
				<< cyng::io::to_str(std::get<1>(tpl)));
		}

	}

	void cluster::db_req_insert(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		//
		//	[*Session,[2ce46726-6bca-44b6-84ed-0efccb67774f],[00000000-0000-0000-0000-000000000000,2018-03-12 17:56:27.10338240,f51f2ae7,data-store,eaec7649-80d5-4b71-8450-3ee2c7ef4917,94aa40f9-70e8-4c13-987e-3ed542ecf7ab,null,session],1]
		//
		//	* table name
		//	* record key
		//	* record data
		//	* generation
		//	
		CYNG_LOG_TRACE(logger_, "db.req.insert - " << cyng::io::to_str(frame));

		auto tpl = cyng::tuple_cast<
			std::string,			//	[0] table name
			cyng::table::key_type,	//	[1] table key
			cyng::table::data_type,	//	[2] record
			std::uint64_t,			//	[3] generation
			boost::uuids::uuid		//	[4] source
		>(frame);

		//
		//	assemble a record
		//
		std::reverse(std::get<1>(tpl).begin(), std::get<1>(tpl).end());
		std::reverse(std::get<2>(tpl).begin(), std::get<2>(tpl).end());
		cyng::table::record rec(cache_.meta(std::get<0>(tpl)), std::get<1>(tpl), std::get<2>(tpl), std::get<3>(tpl));

		if (!cache_.insert(std::get<0>(tpl)
			, std::get<1>(tpl)
			, std::get<2>(tpl)
			, std::get<3>(tpl)
			, std::get<4>(tpl)))
		{
			CYNG_LOG_WARNING(logger_, "db.req.insert failed "
				<< std::get<0>(tpl)		// table name
				<< " - "
				<< cyng::io::to_str(std::get<1>(tpl)));
		}
	}

	void cluster::db_req_remove(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		//
		//	[*Session,[e72bc048-cb37-4a86-b156-d07f22608476]]
		//
		//	* table name
		//	* record key
		//	* source
		//	
		CYNG_LOG_TRACE(logger_, "db.req.remove - " << cyng::io::to_str(frame));

		auto tpl = cyng::tuple_cast<
			std::string,			//	[0] table name
			cyng::table::key_type,	//	[1] table key
			boost::uuids::uuid		//	[2] source
		>(frame);

		//
		//	reordering table key
		//	
		std::reverse(std::get<1>(tpl).begin(), std::get<1>(tpl).end());

		if (!cache_.erase(std::get<0>(tpl), std::get<1>(tpl), std::get<2>(tpl)))
		{
			CYNG_LOG_WARNING(logger_, "db.req.remove failed "
				<< std::get<0>(tpl)		// table name
				<< " - "
				<< cyng::io::to_str(std::get<1>(tpl)));

		}
	}
	void cluster::db_res_remove(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		//
		//	[TDevice,[def8e1ef-4a67-49ff-84a9-fda31509dd8e]]
		//
		//	* table name
		//	* record key
		//	
		CYNG_LOG_TRACE(logger_, "db.res.remove - " << cyng::io::to_str(frame));

		auto tpl = cyng::tuple_cast<
			std::string,			//	[0] table name
			cyng::table::key_type	//	[1] table key
		>(frame);

		//
		//	reordering table key
		//	
		std::reverse(std::get<1>(tpl).begin(), std::get<1>(tpl).end());

		if (!cache_.erase(std::get<0>(tpl), std::get<1>(tpl), ctx.tag()))
		{
			CYNG_LOG_WARNING(logger_, "db.res.remove failed "
				<< std::get<0>(tpl)		// table name
				<< " - "
				<< cyng::io::to_str(std::get<1>(tpl)));
		}
	}

	void cluster::db_res_modify_by_attr(cyng::context& ctx)
	{
		//	[TDevice,[0950f361-7800-4d80-b3bc-c6689f159439],(1:secret)]
		//
		//	* table name
		//	* record key
		//	* attr [column,value]
		//	
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "db.res.modify.by.attr - " << cyng::io::to_str(frame));

		auto tpl = cyng::tuple_cast<
			std::string,			//	[0] table name
			cyng::table::key_type,	//	[1] table key
			cyng::attr_t			//	[2] attribute
		>(frame);

		//
		//	reordering table key
		//	
		std::reverse(std::get<1>(tpl).begin(), std::get<1>(tpl).end());

		if (!cache_.modify(std::get<0>(tpl), std::get<1>(tpl), std::move(std::get<2>(tpl)), ctx.tag()))
		{
			CYNG_LOG_WARNING(logger_, "db.res.modify.by.attr failed "
				<< std::get<0>(tpl)		// table name
				<< " - "
				<< cyng::io::to_str(std::get<1>(tpl)));
		}
	}

	void cluster::db_res_modify_by_param(cyng::context& ctx)
	{
		//	
		//
		//	* table name
		//	* record key
		//	* param [column,value]
		//	
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "db.res.modify.by.param - " << cyng::io::to_str(frame));

		auto tpl = cyng::tuple_cast<
			std::string,			//	[0] table name
			cyng::table::key_type,	//	[1] table key
			cyng::param_t			//	[2] parameter
		>(frame);

		if (!cache_.modify(std::get<0>(tpl), std::get<1>(tpl), std::get<2>(tpl), ctx.tag()))
		{
			CYNG_LOG_WARNING(logger_, "db.res.modify.by.param failed "
				<< std::get<0>(tpl)		// table name
				<< " - "
				<< cyng::io::to_str(std::get<1>(tpl)));
		}
	}

	void cluster::db_req_modify_by_param(cyng::context& ctx)
	{
		//	
		//	[*Session,[35d1d76d-56c3-4df7-b1ff-b7ad374d2e8f],("rx":33344),327,35d1d76d-56c3-4df7-b1ff-b7ad374d2e8f]
		//	[*Cluster,[1e4527b3-6479-4b2c-854b-e4793f40d864],("ping":00:00:0.003736),4,1e4527b3-6479-4b2c-854b-e4793f40d864]
		//
		//	* table name
		//	* record key
		//	* param [column,value]
		//	* generation
		//	* source
		//	
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "db.req.modify.by.param - " << cyng::io::to_str(frame));

		auto tpl = cyng::tuple_cast<
			std::string,			//	[0] table name
			cyng::table::key_type,	//	[1] table key
			cyng::param_t,			//	[2] parameter
			std::uint64_t,			//	[3] generation
			boost::uuids::uuid		//	[4] source
		>(frame);

		//
		//	reordering table key
		//	
		std::reverse(std::get<1>(tpl).begin(), std::get<1>(tpl).end());

		if (!cache_.modify(std::get<0>(tpl), std::get<1>(tpl), std::get<2>(tpl), std::get<4>(tpl)))
		{
			CYNG_LOG_WARNING(logger_, "db.req.modify.by.param failed "
				<< std::get<0>(tpl)		// table name
				<< " - "
				<< cyng::io::to_str(std::get<1>(tpl)));
		}
	}


	void cluster::ws_read(cyng::context& ctx)
	{
		//	[1adb06d2-bfbf-4b00-a7b1-80b49ba48f79,<!261:ws>,{("cmd":subscribe),("channel":status.session),("push":true)}]
		//	
		//	* session tag
		//	* ws object
		//	* json object
		const cyng::vector_t frame = ctx.get_frame();

		auto wsp = cyng::object_cast<http::websocket_session>(frame.at(1));
		if (!wsp)
		{
			CYNG_LOG_FATAL(logger_, "ws.read - no websocket: " << cyng::io::to_str(frame));
			return;
		}
		else
		{
			CYNG_LOG_TRACE(logger_, "ws.read - " << cyng::io::to_str(frame));
		}

		auto reader = cyng::make_reader(frame.at(2));
		const std::string cmd = cyng::value_cast<std::string>(reader.get("cmd"), "");
		if (boost::algorithm::equals(cmd, "subscribe"))
		{
			const std::string channel = cyng::value_cast<std::string>(reader.get("channel"), "");
			CYNG_LOG_TRACE(logger_, "ws.read - subscribe channel [" << channel << "]");

			if (boost::algorithm::starts_with(channel, "config.device"))
			{
				subscribe_devices(channel, cyng::value_cast(frame.at(0), boost::uuids::nil_uuid()));
			}
			else if (boost::algorithm::starts_with(channel, "config.gateway"))
			{
				subscribe_gateways(channel, cyng::value_cast(frame.at(0), boost::uuids::nil_uuid()));
			}
			else if (boost::algorithm::starts_with(channel, "status.session"))
			{
				subscribe_sessions(channel, cyng::value_cast(frame.at(0), boost::uuids::nil_uuid()));
			}
			else if (boost::algorithm::starts_with(channel, "status.target"))
			{
				subscribe_targets(channel, cyng::value_cast(frame.at(0), boost::uuids::nil_uuid()));
			}
			else if (boost::algorithm::starts_with(channel, "status.connection"))
			{
				subscribe_connections(channel, cyng::value_cast(frame.at(0), boost::uuids::nil_uuid()));
			}
			else if (boost::algorithm::starts_with(channel, "status.cluster"))
			{
				subscribe_cluster(channel, cyng::value_cast(frame.at(0), boost::uuids::nil_uuid()));
			}
			else if (boost::algorithm::starts_with(channel, "table.device.count"))
			{
				subscribe_table_device_count(channel, cyng::value_cast(frame.at(0), boost::uuids::nil_uuid()));
			}
			else if (boost::algorithm::starts_with(channel, "table.gateway.count"))
			{
				subscribe_table_gateway_count(channel, cyng::value_cast(frame.at(0), boost::uuids::nil_uuid()));
			}
			else if (boost::algorithm::starts_with(channel, "table.meter.count"))
			{
				subscribe_table_meter_count(channel, cyng::value_cast(frame.at(0), boost::uuids::nil_uuid()));
			}
			else if (boost::algorithm::starts_with(channel, "table.session.count"))
			{
				subscribe_table_session_count(channel, cyng::value_cast(frame.at(0), boost::uuids::nil_uuid()));
			}
			else if (boost::algorithm::starts_with(channel, "table.target.count"))
			{
				subscribe_table_target_count(channel, cyng::value_cast(frame.at(0), boost::uuids::nil_uuid()));
			}
			else if (boost::algorithm::starts_with(channel, "table.connection.count"))
			{
				subscribe_table_connection_count(channel, cyng::value_cast(frame.at(0), boost::uuids::nil_uuid()));
			}
			else if (boost::algorithm::starts_with(channel, "monitor.msg"))
			{
				subscribe_monitor_msg(channel, cyng::value_cast(frame.at(0), boost::uuids::nil_uuid()));
			}		
			else
			{
				CYNG_LOG_WARNING(logger_, "ws.read - unknown subscribe channel [" << channel << "]");
			}
		}
		else if (boost::algorithm::equals(cmd, "update"))
		{
			const std::string channel = cyng::value_cast<std::string>(reader.get("channel"), "");
			CYNG_LOG_TRACE(logger_, "ws.read - pull channel [" << channel << "]");
			if (boost::algorithm::starts_with(channel, "sys.cpu.usage.total"))
			{				
				update_sys_cpu_usage_total(channel, const_cast<http::websocket_session*>(wsp));
			}
			else if (boost::algorithm::starts_with(channel, "sys.cpu.count"))
			{
				update_sys_cpu_count(channel, const_cast<http::websocket_session*>(wsp));
			}
			else if (boost::algorithm::starts_with(channel, "sys.mem.virtual.total"))
			{
				update_sys_mem_virtual_total(channel, const_cast<http::websocket_session*>(wsp));
			}
			else if (boost::algorithm::starts_with(channel, "sys.mem.virtual.used"))
			{
				update_sys_mem_virtual_used(channel, const_cast<http::websocket_session*>(wsp));
			}
			else
			{
				CYNG_LOG_WARNING(logger_, "ws.read - unknown update channel [" << channel << "]");
			}
		}
		else if (boost::algorithm::equals(cmd, "insert"))
		{
			//	{"cmd":"insert","channel":"config.device","rec":{"key":{"pk":"0b5c2a64-5c48-48f1-883b-e5be3a3b1e3d"},"data":{"name":"demo","msisdn":"demo","descr":"comment #55","enabled":true,"pwd":"secret","age":"2018-02-04T14:31:34.000Z"}}}
			const std::string channel = cyng::value_cast<std::string>(reader.get("channel"), "");
			CYNG_LOG_TRACE(logger_, "ws.read - insert channel [" << channel << "]");
			if (boost::algorithm::starts_with(channel, "config.device"))
			{
				ctx.attach(bus_req_db_insert("TDevice"
					//	generate new key
					, cyng::table::key_generator(rgn_())	
					//	build data vector
					, cyng::vector_t{ reader["rec"]["data"].get("name")
						, reader["rec"]["data"].get("pwd")
						, reader["rec"]["data"].get("msisdn")
						, reader["rec"]["data"].get("descr")
						, cyng::make_object("")	//	model
						, cyng::make_object("")	//	version
						, reader["rec"]["data"].get("enabled")
						, cyng::make_now()
						, cyng::make_object(static_cast<std::uint32_t>(6)) }
					, 0
					, ctx.tag()));
			}
			else
			{
				CYNG_LOG_WARNING(logger_, "ws.read - unknown insert channel [" << channel << "]");
			}
		}
		else if (boost::algorithm::equals(cmd, "delete"))
		{
			//	[ce28c0c4-1914-45d1-bcf1-22bcbe461855,{("cmd":delete),("channel":config.device),("key":{("tag":a8b3d691-6ea9-40d3-83aa-b1e4dfbcb2f1)})}]
			const std::string channel = cyng::value_cast<std::string>(reader.get("channel"), "");
			CYNG_LOG_TRACE(logger_, "ws.read - delete channel [" << channel << "]");
			if (boost::algorithm::starts_with(channel, "config.device"))
			{
				//
				//	TDevice key is of type UUID
				//
				try {
					cyng::vector_t vec;
					vec = cyng::value_cast(reader["key"].get("tag"), vec);
					BOOST_ASSERT_MSG(vec.size() == 1, "TDevice key has wrong size");
					const std::string str = cyng::value_cast<std::string>(vec.at(0), "");
					CYNG_LOG_DEBUG(logger_, "TDevice key [" << str << "]");
					auto key = cyng::table::key_generator(boost::uuids::string_generator()(str));

					ctx.attach(bus_req_db_remove("TDevice", key, ctx.tag()));
				}
				catch (std::exception const& ex) {
					CYNG_LOG_ERROR(logger_, "ws.read - delete channel [" << channel << "] failed");
				}
			}
			else
			{
				CYNG_LOG_WARNING(logger_, "ws.read - unknown delete channel [" << channel << "]");
			}
		}
		else if (boost::algorithm::equals(cmd, "modify"))
		{
			const std::string channel = cyng::value_cast<std::string>(reader.get("channel"), "");
			CYNG_LOG_TRACE(logger_, "ws.read - modify channel [" << channel << "]");
			if (boost::algorithm::starts_with(channel, "config.device"))
			{
				//
				//	TDevice key is of type UUID
				//	all values have the same key
				//
				try {

					cyng::vector_t vec;
					vec = cyng::value_cast(reader["rec"].get("key"), vec);
					BOOST_ASSERT_MSG(vec.size() == 1, "TDevice key has wrong size");

					const std::string str = cyng::value_cast<std::string>(vec.at(0), "");
					CYNG_LOG_DEBUG(logger_, "TDevice key [" << str << "]");
					auto key = cyng::table::key_generator(boost::uuids::string_generator()(str));

					cyng::tuple_t tpl;
					tpl = cyng::value_cast(reader["rec"].get("data"), tpl);
					for (auto p : tpl)
					{
						cyng::param_t param;
						ctx.attach(bus_req_db_modify("TDevice"
							, key
							, cyng::value_cast(p, param)
							, 0
							, ctx.tag()));
					}
				}
				catch (std::exception const& ex) {
					CYNG_LOG_ERROR(logger_, "ws.read - modify channel [" << channel << "] failed");
				}
			}
			else
			{
				CYNG_LOG_WARNING(logger_, "ws.read - unknown modify channel [" << channel << "]");
			}
		}
		else if (boost::algorithm::equals(cmd, "stop"))
		{
			const std::string channel = cyng::value_cast<std::string>(reader.get("channel"), "");
			CYNG_LOG_TRACE(logger_, "ws.read - stop channel [" << channel << "]");
			if (boost::algorithm::starts_with(channel, "status.session"))
			{
				//
				//	TDevice key is of type UUID
				//	all values have the same key
				//
				try {

					cyng::vector_t vec;
					vec = cyng::value_cast(reader.get("key"), vec);
					BOOST_ASSERT_MSG(vec.size() == 1, "*Session key has wrong size");

					const std::string str = cyng::value_cast<std::string>(vec.at(0), "");
					CYNG_LOG_DEBUG(logger_, "*Session key [" << str << "]");
					auto key = cyng::table::key_generator(boost::uuids::string_generator()(str));

					ctx.attach(bus_req_stop_client(key, ctx.tag()));

				}
				catch (std::exception const& ex) {
					CYNG_LOG_ERROR(logger_, "ws.read - modify channel [" << channel << "] failed");
				}

			}
		}
		else
		{
			CYNG_LOG_WARNING(logger_, "ws.read - unknown command " << cmd);
		}
	}

	void cluster::update_sys_cpu_usage_total(std::string const& channel, http::websocket_session* wss)
	{
		cache_.access([&](cyng::store::table* tbl) {
			const auto rec = tbl->lookup(cyng::table::key_generator("cpu:load"));
			if (!rec.empty())
			{
				
				CYNG_LOG_WARNING(logger_, cyng::io::to_str(rec["value"]));
				//
				//	Total CPU load of the system in %
				//
				auto tpl = cyng::tuple_factory(
					cyng::param_factory("cmd", std::string("update")),
					cyng::param_factory("channel", channel),
					cyng::param_t("value", rec["value"]));
					//cyng::param_factory("value", cyng::sys::get_total_cpu_load()));

				auto msg = cyng::json::to_string(tpl);

				if (wss)	wss->send_msg(msg);
			}
			else
			{
				CYNG_LOG_WARNING(logger_, "record cpu:load not found");
			}
		}, cyng::store::write_access("*Config"));
	}

	void cluster::update_sys_cpu_count(std::string const& channel, http::websocket_session* wss)
	{
		//
		//	CPU count
		//
		auto tpl = cyng::tuple_factory(
			cyng::param_factory("cmd", std::string("update")),
			cyng::param_factory("channel", channel),
			cyng::param_factory("value", std::thread::hardware_concurrency()));

		auto msg = cyng::json::to_string(tpl);

		if (wss)	wss->send_msg(msg);
	}


	void cluster::update_sys_mem_virtual_total(std::string const& channel, http::websocket_session* wss)
	{
		//
		//	total virtual memory in bytes
		//
		auto tpl = cyng::tuple_factory(
			cyng::param_factory("cmd", std::string("update")),
			cyng::param_factory("channel", channel),
			cyng::param_factory("value", cyng::sys::get_total_virtual_memory()));

		auto msg = cyng::json::to_string(tpl);

		if (wss)	wss->send_msg(msg);
	}

	void cluster::update_sys_mem_virtual_used(std::string const& channel, http::websocket_session* wss)
	{
		//
		//	used virtual memory in bytes
		//
		auto tpl = cyng::tuple_factory(
			cyng::param_factory("cmd", std::string("update")),
			cyng::param_factory("channel", channel),
			cyng::param_factory("value", cyng::sys::get_used_virtual_memory()));

		auto msg = cyng::json::to_string(tpl);

		if (wss)	wss->send_msg(msg);
	}

	void cluster::subscribe_devices(std::string const& channel, boost::uuids::uuid tag)
	{
		server_.add_channel(tag, channel);

		//
		//	send initial data set of device table
		//
		cache_.access([&](cyng::store::table const* tbl) {
			const auto counter = tbl->loop([&](cyng::table::record const& rec) -> bool {

				CYNG_LOG_TRACE(logger_, "ws.read - insert device " << cyng::io::to_str(rec.key()));

				auto tpl = cyng::tuple_factory(
					cyng::param_factory("cmd", std::string("insert")),
					cyng::param_factory("channel", channel),
					cyng::param_factory("rec", rec.convert()));

				auto msg = cyng::json::to_string(tpl);
				server_.send_msg(tag, msg);

				//	continue
				return true;
			});
            BOOST_ASSERT(counter == 0);
			CYNG_LOG_INFO(logger_, tbl->size() << ' ' << tbl->meta().get_name() << " records sent");

		}, cyng::store::read_access("TDevice"));
	}

	void cluster::subscribe_gateways(std::string const& channel, boost::uuids::uuid tag)
	{
		server_.add_channel(tag, channel);

		//
		//	send initial data set of device table
		//
		cache_.access([&](cyng::store::table const* tbl) {
			const auto counter = tbl->loop([&](cyng::table::record const& rec) -> bool {

				CYNG_LOG_TRACE(logger_, "ws.read - insert gateway " << cyng::io::to_str(rec.key()));

				auto tpl = cyng::tuple_factory(
					cyng::param_factory("cmd", std::string("insert")),
					cyng::param_factory("channel", channel),
					cyng::param_factory("rec", rec.convert()));

				auto msg = cyng::json::to_string(tpl);
				server_.send_msg(tag, msg);

				//	continue
				return true;
			});
            BOOST_ASSERT(counter == 0);
			CYNG_LOG_INFO(logger_, tbl->size() << ' ' << tbl->meta().get_name() << " records sent");

		}, cyng::store::read_access("TGateway"));
	}

	void cluster::subscribe_sessions(std::string const& channel, boost::uuids::uuid tag)
	{
		server_.add_channel(tag, channel);

		//
		//	send initial data set of session table
		//
		cache_.access([&](cyng::store::table const* tbl) {
			const auto counter = tbl->loop([&](cyng::table::record const& rec) -> bool {

				CYNG_LOG_TRACE(logger_, "ws.read - insert session " << cyng::io::to_str(rec.key()));

				auto tpl = cyng::tuple_factory(
					cyng::param_factory("cmd", std::string("insert")),
					cyng::param_factory("channel", channel),
					cyng::param_factory("rec", rec.convert()));

				auto msg = cyng::json::to_string(tpl);
				server_.send_msg(tag, msg);

				//	continue
				return true;
			});
            BOOST_ASSERT(counter == 0);
			CYNG_LOG_INFO(logger_, tbl->size() << ' ' << tbl->meta().get_name() << " records sent");

		}, cyng::store::read_access("*Session"));
	}

	void cluster::subscribe_targets(std::string const& channel, boost::uuids::uuid tag)
	{
		server_.add_channel(tag, channel);

		//
		//	send initial data set of target table
		//
		cache_.access([&](cyng::store::table const* tbl) {
			const auto counter = tbl->loop([&](cyng::table::record const& rec) -> bool {

				CYNG_LOG_TRACE(logger_, "ws.read - insert target " << cyng::io::to_str(rec.key()));

				auto tpl = cyng::tuple_factory(
					cyng::param_factory("cmd", std::string("insert")),
					cyng::param_factory("channel", channel),
					cyng::param_factory("rec", rec.convert()));

				auto msg = cyng::json::to_string(tpl);
				server_.send_msg(tag, msg);

				//	continue
				return true;
			});
            BOOST_ASSERT(counter == 0);
			CYNG_LOG_INFO(logger_, tbl->size() << ' ' << tbl->meta().get_name() << " records sent");

		}, cyng::store::read_access("*Target"));
	}

	void cluster::subscribe_connections(std::string const& channel, boost::uuids::uuid tag)
	{
		server_.add_channel(tag, channel);

		//
		//	send initial data set of connection table
		//
		cache_.access([&](cyng::store::table const* tbl) {
			const auto counter = tbl->loop([&](cyng::table::record const& rec) -> bool {

				CYNG_LOG_TRACE(logger_, "ws.read - insert connection " << cyng::io::to_str(rec.key()));

				auto tpl = cyng::tuple_factory(
					cyng::param_factory("cmd", std::string("insert")),
					cyng::param_factory("channel", channel),
					cyng::param_factory("rec", rec.convert()));

				auto msg = cyng::json::to_string(tpl);
				server_.send_msg(tag, msg);

				//	continue
				return true;
			});
			BOOST_ASSERT(counter == 0);
			CYNG_LOG_INFO(logger_, tbl->size() << ' ' << tbl->meta().get_name() << " records sent");

		}, cyng::store::read_access("*Connection"));
	}

	void cluster::subscribe_cluster(std::string const& channel, boost::uuids::uuid tag)
	{
		server_.add_channel(tag, channel);

		//
		//	send initial data set of cluster table
		//
		cache_.access([&](cyng::store::table const* tbl) {
			const auto counter = tbl->loop([&](cyng::table::record const& rec) -> bool {

				CYNG_LOG_TRACE(logger_, "ws.read - insert node " << cyng::io::to_str(rec.key()));

				auto tpl = cyng::tuple_factory(
					cyng::param_factory("cmd", std::string("insert")),
					cyng::param_factory("channel", channel),
					cyng::param_factory("rec", rec.convert()));

				auto msg = cyng::json::to_string(tpl);
				server_.send_msg(tag, msg);

				//	continue
				return true;
			});
			BOOST_ASSERT(counter == 0);
			CYNG_LOG_INFO(logger_, tbl->size() << ' ' << tbl->meta().get_name() << " records sent");

		}, cyng::store::read_access("*Cluster"));
	}

	void cluster::subscribe_table_device_count(std::string const& channel, boost::uuids::uuid tag)
	{
		server_.add_channel(tag, channel);
		const auto size = cache_.size("TDevice");
		update_channel(channel, size);
	}

	void cluster::subscribe_table_gateway_count(std::string const& channel, boost::uuids::uuid tag)
	{
		server_.add_channel(tag, channel);
		const auto size = cache_.size("TGateway");
		update_channel(channel, size);
	}

	void cluster::subscribe_table_meter_count(std::string const& channel, boost::uuids::uuid tag)
	{
		server_.add_channel(tag, channel);
		const auto size = cache_.size("TMeter");
		update_channel(channel, size);
	}

	void cluster::subscribe_table_session_count(std::string const& channel, boost::uuids::uuid tag)
	{
		server_.add_channel(tag, channel);
		const auto size = cache_.size("*Session");
		update_channel(channel, size);
	}

	void cluster::subscribe_table_target_count(std::string const& channel, boost::uuids::uuid tag)
	{
		server_.add_channel(tag, channel);
		const auto size = cache_.size("*Target");
		update_channel(channel, size);
	}

	void cluster::subscribe_table_connection_count(std::string const& channel, boost::uuids::uuid tag)
	{
		server_.add_channel(tag, channel);
		const auto size = cache_.size("*Connection");
		update_channel(channel, size);
	}

	void cluster::subscribe_monitor_msg(std::string const& channel, boost::uuids::uuid tag)
	{
		server_.add_channel(tag, channel);

		//
		//	send initial data set of SysMsg table
		//
		cache_.access([&](cyng::store::table const* tbl) {
			const auto counter = tbl->loop([&](cyng::table::record const& rec) -> bool {

				CYNG_LOG_TRACE(logger_, "ws.read - insert msg " << cyng::io::to_str(rec.key()));

				auto tpl = cyng::tuple_factory(
					cyng::param_factory("cmd", std::string("insert")),
					cyng::param_factory("channel", channel),
					cyng::param_factory("rec", rec.convert()));

				auto msg = cyng::json::to_string(tpl);
				server_.send_msg(tag, msg);

				//	continue
				return true;
			});
			BOOST_ASSERT(counter == 0);
			CYNG_LOG_INFO(logger_, tbl->size() << ' ' << tbl->meta().get_name() << " records sent");

		}, cyng::store::read_access("*SysMsg"));
	}

	void cluster::update_channel(std::string const& channel, std::size_t size)
	{
		auto tpl = cyng::tuple_factory(
			cyng::param_factory("cmd", std::string("update")),
			cyng::param_factory("channel", channel),
			cyng::param_factory("value", size));

		auto msg = cyng::json::to_string(tpl);
		server_.process_event(channel, msg);
	}

	void cluster::sig_ins(cyng::store::table const* tbl
		, cyng::table::key_type const& key
		, cyng::table::data_type const& data
		, std::uint64_t gen
		, boost::uuids::uuid source)
	{
		cyng::table::record rec(tbl->meta_ptr(), key, data, gen);

		if (boost::algorithm::equals(tbl->meta().get_name(), "TDevice"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("insert")),
				cyng::param_factory("channel", "config.device"),
				cyng::param_factory("rec", rec.convert()));

			auto msg = cyng::json::to_string(tpl);
			server_.process_event("config.device", msg);

			update_channel("table.device.count", tbl->size());
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "TGateway"))
		{ 
			//	data: {"cmd": "insert", "channel": "config.device", "rec": {"key": {"pk":"0b5c2a64-5c48-48f1-883b-e5be3a3b1e3d"}, "data": {"creationTime":"2018-02-04 15:31:34.00000000","descr":"comment #55","enabled":true,"id":"ID","msisdn":"1055","name":"device-55","pwd":"crypto","query":6,"vFirmware":"v55"}, "gen": 55}}
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("insert")),
				cyng::param_factory("channel", "config.gateway"),
				cyng::param_factory("rec", rec.convert()));

			auto msg = cyng::json::to_string(tpl);
			server_.process_event("config.gateway", msg);

			update_channel("table.gateway.count", tbl->size());
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "*Session"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("insert")),
				cyng::param_factory("channel", "status.session"),
				cyng::param_factory("rec", rec.convert()));

			auto msg = cyng::json::to_string(tpl);
			server_.process_event("status.session", msg);

			update_channel("table.session.count", tbl->size());
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "*Target"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("insert")),
				cyng::param_factory("channel", "status.target"),
				cyng::param_factory("rec", rec.convert()));

			auto msg = cyng::json::to_string(tpl);
			server_.process_event("status.target", msg);

			update_channel("table.target.count", tbl->size());
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "*Connection"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("insert")),
				cyng::param_factory("channel", "status.connection"),
				cyng::param_factory("rec", rec.convert()));

			auto msg = cyng::json::to_string(tpl);
			server_.process_event("status.connection", msg);

			update_channel("table.connection.count", tbl->size());
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "*Cluster"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("insert")),
				cyng::param_factory("channel", "status.cluster"),
				cyng::param_factory("rec", rec.convert()));

			auto msg = cyng::json::to_string(tpl);
			server_.process_event("status.cluster", msg);

			update_channel("table.cluster.count", tbl->size());
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "*Config"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("insert")),
				cyng::param_factory("channel", "config.sys"),
				cyng::param_factory("rec", rec.convert()));

			auto msg = cyng::json::to_string(tpl);
			server_.process_event("config.sys", msg);
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "*SysMsg"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("insert")),
				cyng::param_factory("channel", "monitor.msg"),
				cyng::param_factory("rec", rec.convert()));

			auto msg = cyng::json::to_string(tpl);
			server_.process_event("monitor.msg", msg);
		}
		else
		{
			CYNG_LOG_WARNING(logger_, "sig.ins - unknown table "
				<< tbl->meta().get_name());
		}
	}

	void cluster::sig_del(cyng::store::table const* tbl, cyng::table::key_type const& key, boost::uuids::uuid source)
	{
		if (boost::algorithm::equals(tbl->meta().get_name(), "TDevice"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("delete")),
				cyng::param_factory("channel", "config.device"),
				cyng::param_factory("key", key));

			auto msg = cyng::json::to_string(tpl);
			server_.process_event("config.device", msg);

			update_channel("table.device.count", tbl->size());

		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "TGateway"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("delete")),
				cyng::param_factory("channel", "config.gateway"),
				cyng::param_factory("key", key));

			auto msg = cyng::json::to_string(tpl);
			server_.process_event("config.gateway", msg);
			update_channel("table.gateway.count", tbl->size());

		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "*Session"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("delete")),
				cyng::param_factory("channel", "status.session"),
				cyng::param_factory("key", key));

			auto msg = cyng::json::to_string(tpl);
			server_.process_event("status.session", msg);

			update_channel("table.session.count", tbl->size());
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "*Target"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("delete")),
				cyng::param_factory("channel", "status.target"),
				cyng::param_factory("key", key));

			auto msg = cyng::json::to_string(tpl);
			server_.process_event("status.target", msg);

			update_channel("table.target.count", tbl->size());
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "*Connection"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("delete")),
				cyng::param_factory("channel", "status.connection"),
				cyng::param_factory("key", key));

			auto msg = cyng::json::to_string(tpl);
			server_.process_event("status.connection", msg);

			update_channel("table.connection.count", tbl->size());
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "*Cluster"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("delete")),
				cyng::param_factory("channel", "status.cluster"),
				cyng::param_factory("key", key));

			auto msg = cyng::json::to_string(tpl);
			server_.process_event("status.cluster", msg);

			update_channel("table.cluster.count", tbl->size());
		}
		else
		{
			CYNG_LOG_ERROR(logger_, "sig.del - unknown table "
				<< tbl->meta().get_name());
		}
	}

	void cluster::sig_clr(cyng::store::table const* tbl, boost::uuids::uuid source)
	{
		if (boost::algorithm::equals(tbl->meta().get_name(), "TDevice"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("clear")),
				cyng::param_factory("channel", "config.device"));

			auto msg = cyng::json::to_string(tpl);
			server_.process_event("config.device", msg);

			update_channel("table.device.count", tbl->size());
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "TGateway"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("clear")),
				cyng::param_factory("channel", "config.gateway"));

			auto msg = cyng::json::to_string(tpl);
			server_.process_event("config.gateway", msg);
			update_channel("table.gateway.count", tbl->size());

		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "*Session"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("clear")),
				cyng::param_factory("channel", "status.session"));

			auto msg = cyng::json::to_string(tpl);
			server_.process_event("status.session", msg);

			update_channel("table.session.count", tbl->size());
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "*Target"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("clear")),
				cyng::param_factory("channel", "status.target"));

			auto msg = cyng::json::to_string(tpl);
			server_.process_event("status.target", msg);

			update_channel("table.target.count", tbl->size());
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "*Connection"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("clear")),
				cyng::param_factory("channel", "status.connection"));

			auto msg = cyng::json::to_string(tpl);
			server_.process_event("status.connection", msg);

			update_channel("table.connection.count", tbl->size());
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "*Cluster"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("clear")),
				cyng::param_factory("channel", "status.cluster"));

			auto msg = cyng::json::to_string(tpl);
			server_.process_event("status.cluster", msg);

			update_channel("table.cluster.count", tbl->size());
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "*SysMsg"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("clear")),
				cyng::param_factory("channel", "monitor.msg"));

			auto msg = cyng::json::to_string(tpl);
			server_.process_event("monitor.msg", msg);

		}
		else
		{
			CYNG_LOG_ERROR(logger_, "sig.clr - unknown table "
				<< tbl->meta().get_name());
		}
	}

	void cluster::sig_mod(cyng::store::table const* tbl
		, cyng::table::key_type const& key
		, cyng::attr_t const& attr
		, std::uint64_t gen
		, boost::uuids::uuid source)
	{
		//	to much noise
		//CYNG_LOG_DEBUG(logger_, "sig.mod - "
		//	<< tbl->meta().get_name());

		//
		//	convert attribute to parameter (as map)
		//
		auto pm = tbl->meta().to_param_map(attr);

		if (boost::algorithm::equals(tbl->meta().get_name(), "TDevice"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("modify")),
				cyng::param_factory("channel", "config.device"),
				cyng::param_factory("key", key),
				cyng::param_factory("value", pm));

			auto msg = cyng::json::to_string(tpl);
			server_.process_event("config.device", msg);
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "TGateway"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("modify")),
				cyng::param_factory("channel", "config.gateway"),
				cyng::param_factory("key", key),
				cyng::param_factory("value", pm));

			auto msg = cyng::json::to_string(tpl);
			server_.process_event("config.gateway", msg);
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "*Session"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("modify")),
				cyng::param_factory("channel", "status.session"),
				cyng::param_factory("key", key),
				cyng::param_factory("value", pm));

			auto msg = cyng::json::to_string(tpl);
			server_.process_event("status.session", msg);
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "*Target"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("modify")),
				cyng::param_factory("channel", "status.target"),
				cyng::param_factory("key", key),
				cyng::param_factory("value", pm));

			auto msg = cyng::json::to_string(tpl);
			server_.process_event("status.target", msg);
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "*Connection"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("modify")),
				cyng::param_factory("channel", "status.connection"),
				cyng::param_factory("key", key),
				cyng::param_factory("value", pm));

			auto msg = cyng::json::to_string(tpl);
			server_.process_event("status.connection", msg);
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "*Cluster"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("modify")),
				cyng::param_factory("channel", "status.cluster"),
				cyng::param_factory("key", key),
				cyng::param_factory("value", pm));

			auto msg = cyng::json::to_string(tpl);
			server_.process_event("status.cluster", msg);
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "*Config"))
		{ }
		else
		{
			CYNG_LOG_WARNING(logger_, "sig.mode - unknown table "
				<< tbl->meta().get_name());

		}
	}

	void cluster::create_cache()
	{
		CYNG_LOG_TRACE(logger_, "create cache tables");

		if (!cache_.create_table(cyng::table::make_meta_table<1, 9>("TDevice",
			{ "pk", "name", "pwd", "msisdn", "descr", "id", "vFirmware", "enabled", "creationTime", "query" },
			{ cyng::TC_UUID, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_BOOL, cyng::TC_TIME_POINT, cyng::TC_UINT32 },
			{ 36, 128, 16, 128, 512, 64, 64, 0, 0, 0 })))
		{
			CYNG_LOG_FATAL(logger_, "cannot create table TDevice");
		}

		if (!cache_.create_table(cyng::table::make_meta_table<1, 13>("TGateway", { "pk"	//	primary key
			, "id"	//	(1) Server-ID (i.e. 0500153B02517E)
			, "manufacturer"	//	(2) manufacturer (i.e. EMH)
			, "model"	//	(3) Typbezeichnung (i.e. Variomuc ETHERNET)
			, "made"	//	(4) production date
			, "vFirmware"	//	(5) firmwareversion (i.e. 11600000)
			, "factoryNr"	//	(6) fabrik nummer (i.e. 06441734)
			, "ifService"	//	(7) MAC of service interface
			, "ifData"	//	(8) MAC od data interface
			, "pwdDef"	//	(9) Default PW
			, "pwdRoot"	//	(10)
			, "mbus"	//	(11)
			, "userName"	//	(12)
			, "userPwd"	//	(13)
			},
			{ cyng::TC_UUID, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_TIME_POINT, cyng::TC_STRING, cyng::TC_MAC48, cyng::TC_MAC48, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING },
			{ 36, 23, 64, 64, 0, 8, 18, 18, 32, 32, 16, 32, 32 })))
		{
			CYNG_LOG_FATAL(logger_, "cannot create table TGateway");
		}

		if (!cache_.create_table(cyng::table::make_meta_table<1, 9>("TMeter", { "pk"
			, "ident"	//	ident nummer (i.e. 1EMH0006441734)
			, "manufacturer"
			, "factoryNr"	//	fabrik nummer (i.e. 06441734)
			, "age"	//	production data
			, "vParam"	//	parametrierversion (i.e. 16A098828.pse)
			, "vFirmware"	//	firmwareversion (i.e. 11600000)
			, "item"	//	 artikeltypBezeichnung = "NXT4-S20EW-6N00-4000-5020-E50/Q"
			, "class"	//	Metrological Class: A, B, C, Q3/Q1, ...
			, "source"	//	source client (UUID)
			},
			{ cyng::TC_UUID, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_TIME_POINT, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_UUID },
			{ 36, 64, 64, 8, 0, 64, 64, 128, 8, 36 })))
		{
			CYNG_LOG_FATAL(logger_, "cannot create table TMeter");
		}

		if (!cache_.create_table(cyng::table::make_meta_table<1, 12>("*Session", { "tag"	//	client session - primary key [uuid]
			, "local"	//	[object] local peer object (hold session reference)
			, "remote"	//	[object] remote peer object (if session connected)
			, "peer"	//	[uuid] remote peer
			, "device"	//	[uuid] - owner of the session
			, "name"	//	[string] - account
			, "source"	//	[uint32] - ipt source id (unique)
			, "loginTime"	//	last login time
			, "rtag"	//	[uuid] client session if connected
			, "layer"	//	[string] protocol layer
			, "rx"		//	[uint64] received bytes (from device)
			, "sx"		//	[uint64] sent bytes (to device)
			, "px"		//	[uint64] sent push data (to push target)
			},
			{ cyng::TC_UUID, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_UUID, cyng::TC_UUID, cyng::TC_STRING, cyng::TC_UINT32, cyng::TC_TIME_POINT, cyng::TC_UUID, cyng::TC_STRING, cyng::TC_UINT64, cyng::TC_UINT64, cyng::TC_UINT64 },
			{ 36, 0, 0, 36, 36, 64, 0, 0, 36, 16, 0, 0, 0 })))
		{
			CYNG_LOG_FATAL(logger_, "cannot create table *Session");
		}

		if (!cache_.create_table(cyng::table::make_meta_table<1, 9>("*Target", { "channel"	//	name - primary key
			, "tag"		//	[uuid] owner session - primary key 
			, "peer"	//	[uuid] peer of owner
			, "name"	//	[uint32] - target id
			, "device"	//	[uuid] - owner of target
			, "account"	//	[string] - name of target owner
			, "pSize"	//	[uint16] - packet size
			, "wSize"	//	[uint8] - window size
			, "regTime"	//	registration time
			, "px"		//	incoming data
			},
			{ cyng::TC_UINT32, cyng::TC_UUID, cyng::TC_UUID, cyng::TC_STRING, cyng::TC_UUID, cyng::TC_STRING, cyng::TC_UINT16, cyng::TC_UINT8, cyng::TC_TIME_POINT, cyng::TC_UINT64 },
			{ 0, 36, 36, 64, 36, 64, 0, 0, 0, 0 })))
		{
			CYNG_LOG_FATAL(logger_, "cannot create table *Target");
		}

		if (!cache_.create_table(cyng::table::make_meta_table<2, 7>("*Connection",
			{ "first"		//	[uuid] primary key 
			, "second"		//	[uuid] primary key 
			, "aName"		//	[string] caller
			, "bName"		//	[string] callee
			, "local"		//	[bool] true if local connection
			, "aLayer"		//	[string] protocol layer of caller
			, "bLayer"		//	[string] protocol layer of callee
			, "throughput"	//	[uint64] data throughput
			, "start"		//	[tp] start time
			},
			{
				cyng::TC_UUID, cyng::TC_UUID,
				cyng::TC_STRING, cyng::TC_STRING, cyng::TC_BOOL, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_UINT64, cyng::TC_TIME_POINT
			},
			{ 0, 0, 128, 128, 0, 16, 16, 0, 0 })))
		{
			CYNG_LOG_FATAL(logger_, "cannot create table *Connection");
		}

		if (!cache_.create_table(cyng::table::make_meta_table<1, 7>("*Cluster", { "tag"	//	client session - primary key [uuid]
			, "class"
			, "loginTime"	//	last login time
			, "version"
			, "clients"	//	client counter
			, "ping"	//	ping time
			, "ep"		//	remote endpoint
			, "pid"		//	process id
			},
			{ cyng::TC_UUID, cyng::TC_STRING, cyng::TC_TIME_POINT, cyng::TC_VERSION, cyng::TC_UINT64, cyng::TC_MICRO_SECOND, cyng::TC_IP_TCP_ENDPOINT, cyng::TC_INT64 },
			{ 36, 0, 32, 0, 0, 0, 0, 0 })))
		{
			CYNG_LOG_FATAL(logger_, "cannot create table *Cluster");
		}

		if (!cache_.create_table(cyng::table::make_meta_table<1, 1>("*Config", { "name"	//	parameter name
			, "value"	//	parameter value
			},
			{ cyng::TC_STRING, cyng::TC_STRING },
			{ 64, 128 })))
		{
			CYNG_LOG_FATAL(logger_, "cannot create table *Config");
		}
		else
		{
			//
			//	set initial value
			//
			cache_.insert("*Config", cyng::table::key_generator("cpu:load"), cyng::table::data_generator(0.0), 0, bus_->vm_.tag());
		}

		if (!cache_.create_table(cyng::table::make_meta_table<1, 3>("*SysMsg", { "id"	//	message number
			, "ts"	//	timestamp
			, "severity"
			, "msg"	//	message text
			},
			{ cyng::TC_UINT64, cyng::TC_TIME_POINT, cyng::TC_UINT8, cyng::TC_STRING },
			{ 0, 0, 0, 128 })))
		{
			CYNG_LOG_FATAL(logger_, "cannot create table *SysMsg");
		}

	}

	void cluster::subscribe_cache()
	{
		cache_.get_listener("TDevice"
			, std::bind(&cluster::sig_ins, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
			, std::bind(&cluster::sig_del, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			, std::bind(&cluster::sig_clr, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&cluster::sig_mod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
		cache_.get_listener("TGateway"
			, std::bind(&cluster::sig_ins, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
			, std::bind(&cluster::sig_del, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			, std::bind(&cluster::sig_clr, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&cluster::sig_mod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
		cache_.get_listener("TMeter"
			, std::bind(&cluster::sig_ins, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
			, std::bind(&cluster::sig_del, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			, std::bind(&cluster::sig_clr, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&cluster::sig_mod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
		cache_.get_listener("*Session"
			, std::bind(&cluster::sig_ins, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
			, std::bind(&cluster::sig_del, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			, std::bind(&cluster::sig_clr, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&cluster::sig_mod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
		cache_.get_listener("*Target"
			, std::bind(&cluster::sig_ins, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
			, std::bind(&cluster::sig_del, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			, std::bind(&cluster::sig_clr, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&cluster::sig_mod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
		cache_.get_listener("*Connection"
			, std::bind(&cluster::sig_ins, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
			, std::bind(&cluster::sig_del, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			, std::bind(&cluster::sig_clr, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&cluster::sig_mod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
		cache_.get_listener("*Cluster"
			, std::bind(&cluster::sig_ins, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
			, std::bind(&cluster::sig_del, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			, std::bind(&cluster::sig_clr, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&cluster::sig_mod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
		cache_.get_listener("*Config"
			, std::bind(&cluster::sig_ins, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
			, std::bind(&cluster::sig_del, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			, std::bind(&cluster::sig_clr, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&cluster::sig_mod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
		cache_.get_listener("*SysMsg"
			, std::bind(&cluster::sig_ins, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
			, std::bind(&cluster::sig_del, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			, std::bind(&cluster::sig_clr, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&cluster::sig_mod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
	}

}
