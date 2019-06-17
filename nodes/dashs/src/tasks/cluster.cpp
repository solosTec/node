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
		, std::size_t timeout
		, std::string const& doc_root
		, auth_dirs const& ad
		, std::set<boost::asio::ip::address> const& blacklist
		, std::map<std::string, std::string> const& redirects)
	: base_(*btp)
		, uidgen_()
		, bus_(bus_factory(btp->mux_, logger, uidgen_(), btp->get_id()))
		, logger_(logger)
		, config_(cfg_cls)
		, cache_()
		, server_(logger, btp->mux_.get_io_service(), ctx, ep, timeout, doc_root, ad, blacklist, redirects, bus_->vm_)
		, dispatcher_(logger, server_.get_cm())
		, db_sync_(logger, cache_)
		, forward_(logger, cache_, server_.get_cm())
		, form_data_(logger)
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
		dispatcher_.register_this(bus_->vm_);

		//
		//	handle form data
		//
		form_data_.register_this(bus_->vm_);

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
		db_sync_.register_this(bus_->vm_);

		forward_.register_this(bus_->vm_);

		bus_->vm_.register_function("http.session.launch", 3, [this](cyng::context& ctx) {
			//	[849a5b98-429c-431e-911d-18a467a818ca,false,127.0.0.1:61383]
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "http.session.launch " << cyng::io::to_str(frame));
		});

		bus_->vm_.register_function("http.authorized", 4, [this](cyng::context& ctx) {
			//	[7f1f9096-dd3b-4a64-a2ed-3b4c97ac9246,true,Basic,sol,/,127.0.0.1:59138]
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "http.authorized " << cyng::io::to_str(frame));
			auto tpl = cyng::tuple_cast<
				boost::uuids::uuid,		//	[0] origin session id
				bool,					//	[1] success
				std::string,			//	[2] authorization type
				std::string,			//	[3] user
				std::string,			//	[4] path
				boost::asio::ip::tcp::endpoint	//	[5] remote endpoint
			>(frame);

			std::stringstream ss;
			ss
				<< std::get<3>(tpl)
				<< " authorized for "
				<< std::get<4>(tpl)
				<< " from "
				<< std::get<5>(tpl)
				<< (std::get<1>(tpl) ? " with" : " without")
				<< " success"
				;

			ctx.queue(bus_insert_msg((std::get<1>(tpl) ? cyng::logging::severity::LEVEL_TRACE : cyng::logging::severity::LEVEL_WARNING), ss.str()));
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

	void cluster::stop(bool shutdown)
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
		sync_table("_CSV");

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

		node::res_subscribe(logger_
			, cache_
			, std::get<0>(tpl)	//	[0] table name
			, std::get<1>(tpl)	//	[1] table key
			, std::get<2>(tpl)	//	[2] record
			, std::get<3>(tpl)	//	[3] generation
			, std::get<4>(tpl)	//	[4] origin session id
			, std::get<5>(tpl));
	}

	void cluster::ws_read(cyng::context& ctx)
	{
		//	[1adb06d2-bfbf-4b00-a7b1-80b49ba48f79,{("cmd":subscribe),("channel":status.session),("push":true)}]
		//	[e9a5fa77-edfc-46b7-8b85-ea4252bfba73,{("cmd":subscribe),("channel":table.connection.count),("push":true)}]
		//	
		//	* session tag
		//	* json object
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "ws.read - " << cyng::io::to_str(frame));

		//
		//	get session tag of websocket
		//
		auto tag_ws = cyng::value_cast(frame.at(0), boost::uuids::nil_uuid());

		//
		//	reader for JSON data
		//
		auto reader = cyng::make_reader(frame.at(1));
		const std::string cmd = cyng::value_cast<std::string>(reader.get("cmd"), "");
		const std::string channel = cyng::value_cast<std::string>(reader.get("channel"), "generic");

		if (boost::algorithm::equals(cmd, "subscribe")) {

			const std::string channel = cyng::value_cast<std::string>(reader.get("channel"), "");
			CYNG_LOG_TRACE(logger_, "ws.read - subscribe channel [" << channel << "]");

			dispatcher_.subscribe_channel(cache_, channel, tag_ws);
		}
		else if (boost::algorithm::equals(cmd, "update"))
		{
			const std::string channel = cyng::value_cast<std::string>(reader.get("channel"), "");
			CYNG_LOG_TRACE(logger_, "ws.read - pull channel [" << channel << "]");
			dispatcher_.pull(cache_, channel, tag_ws);
		}
		else if (boost::algorithm::equals(cmd, "insert"))
		{
			node::fwd_insert(logger_
				, ctx
				, reader
				, uidgen_());
		}
		else if (boost::algorithm::equals(cmd, "delete"))
		{
			node::fwd_delete(logger_
				, ctx
				, reader);
		}
		else if (boost::algorithm::equals(cmd, "modify"))
		{
			node::fwd_modify(logger_
				, ctx
				, reader);
		}
		else if (boost::algorithm::equals(cmd, "stop"))
		{
			node::fwd_stop(logger_
				, ctx
				, reader);
		}
		else if (boost::algorithm::equals(cmd, "com:sml"))
		{
			node::fwd_com_sml(logger_
				, ctx
				, tag_ws
				, channel
				, reader);
		}
		else if (boost::algorithm::equals(cmd, "com:task"))
		{
			node::fwd_com_task(logger_
				, ctx
				, tag_ws
				, channel
				, reader);
		}
		else if (boost::algorithm::equals(cmd, "com:node"))
		{
			node::fwd_com_node(logger_
				, ctx
				, tag_ws
				, channel
				, reader);
		}
		else
		{
			CYNG_LOG_WARNING(logger_, "ws.read - unknown command " << cmd);
		}
	}
}
