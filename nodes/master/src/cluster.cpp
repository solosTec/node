/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 


#include "cluster.h"
#include "session.h"
#include "db.h"
#include <NODE_project_info.h>
#include <smf/cluster/generator.h>

#include <cyng/tuple_cast.hpp>
#include <cyng/parser/buffer_parser.h>
#include <cyng/dom/algorithm.h>
#include <cyng/io/io_buffer.h>

#include <boost/uuid/nil_generator.hpp>

namespace node 
{
	cluster::cluster(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, cyng::store::db& db
		, std::atomic<std::uint64_t>& global_configuration)
	: mux_(mux)
		, logger_(logger)
		, db_(db)
		, global_configuration_(global_configuration)
		, uidgen_()
	{}

	void cluster::register_this(cyng::context& ctx)
	{
		ctx.queue(cyng::register_function("bus.req.gateway.proxy", 7, std::bind(&cluster::bus_req_gateway_proxy, this, std::placeholders::_1)));
		ctx.queue(cyng::register_function("bus.res.gateway.proxy", 9, std::bind(&cluster::bus_res_gateway_proxy, this, std::placeholders::_1)));
		ctx.queue(cyng::register_function("bus.res.attention.code", 7, std::bind(&cluster::bus_res_attention_code, this, std::placeholders::_1)));
	}

	void cluster::bus_req_gateway_proxy(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_INFO(logger_, "bus.req.gateway.proxy " << cyng::io::to_str(frame));
		
		//	[]
		//
		//	* source
		//	* bus sequence
		//	* TGateway/TDevice key
		//	* web-socket tag
		//	* channel
		//	* sections
		//	* params

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] source
			std::uint64_t,			//	[1] sequence
			cyng::vector_t,			//	[2] TGateway/TDevice key
			boost::uuids::uuid,		//	[3] websocket tag
			std::string,			//	[4] channel
			cyng::vector_t,			//	[5] sections (strings)
			cyng::vector_t			//	[6] requests (strings)
		>(frame);

		//
		//	test TGateway key
		//
		BOOST_ASSERT(!std::get<2>(tpl).empty());
		
		//
		//	send process parameter request to an gateway
		//
		db_.access([&](const cyng::store::table* tbl_session, const cyng::store::table* tbl_gw)->void {

			//
			//	Since TGateway and TDevice share the same primary key, we can search for a session 
			//	of that device/gateway. Additionally we get the required access data like
			//	server ID, user and password.
			//
			const auto[peer, rec, server, tag] = find_peer(std::get<2>(tpl), tbl_session, tbl_gw);

			if (peer != nullptr && !rec.empty()) {

				const auto name = cyng::value_cast<std::string>(rec["userName"], "operator");
				const auto pwd = cyng::value_cast<std::string>(rec["userPwd"], "operator");

				peer->vm_.async_run(node::client_req_gateway_proxy(tag	//	ipt session tag
					, std::get<0>(tpl)	//	source tag
					, std::get<1>(tpl)	//	cluster sequence
					, std::get<2>(tpl)	//	TGateway key
					, std::get<3>(tpl)	//	websocket tag
					, std::get<4>(tpl)	//	channel
					, std::get<5>(tpl)	//	sections
					, std::get<6>(tpl)	//	requests
					, server			//	server
					, name
					, pwd));
			}

		}	, cyng::store::read_access("_Session")
			, cyng::store::read_access("TGateway"));

	}

	void cluster::bus_res_gateway_proxy(cyng::context& ctx)
	{
		//	[73285cb1-45bb-4dcb-bae5-363ed6632624,9f773865-e4af-489a-8824-8f78a2311278,20,[df735c77-797f-4ce8-bb74-86280f9884a9],595a3a58-2ac4-4946-a265-93324706491c,
		//	get.proc.param,
		//	[root-ipt-param,root-ipt-state,op-log-status-word],
		//	00:15:3b:02:29:80,
		//	root-ipt-param,
		//	%(("address":192.168.1.21),("idx":1),("local":68ee),("name":LSMTest1),("pwd":LSMTest1),("remote":0000))]
		//
		//	* [uuid] ident tag
		//	* [uuid] source tag
		//	* [u64] cluster seq
		//	* [vec] gateway key
		//	* [uuid] web-socket tag
		//	* [str] channel
		//	* [str] server id
		//	* [str] "OBIS code" as text
		//	* [param_map_t] params
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_INFO(logger_, "bus.res.gateway.proxy " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] ident
			boost::uuids::uuid,		//	[1] source
			std::uint64_t,			//	[2] sequence
			cyng::vector_t,			//	[3] gw key
			boost::uuids::uuid,		//	[4] websocket tag
			std::string,			//	[5] channel
			std::string,			//	[6] server id
			std::string,			//	[7] "OBIS code" as text
			cyng::param_map_t		//	[8] params
		>(frame);

		//
		//	test TGateway key
		//
		BOOST_ASSERT(!std::get<3>(tpl).empty());

		//
		//	routing back
		//
		db_.access([&](const cyng::store::table* tbl_cluster)->void {

			auto rec = tbl_cluster->lookup(cyng::table::key_generator(std::get<1>(tpl)));
			if (!rec.empty()) {
				auto peer = cyng::object_cast<session>(rec["self"]);
				if (peer != nullptr) {

					CYNG_LOG_INFO(logger_, "bus.res.gateway.proxy - send to peer "
						<< cyng::value_cast<std::string>(rec["class"], "")
						<< '@'
						<< peer->vm_.tag());

					//
					//	forward data
					//
					peer->vm_.async_run(node::bus_res_gateway_proxy(
						  std::get<0>(tpl)		//	ident tag
						, std::get<1>(tpl)		//	source tag
						, std::get<2>(tpl)		//	cluster sequence
						, std::get<3>(tpl)		//	TGateway key
						, std::get<4>(tpl)		//	websocket tag
						, std::get<5>(tpl)		//	channel
						, std::get<6>(tpl)		//	server id
						, std::get<7>(tpl)		//	section part
						, std::get<8>(tpl)));	//	params						
				}
			}
			else {
				CYNG_LOG_WARNING(logger_, "bus.res.query.gateway - peer not found " << std::get<1>(tpl));
			}
		}, cyng::store::read_access("_Cluster"));


		//
		//	update specific data in TGateway and TMeter table
		//
		if (boost::algorithm::equals(std::get<7>(tpl), "root-active-devices")) {

			//
			//	Use the incoming active devices to populate TMeter table
			//

			//	%(("class":---),("ident":01-e61e-13090016-3c-07),("number":0009),("timestamp":2018-11-07 12:04:46.00000000),("type":00000000))
			const auto ident = cyng::find_value<std::string>(std::get<8>(tpl), "ident", "");
			const auto meter = cyng::find_value<std::string>(std::get<8>(tpl), "meter", "");
			const auto maker = cyng::find_value<std::string>(std::get<8>(tpl), "maker", "");
			const auto class_name = cyng::find_value<std::string>(std::get<8>(tpl), "class", "A");
			const auto type = cyng::find_value<std::uint32_t>(std::get<8>(tpl), "type", 0u);
			const auto age = cyng::find_value(std::get<8>(tpl), "timestamp", std::chrono::system_clock::now());

			//
			//	preconditions are
			//
			//	* configuration flag for auto insert meters is true
			//	* meter type is SRV_MBUS or SRV_SERIAL - see node::sml::get_srv_type()
			//	* TGateway key has correct size (and is not empty)
			//
			if (is_catch_meters(global_configuration_) && (type < 2) && std::get<3>(tpl).size() == 1)
			{
				CYNG_LOG_TRACE(logger_, "update TMeter " << ident);

				db_.access([&](cyng::store::table* tbl_meter)->void {
					bool found{ false };
					tbl_meter->loop([&](cyng::table::record const& rec) -> bool {

						const auto rec_ident = cyng::value_cast<std::string>(rec["ident"], "");
						if (boost::algorithm::equals(ident, rec_ident)) {
							//	abort loop
							found = true;
						}

						//	continue loop
						return found;
					});

					if (!found) {
						const auto tag = uidgen_();
						CYNG_LOG_INFO(logger_, "insert TMeter " << tag << " : " << meter);

						tbl_meter->insert(cyng::table::key_generator(tag)
							, cyng::table::data_generator(ident, meter, maker, age, "", "", "", "", class_name, std::get<3>(tpl).front())
							, 0
							, std::get<1>(tpl));
					}
				}, cyng::store::write_access("TMeter"));
			}
		}
		else if (boost::algorithm::equals(std::get<4>(tpl), "root-wMBus-status")) {

			//
			//	update TGateway table
			//
			//	"id" contains the "mbus" ID
			cyng::buffer_t tmp;
			const auto id = cyng::find_value(std::get<8>(tpl), "id", tmp);
			CYNG_LOG_INFO(logger_, "Update M-Bus ID: " << cyng::io::to_hex(id));

			db_.access([&](cyng::store::table* tbl_gw)->void {
				//	update "mbus" W-Mbus ID
				tbl_gw->modify(std::get<3>(tpl), cyng::param_factory("mbus", id), std::get<1>(tpl));
			}, cyng::store::write_access("TGateway"));
		}

	}

	void cluster::bus_res_attention_code(cyng::context& ctx)
	{
		//	[da673931-9743-41b9-8a46-6ce946c9fa6c,9f773865-e4af-489a-8824-8f78a2311278,4,5c200bdf-22c0-41bd-bc93-d879d935889e,00:15:3b:02:29:81,8181C7C7FE03,NO SERVER ID]
		//
		//	* [uuid] ident
		//	* [uuid] source
		//	* [u64] cluster seq
		//	* [uuid] ws tag
		//	* [string] server id
		//	* [buffer] attention code
		//	* [string] message
		//
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_INFO(logger_, "bus.res.attention.code " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] ident
			boost::uuids::uuid,		//	[1] source
			std::uint64_t,			//	[2] sequence
			boost::uuids::uuid,		//	[3] ws tag
			std::string,			//	[4] server id
			cyng::buffer_t,			//	[5] attention code (OBIS)
			std::string				//	[6] msg
		>(frame);

		//
		//	emit system message
		//
		insert_msg(db_
			, cyng::logging::severity::LEVEL_INFO
			, ("attention message from " + std::get<4>(tpl) + ": " + std::get<6>(tpl))
			, std::get<1>(tpl));

		//
		//	routing back - forward to receiver
		//
		db_.access([&](const cyng::store::table* tbl_cluster)->void {

			auto rec = tbl_cluster->lookup(cyng::table::key_generator(std::get<1>(tpl)));
			if (!rec.empty()) {
				auto peer = cyng::object_cast<session>(rec["self"]);
				if (peer != nullptr) {

					CYNG_LOG_INFO(logger_, "bus.res.attention.code - send to peer "
						<< cyng::value_cast<std::string>(rec["class"], "")
						<< '@'
						<< peer->vm_.tag());

					//
					//	forward data
					//
					peer->vm_.async_run(node::bus_res_attention_code(
						std::get<0>(tpl)		//	ident tag
						, std::get<1>(tpl)		//	source tag
						, std::get<2>(tpl)		//	cluster sequence
						, std::get<3>(tpl)		//	websocket tag
						, std::get<4>(tpl)		//	server id
						, std::get<5>(tpl)		//	attention code (OBIS)
						, std::get<6>(tpl)));	//	msg
				}
			}
			else {
				CYNG_LOG_WARNING(logger_, "bus.res.attention.code - peer not found " << std::get<1>(tpl));
			}
		}, cyng::store::read_access("_Cluster"));

	}

	std::tuple<session const*, cyng::table::record, cyng::buffer_t, boost::uuids::uuid> cluster::find_peer(cyng::table::key_type const& key_gw
		, const cyng::store::table* tbl_session
		, const cyng::store::table* tbl_gw)
	{
		//
		//	get gateway record
		//
		auto rec_gw = tbl_gw->lookup(key_gw);

		//
		//	gateway and (associated) device share the same key
		//
		auto rec_session = tbl_session->find_first(cyng::param_t("device", key_gw.at(0)));
		if (!rec_gw.empty() && !rec_session.empty())
		{
			auto peer = cyng::object_cast<session>(rec_session["local"]);
			if (peer && (key_gw.size() == 1))
			{
				//
				//	get remote session tag
				//
				auto tag = cyng::value_cast(rec_session.key().at(0), boost::uuids::nil_uuid());
				//BOOST_ASSERT(tag == peer->vm_.tag());

				//
				//	get login parameters
				//
				const auto server = cyng::value_cast<std::string>(rec_gw["serverId"], "05000000000000");
				const auto name = cyng::value_cast<std::string>(rec_gw["userName"], "");
				const auto pwd = cyng::value_cast<std::string>(rec_gw["userPwd"], "");

				const auto id = cyng::parse_hex_string(server);
				if (id.second)
				{
					CYNG_LOG_TRACE(logger_, "gateway "
						<< cyng::io::to_str(rec_session["name"])
						<< ": "
						<< server
						<< ", user: "
						<< name
						<< ", pwd: "
						<< pwd
						<< " has peer: "
						<< peer->vm_.tag());

					return std::make_tuple(peer, rec_gw, id.first, tag);
				}
				else
				{
					CYNG_LOG_WARNING(logger_, "client "
						<< tag
						<< " has an invalid server id: "
						<< server);
				}
			}
		}

		//
		//	not found - peer is a null pointer
		//
		return std::make_tuple(nullptr, rec_gw, cyng::buffer_t{}, boost::uuids::nil_uuid());
	}


}