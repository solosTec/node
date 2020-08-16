/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "cluster.h"
#include "system.h"
#include "../../../shared/src/tables.h"
#include <smf/cluster/generator.h>

#include <cyng/async/task/task_builder.hpp>
#include <cyng/io/serializer.h>
#include <cyng/vm/generator.h>
#include <cyng/vm/domain/log_domain.h>
#include <cyng/vm/domain/store_domain.h>
#include <cyng/table/meta.hpp>
#include <cyng/tuple_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/factory/set_factory.h>
#include <cyng/dom/reader.h>
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
		, boost::uuids::uuid cluster_tag
		, cluster_config_t const& cfg_cls
		, boost::asio::ip::tcp::endpoint ep
		, std::size_t timeout
		, std::uint64_t max_upload_size
		, std::string const& doc_root
		, std::string const& nickname
#ifdef NODE_SSL_INSTALLED
		, auth_dirs const& ad
#endif
		, std::set<boost::asio::ip::address> const& blocklist
		, std::map<std::string, std::string> const& redirects
		, bool https_rewrite)
	: base_(*btp)
		, uidgen_()
		, bus_(bus_factory(btp->mux_, logger, cluster_tag, btp->get_id()))
		, logger_(logger)
		, config_(cfg_cls)
		, cache_()
		, server_(logger
			, btp->mux_.get_io_service()
			, ep
			, timeout
			, max_upload_size
			, doc_root
			, ""	//	no blog
			, nickname
#ifdef NODE_SSL_INSTALLED
			, ad
#endif
			, blocklist
			, redirects
			, bus_->vm_
			, https_rewrite)
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
			CYNG_LOG_TRACE(logger_, ctx.get_name());
			});
		bus_->vm_.register_function("db.trx.commit", 1, [this](cyng::context& ctx) {
			auto const frame = ctx.get_frame();
			CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));
			});
		//bus_->vm_.register_function("bus.res.subscribe", 6, std::bind(&cluster::res_subscribe, this, std::placeholders::_1));
		db_sync_.register_this(bus_->vm_);

		forward_.register_this(bus_->vm_);

		bus_->vm_.register_function("http.session.launch", 3, [this](cyng::context& ctx) {
			//	[849a5b98-429c-431e-911d-18a467a818ca,false,127.0.0.1:61383]
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "http.session.launch " << cyng::io::to_str(frame));
		});

		//
		//	input from HTTP session / ws
		//
		bus_->vm_.register_function("ws.read", 2, std::bind(&cluster::ws_read, this, std::placeholders::_1));
		
		bus_->vm_.async_run(cyng::generate_invoke("log.msg.debug", cyng::invoke("lib.size"), " callbacks registered"));

		//
		//	store local web ui configuration
		//
		cache_.insert("_Config", cyng::table::key_generator("http-session-timeout"), cyng::table::data_generator(timeout), 0u, cluster_tag);
		cache_.insert("_Config", cyng::table::key_generator("http-max-upload-size"), cyng::table::data_generator(max_upload_size), 0u, cluster_tag);
		cache_.insert("_Config", cyng::table::key_generator("https-rewrite"), cyng::table::data_generator(https_rewrite), 0u, cluster_tag);

		//
		//	subscribe to database
		//
		dispatcher_.subscribe(cache_);
		dispatcher_.register_this(bus_->vm_);

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
		cache_.clear("_SysMsg", bus_->vm_.tag());
		cache_.clear("_TimeSeries", bus_->vm_.tag());
		cache_.clear("_LoRaUplink", bus_->vm_.tag());

		for (auto const& tbl : tables::list_) {
			if (!tbl.local_) {
				sync_table(tbl.name_);
			}
		}

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



	void cluster::ws_read(cyng::context& ctx)
	{
		//	[1adb06d2-bfbf-4b00-a7b1-80b49ba48f79,{("cmd":subscribe),("channel":status.session),("push":true)}]
		//	
		//	* session tag
		//	* ws object
		//	* json object
		auto const frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		//
		//	get session tag of websocket
		//
		auto tag_ws = cyng::value_cast(frame.at(0), boost::uuids::nil_uuid());
		BOOST_ASSERT_MSG(!tag_ws.is_nil(), "no websocket");

		//
		//	reader for JSON data
		//
		auto const reader = cyng::make_reader(frame.at(1));
		auto const cmd = cyng::value_cast<std::string>(reader.get("cmd"), "");
		auto const channel = cyng::value_cast<std::string>(reader.get("channel"), "generic");

		if (boost::algorithm::equals(cmd, "subscribe"))
		{
			CYNG_LOG_TRACE(logger_, "ws.read - subscribe channel [" << channel << "]");

			//
			//	Subscribe a table and start initial upload
			//
			dispatcher_.subscribe_channel(cache_, channel, tag_ws);

		}
		else if (boost::algorithm::equals(cmd, "update"))
		{
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
		else if (boost::algorithm::equals(cmd, "com:proxy"))
		{
			node::fwd_com_proxy(logger_
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
			CYNG_LOG_WARNING(logger_, ctx.get_name()
				<< " unknown command ["
				<< cmd
				<< "]");
		}
	}
}
