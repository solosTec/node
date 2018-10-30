/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 


#include "session.h"
#include "client.h"
#include "db.h"
#include "tasks/watchdog.h"
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
		, boost::uuids::uuid mtag // master tag
		, cyng::store::db& db
		, std::string const& account
		, std::string const& pwd
		, boost::uuids::uuid stag
		, std::chrono::seconds monitor
		, std::atomic<std::uint64_t>& global_configuration
		, boost::filesystem::path stat_dir )
	: mux_(mux)
		, logger_(logger)
		, mtag_(mtag)
		, db_(db)
		, vm_(mux.get_io_service(), stag)
		, parser_([this](cyng::vector_t&& prg) {
			CYNG_LOG_TRACE(logger_, prg.size() 
				<< " instructions received (including "
				<< cyng::op_counter(prg, cyng::code::INVOKE)
				<< " invoke(s))");
			CYNG_LOG_DEBUG(logger_, "exec: " << cyng::io::to_str(prg));
			vm_.async_run(std::move(prg));
		})
		, account_(account)
		, pwd_(pwd)
		, cluster_monitor_(monitor)
		, seq_(0)
		, client_(mux, logger, db, global_configuration, stat_dir)
		, cluster_(mux, logger, db)
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
		//	register request handler
		//
		vm_.register_function("bus.req.login", 11, std::bind(&session::bus_req_login, this, std::placeholders::_1));
		vm_.register_function("bus.req.stop.client", 3, std::bind(&session::bus_req_stop_client_impl, this, std::placeholders::_1));
		vm_.register_function("bus.insert.msg", 2, std::bind(&session::bus_insert_msg, this, std::placeholders::_1));
		vm_.register_function("bus.req.push.data", 7, std::bind(&session::bus_req_push_data, this, std::placeholders::_1));

		//
		//	statistical data
		//
		vm_.async_run(cyng::generate_invoke("log.msg.info", cyng::invoke("lib.size"), "callbacks registered"));
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
		db_.access([&](cyng::store::table* tbl_target)->void {
			const auto count = cyng::erase(tbl_target, get_targets_by_peer(tbl_target, ctx.tag()), ctx.tag());
			CYNG_LOG_WARNING(logger_, "cluster member "
				<< ctx.tag()
				<< " removed "
				<< count
				<< " targets");
		}, cyng::store::write_access("_Target"));

		//
		//	remove affected channels.
		//	Channels are 2-way: There is an owner session and a target session. Both could be different.
		//
		db_.access([&](cyng::store::table* tbl_channel)->void {
			const auto count = cyng::erase(tbl_channel, get_channels_by_peer(tbl_channel, ctx.tag()), ctx.tag());
			CYNG_LOG_WARNING(logger_, "cluster member "
				<< ctx.tag()
				<< " removed "
				<< count
				<< " channels");
		}, cyng::store::write_access("_Channel"));

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
		//	remove all client sessions of this node and close open connections
		//
		db_.access([&](cyng::store::table* tbl_session, cyng::store::table* tbl_connection, const cyng::store::table* tbl_device)->void {
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

							ctx.attach(cyng::generate_invoke("log.msg.warning"
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
						//	remove from connection table
						//
						connection_erase(tbl_connection, cyng::table::key_generator(tag, rtag), tag);
					}

				}

				//
				//	generate statistics
				//	There is an error in ec serialization/deserialization
				//	
				auto ec = cyng::value_cast(frame.at(1), boost::system::error_code());
				client_.write_stat(tag, account, "shutdown node", ec.message());

				//	continue
				return true;
			});

			//
			//	remove all session records
			//
			//ctx.attach(cyng::generate_invoke("log.msg.info", "session.cleanup", pks.size()));
			cyng::erase(tbl_session, pks, ctx.tag());

		}	, cyng::store::write_access("_Session")
			, cyng::store::write_access("_Connection")
			, cyng::store::read_access("TDevice"));

		//
		//	cluster table
		//
		std::string node_class;
		db_.access([&](cyng::store::table* tbl_cluster)->void {

			//
			//	get node class
			//
			const auto key = cyng::table::key_generator(cluster_tag_);
			const auto rec = tbl_cluster->lookup(key);
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
				tbl_cluster->erase(key, ctx.tag());
			}


			//
			//	update master record
			//
			tbl_cluster->modify(cyng::table::key_generator(mtag_), cyng::param_factory("clients", tbl_cluster->size()), ctx.tag());
			

		} , cyng::store::write_access("_Cluster"));

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
		insert_msg(db_
			, cyng::logging::severity::LEVEL_WARNING
			, ss.str()
			, ctx.tag());

	}

	void session::bus_req_login(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		//CYNG_LOG_INFO(logger_, "bus.req.login " << cyng::io::to_str(frame));

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
				std::string,				//	[10] platform
				boost::process::pid_t		//	[11] process id
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
		, boost::process::pid_t pid)
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

			//
			//	register additional request handler for database access
			//

			cyng::register_store(this->db_, ctx);

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
			ctx.attach(cyng::register_function("bus.req.subscribe", 3, std::bind(&session::bus_req_subscribe, this, std::placeholders::_1)));
			ctx.attach(cyng::register_function("bus.req.unsubscribe", 2, std::bind(&session::bus_req_unsubscribe, this, std::placeholders::_1)));
			ctx.attach(cyng::register_function("bus.start.watchdog", 7, std::bind(&session::bus_start_watchdog, this, std::placeholders::_1)));

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
			ctx.attach(cyng::generate_invoke("bus.start.watchdog"
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
			ctx.attach(reply(ts, true));

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
			ctx.attach(reply(ts, false));

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
			insert_msg(db_
				, cyng::logging::severity::LEVEL_ERROR
				, ss.str()
				, ctx.tag());

		}
	}

	void session::bus_req_stop_client_impl(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_INFO(logger_, "bus.req.stop.client " << cyng::io::to_str(frame));

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

		//	stop a client session
		//
		db_.access([&](const cyng::store::table* tbl_session)->void {
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
		}, cyng::store::read_access("_Session"));
	}


	void session::bus_start_watchdog(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_INFO(logger_, "bus.start.watchdog " << cyng::io::to_str(frame));

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
		//
		//	cluster table
		//
		db_.access([&](cyng::store::table* tbl_cluster)->void {

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
				tbl_cluster->modify(cyng::table::key_generator(mtag_), cyng::param_factory("clients", tbl_cluster->size()), ctx.tag());
			}

		}, cyng::store::write_access("_Cluster"));

		//
		//	build message strings
		//
		std::stringstream ss;

		//
		//	start watchdog task
		//
		if (cluster_monitor_.count() > 5)
		{
			tsk_watchdog_ = cyng::async::start_task_delayed<watchdog>(mux_, std::chrono::seconds(30)
				, logger_
				, db_
				, frame.at(4)
				, cluster_monitor_).first;

			ss
				<< "start watchdog task #"
				<< tsk_watchdog_
				<< " for cluster member "
				<< cyng::value_cast<std::string>(frame.at(0), "")
				<< ':'
				<< ctx.tag()
				<< " with "
				<< cluster_monitor_.count()
				<< " seconds"
				;
			insert_msg(db_
				, cyng::logging::severity::LEVEL_INFO
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
				<< cluster_monitor_.count()
				<< " second(s)"
				;
			insert_msg(db_
				, cyng::logging::severity::LEVEL_WARNING
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
		insert_msg(db_
			, cyng::logging::severity::LEVEL_INFO
			, ss.str()
			, ctx.tag());


	}

	void session::res_watchdog(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		//CYNG_LOG_INFO(logger_, "bus.res.watchdog " << cyng::io::to_str(frame));

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

		db_.modify("_Cluster"
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
		if (boost::algorithm::equals(tbl->meta().get_name(), "TDevice"))
		{
			vm_.async_run(cyng::generate_invoke("log.msg.debug"	
				, "sig.ins"
				, source
				, tbl->meta().get_name()
				, ((source != vm_.tag()) ? "req" : "res")
				, key
				, data
			));
		}
		else
		{
			vm_.async_run(cyng::generate_invoke("log.msg.debug"
				, "sig.ins"
				, source
				, tbl->meta().get_name()
				, ((source != vm_.tag()) ? "req" : "res")));
		}
#else
		vm_.async_run(cyng::generate_invoke("log.msg.debug", "sig.ins", source, tbl->meta().get_name()));
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
		vm_.async_run(cyng::generate_invoke("log.msg.debug", "sig.del", tbl->meta().get_name(), source));

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
			vm_.async_run(cyng::generate_invoke("log.msg.debug", "sig.clr", tbl->meta().get_name(), source));
			vm_.async_run(bus_db_clear(tbl->meta().get_name()));
		}
	}

	void session::sig_mod(cyng::store::table const* tbl
		, cyng::table::key_type const& key
		, cyng::attr_t const& attr
		, std::uint64_t gen
		, boost::uuids::uuid source)
	{
		vm_.async_run(cyng::generate_invoke("log.msg.debug", "sig.mod", tbl->meta().get_name(), source));

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
				if (!key.empty() && boost::algorithm::equals(cyng::value_cast<std::string>(key.at(0), "?"), "connection-auto-login"))
				{
					client_.set_connection_auto_login(attr.second);
				}
				else if (!key.empty() && boost::algorithm::equals(cyng::value_cast<std::string>(key.at(0), "?"), "connection-auto-enabled"))
				{
					client_.set_connection_auto_enabled(attr.second);
				}
				else if (!key.empty() && boost::algorithm::equals(cyng::value_cast<std::string>(key.at(0), "?"), "connection-superseed"))
				{
					client_.set_connection_superseed(attr.second);
				}
				else if (!key.empty() && boost::algorithm::equals(cyng::value_cast<std::string>(key.at(0), "?"), "generate-time-series"))
				{
					client_.set_generate_time_series(attr.second);

					//
					//	write a line	
					//
					//
					if (client_.is_generate_time_series()) {
						db_.access([&](cyng::store::table const* tbl_session)->void {

							const auto size = tbl_session->size();
							tbl_session->loop([&](cyng::table::record const& rec) -> bool {
								const auto tag = cyng::value_cast(rec["tag"], boost::uuids::nil_uuid());
								const auto name = cyng::value_cast<std::string>(rec["name"], "");
								auto local_peer = cyng::object_cast<session>(rec["local"]);

								if (local_peer != nullptr) {
									const_cast<session*>(local_peer)->client_.write_stat(tag, name, "start recording...", size);
								}
								return true;
							});
						}, cyng::store::read_access("_Session"));
					}
				}
			}
		}
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

		ctx.attach(cyng::generate_invoke("log.msg.info", "bus.req.subscribe", std::get<0>(tpl), std::get<1>(tpl)));

		db_.access([&](cyng::store::table const* tbl)->void {

			CYNG_LOG_INFO(logger_, tbl->meta().get_name() << "->size(" << tbl->size() << ")");
			tbl->loop([&](cyng::table::record const& rec) -> bool {

				ctx.run(bus_res_subscribe(tbl->meta().get_name()
					, rec.key()
					, rec.data()
					, rec.get_generation()
					, std::get<1>(tpl)		//	session tag
					, std::get<2>(tpl)));	//	task id (optional)

				//	continue loop
				return true;
			});

		}, cyng::store::read_access(std::get<0>(tpl)));

		db_.access([&](cyng::store::table* tbl)->void {
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
		}, cyng::store::write_access(std::get<0>(tpl)));

	}

	void session::bus_req_unsubscribe(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_INFO(logger_, "unsubscribe " << cyng::io::to_str(frame));

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
		ctx.run(cyng::generate_invoke("log.msg.trace", "client.insert.msg", frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,			//	[0] origin client tag
			cyng::logging::severity,	//	[1] level
			std::string					//	[2] msg
		>(frame);

		insert_msg(db_
			, std::get<1>(tpl)
			, std::get<2>(tpl)
			, ctx.tag());
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
		ctx.run(cyng::generate_invoke("log.msg.trace", "bus.req.push.data", frame));

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
		db_.access([&](cyng::store::table const* tbl_cluster)->void {
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
		}, cyng::store::read_access("_Cluster"));

		ctx.attach(node::bus_res_push_data(std::get<0>(tpl)	//	seq
			, std::get<1>(tpl)
			, std::get<2>(tpl)
			, counter));
	}

	cyng::object make_session(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, boost::uuids::uuid mtag
		, cyng::store::db& db
		, std::string const& account
		, std::string const& pwd
		, boost::uuids::uuid stag
		, std::chrono::seconds monitor //	cluster watchdog
		, std::atomic<std::uint64_t>& global_configuration
		, boost::filesystem::path stat_dir)
	{
		return cyng::make_object<session>(mux, logger, mtag, db, account, pwd, stag, monitor
			, global_configuration, stat_dir);
	}

}

namespace cyng
{
	namespace traits
	{

#if defined(CYNG_LEGACY_MODE_ON)
		const char type_tag<node::session>::name[] = "session";
#endif
	}	// traits	

}


