/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 


#include "session.h"
#include <NODE_project_info.h>
#include <smf/cluster/generator.h>
#include <smf/ipt/response.hpp>
#include <cyng/vm/domain/log_domain.h>
#include <cyng/vm/domain/store_domain.h>
#include <cyng/io/serializer.h>
#include <cyng/value_cast.hpp>
#include <cyng/object_cast.hpp>
#include <cyng/dom/reader.h>
#include <cyng/tuple_cast.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/algorithm/string/predicate.hpp>

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
		, tag_(boost::uuids::random_generator()())
		, vm_(mux.get_io_service(), tag_)
		, parser_([this](cyng::vector_t&& prg) {
			CYNG_LOG_INFO(logger_, prg.size() << " instructions received");
			CYNG_LOG_TRACE(logger_, cyng::io::to_str(prg));
			vm_.async_run(std::move(prg));
		})
		, account_(account)
		, pwd_(pwd)
		, monitor_(monitor)
		, rng_()
		, distribution_(std::numeric_limits<std::uint32_t>::min(), std::numeric_limits<std::uint32_t>::max())
		, seq_(0)
		, client_(mux, logger, db)
	{
		//
		//	register logger domain
		//
		cyng::register_logger(logger_, vm_);
		vm_.run(cyng::generate_invoke("log.msg.info", "log domain is running"));

		//
		//	increase sequence and set as result value
		//
		vm_.run(cyng::register_function("bus.seq.next", 0, [this](cyng::context& ctx) {
			++this->seq_;
			ctx.push(cyng::make_object(this->seq_));
		}));

		vm_.async_run(cyng::register_function("session.cleanup", 2, std::bind(&session::cleanup, this, std::placeholders::_1)));

		//
		//	register request handler
		//
		vm_.async_run(cyng::register_function("bus.req.login", 5, std::bind(&session::bus_req_login, this, std::placeholders::_1)));
		vm_.async_run(cyng::register_function("client.req.login", 8, std::bind(&session::client_req_login, this, std::placeholders::_1)));
		vm_.async_run(cyng::register_function("client.req.open.push.channel", 10, std::bind(&session::client_req_open_push_channel, this, std::placeholders::_1)));
		vm_.async_run(cyng::register_function("client.req.register.push.target", 5, std::bind(&session::client_req_register_push_target, this, std::placeholders::_1)));
		vm_.async_run(cyng::register_function("client.req.open.connection", 5, std::bind(&session::client_req_open_connection, this, std::placeholders::_1)));
		vm_.async_run(cyng::register_function("client.res.open.connection", 4, std::bind(&session::client_res_open_connection, this, std::placeholders::_1)));
		vm_.async_run(cyng::register_function("client.req.close.connection", 0, std::bind(&session::client_req_close_connection, this, std::placeholders::_1)));

		vm_.async_run(cyng::register_function("client.req.transmit.data", 5, std::bind(&session::client_req_transmit_data, this, std::placeholders::_1)));

		//
		//	ToDo: start maintenance task
		//
	}


	std::size_t session::hash() const noexcept
	{
		boost::hash<boost::uuids::uuid> uuid_hasher;
		return uuid_hasher(tag_);
	}

	void session::cleanup(cyng::context& ctx)
	{
		//	 [session.cleanup,[<!259:session>,asio.misc:2]]
		const cyng::vector_t frame = ctx.get_frame();
		ctx.run(cyng::generate_invoke("log.msg.info", "session.cleanup", frame));

		//
		//	hold reference of this session
		//
		auto session_ref = cyng::object_cast<session>(frame.at(0));

		//	remove all client sessions of this node
		db_.access([&](cyng::store::table* tbl_session)->void {
			cyng::table::key_list_t pks;
			tbl_session->loop([&](cyng::table::record const& rec) -> bool {

				//
				//	build list with all affected session records
				//
				auto phash = cyng::object_cast<session>(rec["local"])->hash();
				ctx.run(cyng::generate_invoke("log.msg.trace", "session.cleanup", this->hash(), phash));
				this->hash();
				if (this->hash() == phash)
				{
					pks.push_back(rec.key());
				}
				//	continue
				return true;
			});

			//
			//	remove all session records
			//
			ctx.run(cyng::generate_invoke("log.msg.info", "session.cleanup", pks.size()));
			for (auto pk : pks)
			{
				tbl_session->erase(pk);
			}
		}, cyng::store::write_access("*Session"));
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
			//	register additional request handler
			//
			cyng::register_store(this->db_, ctx);

			//
			//	subscribe/unsubscribe
			//
			ctx.attach(cyng::register_function("bus.req.subscribe", 3, std::bind(&session::bus_req_subscribe, this, std::placeholders::_1)));
			ctx.attach(cyng::register_function("bus.req.unsubscribe", 1, std::bind(&session::bus_req_unsubscribe, this, std::placeholders::_1)));

			//
			//	send reply
			//
			ctx.attach(reply(frame, true));

		}
		else
		{
			CYNG_LOG_WARNING(logger_, "cluster member login failed: " << cyng::io::to_str(frame));
			ctx.attach(reply(frame, false));

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
		CYNG_LOG_INFO(logger_, "subscribe (1)" << cyng::io::to_str(frame));
		const std::string table = cyng::value_cast<std::string>(frame.at(0), "");
		const boost::uuids::uuid tag = cyng::value_cast(frame.at(1), boost::uuids::nil_uuid());
		const std::size_t tsk = cyng::value_cast(frame.at(2), static_cast<std::size_t>(0));
		CYNG_LOG_INFO(logger_, "subscribe " << table);

		db_.access([this, &ctx, tag, tsk](const cyng::store::table* tbl)->void {

			CYNG_LOG_INFO(logger_, tbl->meta().get_name() << "->size(" << tbl->size() << ")");
			tbl->loop([this, tbl, &ctx, tag, tsk](cyng::table::record const& rec) -> bool {

				//CYNG_LOG_TRACE(logger_, cyng::io::to_str(rec[0]));
				ctx.run(bus_db_insert(tbl->meta().get_name()
					, rec.key()
					, rec.data()
					, rec.get_generation()
					, tag
					, tsk));

				//	continue loop
				return true;
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
		return prg	<< cyng::generate_invoke_unwinded("stream.serialize"
			, success
			, cyng::code::IDENT
			, cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR)
			, frame.at(5)	//	timestamp of sender
			, cyng::make_object(std::chrono::system_clock::now())
			, cyng::invoke_remote("bus.res.login"))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}


	void session::client_req_login(cyng::context& ctx)
	{
		//	[1cea6ddf-5044-478b-9fba-67fa23997ba6,65d9eb67-2187-481b-8770-5ce41801eaa6,1,data-store,secret,plain,%(("security":scrambled),("tp-layer":ipt)),<!259:session>]
		//
		//	* remote client tag
		//	* remote peer tag
		//	* sequence number (should be continually incremented)
		//	* name
		//	* pwd/credentials
		//	* authorization scheme
		//	* bag (to send back)
		//	* session object
		//
		const cyng::vector_t frame = ctx.get_frame();
		//CYNG_LOG_INFO(logger_, "client.req.login " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] remote client tag
			boost::uuids::uuid,		//	[1] peer tag
			std::uint64_t,			//	[2] sequence number
			std::string,			//	[3] name
			std::string,			//	[4] pwd/credential
			std::string,			//	[5] authorization scheme
			cyng::param_map_t		//	[6] bag
		>(frame);

		ctx.attach(client_.req_login(std::get<0>(tpl)
			, std::get<1>(tpl)
			, std::get<2>(tpl)
			, std::get<3>(tpl)
			, std::get<4>(tpl)
			, std::get<5>(tpl)
			, std::get<6>(tpl)
			, frame.at(7)));
	}

	void session::client_req_open_push_channel(cyng::context& ctx)
	{
		//	[00517ac4-e4cb-4746-83c9-51396043678a,0c2058e2-0c7d-4974-98bd-bf1b14c7ba39,12,LSM_Events,,,,,60,%(("seq":52),("tp-layer":ipt))]
		//
		//	* client tag
		//	* peer
		//	* bus sequence
		//	* target name
		//	* device name
		//	* device number
		//	* device software version
		//	* device id
		//	* timeout
		//	* bag
		//
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_INFO(logger_, "client.req.open.push.channel " << cyng::io::to_str(frame));

		const boost::uuids::uuid tag = cyng::value_cast(frame.at(0), boost::uuids::nil_uuid());
		const std::uint64_t seq = cyng::value_cast<std::uint64_t>(frame.at(2), 0);
		const std::string target = cyng::value_cast<std::string>(frame.at(3), "");

		cyng::param_map_t options;
		auto bag = cyng::value_cast(frame.at(9), options);

		options["response-code"] = cyng::make_object<std::uint8_t>(ipt::tp_res_close_push_channel_policy::UNREACHABLE);
		options["channel-status"] = cyng::make_object<std::uint8_t>(0);
		options["packet-size"] = cyng::make_object<std::uint16_t>(0);
		options["window-size"] = cyng::make_object<std::uint8_t>(0);

		ctx.attach(client_res_open_push_channel(tag
			, seq
			, false
			, 0		//	channel
			, 0		//	source
			, 0		//	count
			, options
			, bag));
	}

	void session::client_req_register_push_target(cyng::context& ctx)
	{
		//	[4e5dc7e8-37e7-4267-a3b6-d68a9425816f,76fbb814-3e74-419b-8d43-2b7b59dab7f1,67,data.sink.2,%(("p-size":65535),("seq":2),("tp-layer":ipt),("w-size":1))]
		//
		//	* client tag
		//	* peer
		//	* bus sequence
		//	* target
		//	* bag
		//
		const cyng::vector_t frame = ctx.get_frame();
		ctx.run(cyng::generate_invoke("log.msg.info", "client.req.register.push.target", frame));

		const boost::uuids::uuid tag = cyng::value_cast(frame.at(0), boost::uuids::nil_uuid()); 
		const std::uint64_t seq = cyng::value_cast<std::uint64_t>(frame.at(2), 0);
		cyng::param_map_t options;
		auto bag = cyng::value_cast(frame.at(4), options);

		bool success{ false };
		const auto channel = distribution_(rng_);
		db_.access([&](const cyng::store::table* tbl_session, cyng::store::table* tbl_target)->void {

			auto session_rec = tbl_session->lookup(cyng::table::key_generator(tag));
			if (!session_rec.empty())
			{
				//
				//	session found - insert new target
				//
				auto dom = cyng::make_reader(frame);
				const boost::uuids::uuid peer = cyng::value_cast(dom.get(1), boost::uuids::nil_uuid());
				const std::string target = cyng::value_cast<std::string>(dom.get(3), "");
				const std::uint16_t p_size = cyng::value_cast<std::uint16_t>(dom[4].get("p-size"), 0xffff);
				const std::uint8_t w_size = cyng::value_cast<std::uint8_t>(dom[4].get("w-size"), 1);

				success = tbl_target->insert(cyng::table::key_generator(target, tag)
					, cyng::table::data_generator(peer
						, channel	//	channel
						, session_rec["device"]	//	owner of target
						, p_size
						, w_size
						, std::chrono::system_clock::now()	//	reg-time
						, static_cast<std::uint64_t>(0))
					, 1);
			}
			else
			{
				CYNG_LOG_FATAL(logger_, "client.req.open.push.channel - session "
					<< tag
					<< " not found");
			}

		}	, cyng::store::read_access("*Session")
			, cyng::store::write_access("*Target"));

		options["response-code"] = cyng::make_object<std::uint8_t>(success
			?  ipt::ctrl_res_register_target_policy::OK
			: ipt::ctrl_res_register_target_policy::GENERAL_ERROR);
		options["target-name"] = frame.at(3);

		ctx.attach(client_res_register_push_target(tag
			, seq
			, success
			, channel		//	channel
			, options
			, bag));
	}

	void session::client_req_open_connection(cyng::context& ctx)
	{
		//	[client.req.open.connection,[29feef99-3f33-4c90-b0db-8c2c50d3d098,90732ba2-1c8a-4587-a19f-caf27752c65a,3,LSMTest1,%(("seq":1),("tp-layer":ipt))]]
		//
		//	* client tag
		//	* peer
		//	* bus sequence
		//	* number
		//	* bag
		//
		const cyng::vector_t frame = ctx.get_frame();
		ctx.run(cyng::generate_invoke("log.msg.info", "client.req.open.connection", frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] remote client tag
			boost::uuids::uuid,		//	[1] peer tag
			std::uint64_t,			//	[2] sequence number
			std::string,			//	[3] number
			cyng::param_map_t		//	[4] bag
		>(frame);

		ctx.attach(client_.req_open_connection(std::get<0>(tpl)
			, std::get<1>(tpl)
			, std::get<2>(tpl)
			, std::get<3>(tpl)
			, std::get<4>(tpl)));

	}

	void session::client_res_open_connection(cyng::context& ctx)
	{
		//	[client.res.open.connection,
		//	[232e356c-7188-4c2b-bc4b-ee3247061904,b81f32c5-eecd-43f2-9183-cacc255ad03b,0,false,
		//	%(("seq":1),("tp-layer":ipt))]]
		//
		//	* origin session tag
		//	* peer (origin)
		//	* cluster sequence
		//	* success
		//	* options
		//	* bag (origin)
		const cyng::vector_t frame = ctx.get_frame();
		//ctx.run(cyng::generate_invoke("log.msg.info", "client.res.open.connection", frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] origin client tag
			boost::uuids::uuid,		//	[1] peer tag
			std::uint64_t,			//	[2] sequence number
			bool,					//	[3] success
			cyng::param_map_t,		//	[4] options
			cyng::param_map_t		//	[5] bag
		>(frame);

		ctx.attach(client_.res_open_connection(std::get<0>(tpl)
			, std::get<1>(tpl)
			, std::get<2>(tpl)
			, std::get<3>(tpl)
			, std::get<4>(tpl)
			, std::get<5>(tpl)));

	}

	void session::client_req_close_connection(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		ctx.run(cyng::generate_invoke("log.msg.info", "client.req.close.connection", frame));

	}

	void session::client_req_transmit_data(cyng::context& ctx)
	{
		//	 [client.req.transmit.data,
		//	[540a2bda-5891-44a6-909b-89bfdc494a02,767a51e3-ded3-4eb3-bc61-8bfd931afc75,7,
		//	%(("tp-layer":ipt))
		//	&&1B1B1B1B010101017681063138...8C000000001B1B1B1B1A03E6DD]]
		const cyng::vector_t frame = ctx.get_frame();
		ctx.run(cyng::generate_invoke("log.msg.trace", "client.req.transmit.data", frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] origin client tag
			boost::uuids::uuid,		//	[1] peer tag
			std::uint64_t,			//	[2] sequence number
			cyng::param_map_t		//	[3] bag
		>(frame);

		client_.req_transmit_data(std::get<0>(tpl)
			, std::get<1>(tpl)
			, std::get<2>(tpl)
			, std::get<3>(tpl)
			, frame.at(4));

	}
	
	cyng::object make_session(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, cyng::store::db& db
		, std::string const& account
		, std::string const& pwd
		, std::chrono::seconds const& monitor)
	{
		return cyng::make_object<session>(mux, logger, db, account, pwd, monitor);
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


