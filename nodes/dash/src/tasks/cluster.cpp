/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "cluster.h"
#include <smf/cluster/generator.h>

#include <cyng/async/task/task_builder.hpp>
#include <cyng/io/serializer.h>
#include <cyng/vm/generator.h>
#include <cyng/table/meta.hpp>
#include <cyng/tuple_cast.hpp>
#include <cyng/dom/reader.h>
#include <cyng/sys/cpu.h>
#include <cyng/sys/memory.h>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/nil_generator.hpp>
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
		, bus_(bus_factory(btp->mux_, logger, boost::uuids::random_generator()(), btp->get_id()))
		, logger_(logger)
		, config_(cfg_cls)
		, cache_()
		, server_(logger, btp->mux_.get_io_service(), ep, doc_root, bus_, cache_)
		, master_(0)
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
		bus_->vm_.run(cyng::register_function("db.insert", 6, std::bind(&cluster::db_insert, this, std::placeholders::_1)));
		bus_->vm_.run(cyng::register_function("db.modify.by.attr", 3, std::bind(&cluster::db_modify_by_attr, this, std::placeholders::_1)));

		//
		//	input from HTTP session / ws
		//
		bus_->vm_.run(cyng::register_function("ws.read", 2, std::bind(&cluster::ws_read, this, std::placeholders::_1)));

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
		//	ToDo: stop all sessions
		//

		//
		//	sign off from cloud
		//
		bus_->vm_.async_run(bus_shutdown());
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
		snyc_table("TDevice");
		snyc_table("TGateway");
		snyc_table("*Session");

		return cyng::continuation::TASK_CONTINUE;
	}

	void cluster::snyc_table(std::string const& name)
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
		//	tell server to scratch all data
		//
		//server_.close();

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

	void cluster::db_insert(cyng::context& ctx)
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
		CYNG_LOG_TRACE(logger_, "db.insert - " << cyng::io::to_str(frame));

		auto tpl = cyng::tuple_cast<
			std::string,			//	[0] table name
			cyng::table::key_type,	//	[1] table key
			cyng::table::data_type,	//	[2] record
			std::uint64_t,			//	[3] generation
			boost::uuids::uuid,		//	[4] origin session id
			std::size_t				//	[5] origin task id
		>(frame);

		//const std::string table = cyng::value_cast<std::string>(frame.at(0), "");
		CYNG_LOG_TRACE(logger_, "db.insert " 
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
			CYNG_LOG_WARNING(logger_, "db.insert failed "
				<< std::get<0>(tpl)		// table name
				<< " - "
				<< cyng::io::to_str(std::get<1>(tpl)));
		}
	}

	void cluster::db_modify_by_attr(cyng::context& ctx)
	{
		//	[TDevice,[eaec7649-80d5-4b71-8450-3ee2c7ef4917],(4:ipt:store)]
		//
		//	* table name
		//	* record key
		//	* attr [column,value]
		//	
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "db.modify.by.attr - " << cyng::io::to_str(frame));
		const std::string table = cyng::value_cast<std::string>(frame.at(0), "");
	}

	void cluster::ws_read(cyng::context& ctx)
	{
		//	[29198287-c6c2-45ac-a405-ab6afd2bbec1,{("cmd":subscribe),("channel":sys.cpu.load)}]
		//	
		//	* session tag
		//	* json object
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "ws.read - " << cyng::io::to_str(frame));

		auto reader = cyng::make_reader(frame.at(1));
		const std::string cmd = cyng::value_cast<std::string>(reader.get("cmd"), "");
		if (boost::algorithm::equals(cmd, "subscribe"))
		{
			const std::string channel = cyng::value_cast<std::string>(reader.get("channel"), "");
			CYNG_LOG_TRACE(logger_, "ws.read - subscribe channel [" << channel << "]");
			if (boost::algorithm::starts_with(channel, "config.device"))
			{
				subscribe_devices(channel, cyng::value_cast(frame.at(0), boost::uuids::nil_uuid()));
			}
			else if (boost::algorithm::starts_with(channel, "status.session"))
			{
				subscribe_sessions(channel, cyng::value_cast(frame.at(0), boost::uuids::nil_uuid()));
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
				update_sys_cpu_usage_total(channel, cyng::value_cast(frame.at(0), boost::uuids::nil_uuid()));
			}
			else if (boost::algorithm::starts_with(channel, "sys.mem.virtual.total"))
			{
				update_sys_mem_virtual_total(channel, cyng::value_cast(frame.at(0), boost::uuids::nil_uuid()));
			}
			else if (boost::algorithm::starts_with(channel, "sys.mem.virtual.used"))
			{
				update_sys_mem_virtual_used(channel, cyng::value_cast(frame.at(0), boost::uuids::nil_uuid()));
			}
			else
			{
				CYNG_LOG_WARNING(logger_, "ws.read - unknown update channel [" << channel << "]");
			}
		}
		else
		{
			CYNG_LOG_WARNING(logger_, "ws.read - unknown command " << cmd);

		}
	}

	void cluster::update_sys_cpu_usage_total(std::string const& channel, boost::uuids::uuid tag)
	{
		//
		//	Total CPU load of the system in %
		//
		server_.send_ws(tag, cyng::generate_invoke("ws.send.json", cyng::tuple_factory(
			cyng::param_factory("cmd", std::string("update")),
			cyng::param_factory("channel", channel),
			cyng::param_factory("value", cyng::sys::get_total_cpu_load()))));
	}

	void cluster::update_sys_mem_virtual_total(std::string const& channel, boost::uuids::uuid tag)
	{
		//
		//	total virtual memory in bytes
		//
		server_.send_ws(tag, cyng::generate_invoke("ws.send.json", cyng::tuple_factory(
			cyng::param_factory("cmd", std::string("update")),
			cyng::param_factory("channel", channel),
			cyng::param_factory("value", cyng::sys::get_total_virtual_memory()))));
	}

	void cluster::update_sys_mem_virtual_used(std::string const& channel, boost::uuids::uuid tag)
	{
		//
		//	used virtual memory in bytes
		//
		server_.send_ws(tag, cyng::generate_invoke("ws.send.json", cyng::tuple_factory(
			cyng::param_factory("cmd", std::string("update")),
			cyng::param_factory("channel", channel),
			cyng::param_factory("value", cyng::sys::get_used_virtual_memory()))));
	}

	void cluster::subscribe_devices(std::string const& channel, boost::uuids::uuid tag)
	{
		//
		//	send initial data set of device table
		//
		cache_.access([&](cyng::store::table const* tbl) {
			const auto counter = tbl->loop([&](cyng::table::record const& rec) -> bool {

				CYNG_LOG_TRACE(logger_, "ws.read - insert device " << cyng::io::to_str(rec.key()));

				//	{ "pk", "name", "pwd", "number", "descr", "id", "vFirmware", "enabled", "creationTime", "query" },


				server_.send_ws(tag, cyng::generate_invoke("ws.send.json", cyng::tuple_factory(
					cyng::param_factory("cmd", std::string("insert")),
					cyng::param_factory("channel", channel),
					cyng::param_factory("rec", rec.convert()))));

				//	continue
				return true;
			});
			CYNG_LOG_INFO(logger_, counter << "TDevice records sent");

		}, cyng::store::read_access("TDevice"));
	}

	void cluster::subscribe_sessions(std::string const& channel, boost::uuids::uuid tag)
	{
		//
		//	send initial data set of session table
		//
		cache_.access([&](cyng::store::table const* tbl) {
			const auto counter = tbl->loop([&](cyng::table::record const& rec) -> bool {

				CYNG_LOG_TRACE(logger_, "ws.read - insert session " << cyng::io::to_str(rec.key()));

				server_.send_ws(tag, cyng::generate_invoke("ws.send.json", cyng::tuple_factory(
					cyng::param_factory("cmd", std::string("insert")),
					cyng::param_factory("channel", channel),
					cyng::param_factory("rec", rec.convert()))));

				//	continue
				return true;
			});
			CYNG_LOG_INFO(logger_, counter << "*Session records sent");

		}, cyng::store::read_access("*Session"));
	}


	void cluster::create_cache()
	{
		CYNG_LOG_TRACE(logger_, "create cache tables");

		cache_.create_table(cyng::table::make_meta_table<1, 9>("TDevice",
			{ "pk", "name", "pwd", "msisdn", "descr", "id", "vFirmware", "enabled", "creationTime", "query" },
			{ cyng::TC_UUID, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_BOOL, cyng::TC_TIME_POINT, cyng::TC_UINT32 },
			{ 36, 128, 16, 128, 512, 64, 64, 0, 0, 0 }));

		cache_.create_table(cyng::table::make_meta_table<1, 13>("TGateway", { "pk"	//	primary key
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
			{ 36, 23, 64, 64, 0, 8, 18, 18, 32, 32, 16, 32, 32 }));

		if (!cache_.create_table(cyng::table::make_meta_table<1, 8>("*Session", { "tag"	//	client session - primary key [uuid]
			, "local"	//	[object] local peer object (hold session reference)
			, "remote"	//	[object] remote peer object (if session connected)
			, "peer"	//	[uuid] remote peer
			, "device"	//	[uuid] - owner of the session
			, "name"	//	[string] - account
			, "source"	//	[uint32] - ipt source id (unique)
			, "login-time"	//	last login time
			, "rtag"	//	[uuid] client session if connected
			},
			{ cyng::TC_UUID, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_UUID, cyng::TC_UUID, cyng::TC_STRING, cyng::TC_UINT32, cyng::TC_TIME_POINT, cyng::TC_UUID },
			{ 36, 0, 0, 36, 36, 64, 0, 0, 36 })))
		{
			CYNG_LOG_FATAL(logger_, "cannot create table *Session");
		}

	}

}
