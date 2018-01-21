/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 


#include "session.h"
#include <NODE_project_info.h>
#include <smf/cluster/generator.h>
#include <cyng/vm/domain/log_domain.h>
#include <cyng/vm/domain/store_domain.h>
#include <cyng/io/serializer.h>
#include <cyng/value_cast.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/nil_generator.hpp>

namespace node 
{
	session::session(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, cyng::store::db& db
		, std::string const& account
		, std::string const& pwd
		, std::chrono::seconds const& monitor)
	: mux_(mux)
		, logger_(logger)
		, db_(db)
		, vm_(mux.get_io_service(), boost::uuids::random_generator()())
		, parser_([this](cyng::vector_t&& prg) {
			CYNG_LOG_INFO(logger_, prg.size() << " instructions received");
			CYNG_LOG_TRACE(logger_, cyng::io::to_str(prg));
			vm_.async_run(std::move(prg));
		})
		, account_(account)
		, pwd_(pwd)
		, monitor_(monitor)
	{
		//
		//	register logger domain
		//
		cyng::register_logger(logger_, vm_);
		vm_.async_run(cyng::generate_invoke("log.msg.info", "log domain is running"));

		//
		//	register request handler
		//
		vm_.async_run(cyng::register_function("bus.req.login", 5, std::bind(&session::bus_req_login, this, std::placeholders::_1)));
		vm_.async_run(cyng::register_function("client.req.login", 7, std::bind(&session::client_req_login, this, std::placeholders::_1)));
		vm_.async_run(cyng::register_function("client.req.open.push.channel", 7, std::bind(&session::client_req_open_push_channel, this, std::placeholders::_1)));
		//
		//	ToDo: start maintenance task
		//
	}

	void session::bus_req_login(cyng::context& ctx)
	{
		//	[-1:00:0.000000,2018-01-08 01:41:18.88740470,setup,0.2,23ec874e-74cf-45cf-8dcf-f3daf720b92f,root,kKU0j1UT]
		const cyng::vector_t frame = ctx.get_frame();

		const std::string account = cyng::value_cast<std::string>(frame.at(0), "");
		const std::string pwd = cyng::value_cast<std::string>(frame.at(1), "");
		if ((account == account_) && (pwd == pwd_))
		{
			CYNG_LOG_INFO(logger_, "cluster member successful authorized with " << cyng::io::to_str(frame));

			//
			//	send reply
			//
			ctx.run(reply(frame, true));

			//
			//	register additional request handler
			//
			cyng::register_store(this->db_, ctx);

			//
			//	subscribe/unsubscribe
			//
			ctx.run(cyng::register_function("bus.req.subscribe", 1, std::bind(&session::bus_req_subscribe, this, std::placeholders::_1)));
			ctx.run(cyng::register_function("bus.req.unsubscribe", 1, std::bind(&session::bus_req_unsubscribe, this, std::placeholders::_1)));
		}
		else
		{
			CYNG_LOG_WARNING(logger_, "cluster member login failed: " << cyng::io::to_str(frame));
			ctx.run(reply(frame, false));

		}
	}

	void session::bus_req_subscribe(cyng::context& ctx)
	{
		//	[TDevice,2a88dd99-0592-4f4b-8a6c-251e1dbe49fe]
		const cyng::vector_t frame = ctx.get_frame();
		const std::string table = cyng::value_cast<std::string>(frame.at(0), "");
		CYNG_LOG_INFO(logger_, "subscribe " << table);

		db_.access([this, &ctx](const cyng::store::table* tbl)->void {

			CYNG_LOG_INFO(logger_, tbl->meta().get_name() << "->size(" << tbl->size() << ")");
			tbl->loop([this, tbl, &ctx](cyng::table::record const& rec) {

				//CYNG_LOG_TRACE(logger_, cyng::io::to_str(rec[0]));
				ctx.run(bus_db_insert(tbl->meta().get_name()
					, rec.key()
					, rec.data()
					, rec.get_generation()));
			});

		}, cyng::store::read_access(table));

	}

	void session::bus_req_unsubscribe(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_INFO(logger_, "unsubscribe " << cyng::io::to_str(frame));
	}

	cyng::vector_t session::reply(cyng::vector_t const& frame, bool success)
	{
		cyng::vector_t prg;
		return prg	<< cyng::generate_invoke("stream.serialize"
			, success
			, cyng::code::IDENT
			, cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR)
			, frame.at(5)	//	timestamp of sender
			, cyng::make_object(std::chrono::system_clock::now())
			, cyng::invoke_remote("bus.res.login"))
			<< cyng::generate_invoke("stream.flush")
			;
	}


	void session::client_req_login(cyng::context& ctx)
	{
		//	[7171b8b7-1710-47ab-bd04-83abc9f6234d,6e53a173-6fed-4607-81eb-74ac46bab5e2,1,LSMTest5,LSMTest5,plain,("tp-layer":ipt)]
		//
		//	* client tag
		//	* peer tag
		//	* sequence number (should be continually incremented)
		//	* name
		//	* pwd/credentials
		//	* authorization scheme
		//	* bag (to send back)
		//
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_INFO(logger_, "client.req.login " << cyng::io::to_str(frame));

		const boost::uuids::uuid tag = cyng::value_cast(frame.at(0), boost::uuids::nil_uuid());
		const std::uint64_t seq = cyng::value_cast<std::uint64_t>(frame.at(2), 0);
		const std::string account = cyng::value_cast<std::string>(frame.at(3), "");
		const std::string pwd = cyng::value_cast<std::string>(frame.at(4), "");
		const std::string scheme = cyng::value_cast<std::string>(frame.at(5), "plain");

		cyng::param_map_t bag;
		bag = cyng::value_cast(frame.at(6), bag);

		//
		//	ToDo: test 
		//

		ctx.run(client_res_login(tag
			, seq
			, true
			, "unknown device"
			, bag));
	}

	void session::client_req_open_push_channel(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_INFO(logger_, "client.req.open.push.channel " << cyng::io::to_str(frame));

	}

}



