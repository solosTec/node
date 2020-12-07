/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <tasks/cluster.h>
#include <tasks/client.h>

#include <smf/cluster/generator.h>
#include <smf/shared/protocols.h>
#include <smf/sml/parser/srv_id_parser.h>

#include <cyng/async/task/task_builder.hpp>
#include <cyng/io/serializer.h>
#include <cyng/vm/generator.h>
#include <cyng/tuple_cast.hpp>
#include <cyng/numeric_cast.hpp>
#include <cyng/vm/domain/log_domain.h>

#include <boost/uuid/random_generator.hpp>

namespace node
{
	cluster::cluster(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, boost::uuids::uuid tag
		, cluster_config_t const& cfg
		, boost::asio::ip::tcp::endpoint ep
		, bool client_login
		, bool verbose
		, std::string const& target)
	: base_(*btp)
		, bus_(bus_factory(btp->mux_, logger, boost::uuids::random_generator()(), btp->get_id()))
		, logger_(logger)
		, config_(cfg)
		, client_login_(client_login)
		, verbose_(verbose)
		, target_(target)
		, cache_()
		, db_sync_(logger, cache_, bus_->vm_)
		, server_(btp->mux_.get_io_service(), logger, bus_->vm_, ep)
		, uuid_gen_()
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");

		//
		//	initialize log domain
		//
		cyng::register_logger(logger_, bus_->vm_);

		//
		//	init cache
		//
		create_cache(logger_, cache_);

		//
		//	implement request handler
		//
		bus_->vm_.register_function("bus.reconfigure", 1, std::bind(&cluster::reconfigure, this, std::placeholders::_1));
		bus_->vm_.async_run(cyng::generate_invoke("log.msg.debug", cyng::invoke("lib.size"), " callbacks registered"));

		//
		//	data handling
		//
		bus_->vm_.register_function("db.trx.start", 0, [this](cyng::context& ctx) {
			CYNG_LOG_TRACE(logger_, ctx.get_name());
		});

		bus_->vm_.register_function("db.trx.commit", 1, [this, tag, ep](cyng::context& ctx) {
			auto const frame = ctx.get_frame();
			auto const table = cyng::value_cast<std::string>(frame.at(0), "");;
			CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << table);
			CYNG_LOG_INFO(logger_, table << " size: " << cache_.size(table));

			if (boost::algorithm::equals(table, "_Broker")) {

				ctx.queue(bus_req_db_insert(table
					//	generate new key
					, cyng::table::key_generator(tag)
					//	build data vector
					, cyng::table::data_generator(ep.address(), ep.port(), protocol_IEC)
					, 0
					, ctx.tag()));
			}
		});

		//
		//	register all database/cache related functions
		//
		db_sync_.register_this();

		bus_->vm_.register_function("iec.client.start", 0, [this](cyng::context& ctx) {

			//	9158fa72-7855-403c-a0fb-55a96e1d3af7,"10.132.24.165"ip.v4,1770u16,00:15:0.000000
			auto const frame = ctx.get_frame();

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,				//	[0] key to TMeter table
				boost::asio::ip::address,		//	[1] address
				std::uint16_t,					//	[2] port
				std::chrono::seconds			//	[3] monitor
			>(frame);

			//
			//	lookup for associated meter
			//
			auto const rec = cache_.lookup("TMeter", cyng::table::key_generator(std::get<0>(tpl)));
			if (!rec.empty()) {

				auto const str = cyng::value_cast(rec["protocol"], protocol_any);
				auto const protocol = from_str(str);

				if (protocol == protocol_e::IEC) {

					//
					//	meter number (i.e. 16000913)
					//
					auto const meter = cyng::value_cast(rec["meter"], "00000000");
					auto const ident = cyng::value_cast(rec["ident"], "");	//	ident nummer (i.e. 1EMH0006441734, 01-e61e-13090016-3c-07)
					auto const srv_id = sml::parse_srv_id(ident);
					if (!srv_id.second) {
						CYNG_LOG_ERROR(logger_,
							ctx.get_name()
							<< " - invalid server ID ["
							<< ident
							<< "]");
					}
					else {

						auto const tag = uuid_gen_();

						auto tsk = cyng::async::start_task_detached<client>(base_.mux_
							, bus_->vm_
							, tag
							, logger_
							, cache_
							, boost::asio::ip::tcp::endpoint{ std::get<1>(tpl), std::get<2>(tpl) }
							, meter
							, srv_id.first
							, rec.key()
							, client_login_
							, verbose_
							, target_);

						CYNG_LOG_TRACE(logger_,
							ctx.get_name()
							<< " #"
							<< tsk
							<< " - "
							<< meter
							<< " ["
							<< bus_->vm_.tag()
							<< " => "
							<< tag
							<< "]");
						
						db_sync_.add_client(rec.key(), tsk, bus_->vm_.tag());
					}
				}
				else {
					CYNG_LOG_TRACE(logger_,
						ctx.get_name()
						<< " - skip protocol "
						<< str);
				}
			}
			else {
				CYNG_LOG_WARNING(logger_,
					ctx.get_name()
					<< " IEC record "
					<< std::get<0>(tpl)
					<< " has no meter data");
			}
		});

		bus_->vm_.register_function("iec.client.stop", 0, [this](cyng::context& ctx) {
			auto const frame = ctx.get_frame();
			CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_type(frame));
			});

		bus_->vm_.register_function("mux.stop.task", 1, [this](cyng::context& ctx) {
			auto const frame = ctx.get_frame();
			auto const tsk = cyng::numeric_cast<std::size_t>(frame.at(0), 0);
			CYNG_LOG_WARNING(logger_, ctx.get_name()
				<< " #"
				<< tsk);
			base_.mux_.stop(tsk);
		});

		bus_->vm_.register_function("mux.readout.task", 2, [this](cyng::context& ctx) {
			auto const frame = ctx.get_frame();
			auto const tsk = cyng::numeric_cast<std::size_t>(frame.at(0), 0);
			CYNG_LOG_TRACE(logger_, ctx.get_name()
				<< " #"
				<< tsk);
			base_.mux_.post(tsk, 0, cyng::tuple_t{ frame.at(1) });

			});

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
		//	start IEC broker server
		//
 		server_.run();

		//
		//	sync tables
		//
		for (auto const& tbl : db_sync::tables_) {
			if (!tbl.custom_) {
				CYNG_LOG_INFO(logger_, "subscribe table: " << tbl.name_);
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
			, NODE::classes[NODE::class_e::_BROKER_IEC]));

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
			<< cyng::to_str(config_.get().monitor_));

		base_.suspend(config_.get().monitor_);

	}

}
