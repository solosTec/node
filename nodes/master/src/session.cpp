/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 


#include <session.h>
#include <client.h>
#include <db.h>
#include <tasks/watchdog.h>

#include <NODE_project_info.h>

#include <smf/cluster/generator.h>
#include <smf/ipt/response.hpp>

#include <cyng/vm/domain/log_domain.h>
#include <cyng/vm/domain/store_domain.h>
#include <cyng/io/serializer.h>
#include <cyng/io/io_chrono.hpp>
#include <cyng/value_cast.hpp>
#include <cyng/object_cast.hpp>
#include <cyng/tuple_cast.hpp>
#include <cyng/dom/reader.h>
#include <cyng/async/task/task_builder.hpp>

#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/predef.h>

namespace node 
{
	session::session(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, cache& cfg
		, std::string const& account
		, std::string const& pwd
		, boost::uuids::uuid stag)
	: mux_(mux)
		, logger_(logger)
		, cache_(cfg)
		, vm_(mux.get_io_service(), stag)
		, parser_([this](cyng::vector_t&& prg) {
			//CYNG_LOG_TRACE(logger_, prg.size() 
			//	<< " instructions received (including "
			//	<< cyng::op_counter(prg, cyng::code::INVOKE)
			//	<< " invoke(s))");
			//CYNG_LOG_DEBUG(logger_, "exec: " << cyng::io::to_str(prg));
			vm_.async_run(std::move(prg));
		})
		, account_(account)
		, pwd_(pwd)
		, seq_(0)
		, client_(mux, logger, cfg, stag)
		, cluster_(mux, logger, cfg)
		, subscriptions_()
		, tsk_watchdog_(cyng::async::NO_TASK)
		, group_(0)
		, cluster_tag_(boost::uuids::nil_uuid())
	{
		//
		//	register logger domain
		//
		cyng::register_logger(logger_, vm_);
		vm_.async_run(cyng::generate_invoke("log.msg.info", "log domain is running"));

		//
		//	increase sequence and set as result value
		//
		vm_.register_function("bus.seq.next", 0, [this](cyng::context& ctx) {
			++this->seq_;
			ctx.push(cyng::make_object(this->seq_));
		});

		//
		//	push last bus sequence number on stack
		//
		vm_.register_function("bus.seq.push", 0, [this](cyng::context& ctx) {
			ctx.push(cyng::make_object(this->seq_));
		});


		vm_.register_function("bus.res.watchdog", 3, std::bind(&session::res_watchdog, this, std::placeholders::_1));

		//
		//	session shutdown - initiated by connection
		//
		vm_.register_function("session.cleanup", 2, std::bind(&session::cleanup, this, std::placeholders::_1));

		//
		//	start/stop time series
		//
		vm_.register_function("session.time.series", 1, std::bind(&session::time_series, this, std::placeholders::_1));

		//
		//	register request handler
		//
		vm_.register_function("bus.req.login", 11, std::bind(&session::bus_req_login, this, std::placeholders::_1));
		vm_.register_function("bus.req.stop.client", 3, std::bind(&session::bus_req_stop_client_impl, this, std::placeholders::_1));
		vm_.register_function("bus.insert.msg", 2, std::bind(&session::bus_insert_msg, this, std::placeholders::_1));
		vm_.register_function("bus.req.push.data", 7, std::bind(&session::bus_req_push_data, this, std::placeholders::_1));
		vm_.register_function("bus.insert.LoRa.uplink", 11, std::bind(&session::bus_insert_lora_uplink, this, std::placeholders::_1));
		vm_.register_function("bus.insert.wMBus.uplink", 8, std::bind(&session::bus_insert_wmbus_uplink, this, std::placeholders::_1));
		vm_.register_function("bus.insert.IEC.uplink", 5, std::bind(&session::bus_insert_iec_uplink, this, std::placeholders::_1));

		//
		//	execute data cleanup
		//
		vm_.register_function("bus.cleanup", 2, std::bind(&session::bus_cleanup, this, std::placeholders::_1));

		//
		//	statistical data
		//
		vm_.async_run(cyng::generate_invoke("log.msg.debug", cyng::invoke("lib.size"), " callbacks registered"));

		//
		//	register additional request handler for database access
		//
		cyng::register_store(this->cache_.db_, vm_);

	}


	std::size_t session::hash() const noexcept
	{
		return vm_.hash();
	}

	void session::stop(cyng::object obj)
	{
		vm_.access(std::bind(&session::stop_cb, this, std::placeholders::_1, obj));
	}

	void session::stop_cb(cyng::vm& vm, cyng::object obj)
	{
		//
		//	object reference is needed to make sure that VM is in valid state.
		//
		vm.run(cyng::generate_invoke("log.msg.info", "fast shutdown"));
		vm.run(cyng::generate_invoke("ip.tcp.socket.shutdown"));
		vm.run(cyng::generate_invoke("ip.tcp.socket.close"));
		vm.run(cyng::vector_t{ cyng::make_object(cyng::code::HALT) });
	}

	void session::cleanup(cyng::context& ctx)
	{
		BOOST_ASSERT(this->vm_.tag() == ctx.tag());

		//
		//	session is being closed.
		//	no further VM calls allowed!
		//

		//	 [<!259:session>,asio.misc:2]
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_WARNING(logger_, "cluster member "
			<< ctx.tag()
			<< " lost");

		//
		//	stop watchdog task
		//
		if (tsk_watchdog_ != cyng::async::NO_TASK)
		{
			CYNG_LOG_INFO(logger_, "stop watchdog task #" << tsk_watchdog_);
			mux_.stop(tsk_watchdog_);
			tsk_watchdog_ = cyng::async::NO_TASK;
		}

		//
		//	remove affected targets
		//
		cache_.write_table("_Target", [&](cyng::store::table* tbl_target)->void {
			auto const count = cyng::erase(tbl_target, get_targets_by_peer(tbl_target, ctx.tag()), ctx.tag());
			CYNG_LOG_WARNING(logger_, "cluster member "
				<< ctx.tag()
				<< " removed "
				<< count
				<< " targets");
		});

		//
		//	remove affected channels.
		//	Channels are 2-way: There is an owner session and a target session. Both could be different.
		//
		cache_.write_table("_Channel", [&](cyng::store::table* tbl_channel)->void {
			auto const count = cyng::erase(tbl_channel, get_channels_by_peer(tbl_channel, ctx.tag()), ctx.tag());
			CYNG_LOG_WARNING(logger_, "cluster member "
				<< ctx.tag()
				<< " removed "
				<< count
				<< " channels");
		});

		//
		//	remove all open subscriptions
		//
		CYNG_LOG_INFO(logger_, "cluster member "
			<< ctx.tag()
			<< " removes "
			<< subscriptions_.size()
			<< " subscriptions");
		cyng::store::close_subscription(subscriptions_);
		
		//
		//	get max event limit
		//
		auto const max_events = cache_.get_max_events();

		//
		//	remove all client sessions of this node and close open connections
		//
		cache_.db_.access([&](cyng::store::table* tbl_session
			, cyng::store::table* tbl_connection
			, cyng::store::table const* tbl_device
			, cyng::store::table* tbl_tsdb)->void {

			cyng::table::key_list_t pks;
			tbl_session->loop([&](cyng::table::record const& rec) -> bool {

				//
				//	build list with all affected session records
				//
				auto local_peer = cyng::object_cast<session>(rec["local"]);
				auto remote_peer = cyng::object_cast<session>(rec["remote"]);
				auto account = cyng::value_cast<std::string>(rec["name"], "");
				auto tag = cyng::value_cast(rec["tag"], boost::uuids::nil_uuid());

				//
				//	Check if the client belongs to this session.
				//
				if (local_peer && (local_peer->vm_.tag() == ctx.tag()))
				{
					pks.push_back(rec.key());
					if (remote_peer)
					{
						//	remote connection
						auto rtag = cyng::value_cast(rec["rtag"], boost::uuids::nil_uuid());

						//
						//	close connection
						//
						if (local_peer->hash() != remote_peer->hash())
						{

							ctx.queue(cyng::generate_invoke("log.msg.warning"
								, "close distinct connection to "
								, rtag));

							//	shutdown mode
							remote_peer->vm_.async_run(client_req_close_connection_forward(rtag
								, tag
								, 0u	//	no sequence number
								, true	//	shutdown mode
								, cyng::param_map_t()
								, cyng::param_map_factory("origin-tag", tag)("local-peer", ctx.tag())("local-connect", true)));
						}


						//
						//	generate statistics
						//	There is an error in ec serialization/deserialization
						//	
						auto ec = cyng::value_cast(frame.at(1), boost::system::error_code());
						client_.write_stat(tbl_tsdb, tag, account, "shutdown node " + client_.get_class(), ec.message(), max_events);
						
						//
						//	remove from connection table
						//
						connection_erase(tbl_connection, cyng::table::key_generator(tag, rtag), tag);
					}
				}

				//	continue
				return true;
			});

			//
			//	remove all session records
			//
			cyng::erase(tbl_session, pks, ctx.tag());

		}	, cyng::store::write_access("_Session")
			, cyng::store::write_access("_Connection")
			, cyng::store::read_access("TDevice")
			, cyng::store::write_access("_TimeSeries"));

		//
		//	clean up cluster table (CSV tasks)
		//

		auto node_class = cleanup_cluster_table(ctx.tag());

		//
		//	clean up CSV task
		//
		if (boost::algorithm::equals(node_class, "csv")) {

			cache_.write_table("_CSV", [&](cyng::store::table* tbl_csv)->void {

				//
				//	remove from CSV table (same key)
				//
				auto const key = cyng::table::key_generator(cluster_tag_);
				tbl_csv->erase(key, ctx.tag());
			});
		}

		//
		//	clean up "EN13757-4 wM-Bus broker" broker
		//
		if (boost::algorithm::equals(node_class, "EN13757-4 wM-Bus broker")) {

			cache_.write_table("_Broker", [&](cyng::store::table* tbl_broker)->void {

				//
				//	remove from TBroker table (same key)
				//
				auto const key = cyng::table::key_generator(cluster_tag_);
				tbl_broker->erase(key, ctx.tag());
			});
		}

		//
		//	clean up "IEC-62056-21:2002 broker" broker
		//
		if (boost::algorithm::equals(node_class, "IEC-62056-21:2002 broker")) {

			cache_.write_table("_Broker", [&](cyng::store::table* tbl_broker)->void {

				//
				//	remove from TBroker table (same key)
				//
				auto const key = cyng::table::key_generator(cluster_tag_);
				tbl_broker->erase(key, ctx.tag());
			});
		}

		//
		//	emit a system message
		//
		std::stringstream ss;
		ss
			<< "cluster member "
			<< node_class
			<< ':'
			<< ctx.tag()
			<< " closed"
			;
		cache_.insert_msg(cyng::logging::severity::LEVEL_WARNING
			, ss.str()
			, ctx.tag());

	}

	std::string session::cleanup_cluster_table(boost::uuids::uuid tag)
	{
		std::string node_class;
		cache_.write_table("_Cluster", [&](cyng::store::table* tbl_cluster)->void {

			//
			//	get node class
			//
			auto const key = cyng::table::key_generator(cluster_tag_);
			auto const rec = tbl_cluster->lookup(key);
			if (!rec.empty())
			{
				//
				//	get node class
				//
				node_class = cyng::value_cast<std::string>(rec["class"], "");

				CYNG_LOG_INFO(logger_, "cluster member "
					<< node_class
					<< '@'
					<< cluster_tag_
					<< " will be removed");

				//
				//	remove from cluster table
				//
				tbl_cluster->erase(key, tag);
			}

			//
			//	update master record
			//
			tbl_cluster->modify(cyng::table::key_generator(cache_.get_tag()), cyng::param_factory("clients", tbl_cluster->size()), tag);

		});

		return node_class;
	}

	void session::bus_req_login(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - " << cyng::io::to_type(frame));

		//	[root,X0kUj59N,46ca0590-e852-417d-a1c7-54ed17d9148f,setup,0.2,-1:00:0.000000,2018-03-19 10:01:36.40087970,false]
		//
		//	* account
		//	* password
		//	* tag
		//	* class
		//	* version
		//	* plattform (since v0.4)
		//	* timezone delta
		//	* timestamp
		//	* auto config allowed
		//	* group id
		//	* remote ep
		//

		//
		// To be compatible with version prior v0.4 we have first to detect the structure.
		// Prior v0.4 the first element was the account name and version was on index 4.
		//
		if (frame.at(0).get_class().tag() == cyng::TC_STRING
			&& frame.at(4).get_class().tag() == cyng::TC_VERSION)
		{
			//
			//	v0.3
			//
			auto const tpl = cyng::tuple_cast<
				std::string,			//	[0] account
				std::string,			//	[1] password
				boost::uuids::uuid,		//	[2] cluster tag
				std::string,			//	[3] class
				cyng::version,			//	[4] version
				std::chrono::minutes,	//	[5] delta
				std::chrono::system_clock::time_point,	//	[6] timestamp
				bool,					//	[7] autologin
				std::uint32_t,			//	[8] group
				boost::asio::ip::tcp::endpoint	//	[9] remote ep
			>(frame);

			BOOST_ASSERT_MSG(std::get<4>(tpl) == cyng::version(0, 3), "version 0.3 expected");

			CYNG_LOG_WARNING(logger_, "cluster member "
				<< std::get<0>(tpl)
				<< " login with version "
				<< std::get<4>(tpl));

			bus_req_login_impl(ctx
				, std::get<4>(tpl)
				, std::get<0>(tpl)
				, std::get<1>(tpl)
				, std::get<2>(tpl)
				, std::get<3>(tpl)
				, std::get<5>(tpl)
				, std::get<6>(tpl)
				, std::get<7>(tpl)
				, std::get<8>(tpl)
				, std::get<9>(tpl)
				, "unknown"
				, 0);
		}
		else
		{
			//
			//	v0.4 or higher
			//
			auto const tpl = cyng::tuple_cast<
				cyng::version,			//	[0] version
				std::string,			//	[1] account
				std::string,			//	[2] password
				boost::uuids::uuid,		//	[3] session tag
				std::string,			//	[4] class
				std::chrono::minutes,	//	[5] delta
				std::chrono::system_clock::time_point,	//	[6] timestamp
				bool,					//	[7] autologin
				std::uint32_t,			//	[8] group
				boost::asio::ip::tcp::endpoint,	//	[9] remote ep
				std::string,			//	[10] platform
				std::int64_t			//	[11] process id (boost::process::pid_t)
			>(frame);

			BOOST_ASSERT_MSG(std::get<0>(tpl) > cyng::version(0, 3), "version 0.4 or higher expected");

			bus_req_login_impl(ctx
				, std::get<0>(tpl)
				, std::get<1>(tpl)
				, std::get<2>(tpl)
				, std::get<3>(tpl)
				, std::get<4>(tpl)
				, std::get<5>(tpl)
				, std::get<6>(tpl)
				, std::get<7>(tpl)
				, std::get<8>(tpl)
				, std::get<9>(tpl)
				, std::get<10>(tpl)
				, std::get<11>(tpl));
		}
	}

	void session::bus_req_login_impl(cyng::context& ctx
		, cyng::version ver
		, std::string const& account
		, std::string const& pwd
		, boost::uuids::uuid cluster_tag
		, std::string const& node_class	
		, std::chrono::minutes delta
		, std::chrono::system_clock::time_point ts
		, bool autologin
		, std::uint32_t group
		, boost::asio::ip::tcp::endpoint ep
		, std::string platform
		, std::int64_t pid) 
	{
		//
		//	ToDo: check for duplicate tags
		//	Note: this requires to extend _Cluster table with the original tag
		//
		//db_.access([&](const cyng::store::table* tbl_cfg)->void {
		//	tbl_cfg->exist(cyng::table::key_generator(tag));
		//}, cyng::store::read_access("_Config"));

		if ((account == account_) && (pwd == pwd_))
		{
			CYNG_LOG_INFO(logger_, "cluster member "
				<< cluster_tag
				//<< ctx.tag()
				<< " - "
				<< node_class
				<< '@'
				<< ep
				<< " successful authorized");

			////
			////	register additional request handler for database access
			////
			//cyng::register_store(this->cache_.db_, ctx);

			//
			//	register client functions
			//
			client_.register_this(ctx);

			//
			//	register cluster bus functions
			//
			cluster_.register_this(ctx);

			//
			//	subscribe/unsubscribe
			//
			ctx.queue(cyng::register_function("bus.req.subscribe", 3, std::bind(&session::bus_req_subscribe, this, std::placeholders::_1)));
			ctx.queue(cyng::register_function("bus.req.unsubscribe", 2, std::bind(&session::bus_req_unsubscribe, this, std::placeholders::_1)));
			ctx.queue(cyng::register_function("bus.start.watchdog", 7, std::bind(&session::bus_start_watchdog, this, std::placeholders::_1)));

			//
			//	set node class and group id
			//
			group_ = group;
			client_.set_class(node_class);
			cluster_tag_ = cluster_tag;

			//
			//	insert into cluster table
			//
			std::chrono::microseconds ping = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - ts);
			ctx.queue(cyng::generate_invoke("bus.start.watchdog"
				, node_class
				, ts
				, ver
				, ping
				, cyng::invoke("push.session")
				, ep
				, pid));

			//
			//	send reply
			//
			ctx.queue(reply(ts, true));

		}
		else
		{
			CYNG_LOG_WARNING(logger_, "cluster member login failed: " 
				<< ctx.tag()
				<< " - "
				<< account
				<< ':'
				<< pwd);

			//
			//	send reply
			//
			ctx.queue(reply(ts, false));

			//
			//	emit system message
			//
			std::stringstream ss;
			ss
				<< "cluster member login failed: " 
				<< node_class
				<< '@'
				<< ep
				;
			cache_.insert_msg(cyng::logging::severity::LEVEL_ERROR
				, ss.str()
				, ctx.tag());

		}
	}

	void session::bus_req_stop_client_impl(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_INFO(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		//	[2,[1ca8529b-9b95-4a3a-ba2f-26c7280aa878],ac70b95e-76b9-463a-a961-bb02f70e86c8]
		//
		//	* bus sequence
		//	* session key
		//	* source

		auto const tpl = cyng::tuple_cast<
			std::uint64_t,			//	[0] sequence
			cyng::vector_t,			//	[1] session key
			boost::uuids::uuid		//	[2] source
		>(frame);

		//
		//	stop a client session
		//
		cache_.write_table("_Session", [&](const cyng::store::table* tbl_session)->void {
			cyng::table::record rec = tbl_session->lookup(std::get<1>(tpl));
			if (!rec.empty())
			{
				auto peer = cyng::object_cast<session>(rec["local"]);
				BOOST_ASSERT(peer->vm_.tag() != ctx.tag());
				if (peer && (peer->vm_.tag() != ctx.tag()) && (std::get<1>(tpl).size() == 1))
				{ 
					//
					//	node will send a client close response
					//
					auto tag = cyng::value_cast(std::get<1>(tpl).at(0), boost::uuids::nil_uuid());
					peer->vm_.async_run(node::client_req_close(tag, 0));
				}
			}
			else
			{
				CYNG_LOG_WARNING(logger_, "bus.req.stop.client not found " << cyng::io::to_str(frame));

			}
		});
	}


	void session::bus_start_watchdog(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_INFO(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		//	[setup,2018-05-02 09:55:02.49315500,0.4,00:00:0.034161,<!259:session>,127.0.0.1:53116,10984]
		//
		//	* class
		//	* login
		//	* version
		//	* ping
		//	* session object
		//	* ep
		//	* pid
		
		//
		//	insert into cluster table
		//
		cache_.write_table("_Cluster", [&](cyng::store::table* tbl_cluster)->void {

			//
			//	insert into cluster table
			//
			if (!tbl_cluster->insert(cyng::table::key_generator(cluster_tag_)	//	ctx.tag()
				, cyng::table::data_generator(frame.at(0), frame.at(1), frame.at(2), 0u, frame.at(3), frame.at(5), frame.at(6), frame.at(4))
				, 0u, ctx.tag()))
			{
				//
				//	duplicate session tags
				//
				CYNG_LOG_FATAL(logger_, "bus.start.watchdog - Cluster table insert failed" << cyng::io::to_str(frame));

			}
			else {

				//
				//	update master record
				//
				tbl_cluster->modify(cyng::table::key_generator(cache_.get_tag()), cyng::param_factory("clients", tbl_cluster->size()), ctx.tag());
			}

		});

		//
		//	build message strings
		//
		std::stringstream ss;

		//
		//	start watchdog task
		//
		auto const monitor = cache_.get_cluster_hartbeat();
		if (monitor.count() > 5)
		{
			tsk_watchdog_ = cyng::async::start_task_delayed<watchdog>(mux_, std::chrono::seconds(30)
				, logger_
				, frame.at(4)	//	session object
				, monitor).first;

			//
			//	system message
			//
			ss
				<< "start watchdog task #"
				<< tsk_watchdog_
				<< " for cluster member "
				<< cyng::value_cast<std::string>(frame.at(0), "")
				<< ':'
				<< ctx.tag()
				<< " with "
				<< monitor.count()
				<< " seconds"
				;
			cache_.insert_msg(cyng::logging::severity::LEVEL_INFO
				, ss.str()
				, ctx.tag());

		}
		else
		{
			ss
				<< "do not start watchdog task for cluster member "
				<< cyng::value_cast<std::string>(frame.at(0), "")
				<< ':'
				<< ctx.tag()
				<< " - watchdog timer is "
				<< monitor.count()
				<< " second(s)"
				;
			cache_.insert_msg(cyng::logging::severity::LEVEL_WARNING
				, ss.str()
				, ctx.tag());
		}

		//
		//	print system message
		//	cluster member modem:c97ff026-768d-4c71-b9bc-7bb2ad186c34 joined
		//
		ss.str("");
		ss
			<< "cluster member "
			<< cyng::value_cast<std::string>(frame.at(0), "")
			<< ':'
			<< ctx.tag()
			<< " joined"
			;
		cache_.insert_msg(cyng::logging::severity::LEVEL_INFO
			, ss.str()
			, ctx.tag());


	}

	void session::res_watchdog(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		//CYNG_LOG_INFO(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] session tag
			std::uint64_t,			//	[1] sequence
			std::chrono::system_clock::time_point	//	[2] timestamp
		>(frame);

		//
		//	calculate ping time
		//
		auto ping = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - std::get<2>(tpl));
		CYNG_LOG_INFO(logger_, "bus.res.watchdog "
			<< std::get<0>(tpl)
			<< " - ping time " 
			<< cyng::to_str(ping));

		cache_.db_.modify("_Cluster"
			, cyng::table::key_generator(std::get<0>(tpl))
			, cyng::param_factory("ping", ping)
			, ctx.tag());
	}



	void session::sig_ins(cyng::store::table const* tbl
		, cyng::table::key_type const& key
		, cyng::table::data_type const& data
		, std::uint64_t gen
		, boost::uuids::uuid source)
	{
#ifdef _DEBUG
		if (boost::algorithm::equals(tbl->meta().get_name(), "TLoRaDevice")) {
			BOOST_ASSERT_MSG(data.at(0).get_class().tag() == cyng::TC_MAC64, "DevEUI has wrong data type");
		}
#endif

#ifdef _DEBUG
		if (boost::algorithm::equals(tbl->meta().get_name(), "TDevice"))
		{
			vm_.async_run(cyng::generate_invoke("log.msg.debug"	
				, "sig.ins(source: "
				, source
				, ", table: "
				, tbl->meta().get_name()
				, ", type: "
				, ((source != vm_.tag()) ? "req" : "res")
				, ", key: "
				, key
				, ", data: "
				, data
			));
		}
		else
		{
			vm_.async_run(cyng::generate_invoke("log.msg.debug"
				, "sig.ins(source: "
				, source
				, ", table: "
				, tbl->meta().get_name()
				, ", type: "
				, ((source != vm_.tag()) ? "req" : "res")
				, ")"));
		}
#else
		vm_.async_run(cyng::generate_invoke("log.msg.debug", "sig.ins(source:", source, ", table: ", tbl->meta().get_name(), ")"));
#endif

		//
		//	don't send data back to sender
		//
		if (source != vm_.tag())
		{
			//	forward request
			vm_.async_run(bus_req_db_insert(tbl->meta().get_name()
				, key
				, data
				, gen
				, source));
		}
		else
		{
			//	send response
			vm_.async_run(bus_res_db_insert(tbl->meta().get_name()
				, key
				, data
				, gen));
		}
	}

	void session::sig_del(cyng::store::table const* tbl, cyng::table::key_type const& key, boost::uuids::uuid source)
	{
		vm_.async_run(cyng::generate_invoke("log.msg.debug", "sig.del(table: ", tbl->meta().get_name(), ", source: ", source, ")"));

		//
		//	don't send data back to sender
		//
		if (source != vm_.tag())
		{
			vm_.async_run(bus_req_db_remove(tbl->meta().get_name(), key, source));
		}
		else
		{
			vm_.async_run(bus_res_db_remove(tbl->meta().get_name(), key));

		}
	}

	void session::sig_clr(cyng::store::table const* tbl, boost::uuids::uuid source)
	{
		//
		//	don't send data back to sender
		//
		if (source != vm_.tag())
		{
			vm_.async_run(cyng::generate_invoke("log.msg.debug", "sig.clr(table: ", tbl->meta().get_name(), ", source: ", source, ")"));
			vm_.async_run(bus_db_clear(tbl->meta().get_name()));
		}
	}

	void session::sig_mod(cyng::store::table const* tbl
		, cyng::table::key_type const& key
		, cyng::attr_t const& attr
		, std::uint64_t gen
		, boost::uuids::uuid source)
	{
		vm_.async_run(cyng::generate_invoke("log.msg.debug", "sig.mod(table: ", tbl->meta().get_name(), ", source: ", source, ")"));

		if (source != vm_.tag())
		{
			//
			//	forward modify request with parameter value
			//
			vm_.async_run(bus_req_db_modify(tbl->meta().get_name(), key, tbl->meta().to_param(attr), gen, source));
		}
		else
		{
			//	send response
			vm_.async_run(bus_res_db_modify(tbl->meta().get_name(), key, attr, gen));
			
			if (boost::algorithm::equals(tbl->meta().get_name(), "_Config"))
			{
				if (!key.empty())
				{
					auto const name = cyng::value_cast<std::string>(key.at(0), "?");

					if (boost::algorithm::equals(name, "connection-auto-login"))
					{
						cache_.set_connection_auto_login(cyng::value_cast(attr.second, false));
					}
					else if (boost::algorithm::equals(name, "connection-auto-enabled"))
					{
						cache_.set_connection_auto_enabled(cyng::value_cast(attr.second, false));
					}
					else if (boost::algorithm::equals(name, "gw-cache"))
					{
						cache_.set_gw_cache_enabled(cyng::value_cast(attr.second, false));
					}
					else if (boost::algorithm::equals(name, "connection-superseed"))
					{
						cache_.set_connection_superseed(cyng::value_cast(attr.second, false));
					}
					else if (boost::algorithm::equals(name, "catch-meters"))
					{
						cache_.set_catch_meters(cyng::value_cast(attr.second, false));
					}
					else if (boost::algorithm::equals(name, "catch-lora"))
					{
						cache_.set_catch_lora(cyng::value_cast(attr.second, false));
					}
					else if (boost::algorithm::equals(name, "generate-time-series"))
					{
						auto const b = cyng::value_cast(attr.second, false);
						cache_.set_generate_time_series(b);

						//
						//	generate a time series event.
						//	We have to be careful to avoid a deadlock with tables.
						//
						vm_.async_run(cyng::generate_invoke("session.time.series", b));
						
					}
					else {

						CYNG_LOG_WARNING(logger_, "sig.mod - table "
							<< tbl->meta().get_name()
							<< " ignores key: "
							<< name);
					}
				}
				else {

					CYNG_LOG_WARNING(logger_, "sig.mod - table "
						<< tbl->meta().get_name()
						<< " comes with empty key");

				}
			}
		}
	}

	void session::time_series(cyng::context& ctx)
	{
		cyng::vector_t const frame = ctx.get_frame();
		auto const ts_on = cyng::value_cast(frame.at(0), false);

		//
		//	read max number of events from config table
		//
		auto const max_events = cache_.get_max_events();

		std::size_t counter{ 0 };
		cache_.db_.access([&](cyng::store::table const* tbl_session, cyng::store::table* tbl_ts)->void {

			auto const size = tbl_session->size();
			tbl_session->loop([&](cyng::table::record const& rec) -> bool {

				auto const tag = cyng::value_cast(rec["tag"], boost::uuids::nil_uuid());
				auto const name = cyng::value_cast<std::string>(rec["name"], "");

				//
				//	create a time series event
				//
				std::stringstream ss;
				ss
					<< ++counter
					<< "/"
					<< size
					;
				if (ts_on) {
					insert_ts_event(tbl_ts, tag, name, "start recording...", cyng::make_object(ss.str()), max_events);
				}
				else {
					insert_ts_event(tbl_ts, tag, name, "...stop recording", cyng::make_object(ss.str()), max_events);
				}

				return true;
				});
			}	, cyng::store::read_access("_Session")
				, cyng::store::write_access("_TimeSeries"));
	}

	void session::bus_req_subscribe(cyng::context& ctx)
	{
		//
		//	[TDevice,cfa27fa4-3164-4d2a-80cc-504541c673db,3]
		//
		//	* table to subscribe
		//	* remote session id
		//	* optional task id (to redistribute by receiver)
		//	
		const cyng::vector_t frame = ctx.get_frame();

		auto const tpl = cyng::tuple_cast<
			std::string,			//	[0] table name
			boost::uuids::uuid,		//	[1] remote session tag
			std::size_t				//	[2] task id
		>(frame);

		ctx.queue(cyng::generate_invoke("log.msg.info", "bus.req.subscribe(table: ", std::get<0>(tpl), ", tag: ", std::get<1>(tpl), ")" ));

		cache_.db_.access([&](cyng::store::table const* tbl)->void {

			CYNG_LOG_INFO(logger_, tbl->meta().get_name() << "->size(" << tbl->size() << ")");
			tbl->loop([&](cyng::table::record const& rec) -> bool {

				BOOST_ASSERT_MSG(!rec.key().empty(), "empty key");

#ifdef _DEBUG
				if (boost::algorithm::equals(tbl->meta().get_name(), "TLoRaDevice")) {
					BOOST_ASSERT_MSG(rec.data().at(0).get_class().tag() == cyng::TC_MAC64, "DevEUI has wrong data type");
				}
#endif

				ctx.queue(bus_res_subscribe(tbl->meta().get_name()
					, rec.key()
					, rec.data()
					, rec.get_generation()
					, std::get<1>(tpl)		//	session tag
					, std::get<2>(tpl)));

				//	continue loop
				return true;
			});

		}, cyng::store::read_access(std::get<0>(tpl)));

		cache_.write_table(std::get<0>(tpl), [&](cyng::store::table* tbl)->void {
			//
			//	One set of callbacks for multiple tables.
			//	Store connections array for clean disconnect
			//
			cyng::store::add_subscription(subscriptions_
				, std::get<0>(tpl)
				, tbl->get_listener(std::bind(&session::sig_ins, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
					, std::bind(&session::sig_del, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
					, std::bind(&session::sig_clr, this, std::placeholders::_1, std::placeholders::_2)
					, std::bind(&session::sig_mod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)));
		});

	}

	void session::bus_req_unsubscribe(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_INFO(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			std::string,			//	[0] table name
			boost::uuids::uuid		//	[1] remote session tag
		>(frame);

		cyng::store::close_subscription(subscriptions_, std::get<0>(tpl));
	}

	cyng::vector_t session::reply(std::chrono::system_clock::time_point ts, bool success)
	{
		cyng::vector_t prg;
		prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("bus.res.login"
				, success
				, cyng::code::IDENT
				, cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR)
				, ts	//	timestamp of sender
				, std::chrono::system_clock::now()));
		prg
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
		CYNG_LOG_TRACE(logger_, "reply: " << cyng::io::to_str(prg));
		return prg;
	}

	void session::bus_insert_msg(cyng::context& ctx)
	{
		//	[2308f3d1-4a68-45ae-b5fa-cedf019e29b7,TRACE,http.upload.progress: 72%]
		//
		//	* session tag
		//	* severity
		//	* message
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,			//	[0] origin client tag
			cyng::logging::severity,	//	[1] level
			std::string					//	[2] msg
		>(frame);

		cache_.insert_msg(std::get<1>(tpl)
			, std::get<2>(tpl)
			, ctx.tag());
	}

	void session::bus_insert_lora_uplink(cyng::context& ctx)
	{
		auto const frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,			//	[0] origin client tag
			std::chrono::system_clock::time_point,	//	[1] tp
			cyng::mac64,					//	[2] devEUI
			std::uint16_t,					//	[3] FPort
			std::uint32_t,					//	[4] FCntUp
			std::uint32_t,					//	[5] ADRbit
			std::uint32_t,					//	[6] MType
			std::uint32_t,					//	[7] FCntDn
			std::string,					//	[8] customerID
			std::string,					//	[9] payload
			boost::uuids::uuid				//	[10] tag
		>(frame);

		insert_lora_uplink(cache_.db_
			, std::get<1>(tpl)
			, std::get<2>(tpl)
			, std::get<3>(tpl)
			, std::get<4>(tpl)
			, std::get<5>(tpl)
			, std::get<6>(tpl)
			, std::get<7>(tpl)
			, std::get<8>(tpl)
			, std::get<9>(tpl)
			, std::get<10>(tpl)
			, ctx.tag());
	}

	void session::bus_insert_wmbus_uplink(cyng::context& ctx)
	{
		auto const frame = ctx.get_frame();

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,			//	[0] origin client tag
			std::chrono::system_clock::time_point,	//	[1] tp
			std::string,			// [2] payload
			std::uint8_t,			// [3] medium
			std::string,			// [4] manufacturer
			std::uint8_t,			// [5] frame_type
			std::string,			// [6] payload
			boost::uuids::uuid		// [7] tag
		>(frame);

		if (!insert_wmbus_uplink(cache_.db_
			, std::get<1>(tpl)	//	pk
			, std::get<2>(tpl)	//	serverId
			, std::get<3>(tpl)	//	serverId
			, std::get<4>(tpl)	//	serverId
			, std::get<5>(tpl)	//	serverId
			, std::get<6>(tpl)	//	payload
			, std::get<7>(tpl)	//	source tag
			, ctx.tag())) {

			CYNG_LOG_WARNING(logger_, ctx.get_name() 
				<< " failed: " 
				<< cyng::io::to_str(frame));
		}
	}

	void session::bus_insert_iec_uplink(cyng::context& ctx)
	{
		auto const frame = ctx.get_frame();

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,			//	[0] origin client tag
			std::chrono::system_clock::time_point,	//	[1] tp
			std::string,			// [2] event
			boost::asio::ip::tcp::endpoint,		//	[3] ep
			boost::uuids::uuid		// [4] tag
		>(frame);

		if (!insert_iec_uplink(cache_.db_
			, std::get<1>(tpl)	//	tp
			, std::get<2>(tpl)	//	event
			, std::get<3>(tpl)	//	ep
			, std::get<4>(tpl)	//	tag
			, ctx.tag())) {

			CYNG_LOG_WARNING(logger_, ctx.get_name()
				<< " failed: "
				<< cyng::io::to_str(frame));
		}
	}

	void session::bus_cleanup(cyng::context& ctx)
	{
		auto const frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			std::string,			//	[0] table
			boost::uuids::uuid		//	[1] origin client tag
		>(frame);

		//
		//	remove all orphaned entries from TBridge
		//
		std::size_t counter{ 0 };
		cache_.db_.access([&](cyng::store::table const* tbl_meter, cyng::store::table* tbl_iec)->void {

			cyng::table::key_list_t orphanes;

			tbl_iec->loop([&](cyng::table::record const& rec) -> bool {

				if (!tbl_meter->exist(rec.key())) {
					
					CYNG_LOG_DEBUG(logger_, ctx.get_name()
						<< " - entry "
						<< cyng::io::to_type(rec.convert_data())
						<< " is orphaned");

					orphanes.push_back(rec.key());
				}

				//	continue
				return true;
			});

			//
			//	remove all orphanes
			//
			CYNG_LOG_WARNING(logger_, ctx.get_name()
				<< " remove "
				<< orphanes.size()
				<< "orphaned entries from TBridge");

			for (auto const& key : orphanes) {
				tbl_iec->erase(key, std::get<1>(tpl));
			}

		}	, cyng::store::read_access("TMeter")
			, cyng::store::write_access("TBridge"));

	}


	void session::bus_req_push_data(cyng::context& ctx)
	{
		//	[1,store,TLoraMeta,false
		//	[7df1353e-8968-47b7-81bd-1c3097dcc878],
		//	[5,52,100000728,29000071,1,LC2,G1,12,7,-99,444eefd3,32332e31342c2032332e32302c20313031342c2035352e3538,148,0,1,1,0001,2015-10-22 13:39:59.48900000,F03D291000001180],
		//	9228e436-7a13-4140-82f1-2c38229d0f7a]
		//
		//	* bus sequene
		//	* class name
		//	* channel/table name
		//	* distribution (single=false/all=true)
		//	* key
		//	* data
		//	* source
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			std::uint64_t,		//	[0] sequence number
			std::string,		//	[1] class
			std::string,		//	[2] channel
			bool,				//	[3] distribution
			cyng::vector_t,		//	[4] key
			cyng::vector_t,		//	[5] data
			boost::uuids::uuid	//	[6] source
		>(frame);

		//
		//	search for requested class
		//
		std::size_t counter{ 0 };
		cache_.read_table("_Cluster", [&](cyng::store::table const* tbl_cluster)->void {
			tbl_cluster->loop([&](cyng::table::record const& rec) -> bool {

				//
				//	search class
				//
				//CYNG_LOG_TRACE(logger_, "search node class " << std::get<1>(tpl) << " ?= " << cyng::value_cast<std::string>(rec["class"], ""));
				if (boost::algorithm::equals(cyng::value_cast<std::string>(rec["class"], ""), std::get<1>(tpl)))
				{
					//
					//	required class found
					//
					CYNG_LOG_TRACE(logger_, "node class " 
						<< std::get<1>(tpl) 
						<< " found: "
						<< cyng::value_cast<>(rec["ep"], boost::asio::ip::tcp::endpoint()));

					auto peer = cyng::object_cast<session>(rec["self"]);
					if (peer)
					{
						counter++;
						peer->vm_.async_run(node::bus_req_push_data(std::get<0>(tpl), std::get<2>(tpl), std::get<4>(tpl), std::get<5>(tpl), std::get<6>(tpl)));
					}

					//
					//	single=false / all=true
					//
					return std::get<3>(tpl);
				}
				//	continue
				return true;
			});
		});

		ctx.queue(node::bus_res_push_data(std::get<0>(tpl)	//	seq
			, std::get<1>(tpl)
			, std::get<2>(tpl)
			, counter));
	}

	cyng::object make_session(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, cache& db
		, std::string const& account
		, std::string const& pwd
		, boost::uuids::uuid stag)
	{
		return cyng::make_object<session>(mux, logger, db, account, pwd, stag);
	}

}

namespace cyng
{
	namespace traits
	{
#if !defined(__CPP_SUPPORT_N2235)
		const char type_tag<node::session>::name[] = "session";
#endif
	}	// traits	

}


