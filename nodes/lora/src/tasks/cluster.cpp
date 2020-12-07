/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "cluster.h"
#include <smf/cluster/generator.h>
#include <smf/shared/db_schemes.h>

#include <cyng/async/task/task_builder.hpp>
#include <cyng/io/serializer.h>
#include <cyng/vm/generator.h>
#include <cyng/vm/domain/log_domain.h>

namespace node
{
	cluster::cluster(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, boost::asio::ssl::context& ctx
		, boost::uuids::uuid tag
		, bool keep_xml_files
		, cluster_config_t const& cfg
		, boost::asio::ip::tcp::endpoint ep
		, std::size_t timeout
		, std::uint64_t max_upload_size
		, std::string const& doc_root
		, auth_dirs const& ad
		, std::set<boost::asio::ip::address> const& blocklist
		, std::map<std::string, std::string> const& redirects)
	: base_(*btp)
		, bus_(bus_factory(btp->mux_, logger, boost::uuids::random_generator()(), btp->get_id()))
		, logger_(logger)
		, config_(cfg)
		, server_(logger
			, btp->mux_.get_io_service()
			, ctx
			, ep
			, timeout
			, max_upload_size
			, doc_root
			, "LoRy"
			, ad
			, blocklist
			, redirects
			, bus_->vm_)
		, cache_()
		, processor_(logger, keep_xml_files, cache_, btp->mux_.get_io_service(), tag, bus_)
		, dispatcher_(logger, cache_)
		, db_sync_(logger, cache_)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");

		//
		//	register logger domain
		//
		cyng::register_logger(logger_, bus_->vm_);

		//
		//	init cache
		//
		create_cache(logger_, cache_);

		//
		//	subscribe to database
		//
		dispatcher_.register_this(bus_->vm_);
		dispatcher_.subscribe();

		//
		//	implement request handler
		//
		bus_->vm_.register_function("bus.reconfigure", 1, std::bind(&cluster::reconfigure, this, std::placeholders::_1));

		//
		//	register sync. functions
		//
		db_sync_.register_this(bus_->vm_);

		//
		//	report library size
		//
		bus_->vm_.async_run(cyng::generate_invoke("log.msg.debug", cyng::invoke("lib.size"), " callbacks registered"));

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
		//	start http server
		//
		server_.run();

		//
		//	sync tables
		//
		sync_table("TLoRaDevice");
		sync_table("_Config");

		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation cluster::process()
	{
		//
		//	Connection to master lost
		//

		//
		//	tell server to discard all data
		//
		clear_cache(cache_, bus_->vm_.tag());

		//
		//	switch to other configuration
		//
		reconfigure_impl();
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
			, NODE::classes[NODE::class_e::_LORA]));

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

	void cluster::session_callback(boost::uuids::uuid tag, cyng::vector_t&& prg)
	{
		CYNG_LOG_TRACE(logger_, "received " << prg.size() << " HTTP(s) instructions from " << tag);
		//CYNG_LOG_TRACE(logger_, "session callback: " << cyng::io::to_str(prg));
		this->processor_.vm().async_run(std::move(prg));
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

}
