/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "cluster.h"
#include "system.h"
#include "../../../dash_shared/src/sync_db.h"
#include <smf/cluster/generator.h>

#include <cyng/async/task/task_builder.hpp>
#include <cyng/io/serializer.h>
#include <cyng/vm/generator.h>
#include <cyng/vm/domain/log_domain.h>
#include <cyng/table/meta.hpp>
#include <cyng/tuple_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/factory/set_factory.h>
#include <cyng/dom/reader.h>
#include <cyng/sys/memory.h>
#include <cyng/json.h>
#include <cyng/xml.h>
#include <cyng/parser/chrono_parser.h>
#include <cyng/io/io_chrono.hpp>

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
		, std::set<boost::asio::ip::address> const& blacklist)
	: base_(*btp)
		, rgn_()
		, bus_(bus_factory(btp->mux_, logger, rgn_(), btp->get_id()))
		, logger_(logger)
		, config_(cfg_cls)
		, cache_()
		, server_(logger, btp->mux_.get_io_service(), ep, doc_root, blacklist, bus_, cache_)
		, dispatcher_(logger, server_.get_cm())
		, sys_tsk_(cyng::async::NO_TASK)
		, form_data_()
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");

		//
		//	init cache
		//
		create_cache(logger_, cache_);
		//subscribe_cache();

		//
		//	subscribe to database
		//
		dispatcher_.subscribe(cache_);

		//
		//	implement request handler
		//
		bus_->vm_.register_function("bus.reconfigure", 1, std::bind(&cluster::reconfigure, this, std::placeholders::_1));

		//
		//	data handling
		//
		bus_->vm_.register_function("db.trx.start", 0, [this](cyng::context& ctx) {
			CYNG_LOG_TRACE(logger_, "db.trx.start");
		});
		bus_->vm_.register_function("db.trx.commit", 0, [this](cyng::context& ctx) {
			CYNG_LOG_TRACE(logger_, "db.trx.commit");
		});
		bus_->vm_.register_function("bus.res.subscribe", 6, std::bind(&cluster::res_subscribe, this, std::placeholders::_1));
		bus_->vm_.register_function("db.res.insert", 4, std::bind(&cluster::db_res_insert, this, std::placeholders::_1));
		bus_->vm_.register_function("db.res.remove", 2, std::bind(&cluster::db_res_remove, this, std::placeholders::_1));
		bus_->vm_.register_function("db.res.modify.by.attr", 3, std::bind(&cluster::db_res_modify_by_attr, this, std::placeholders::_1));
		bus_->vm_.register_function("db.res.modify.by.param", 3, std::bind(&cluster::db_res_modify_by_param, this, std::placeholders::_1));
		bus_->vm_.register_function("db.req.insert", 4, std::bind(&cluster::db_req_insert, this, std::placeholders::_1));
		bus_->vm_.register_function("db.req.remove", 3, std::bind(&cluster::db_req_remove, this, std::placeholders::_1));
		bus_->vm_.register_function("db.req.modify.by.param", 5, std::bind(&cluster::db_req_modify_by_param, this, std::placeholders::_1));

		//
		//	input from HTTP session / ws
		//
		bus_->vm_.register_function("ws.read", 3, std::bind(&cluster::ws_read, this, std::placeholders::_1));

		bus_->vm_.register_function("http.upload.start", 2, std::bind(&cluster::http_upload_start, this, std::placeholders::_1));
		bus_->vm_.register_function("http.upload.data", 5, std::bind(&cluster::http_upload_data, this, std::placeholders::_1));
		bus_->vm_.register_function("http.upload.var", 3, std::bind(&cluster::http_upload_var, this, std::placeholders::_1));
		bus_->vm_.register_function("http.upload.progress", 4, std::bind(&cluster::http_upload_progress, this, std::placeholders::_1));
		bus_->vm_.register_function("http.upload.complete", 4, std::bind(&cluster::http_upload_complete, this, std::placeholders::_1));
		//bus_->vm_.register_function("http.send.moved", 2, std::bind(&cluster::http_moved, this, std::placeholders::_1));

		bus_->vm_.register_function("cfg.download.devices", 2, std::bind(&cluster::cfg_download_devices, this, std::placeholders::_1));
		bus_->vm_.register_function("cfg.download.gateways", 2, std::bind(&cluster::cfg_download_gateways, this, std::placeholders::_1));
		bus_->vm_.register_function("cfg.download.messages", 2, std::bind(&cluster::cfg_download_messages, this, std::placeholders::_1));
		bus_->vm_.register_function("cfg.download.LoRa", 2, std::bind(&cluster::cfg_download_LoRa, this, std::placeholders::_1));

		bus_->vm_.register_function("cfg.upload.devices", 2, std::bind(&cluster::cfg_upload_devices, this, std::placeholders::_1));
		bus_->vm_.register_function("cfg.upload.gateways", 2, std::bind(&cluster::cfg_upload_gateways, this, std::placeholders::_1));
		bus_->vm_.register_function("cfg.upload.meter", 2, std::bind(&cluster::cfg_upload_meter, this, std::placeholders::_1));
		
		bus_->vm_.register_function("bus.res.query.srv.visible", 9, std::bind(&cluster::res_query_srv_visible, this, std::placeholders::_1));
		bus_->vm_.register_function("bus.res.query.srv.active", 9, std::bind(&cluster::res_query_srv_active, this, std::placeholders::_1));
		bus_->vm_.register_function("bus.res.query.firmware", 8, std::bind(&cluster::res_query_firmware, this, std::placeholders::_1));

		bus_->vm_.async_run(cyng::generate_invoke("log.msg.info", cyng::invoke("lib.size"), "callbacks registered"));

	}

    void cluster::start_sys_task()
    {
        //
        // start collecting system data
        //
        auto r = cyng::async::start_task_delayed<system>(base_.mux_, std::chrono::seconds(2), logger_, cache_, bus_->vm_.tag());
        if (r.second)
        {
            sys_tsk_ = r.first;
        }
        else
        {
            CYNG_LOG_ERROR(logger_, "could not start system task");
        }
    }

    void cluster::stop_sys_task()
    {
        if (sys_tsk_ != cyng::async::NO_TASK)
        {
            base_.mux_.stop(sys_tsk_);
        }
    }

	cyng::continuation cluster::run()
	{	
		if (!bus_->is_online())
		{
			connect();
		}
		else
		{
			CYNG_LOG_DEBUG(logger_, "cluster bus is online");
		}

		return cyng::continuation::TASK_CONTINUE;
	}

	void cluster::stop()
	{
        //
        //  stop collecting system data
        //
        stop_sys_task();

		//
		//	stop server
		//
		server_.close();

		//
		//	sign off from cluster
		//
        bus_->stop();

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> stopped - leaving cluster");
	}

	//	slot 0
	cyng::continuation cluster::process(cyng::version const& v)
	{
		if (v < cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR))
		{
			CYNG_LOG_WARNING(logger_, "insufficient cluster protocol version: "	<< v);
		}

        //
        //  collect system data
        //
        start_sys_task();

		//
		//	start http server
		//
		server_.run();

		//
		//	sync tables
		//
		sync_table("TDevice");
		sync_table("TGateway");
		sync_table("TLoRaDevice");
		sync_table("TMeter");
		sync_table("_Session");
		sync_table("_Target");
		sync_table("_Connection");
		sync_table("_Cluster");
		sync_table("_Config");
		cache_.clear("_SysMsg", bus_->vm_.tag());
		sync_table("_SysMsg");

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
        //  stop collecting system data
        //
        stop_sys_task();

		//
		//	tell server to discard all data
		//
		clear_cache(cache_, bus_->vm_.tag());

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
		BOOST_ASSERT_MSG(!bus_->vm_.is_halted(), "cluster bus is halted");
		bus_->vm_.async_run(bus_req_login(config_.get().host_
			, config_.get().service_
			, config_.get().account_
			, config_.get().pwd_
			, config_.get().auto_config_
			, config_.get().group_
			, "dash"));

		CYNG_LOG_INFO(logger_, "cluster login request is sent to " 
			<< config_.get().host_
			<< ':'
			<< config_.get().service_);

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
		if (config_.next())
		{
			CYNG_LOG_INFO(logger_, "switch to redundancy "
				<< config_.get().host_
				<< ':'
				<< config_.get().service_);
		}
		else
		{
			CYNG_LOG_ERROR(logger_, "cluster login failed - no other redundancy available");
		}

		//
		//	trigger reconnect 
		//
		CYNG_LOG_INFO(logger_, "reconnect to cluster in "
			<< config_.get().monitor_.count()
			<< " seconds");
		base_.suspend(config_.get().monitor_);

	}

	void cluster::res_subscribe(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		//
		//	examples:
		//	[TDevice,[911fc4a1-8d9b-4d18-97f7-84a1cd576139],[00000006,2018-02-04 15:31:37.00000000,true,v88,ID,comment #88,1088,secret,device-88],88,dfa6b9a1-4170-41bd-8945-80b936059231,1]
		//	[TGateway,[dca135f3-ff2b-4bf7-8371-a9904c74522b],[operator,operator,mbus,pwd,user,00:01:02:03:04:06,00:01:02:03:04:05,factory-nr,VSES-1.13_1133000038X025d,2018-06-05 16:01:06.29959300,EMH-VSES,EMH,05000000000000],0,e197fc51-0f13-4643-968d-8d0332bae068,1]
		//	[*SysMsg,[14],[cluster member dash:63efc328-218a-4635-a582-1cb4ddc7af25 closed,4,2018-06-05 16:17:50.88472100],1,e197fc51-0f13-4643-968d-8d0332bae068,1]
		//
		//	* table name
		//	* record key
		//	* record data
		//	* generation
		//	* origin session id
		//	* optional task id
		//	
		//CYNG_LOG_TRACE(logger_, "res.subscribe - " << cyng::io::to_str(frame));

		auto tpl = cyng::tuple_cast<
			std::string,			//	[0] table name
			cyng::table::key_type,	//	[1] table key
			cyng::table::data_type,	//	[2] record
			std::uint64_t,			//	[3] generation
			boost::uuids::uuid,		//	[4] origin session id
			std::size_t				//	[5] optional task id
		>(frame);

		CYNG_LOG_TRACE(logger_, "res.subscribe " 
			<< std::get<0>(tpl)		// table name
			<< " - "
			<< cyng::io::to_str(std::get<1>(tpl)));

		//
		//	reorder vectors
		//
		std::reverse(std::get<1>(tpl).begin(), std::get<1>(tpl).end());
		std::reverse(std::get<2>(tpl).begin(), std::get<2>(tpl).end());

		//
		//	Boost gateway records with additional data from the TDevice table
		//
		if (boost::algorithm::equals(std::get<0>(tpl), "TGateway"))
		{
			//
			//	Additional values for TGateway
			//
			cache_.access([&](const cyng::store::table* tbl_dev, const cyng::store::table* tbl_ses) {

				//
				//	Gateway and Device table share the same table key
				//	look for a session of this device
				//
				auto dev_rec = tbl_dev->lookup(std::get<1>(tpl));
				auto ses_rec = tbl_ses->find_first(cyng::param_t("device", std::get<1>(tpl).at(0)));

				//
				//	set device name
				//	set model
				//	set firmware
				//	set online state
				//
				if (!dev_rec.empty())
				{
					std::get<2>(tpl).push_back(dev_rec["name"]);
					std::get<2>(tpl).push_back(dev_rec["serverId"]);
					std::get<2>(tpl).push_back(dev_rec["vFirmware"]);
					std::get<2>(tpl).push_back(cyng::make_object(!ses_rec.empty()));
				}
				else
				{
					CYNG_LOG_WARNING(logger_, "res.subscribe - gateway"
						<< cyng::io::to_str(std::get<1>(tpl))
						<< " has no associated device");
				}
			}	, cyng::store::read_access("TDevice")
				, cyng::store::read_access("_Session"));

		}

		if (!cache_.insert(std::get<0>(tpl)	//	table name
			, std::get<1>(tpl)	//	table key
			, std::get<2>(tpl)	//	table data
			, std::get<3>(tpl)	//	generation
			, std::get<4>(tpl)))
		{
			CYNG_LOG_WARNING(logger_, "res.subscribe failed "
				<< std::get<0>(tpl)		// table name
				<< " - "
				<< cyng::io::to_str(std::get<1>(tpl)));
		}
		else
		{
			if (boost::algorithm::equals(std::get<0>(tpl), "_Session"))
			{
				//
				//	mark gateways as online
				//
				cache_.access([&](cyng::store::table* tbl_gw, const cyng::store::table* tbl_ses) {

					//
					//	[*Session,[2ce46726-6bca-44b6-84ed-0efccb67774f],[00000000-0000-0000-0000-000000000000,2018-03-12 17:56:27.10338240,f51f2ae7,data-store,eaec7649-80d5-4b71-8450-3ee2c7ef4917,94aa40f9-70e8-4c13-987e-3ed542ecf7ab,null,session],1]
					//	Gateway and Device table share the same table key
					//
					auto rec = tbl_ses->lookup(std::get<1>(tpl));
					if (!rec.empty())
					{
						//	set online state
						tbl_gw->modify(cyng::table::key_generator(rec["device"]), cyng::param_factory("online", true), std::get<4>(tpl));
					}
				}	, cyng::store::write_access("TGateway")
					, cyng::store::read_access("_Session"));
			}
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

		if (boost::algorithm::equals(std::get<0>(tpl), "TGateway"))
		{
			cache_.access([&](cyng::store::table* tbl_gw, const cyng::store::table* tbl_dev, const cyng::store::table* tbl_ses) {

				//
				//	search session with this device/GW tag
				//
				auto dev_rec = tbl_dev->lookup(std::get<1>(tpl));

				//
				//	set device name
				//	set model
				//	set firmware
				//	set online state
				//
				if (!dev_rec.empty())
				{
					std::get<2>(tpl).push_back(dev_rec["name"]);
					std::get<2>(tpl).push_back(dev_rec["serverId"]);
					std::get<2>(tpl).push_back(dev_rec["vFirmware"]);

					//
					//	get online state
					//
					auto ses_rec = tbl_ses->find_first(cyng::param_t("device", std::get<1>(tpl).at(0)));
					std::get<2>(tpl).push_back(cyng::make_object(!ses_rec.empty()));	//	add on/offline flag

					if (!tbl_gw->insert(std::get<1>(tpl), std::get<2>(tpl), std::get<3>(tpl), std::get<4>(tpl)))
					{

						CYNG_LOG_WARNING(logger_, "db.req.insert failed "
							<< std::get<0>(tpl)		// table name
							<< " - "
							<< cyng::io::to_str(std::get<1>(tpl)));

					}
				}
				else {
					CYNG_LOG_WARNING(logger_, "gateway "
						<< cyng::io::to_str(std::get<1>(tpl))
						<< " has no associated device" );
				}
			}	, cyng::store::write_access("TGateway")
				, cyng::store::read_access("TDevice")
				, cyng::store::read_access("_Session"));
		}
		else if (!cache_.insert(std::get<0>(tpl)
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
		else 
		{
			if (boost::algorithm::equals(std::get<0>(tpl), "_Session"))
			{
				cache_.access([&](cyng::store::table* tbl_gw, const cyng::store::table* tbl_ses) {

					//
					//	[*Session,[2ce46726-6bca-44b6-84ed-0efccb67774f],[00000000-0000-0000-0000-000000000000,2018-03-12 17:56:27.10338240,f51f2ae7,data-store,eaec7649-80d5-4b71-8450-3ee2c7ef4917,94aa40f9-70e8-4c13-987e-3ed542ecf7ab,null,session],1]
					//	Gateway and Device table share the same table key
					//
					auto rec = tbl_ses->lookup(std::get<1>(tpl));
					if (!rec.empty())
					{
						//	set online state
						tbl_gw->modify(cyng::table::key_generator(rec["device"]), cyng::param_factory("online", true), std::get<4>(tpl));
					}
				}	, cyng::store::write_access("TGateway")
					, cyng::store::read_access("_Session"));
			}
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

		//
		//	we have to query session data before session will be removed
		//
		if (boost::algorithm::equals(std::get<0>(tpl), "_Session"))
		{
			cache_.access([&](cyng::store::table* tbl_gw, const cyng::store::table* tbl_ses) {

				//
				//	Gateway and Device table share the same table key
				//
				auto rec = tbl_ses->lookup(std::get<1>(tpl));
				if (!rec.empty())
				{
					//	set online state
					tbl_gw->modify(cyng::table::key_generator(rec["device"]), cyng::param_factory("online", false), std::get<2>(tpl));
				}
			}	, cyng::store::write_access("TGateway")
				, cyng::store::read_access("_Session"));
		}

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

	void cluster::read_device_configuration_3_2(cyng::context& ctx, pugi::xml_document const& doc)
	{
		//
		//	example line:
		//	<device auto_update="false" description="BF Haus 18" enabled="true" expires="never" gid="2" latitude="0.000000" limit="no" locked="false" longitude="0.000000" monitor="00:00:12" name="a00153B018EEFen" number="20113083736" oid="2" pwd="aen_ict_1111" query="6" redirect="" watchdog="00:12:00">cc6a0256-5689-4804-a5fc-bf3599fac23a</device>
		//

		std::size_t counter{ 0 };
		pugi::xpath_node_set data = doc.select_nodes("config/units/device");
		for (pugi::xpath_node_set::const_iterator it = data.begin(); it != data.end(); ++it)
		{
			counter++;
			pugi::xml_node node = it->node();
			const std::string pk = node.child_value();
			
			const std::string name = node.attribute("name").value();

			CYNG_LOG_TRACE(logger_, "session "
				<< ctx.tag()
				<< " - insert device "
				<< pk
				<< " - "
				<< name);

			//
			//	forward to process bus task
			//
			ctx.attach(bus_req_db_insert("TDevice"
				//	generate new key
				, cyng::table::key_generator(boost::uuids::string_generator()(pk))
				//	build data vector
				, cyng::table::data_generator(name
					, node.attribute("pwd").value()
					, node.attribute("number").value()
					, node.attribute("description").value()
					, std::string("")	//	model
					, std::string("")	//	version
					, node.attribute("enabled").as_bool()
					, std::chrono::system_clock::now()
					, node.attribute("query").as_uint())
				, 0
				, ctx.tag()));
		}

		std::stringstream ss;
		ss
			<< counter
			<< " device records (v3.2) uploaded"
			;
		ctx.attach(bus_insert_msg(cyng::logging::severity::LEVEL_INFO, ss.str()));
		
	}

	void cluster::read_device_configuration_4_0(cyng::context& ctx, pugi::xml_document const& doc)
	{
		std::size_t counter{ 0 };
		pugi::xpath_node_set data = doc.select_nodes("configuration/records/device");
		for (pugi::xpath_node_set::const_iterator it = data.begin(); it != data.end(); ++it)
		{
			counter++;
			pugi::xpath_node node = *it;
			const std::string name = node.node().child_value("name");

			CYNG_LOG_TRACE(logger_, "session "
				<< ctx.tag()
				<< " - insert device "
				<< name);

			//auto obj = cyng::traverse(node.node());
			//CYY_LOG_TRACE(logger_, "session "
			//	<< cyy::uuid_short_format(vm_.tag())
			//	<< " - "
			//	<< cyy::to_literal(obj, cyx::io::custom));

			//	("device":
			//	%(("age":"2016.11.24 14:48:09.00000000"ts),
			//	("config":%(("pwd":"LUsregnP"),("scheme":"plain"))),
			//	("descr":"-[0008]-"),
			//	("enabled":true),
			//	("id":""),
			//	("name":"oJtrQQfSCRrF"),
			//	("number":"609971066"),
			//	("revision":0u32),
			//	("source":"d365c4c7-e89d-4187-86ae-a143d54f4cfe"uuid),
			//	("tag":"e29938f5-71a9-4860-97e2-0a78097a6858"uuid),
			//	("ver":"")))

			//cyy::object_reader reader(obj);

			//
			//	collect data
			//	forward to process bus task
			//
			//insert("TDevice"
			//	, cyy::store::key_generator(reader["device"].get_uuid("tag"))
			//	, cyy::store::data_generator(reader["device"].get_object("revision")
			//		, reader["device"].get_object("name")
			//		, reader["device"].get_object("number")
			//		, reader["device"].get_object("descr")
			//		, reader["device"].get_object("id")	//	device ID
			//		, reader["device"].get_object("ver")	//	firmware 
			//		, reader["device"].get_object("enabled")
			//		, reader["device"].get_object("age")

			//		//	configuration as parameter map
			//		, reader["device"].get_object("config")
			//		, reader["device"].get_object("source")));

			//ctx.attach(bus_req_db_insert("TDevice"
			//	//	generate new key
			//	, cyng::table::key_generator(boost::uuids::string_generator()(pk))
			//	//	build data vector
			//	, cyng::table::data_generator(name
			//		, node.attribute("pwd").value()
			//		, node.attribute("number").value()
			//		, node.attribute("description").value()
			//		, std::string("")	//	model
			//		, std::string("")	//	version
			//		, node.attribute("enabled").as_bool()
			//		, std::chrono::system_clock::now()
			//		, node.attribute("query").as_uint())
			//	, 0
			//	, ctx.tag()));
		}

		std::stringstream ss;
		ss
			<< counter
			<< " device records (v4.0) uploaded"
			;
		ctx.attach(bus_insert_msg(cyng::logging::severity::LEVEL_INFO, ss.str()));

	}

	void cluster::read_device_configuration_5_x(cyng::context& ctx, pugi::xml_document const& doc)
	{
		std::size_t counter{ 0 };
		pugi::xpath_node_set data = doc.select_nodes("TDevice/record");
		auto meta = cache_.meta("TDevice");
		for (pugi::xpath_node_set::const_iterator it = data.begin(); it != data.end(); ++it)
		{
			counter++;
			pugi::xml_node node = it->node();

			auto rec = cyng::xml::read(node, meta);

			CYNG_LOG_TRACE(logger_, "session "
				<< ctx.tag()
				<< " - insert device #"
				<< counter
				<< " "
				<< cyng::value_cast(rec["pk"], boost::uuids::nil_uuid())
				<< " - "
				<< cyng::value_cast<std::string>(rec["name"], ""));

			ctx.attach(bus_req_db_insert("TDevice", rec.key(), rec.data(), rec.get_generation(), ctx.tag()));
		}
	}

	void cluster::http_upload_start(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,	//	[0] session tag
			std::uint32_t,		//	[1] HTTP version
			std::string,		//	[2] target
			std::uint32_t		//	[3] payload size
		>(frame);

		CYNG_LOG_TRACE(logger_, "http.upload.start - "
			<< std::get<0>(tpl)
			<< ", "
			<< std::get<2>(tpl)
			<< ", payload size: "
			<< std::get<3>(tpl));

		form_data_.erase(std::get<0>(tpl));
		auto r = form_data_.emplace(std::get<0>(tpl), std::map<std::string, std::string>());
		if (r.second) {
			r.first->second.emplace("target", std::get<2>(tpl));
		}

	}

	void cluster::http_upload_data(cyng::context& ctx)
	{
		//	http.upload.data - [ecf139d4-184e-441d-a857-9d02eb58148b,devConf_0,device_localhost.xml,text/xml,3C3F786D6C2....]
		//
		//	* session tag
		//	* variable name
		//	* file name
		//	* mime type
		//	* content
		//	
		const cyng::vector_t frame = ctx.get_frame();
		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,	//	[0] session tag
			std::string,		//	[1] variable name
			std::string,		//	[2] file name
			std::string		//	[3] mime type
			//cyng::buffer_t		//	[4] data (skipped to save memory)
		>(frame);

		CYNG_LOG_TRACE(logger_, "http.upload.data - " 
			<< cyng::value_cast<std::string>(frame.at(2), "no file name specified")
			<< " - "
			<< cyng::value_cast<std::string>(frame.at(3), "no mime type specified"));

		auto pos = form_data_.find(std::get<0>(tpl));
		if (pos != form_data_.end()) {
			CYNG_LOG_TRACE(logger_, "http.upload.data - "
				<< std::get<0>(tpl)
				<< ", "
				<< std::get<1>(tpl)
				<< ", "
				<< std::get<2>(tpl)
				<< ", mime type: "
				<< std::get<3>(tpl));

			//
			//	write temporary file
			//
			auto p = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path("smf-dash-%%%%-%%%%-%%%%-%%%%.tmp");
			auto ptr = cyng::object_cast<cyng::buffer_t>(frame.at(4));
			std::ofstream of(p.string());
			if (of.is_open() && ptr != nullptr) {
				CYNG_LOG_DEBUG(logger_, "http.upload.data - write "
					<< ptr->size()
					<< " bytes into file "
					<< p);
				of.write(ptr->data(), ptr->size());
				of.close();
			}
			else {
				CYNG_LOG_FATAL(logger_, "http.upload.data - cannot open "
					<< p);
			}
			pos->second.emplace(std::get<1>(tpl), p.string());
			pos->second.emplace(std::get<2>(tpl), std::get<3>(tpl));
		}
		else {
			CYNG_LOG_WARNING(logger_, "http.upload.data - session "
				<< std::get<0>(tpl)
				<< " not found");
		}
	}

	void cluster::http_upload_var(cyng::context& ctx)
	{
		//	[5910f652-95ce-4d94-b60f-6ff4a26730ba,smf-upload-config-device-version,v5.0]
		const cyng::vector_t frame = ctx.get_frame();
		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,	//	[0] session tag
			std::string,		//	[1] variable name
			std::string			//	[2] value
		>(frame);


		auto pos = form_data_.find(std::get<0>(tpl));
		if (pos != form_data_.end()) {
			CYNG_LOG_TRACE(logger_, "http.upload.var - "
				<< std::get<0>(tpl)
				<< " - "
				<< std::get<1>(tpl)
				<< " = "
				<< std::get<2>(tpl));
			pos->second.emplace(std::get<1>(tpl), std::get<2>(tpl));
		}
		else {
			CYNG_LOG_WARNING(logger_, "http.upload.var - session "
				<< std::get<0>(tpl)
				<< " not found");
		}

	}

	void cluster::http_upload_progress(cyng::context& ctx)
	{
		//	[ecf139d4-184e-441d-a857-9d02eb58148b,0000a84c,0000a84c,00000064]
		//
		//	* session tag
		//	* upload size
		//	* content size
		//	* progress in %
		//	
#ifdef __DEBUG
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "http.upload.progress - " << cyng::value_cast<std::uint32_t>(frame.at(3), 0) << "%");
#endif

	}

	void cluster::http_upload_complete(cyng::context& ctx)
	{
		//	[300dc453-d52d-40c0-843c-e8fd7495ff0e,true,0000a84a,/upload/config/device/]
		//
		//	* session tag
		//	* success flag
		//	* total size in bytes
		//	* target path
		//
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "http.upload.complete - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,	//	[0] session tag
			bool,				//	[1] success
			std::uint32_t,		//	[2] size in bytes
			std::string			//	[3] target
		>(frame);

		//
		//	post a system message
		//
		auto pos = form_data_.find(std::get<0>(tpl));
		auto count = (pos != form_data_.end())
			? pos->second.size()
			: 0
			;
		std::stringstream ss;
		ss
			<< "upload/download "
			<< std::get<2>(tpl)
			<< " bytes to "
			<< std::get<3>(tpl)
			<< " with "
			<< count
			<< " variable(s)"
			;
		ctx.attach(bus_insert_msg((std::get<1>(tpl) ? cyng::logging::severity::LEVEL_TRACE : cyng::logging::severity::LEVEL_WARNING), ss.str()));

		//
		//	start procedure
		//
		bool found = false;
		if (pos != form_data_.end()) {
			auto idx = pos->second.find("smf-procedure");
			if (idx != pos->second.end()) {

				CYNG_LOG_INFO(logger_, "run proc "
					<< idx->second
					<< ":"
					<< std::get<0>(tpl));

				cyng::param_map_t params;
				for (auto const& v : pos->second) {
					params.insert(cyng::param_factory(v.first, v.second));
				}
				bus_->vm_.async_run(cyng::generate_invoke(idx->second, std::get<0>(tpl), params));
				found = true;
			}
		}

		//
		//	cleanup form data
		//
		form_data_.erase(std::get<0>(tpl));

		//
		//	consider to send a 302 - Object moved response
		//
		if (!found) {
			server_.send_moved(std::get<0>(tpl), std::get<3>(tpl));
		}
	}

	void cluster::cfg_download_devices(cyng::context& ctx)
	{
		//
		//	example
		//	[%(("smf-procedure":cfg.download.devices),("smf-upload-config-device-version":v5.0),("target":/api/download/config/device))]
		//
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "cfg.download.devices - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,	//	[0] session tag
			cyng::param_map_t	//	[1] variables
		>(frame);

		trigger_download(std::get<0>(tpl), "TDevice", "device.xml");

	}

	void cluster::cfg_download_gateways(cyng::context& ctx)
	{
		//
		//	example
		//	[%(("smf-procedure":cfg.download.devices),("smf-upload-config-device-version":v5.0),("target":/api/download/config/device))]
		//
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "cfg.download.gateways - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,	//	[0] session tag
			cyng::param_map_t	//	[1] variables
		>(frame);

		trigger_download(std::get<0>(tpl), "TGateway", "gateway.xml");

	}
	
	void cluster::cfg_download_messages(cyng::context& ctx)
	{
		//
		//	example
		//	[%(("smf-procedure":cfg.download.devices),("smf-upload-config-device-version":v5.0),("target":/api/download/config/device))]
		//
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "cfg.download.messages - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,	//	[0] session tag
			cyng::param_map_t	//	[1] variables
		>(frame);

		trigger_download(std::get<0>(tpl), "_SysMsg", "messages.xml");

	}

	void cluster::cfg_download_LoRa(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "cfg.download.LoRa - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,	//	[0] session tag
			cyng::param_map_t	//	[1] variables
		>(frame);

		trigger_download(std::get<0>(tpl), "TLoRaDevice", "LoRa.xml");
	}

	void cluster::cfg_upload_devices(cyng::context& ctx)
	{
		//	
		//	[181f86c7-23e5-4e01-a4d9-f6c9855962bf,
		//	%(
		//		("devConf_0":text/xml),
		//		("devices.xml":C:\\Users\\Pyrx\\AppData\\Local\\Temp\\smf-dash-ea2e-1fe1-6628-93ff.tmp),
		//		("smf-procedure":cfg.upload.devices),
		//		("smf-upload-config-device-version":v5.0),
		//		("target":/upload/config/device/)
		//	)]
		//
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "cfg.upload.device - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,	//	[0] session tag
			cyng::param_map_t	//	[1] variables
		>(frame);

		auto file_name = cyng::value_cast<std::string>(cyng::find(std::get<1>(tpl), "dev-conf-0"), "");
		auto version = cyng::value_cast<std::string>(cyng::find(std::get<1>(tpl), "smf-upload-config-device-version"), "v0.5");

		//
		//	get pointer to XML data
		//
		pugi::xml_document doc;
		const pugi::xml_parse_result result = doc.load_file(file_name.c_str());
		if (result) {

			if (boost::algorithm::equals(version, "v3.2")) {
				read_device_configuration_3_2(ctx, doc);
			}
			else {
				read_device_configuration_5_x(ctx, doc);
			}
		}
		else {
			std::stringstream ss;
			ss
				<< "XML ["
				<< file_name
				<< "] parsed with errors: ["
				<< result.description()
				<< "]\n"
				<< "Error offset: "
				<< result.offset
				;

			ctx.attach(bus_insert_msg(cyng::logging::severity::LEVEL_WARNING, ss.str()));
		}


	}

	void cluster::cfg_upload_gateways(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "cfg.upload.gateways - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,	//	[0] session tag
			cyng::param_map_t	//	[1] variables
		>(frame);

		auto file_name = cyng::value_cast<std::string>(cyng::find(std::get<1>(tpl), "gw-conf-0"), "");

		//
		//	get pointer to XML data
		//
		pugi::xml_document doc;
		const pugi::xml_parse_result result = doc.load_file(file_name.c_str());
		if (result) {

			std::size_t counter{ 0 };
			pugi::xpath_node_set data = doc.select_nodes("TGateway/record");
			auto meta = cache_.meta("TGateway");
			for (pugi::xpath_node_set::const_iterator it = data.begin(); it != data.end(); ++it)
			{
				counter++;
				pugi::xml_node node = it->node();

				auto rec = cyng::xml::read(node, meta);

				CYNG_LOG_TRACE(logger_, "session "
					<< ctx.tag()
					<< " - insert gateway #"
					<< counter
					<< " "
					<< cyng::value_cast(rec["pk"], boost::uuids::nil_uuid())
					<< " - "
					<< cyng::value_cast<std::string>(rec["name"], ""));

				ctx.attach(bus_req_db_insert("TGateway", rec.key(), rec.data(), rec.get_generation(), ctx.tag()));
			}

		}
		else {
			std::stringstream ss;
			ss
				<< "XML ["
				<< file_name
				<< "] parsed with errors: ["
				<< result.description()
				<< "]\n"
				<< "Error offset: "
				<< result.offset
				;

			ctx.attach(bus_insert_msg(cyng::logging::severity::LEVEL_WARNING, ss.str()));
		}
	}

	void cluster::cfg_upload_meter(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "cfg.upload.meter - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,	//	[0] session tag
			cyng::param_map_t	//	[1] variables
		>(frame);
	}

	void cluster::res_query_srv_visible(cyng::context& ctx)
	{
		//	[d6529e83-fa7f-469d-8681-798501842252,1,637c19e7-b20d-462f-afbf-81fdbb064c82,0005,00:15:3b:02:29:7e,01-e61e-29436587-bf-03,---,2018-08-30 09:37:13.00000000]
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "bus.res.query.srv.visible - " << cyng::io::to_str(frame));

		auto const data = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] source
			std::uint64_t,			//	[1] sequence
			boost::uuids::uuid,		//	[2] websocket tag
			std::uint16_t,			//	[3] list number
			std::string,			//	[4] server id (key)
			std::string,			//	[5] meter
			std::string,			//	[6] device class
			std::chrono::system_clock::time_point,	//	[7] last status update
			std::uint32_t			//	[8] server type
		>(frame);

		//
		//	reformatting timestamp
		//
		auto stp = (cyng::chrono::same_day(std::get<7>(data), std::chrono::system_clock::now()))
			? cyng::ts_to_str(cyng::chrono::duration_of_day(std::get<7>(data)))
			: cyng::date_to_str(std::get<7>(data))
			;


		auto tpl = cyng::tuple_factory(
			cyng::param_factory("cmd", std::string("append")),
			cyng::param_factory("channel", "status.gateway.visible"),
			cyng::param_factory("rec", cyng::tuple_factory(
				cyng::param_factory("nr", std::get<3>(data)),
				cyng::param_factory("srv", std::get<4>(data)),
				cyng::param_factory("meter", std::get<5>(data)),
				cyng::param_factory("class", std::get<6>(data)),
				cyng::param_factory("tp", stp),
				cyng::param_factory("type", std::get<8>(data))
			)));

		auto msg = cyng::json::to_string(tpl);
		server_.get_cm().ws_msg(std::get<2>(data), msg);

	}

	void cluster::res_query_srv_active(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "bus.res.query.srv.active - " << cyng::io::to_str(frame));

		auto const data = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] source
			std::uint64_t,			//	[1] sequence
			boost::uuids::uuid,		//	[2] websocket tag
			std::uint16_t,			//	[3] list number
			std::string,			//	[4] server id (key)
			std::string,			//	[5] meter
			std::string,			//	[6] device class
			std::chrono::system_clock::time_point,	//	[7] last status update
			std::uint32_t			//	[8] server type
		>(frame);

		//
		//	reformatting timestamp
		//
		auto stp = (cyng::chrono::same_day(std::get<7>(data), std::chrono::system_clock::now()))
			? cyng::ts_to_str(cyng::chrono::duration_of_day(std::get<7>(data)))
			: cyng::date_to_str(std::get<7>(data))
			;

		auto tpl = cyng::tuple_factory(
			cyng::param_factory("cmd", std::string("append")),
			cyng::param_factory("channel", "status.gateway.active"),
			cyng::param_factory("rec", cyng::tuple_factory(
				cyng::param_factory("nr", std::get<3>(data)),
				cyng::param_factory("srv", std::get<4>(data)),
				cyng::param_factory("meter", std::get<5>(data)),
				cyng::param_factory("class", std::get<6>(data)),
				cyng::param_factory("tp", stp),
				cyng::param_factory("type", std::get<8>(data))
			)));

		auto msg = cyng::json::to_string(tpl);
		server_.get_cm().ws_msg(std::get<2>(data), msg);

	}

	void cluster::res_query_firmware(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "bus.res.query.firmware - " << cyng::io::to_str(frame));

		auto const data = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] source
			std::uint64_t,			//	[1] sequence
			boost::uuids::uuid,		//	[2] websocket tag
			std::uint32_t,			//	[3] list number
			std::string,			//	[4] server id (key)
			std::string,			//	[5] section
			std::string,			//	[6] version
			bool					//	[7] active
		>(frame);

		auto tpl = cyng::tuple_factory(
			cyng::param_factory("cmd", std::string("append")),
			cyng::param_factory("channel", "status.gateway.firmware"),
			cyng::param_factory("rec", cyng::tuple_factory(
				cyng::param_factory("nr", std::get<3>(data)),
				cyng::param_factory("srv", std::get<4>(data)),
				cyng::param_factory("section", std::get<5>(data)),
				cyng::param_factory("version", std::get<6>(data)),
				cyng::param_factory("active", std::get<7>(data))
			)));

		auto msg = cyng::json::to_string(tpl);
		server_.get_cm().ws_msg(std::get<2>(data), msg);

	}

	void cluster::trigger_download(boost::uuids::uuid tag, std::string table, std::string filename)
	{
		//
		//	generate XML download file
		//
		pugi::xml_document doc_;
		auto declarationNode = doc_.append_child(pugi::node_declaration);
		declarationNode.append_attribute("version") = "1.0";
		declarationNode.append_attribute("encoding") = "UTF-8";
		declarationNode.append_attribute("standalone") = "yes";

		pugi::xml_node root = doc_.append_child(table.c_str());
		root.append_attribute("xmlns:xsi") = "w3.org/2001/XMLSchema-instance";
		cache_.access([&](cyng::store::table const* tbl) {

			const auto counter = tbl->loop([&](cyng::table::record const& rec) -> bool {

				//
				//	write record
				//
				cyng::xml::write(root.append_child("record"), rec.convert());

				//	continue
				return true;
			});
			BOOST_ASSERT(counter == 0);
			boost::ignore_unused(counter);	//	release version
			CYNG_LOG_INFO(logger_, tbl->size() << ' ' << tbl->meta().get_name() << " records sent");

		}, cyng::store::read_access(table));

		auto out = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path("record-%%%%-%%%%-%%%%-%%%%.xml");
		doc_.save_file(out.c_str(), PUGIXML_TEXT("  "));

		//
		//	trigger download
		//
		server_.trigger_download(tag, out.string(), filename);

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

		//
		//	get session tag of websocket
		//
		auto tag_ws = cyng::value_cast(frame.at(0), boost::uuids::nil_uuid());

		//
		//	reader for JSON data
		//
		auto reader = cyng::make_reader(frame.at(2));
		const std::string cmd = cyng::value_cast<std::string>(reader.get("cmd"), "");

		if (boost::algorithm::equals(cmd, "subscribe"))
		{
			const std::string channel = cyng::value_cast<std::string>(reader.get("channel"), "");
			CYNG_LOG_TRACE(logger_, "ws.read - subscribe channel [" << channel << "]");

			if (boost::algorithm::starts_with(channel, "config.device"))
			{
				subscribe("TDevice", channel, tag_ws);
			}
			else if (boost::algorithm::starts_with(channel, "config.gateway"))
			{
				subscribe("TGateway", channel, tag_ws);
			}
			else if (boost::algorithm::starts_with(channel, "config.lora"))
			{
				subscribe("TLoRaDevice", channel, tag_ws);
			}
			else if (boost::algorithm::starts_with(channel, "config.system"))
			{
				subscribe("_Config", channel, tag_ws);
			}
			else if (boost::algorithm::starts_with(channel, "status.session"))
			{
				subscribe("_Session", channel, tag_ws);
			}
			else if (boost::algorithm::starts_with(channel, "status.target"))
			{
				subscribe("_Target", channel, tag_ws);
			}
			else if (boost::algorithm::starts_with(channel, "status.connection"))
			{
				subscribe("_Connection", channel, tag_ws);
			}
			else if (boost::algorithm::starts_with(channel, "status.cluster"))
			{
				subscribe("_Cluster", channel, tag_ws);
			}
			else if (boost::algorithm::starts_with(channel, "table.device.count"))
			{
				subscribe_table_device_count(channel, tag_ws);
			}
			else if (boost::algorithm::starts_with(channel, "table.gateway.count"))
			{
				subscribe_table_gateway_count(channel, tag_ws);
			}
			else if (boost::algorithm::starts_with(channel, "table.meter.count"))
			{
				subscribe_table_meter_count(channel, tag_ws);
			}
			else if (boost::algorithm::starts_with(channel, "table.session.count"))
			{
				subscribe_table_session_count(channel, tag_ws);
			}
			else if (boost::algorithm::starts_with(channel, "table.target.count"))
			{
				subscribe_table_target_count(channel, tag_ws);
			}
			else if (boost::algorithm::starts_with(channel, "table.connection.count"))
			{
				subscribe_table_connection_count(channel, tag_ws);
			}
			else if (boost::algorithm::starts_with(channel, "table.msg.count"))
			{
				subscribe_table_msg_count(channel, tag_ws);
			}
			else if (boost::algorithm::starts_with(channel, "table.LoRa.count"))
			{
				subscribe_table_LoRa_count(channel, tag_ws);
			}
			else if (boost::algorithm::starts_with(channel, "monitor.msg"))
			{
				subscribe("_SysMsg", channel, tag_ws);
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
			else if (boost::algorithm::starts_with(channel, "config.gateway"))
			{
				//
				//	TGateway key is of type UUID
				//
				try {
					cyng::vector_t vec;
					vec = cyng::value_cast(reader["key"].get("tag"), vec);
					BOOST_ASSERT_MSG(vec.size() == 1, "TGateway key has wrong size");
					const std::string str = cyng::value_cast<std::string>(vec.at(0), "");
					CYNG_LOG_DEBUG(logger_, "TGateway key [" << str << "]");
					auto key = cyng::table::key_generator(boost::uuids::string_generator()(str));

					ctx.attach(bus_req_db_remove("TGateway", key, ctx.tag()));
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
			else if (boost::algorithm::starts_with(channel, "config.gateway"))
			{
				//
				//	TGateway key is of type UUID
				//	all values have the same key
				//
				try {

					cyng::vector_t vec;
					vec = cyng::value_cast(reader["rec"].get("key"), vec);
					BOOST_ASSERT_MSG(vec.size() == 1, "TGateway key has wrong size");

					const std::string str = cyng::value_cast<std::string>(vec.at(0), "");
					CYNG_LOG_DEBUG(logger_, "TGateway key [" << str << "]");
					auto key = cyng::table::key_generator(boost::uuids::string_generator()(str));

					cyng::tuple_t tpl;
					tpl = cyng::value_cast(reader["rec"].get("data"), tpl);
					for (auto p : tpl)
					{
						cyng::param_t param;
						param = cyng::value_cast(p, param);

						if (boost::algorithm::equals(param.first, "serverId")) {
							//
							//	to uppercase
							//
							std::string server_id;
							server_id = cyng::value_cast(param.second, server_id);
							boost::algorithm::to_upper(server_id);
							ctx.attach(bus_req_db_modify("TGateway"
								, key
								, cyng::param_factory("serverId", server_id)
								, 0
								, ctx.tag()));

						}
						else {
							ctx.attach(bus_req_db_modify("TGateway"
								, key
								, cyng::value_cast(p, param)
								, 0
								, ctx.tag()));
						}
					}
				}
				catch (std::exception const& ex) {
					CYNG_LOG_ERROR(logger_, "ws.read - modify channel [" << channel << "] failed");
				}
			}
			else if (boost::algorithm::starts_with(channel, "config.system"))
			{
				//	{"cmd":"modify","channel":"config.system","rec":{"key":{"name":"connection-auto-login"},"data":{"value":true}}}
				const std::string name = cyng::value_cast<std::string>(reader["rec"]["key"].get("name"), "?");
				const cyng::object value = reader["rec"]["data"].get("value");
				ctx.attach(bus_req_db_modify("_Config"
					//	generate new key
					, cyng::table::key_generator(reader["rec"]["key"].get("name"))
					//	build parameter
					, cyng::param_t("value", value)
					, 0
					, ctx.tag()));

				std::stringstream ss;
				ss
					<< "system configuration changed "
					<< name
					<< " ["
					<< value.get_class().type_name()
					<< "] = "
					<< cyng::io::to_str(value)
					;

				ctx.attach(bus_insert_msg(cyng::logging::severity::LEVEL_WARNING, ss.str()));

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
			else
			{
				CYNG_LOG_WARNING(logger_, "ws.read - unknown stop channel [" << channel << "]");
			}
		}
		else if (boost::algorithm::equals(cmd, "reboot"))
		{
			const std::string channel = cyng::value_cast<std::string>(reader.get("channel"), "");
			CYNG_LOG_TRACE(logger_, "ws.read - reboot channel [" << channel << "]");
			if (boost::algorithm::starts_with(channel, "config.gateway"))
			{
				//	{"cmd":"reboot","channel":"config.gateway","key":["dca135f3-ff2b-4bf7-8371-a9904c74522b"]}
				cyng::vector_t vec;
				vec = cyng::value_cast(reader.get("key"), vec);
				CYNG_LOG_INFO(logger_, "reboot gateway " << cyng::io::to_str(vec));

				const std::string str = cyng::value_cast<std::string>(vec.at(0), "");
				CYNG_LOG_DEBUG(logger_, "TGateway key [" << str << "]");
				auto key = cyng::table::key_generator(boost::uuids::string_generator()(str));

				ctx.attach(bus_req_reboot_client(key, ctx.tag()));

			}
			else
			{
				CYNG_LOG_WARNING(logger_, "ws.read - unknown reboot channel [" << channel << "]");
			}
		}
		else if (boost::algorithm::equals(cmd, "query:srv:visible"))
		{
			const std::string channel = cyng::value_cast<std::string>(reader.get("channel"), "");
			CYNG_LOG_TRACE(logger_, "ws.read - query:srv:visible channel [" << channel << "]");
			if (boost::algorithm::starts_with(channel, "config.gateway"))
			{
				cyng::vector_t vec;
				vec = cyng::value_cast(reader.get("key"), vec);
				CYNG_LOG_INFO(logger_, "query:srv:visible " << cyng::io::to_str(vec));

				const std::string str = cyng::value_cast<std::string>(vec.at(0), "");
				CYNG_LOG_DEBUG(logger_, "TGateway key [" << str << "]");
				auto key = cyng::table::key_generator(boost::uuids::string_generator()(str));

				ctx.attach(bus_req_query_srv_visible(key, ctx.tag(), tag_ws));

			}
			else
			{
				CYNG_LOG_WARNING(logger_, "ws.read - unknown query:srv:visible channel [" << channel << "]");
			}
		}
		else if (boost::algorithm::equals(cmd, "query:srv:active"))
		{
			const std::string channel = cyng::value_cast<std::string>(reader.get("channel"), "");
			CYNG_LOG_TRACE(logger_, "ws.read - query:srv:active channel [" << channel << "]");
			if (boost::algorithm::starts_with(channel, "config.gateway"))
			{
				cyng::vector_t vec;
				vec = cyng::value_cast(reader.get("key"), vec);
				CYNG_LOG_INFO(logger_, "query:srv:active " << cyng::io::to_str(vec));

				const std::string str = cyng::value_cast<std::string>(vec.at(0), "");
				CYNG_LOG_DEBUG(logger_, "TGateway key [" << str << "]");
				auto key = cyng::table::key_generator(boost::uuids::string_generator()(str));

				ctx.attach(bus_req_query_srv_active(key, ctx.tag(), tag_ws));

			}
			else
			{
				CYNG_LOG_WARNING(logger_, "ws.read - unknown query:srv:active channel [" << channel << "]");
			}
		}
		else if (boost::algorithm::equals(cmd, "query:firmware"))
		{
			const std::string channel = cyng::value_cast<std::string>(reader.get("channel"), "");
			CYNG_LOG_TRACE(logger_, "ws.read - query:firmware channel [" << channel << "]");
			if (boost::algorithm::starts_with(channel, "config.gateway"))
			{
				cyng::vector_t vec;
				vec = cyng::value_cast(reader.get("key"), vec);
				CYNG_LOG_INFO(logger_, "query:firmware " << cyng::io::to_str(vec));

				const std::string str = cyng::value_cast<std::string>(vec.at(0), "");
				CYNG_LOG_DEBUG(logger_, "TGateway key [" << str << "]");
				auto key = cyng::table::key_generator(boost::uuids::string_generator()(str));

				ctx.attach(bus_req_query_firmware(key, ctx.tag(), tag_ws));

			}
			else
			{
				CYNG_LOG_WARNING(logger_, "ws.read - unknown query:firmware channel [" << channel << "]");
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

				auto msg = cyng::json::to_string(tpl);

				if (wss)	wss->send_msg(msg);
			}
			else
			{
				CYNG_LOG_WARNING(logger_, "record cpu:load not found");
			}
		}, cyng::store::write_access("_Config"));
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

	void cluster::subscribe(std::string table, std::string const& channel, boost::uuids::uuid tag)
	{
		//
		//	install channel
		//
		server_.add_channel(tag, channel);

		//
		//	send initial data set of device table
		//
		cache_.access([&](cyng::store::table const* tbl) {

			//
			//	inform client that data upload is starting
			//
			display_loading_icon(tag, true, channel);

			//
			//	get total record size
			//
			auto size = tbl->size();
			std::size_t percent{ 0 }, idx{ 0 };

			//
			//	upload data
			//
			const auto counter = tbl->loop([&](cyng::table::record const& rec) -> bool {

				CYNG_LOG_TRACE(logger_, "ws.read - insert " << table << cyng::io::to_str(rec.key()));

				auto tpl = cyng::tuple_factory(
					cyng::param_factory("cmd", std::string("insert")),
					cyng::param_factory("channel", channel),
					cyng::param_factory("rec", rec.convert()));

				auto msg = cyng::json::to_string(tpl);
				server_.get_cm().ws_msg(tag, msg);

				++idx;

				//
				//	calculate charge status in percent
				//
				const auto prev_percent = percent;
				percent = (100u * idx) / size;

				if (prev_percent != percent) {
					display_loading_level(tag, percent, channel);
					//
					//	give GUI a change to refresh
					//
					std::this_thread::sleep_for(std::chrono::milliseconds(120));
				}

				return true;	//	continue
			});
			BOOST_ASSERT(counter == 0);
			boost::ignore_unused(counter);	//	release version
			CYNG_LOG_INFO(logger_, tbl->size() << ' ' << tbl->meta().get_name() << " records sent");

			//
			//	inform client that data upload is finished
			//
			display_loading_icon(tag, false, channel);
		}, cyng::store::read_access(table));
	}

	void cluster::display_loading_icon(boost::uuids::uuid tag, bool b, std::string const& channel)
	{
		server_.get_cm().ws_msg(tag
			, cyng::json::to_string(cyng::tuple_factory(cyng::param_factory("cmd", std::string("load"))
				, cyng::param_factory("channel", channel)
				, cyng::param_factory("show", b))));
	}

	void cluster::display_loading_level(boost::uuids::uuid tag, std::size_t level, std::string const& channel)
	{
		server_.get_cm().ws_msg(tag
			, cyng::json::to_string(cyng::tuple_factory(cyng::param_factory("cmd", std::string("load"))
				, cyng::param_factory("channel", channel)
				, cyng::param_factory("level", level))));
	}

	void cluster::subscribe_table_device_count(std::string const& channel, boost::uuids::uuid tag)
	{
		server_.add_channel(tag, channel);
		const auto size = cache_.size("TDevice");
		dispatcher_.update_channel(channel, size);
	}

	void cluster::subscribe_table_gateway_count(std::string const& channel, boost::uuids::uuid tag)
	{
		server_.add_channel(tag, channel);
		const auto size = cache_.size("TGateway");
		dispatcher_.update_channel(channel, size);
	}

	void cluster::subscribe_table_meter_count(std::string const& channel, boost::uuids::uuid tag)
	{
		server_.add_channel(tag, channel);
		const auto size = cache_.size("TMeter");
		dispatcher_.update_channel(channel, size);
	}

	void cluster::subscribe_table_session_count(std::string const& channel, boost::uuids::uuid tag)
	{
		server_.add_channel(tag, channel);
		const auto size = cache_.size("_Session");
		dispatcher_.update_channel(channel, size);
	}

	void cluster::subscribe_table_target_count(std::string const& channel, boost::uuids::uuid tag)
	{
		server_.add_channel(tag, channel);
		const auto size = cache_.size("_Target");
		dispatcher_.update_channel(channel, size);
	}

	void cluster::subscribe_table_connection_count(std::string const& channel, boost::uuids::uuid tag)
	{
		server_.add_channel(tag, channel);
		const auto size = cache_.size("_Connection");
		dispatcher_.update_channel(channel, size);
	}

	void cluster::subscribe_table_msg_count(std::string const& channel, boost::uuids::uuid tag)
	{
		server_.add_channel(tag, channel);
		const auto size = cache_.size("_SysMsg");
		dispatcher_.update_channel(channel, size);
	}

	void cluster::subscribe_table_LoRa_count(std::string const& channel, boost::uuids::uuid tag)
	{
		server_.add_channel(tag, channel);
		const auto size = cache_.size("TLoRaDevice");
		dispatcher_.update_channel(channel, size);
	}
}
