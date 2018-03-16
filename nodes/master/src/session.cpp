/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 


#include "session.h"
#include "client.h"
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
		, std::chrono::seconds connection_open_timeout
		, std::chrono::seconds connection_close_timeout
		, bool connection_auto_login
		, bool connection_auto_enabled
		, bool connection_superseed)
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
		, connection_open_timeout_(connection_open_timeout)
		, connection_close_timeout_(connection_close_timeout)
		, seq_(0)
		, client_(mux, logger, db, connection_auto_login, connection_auto_enabled, connection_superseed)
		, subscriptions_()
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

		vm_.run(cyng::register_function("session.cleanup", 2, std::bind(&session::cleanup, this, std::placeholders::_1)));

		//
		//	register request handler
		//
		vm_.run(cyng::register_function("bus.req.login", 5, std::bind(&session::bus_req_login, this, std::placeholders::_1)));
		vm_.run(cyng::register_function("client.req.login", 8, std::bind(&session::client_req_login, this, std::placeholders::_1)));
		vm_.run(cyng::register_function("client.req.close", 2, std::bind(&session::client_req_close, this, std::placeholders::_1)));
		vm_.run(cyng::register_function("client.req.open.push.channel", 10, std::bind(&session::client_req_open_push_channel, this, std::placeholders::_1)));
		vm_.run(cyng::register_function("client.req.close.push.channel", 5, std::bind(&session::client_req_close_push_channel, this, std::placeholders::_1)));
		vm_.run(cyng::register_function("client.req.register.push.target", 5, std::bind(&session::client_req_register_push_target, this, std::placeholders::_1)));
		vm_.run(cyng::register_function("client.req.open.connection", 5, std::bind(&session::client_req_open_connection, this, std::placeholders::_1)));
		vm_.run(cyng::register_function("client.res.open.connection", 4, std::bind(&session::client_res_open_connection, this, std::placeholders::_1)));
		vm_.run(cyng::register_function("client.req.close.connection", 0, std::bind(&session::client_req_close_connection, this, std::placeholders::_1)));
		vm_.run(cyng::register_function("client.req.transfer.pushdata", 6, std::bind(&session::client_req_transfer_pushdata, this, std::placeholders::_1)));

		vm_.run(cyng::register_function("client.req.transmit.data", 5, std::bind(&session::client_req_transmit_data, this, std::placeholders::_1)));

		vm_.run(cyng::register_function("client.update.attr", 6, std::bind(&session::client_update_attr, this, std::placeholders::_1)));

		//
		//	ToDo: start maintenance task
		//
	}


	std::size_t session::hash() const noexcept
	{
		return vm_.hash();
	}

	void session::cleanup(cyng::context& ctx)
	{
		//	 [session.cleanup,[<!259:session>,asio.misc:2]]
		const cyng::vector_t frame = ctx.get_frame();
		ctx.attach(cyng::generate_invoke("log.msg.info", "session.cleanup", frame));

		//
		//	remove affected targets
		//
		db_.access([&](cyng::store::table* tbl_target)->void {
			const auto count = cyng::erase(tbl_target, get_targets_by_peer(tbl_target, this->hash()), ctx.tag());
			ctx.attach(cyng::generate_invoke("log.msg.info", "session.cleanup", count, "targets"));
		}, cyng::store::write_access("*Target"));

		//
		//	remove affected channels
		//
		db_.access([&](cyng::store::table* tbl_channel)->void {
			const auto count = cyng::erase(tbl_channel, get_channels_by_peer(tbl_channel, this->hash()), ctx.tag());
			ctx.attach(cyng::generate_invoke("log.msg.info", "session.cleanup", count, "channels"));
		}, cyng::store::write_access("*Channel"));

		//
		//	remove all open subscriptions
		//
		ctx.attach(cyng::generate_invoke("log.msg.trace", "close subscriptions", subscriptions_.size()));
		cyng::store::close_subscription(subscriptions_);
		
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
				ctx.attach(cyng::generate_invoke("log.msg.trace", "session.cleanup", this->hash(), phash));
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
			ctx.attach(cyng::generate_invoke("log.msg.info", "session.cleanup", pks.size()));
			cyng::erase(tbl_session, pks, ctx.tag());

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
			ctx.attach(cyng::register_function("bus.req.unsubscribe", 2, std::bind(&session::bus_req_unsubscribe, this, std::placeholders::_1)));

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

		auto const tpl = cyng::tuple_cast<
			std::string,			//	[0] table name
			boost::uuids::uuid,		//	[1] remote session tag
			std::size_t				//	[2] task id
		>(frame);

		ctx.attach(cyng::generate_invoke("log.msg.info", "bus.req.subscribe", std::get<0>(tpl), std::get<1>(tpl)));

		db_.access([&](cyng::store::table* tbl)->void {

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

	void session::sig_ins(cyng::store::table const* tbl
		, cyng::table::key_type const& key
		, cyng::table::data_type const& data
		, std::uint64_t gen
		, boost::uuids::uuid source)
	{
		vm_.async_run(cyng::generate_invoke("log.msg.debug", "sig.ins", source, tbl->meta().get_name()));

		//
		//	don't send data back to sender
		//
		if (source != vm_.tag())
		{
			vm_.async_run(bus_req_db_insert(tbl->meta().get_name()
				, key
				, data
				, gen
				, source));
		}
		else
		{
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
			vm_.async_run(bus_res_db_modify(tbl->meta().get_name(), key, attr, gen));
		}
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

	void session::client_req_close(cyng::context& ctx)
	{
		//	[d673954a-741d-41c1-944c-d0036be5353f,47e74fa2-1410-4f65-b4fb-b8bce05d8061,16,10054]
		//
		//	* remote client tag
		//	* remote peer tag
		//	* sequence number (should be continually incremented)
		//	* error code value
		//
		const cyng::vector_t frame = ctx.get_frame();
		//CYNG_LOG_INFO(logger_, "client.req.close " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] remote client tag
			boost::uuids::uuid,		//	[1] peer tag
			std::uint64_t,			//	[2] sequence number
			int						//	[3] error code
		>(frame);

		ctx.attach(client_.req_close(std::get<0>(tpl)
			, std::get<1>(tpl)
			, std::get<2>(tpl)
			, std::get<3>(tpl)));

	}

	void session::client_req_open_push_channel(cyng::context& ctx)
	{
		//	[00517ac4-e4cb-4746-83c9-51396043678a,0c2058e2-0c7d-4974-98bd-bf1b14c7ba39,12,LSM_Events,,,,,003c,%(("seq":52),("tp-layer":ipt))]
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

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] remote client tag
			boost::uuids::uuid,		//	[1] peer tag
			std::uint64_t,			//	[2] sequence number
			std::string,			//	[3] target name
			std::string,			//	[4] device name
			std::string,			//	[5] device number
			std::string,			//	[6] device software version
			std::string,			//	[7] device id
			std::chrono::seconds,	//	[8] timeout
			cyng::param_map_t		//	[9] bag
		>(frame);

		ctx.attach(client_.req_open_push_channel(std::get<0>(tpl)
			, std::get<1>(tpl)
			, std::get<2>(tpl)
			, std::get<3>(tpl)
			, std::get<4>(tpl)
			, std::get<5>(tpl)
			, std::get<6>(tpl)
			, std::get<7>(tpl)
			, std::get<8>(tpl)
			, std::get<9>(tpl)));
	}

	void session::client_req_close_push_channel(cyng::context& ctx)
	{
		//	[b7fcf460-84e4-493e-910e-2f9736774351,fbda6a25-d406-4d22-aca0-0ba3a8cd589a,682,2fd6e208,%(("seq":d7),("tp-layer":ipt))]
		//
		//	* session tag
		//	* peer
		//	* cluster sequence
		//	* channel
		//	* bag
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_INFO(logger_, "client.req.close.push.channel " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] remote client tag
			boost::uuids::uuid,		//	[1] peer tag
			std::uint64_t,			//	[2] sequence number
			std::uint32_t,			//	[3] channel
			cyng::param_map_t		//	[4] bag
		>(frame);

		ctx.attach(client_.req_close_push_channel(std::get<0>(tpl)
			, std::get<1>(tpl)
			, std::get<2>(tpl)
			, std::get<3>(tpl)
			, std::get<4>(tpl)));

	}

	void session::client_req_register_push_target(cyng::context& ctx)
	{
		//	[4e5dc7e8-37e7-4267-a3b6-d68a9425816f,76fbb814-3e74-419b-8d43-2b7b59dab7f1,67,data.sink.2,%(("pSize":65535),("seq":2),("tp-layer":ipt),("wSize":1))]
		//
		//	* client tag
		//	* peer
		//	* bus sequence
		//	* target
		//	* bag
		//
		const cyng::vector_t frame = ctx.get_frame();
		ctx.run(cyng::generate_invoke("log.msg.info", "client.req.register.push.target", frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] remote client tag
			boost::uuids::uuid,		//	[1] peer tag
			std::uint64_t,			//	[2] sequence number
			std::string,			//	[3] target name
			cyng::param_map_t		//	[4] bag
		>(frame);

		ctx.attach(client_.req_register_push_target(std::get<0>(tpl)
			, std::get<1>(tpl)
			, std::get<2>(tpl)
			, std::get<3>(tpl)
			, std::get<4>(tpl)));

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
		ctx.run(cyng::generate_invoke("log.msg.error", "client.req.close.connection - not implemented yet", frame));

	}

	void session::client_req_transfer_pushdata(cyng::context& ctx)
	{
		//	[89fd8b7b-ceae-4803-bdd9-e88dfa07cbf0,1bbcd541-5d5f-46fa-b6a8-2e8c5fa25d26,17,4ee4092b,f807b7df,%(("block":0),("seq":a3),("status":c1),("tp-layer":ipt)),1B1B1B1B010101017608343131323237...53B01EC46010163FA7D00]
		//
		//	* session tag
		//	* peer
		//	* cluster seq
		//	* channel
		//	* source
		//	* bag
		//	* data
		const cyng::vector_t frame = ctx.get_frame();
		//ctx.run(cyng::generate_invoke("log.msg.info", "client.req.transfer.pushdata", frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] origin client tag
			boost::uuids::uuid,		//	[1] peer tag
			std::uint64_t,			//	[2] sequence number
			std::uint32_t,			//	[3] channel
			std::uint32_t,			//	[4] source
			cyng::param_map_t		//	[5] bag
		>(frame);

		ctx.attach(client_.req_transfer_pushdata(std::get<0>(tpl)
			, std::get<1>(tpl)
			, std::get<2>(tpl)
			, std::get<3>(tpl)
			, std::get<4>(tpl)
			, std::get<5>(tpl)
			, frame.at(6)));

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
	
	void session::client_update_attr(cyng::context& ctx)
	{
		//	[a960e209-c2bb-427f-83a5-b0c0c2e75251,944d1f66-c6dd-4ed8-a96b-fbcc2d07b82a,13,software.version,0.2.2018030,%(("seq":2),("tp-layer":ipt))]
		//
		//	* session tag
		//	* peer
		//	* cluster seq
		//	* attribute name
		//	* value
		//	* bag
		const cyng::vector_t frame = ctx.get_frame();
		ctx.run(cyng::generate_invoke("log.msg.trace", "client.update.attr", frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] origin client tag
			boost::uuids::uuid,		//	[1] peer tag
			std::uint64_t,			//	[2] sequence number
			std::string,			//	[3] name
			cyng::param_map_t		//	[4] bag
		>(frame);

		ctx.attach(client_.update_attr(std::get<0>(tpl)
			, std::get<1>(tpl)
			, std::get<2>(tpl)
			, std::get<3>(tpl)
			, frame.at(5)
			, std::get<4>(tpl)));

	}

	cyng::object make_session(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, cyng::store::db& db
		, std::string const& account
		, std::string const& pwd
		, std::chrono::seconds connection_open_timeout
		, std::chrono::seconds connection_close_timeout
		, bool connection_auto_login
		, bool connection_auto_emabled
		, bool connection_superseed)
	{
		return cyng::make_object<session>(mux, logger, db, account, pwd
			, connection_open_timeout
			, connection_close_timeout
			, connection_auto_login
			, connection_auto_emabled
			, connection_superseed);
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


