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
		, boost::asio::ssl::context& ctx
		, cluster_config_t const& cfg_cls
		, boost::asio::ip::tcp::endpoint ep
		, std::string const& doc_root
		, std::set<boost::asio::ip::address> const& blacklist)
	: base_(*btp)
		, uidgen_()
		, bus_(bus_factory(btp->mux_, logger, uidgen_(), btp->get_id()))
		, logger_(logger)
		, config_(cfg_cls)
		, cache_()
		, server_(logger, btp->mux_.get_io_service(), ctx, ep, doc_root, blacklist, bus_->vm_)
		, dispatcher_(logger, server_.get_cm())
        , sys_tsk_(cyng::async::NO_TASK)
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

		bus_->vm_.register_function("http.session.launch", 0, [this](cyng::context& ctx) {
			CYNG_LOG_TRACE(logger_, "http.session.launch!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
		});

		bus_->vm_.register_function("ws.read", 2, std::bind(&cluster::ws_read, this, std::placeholders::_1));

		bus_->vm_.async_run(cyng::generate_invoke("log.msg.info", cyng::invoke("lib.size"), "callbacks registered"));

	}

    void cluster::start_sys_task()
    {
        
         //start collecting system data
        
        auto r = cyng::async::start_task_delayed<system>(base_.mux_, std::chrono::seconds(2), logger_, cache_, bus_->vm_.tag());
        if (r.second)	{
            sys_tsk_ = r.first;
        }
        else	{
            CYNG_LOG_ERROR(logger_, "could not start system task");
        }
    }

    void cluster::stop_sys_task()
    {
        if (sys_tsk_ != cyng::async::NO_TASK)	{
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
			, "dashs"));

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


	void cluster::ws_read(cyng::context& ctx)
	{
		//	[1adb06d2-bfbf-4b00-a7b1-80b49ba48f79,<!261:ws>,{("cmd":subscribe),("channel":status.session),("push":true)}]
		//	
		//	* session tag
		//	* json object
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "ws.read - " << cyng::io::to_str(frame));

	}




}
