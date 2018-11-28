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
		//ctx.attach(cyng::register_function("bus.req.reboot.client", 4, std::bind(&cluster::bus_req_reboot_client, this, std::placeholders::_1)));
		//ctx.attach(cyng::register_function("bus.req.query.gateway", 5, std::bind(&cluster::bus_req_query_gateway, this, std::placeholders::_1)));
		//ctx.attach(cyng::register_function("bus.req.modify.gateway", 5, std::bind(&cluster::bus_req_modify_gateway, this, std::placeholders::_1)));
		ctx.attach(cyng::register_function("bus.req.gateway.proxy", 7, std::bind(&cluster::bus_req_gateway_proxy, this, std::placeholders::_1)));
		ctx.attach(cyng::register_function("bus.res.gateway.proxy", 9, std::bind(&cluster::bus_res_gateway_proxy, this, std::placeholders::_1)));

		//ctx.attach(cyng::register_function("bus.res.query.gateway", 6, std::bind(&cluster::bus_res_query_gateway, this, std::placeholders::_1)));
		//ctx.attach(cyng::register_function("bus.res.attention.code", 6, std::bind(&cluster::bus_res_attention_code, this, std::placeholders::_1)));
	}

//	void cluster::bus_req_reboot_client(cyng::context& ctx)
//	{
//		const cyng::vector_t frame = ctx.get_frame();
//		CYNG_LOG_INFO(logger_, "bus.req.reboot.client " << cyng::io::to_str(frame));
//
//		//	bus.req.reboot.client not found 
//		//	[1,[dca135f3-ff2b-4bf7-8371-a9904c74522b],430681d9-a735-47be-a26c-c9cac94d6729]
//		//
//		//	* bus sequence
//		//	* session key
//		//	* source
//		//	* ws tag
//
//		auto const tpl = cyng::tuple_cast<
//			std::uint64_t,			//	[0] sequence
//			cyng::table::key_type,	//	[1] gw key
//			boost::uuids::uuid,		//	[2] source
//			boost::uuids::uuid		//	[3] ws tag
//		>(frame);
//
//		//
//		//	test session key
//		//
//		BOOST_ASSERT(!std::get<1>(tpl).empty());
//
//		//	reboot a gateway (client)
//		//
//		db_.access([&](const cyng::store::table* tbl_session, const cyng::store::table* tbl_gw)->void {
//
//#if _MSC_VER || (__GNUC__ > 6)
//			//
//			//	C++17 feature: structured binding
//			//
//			//	peer - 
//			//	tag - target session tag
//			auto[peer, rec, server, tag] = find_peer(std::get<1>(tpl), tbl_session, tbl_gw);
//			if (peer != nullptr) {
//				const auto name = cyng::value_cast<std::string>(rec["userName"], "");
//				const auto pwd = cyng::value_cast<std::string>(rec["userPwd"], "");
//				peer->vm_.async_run(client_req_reboot(tag	//	ipt session tag
//					, ctx.tag()			//	source peer
//					, std::get<0>(tpl)	//	cluster sequence
//					, std::get<3>(tpl)	//	ws tag
//					, server			//	server
//					, name
//					, pwd));
//			}
//#else
//			auto res = find_peer(std::get<1>(tpl), tbl_session, tbl_gw);
//			if (std::get<0>(res) != nullptr) {
//				const auto name = cyng::value_cast<std::string>(std::get<1>(res)["userName"], "");
//				const auto pwd = cyng::value_cast<std::string>(std::get<1>(res)["userPwd"], "");
//				std::get<0>(res)->vm_.async_run(client_req_reboot(std::get<3>(res)	//	ipt session tag
//					, ctx.tag()			//	source peer
//					, std::get<0>(tpl)	//	cluster sequence
//					, std::get<3>(tpl)	//	ws tag
//					, std::get<2>(res)	//	server
//					, name
//					, pwd));
//			}
//#endif
//
//
//		}	, cyng::store::read_access("_Session")
//			, cyng::store::read_access("TGateway"));
//	}

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
						
					//peer->vm_.async_run(node::bus_res_query_gateway(std::get<0>(tpl)
					//	, std::get<1>(tpl)		//	sequence
					//	, std::get<2>(tpl)		//	websocket tag
					//	, std::get<3>(tpl)		//	server id
					//	, std::get<4>(tpl)		//	OBIS name
					//	, std::get<5>(tpl)));	//	params
				}
			}
			else {
				CYNG_LOG_WARNING(logger_, "bus.res.query.gateway - peer not found " << std::get<1>(tpl));
			}
		}, cyng::store::read_access("_Cluster"));


		//
		//	update specific data in TGateway and TMeter table
		//
		if (boost::algorithm::equals(std::get<4>(tpl), "root-active-devices")) {

			//
			//	Use the incoming active devices to populate TMeter table
			//

			//	%(("class":---),("meter":01-e61e-13090016-3c-07),("number":0009),("timestamp":2018-11-07 12:04:46.00000000),("type":00000000))
			const auto ident = cyng::find_value<std::string>(std::get<8>(tpl), "ident", "");
			const auto meter = cyng::find_value<std::string>(std::get<8>(tpl), "meter", "");
			const auto class_name = cyng::find_value<std::string>(std::get<8>(tpl), "class", "A");
			const auto type = cyng::find_value<std::uint32_t>(std::get<8>(tpl), "type", 0u);
			const auto age = cyng::find_value(std::get<8>(tpl), "timestamp", std::chrono::system_clock::now());

			if (is_catch_meters(global_configuration_) && (type < 2))
			{
				CYNG_LOG_TRACE(logger_, "update TMeter " << ident);

				db_.access([&](cyng::store::table* tbl_meter)->void {
					bool found{ false };
					tbl_meter->loop([&](cyng::table::record const& rec) -> bool {

						const auto rec_ident = cyng::value_cast<std::string>(rec["ident"], "");
						if (boost::algorithm::equals(meter, rec_ident)) {
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
							, cyng::table::data_generator(ident, meter, "", age, "", "", "", "", class_name, std::get<0>(tpl))
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

//	void cluster::bus_req_query_gateway(cyng::context& ctx)
//	{
//		const cyng::vector_t frame = ctx.get_frame();
//		CYNG_LOG_INFO(logger_, "bus.req.query.gateway " << cyng::io::to_str(frame));
//
//		//	[4,[54c15b9e-858a-431c-9c2c-8b654c7d7651],dfc987b3-3853-4e30-a6ac-7c4ab52bb33f,[firmware,srv:active,srv:visible,op-log-status-word],7ba6186d-3f0d-428e-b5c0-59d1833f01a1]
//		//
//		//	* bus sequence
//		//	* session key
//		//	* source
//		//	* tag_ws
//
//		auto const tpl = cyng::tuple_cast<
//			std::uint64_t,			//	[0] sequence
//			cyng::vector_t,			//	[1] gw key
//			boost::uuids::uuid,		//	[2] source
//			cyng::vector_t,			//	[3] requests (strings)
//			boost::uuids::uuid		//	[4] websocket tag
//		>(frame);
//
//		//
//		//	test session key
//		//
//		BOOST_ASSERT(!std::get<1>(tpl).empty());
//
//		//
//		//	send process parameter request to an gateway
//		//
//		db_.access([&](const cyng::store::table* tbl_session, const cyng::store::table* tbl_gw)->void {
//
//#if _MSC_VER || (__GNUC__ > 6)
//
//			//
//			//	C++17 feature
//			//
//			auto[peer, rec, server, tag] = find_peer(std::get<1>(tpl), tbl_session, tbl_gw);
//			if (peer != nullptr) {
//				const auto name = cyng::value_cast<std::string>(rec["userName"], "");
//				const auto pwd = cyng::value_cast<std::string>(rec["userPwd"], "");
//				peer->vm_.async_run(node::client_req_query_gateway(tag	//	ipt session tag
//					, std::get<2>(tpl)	//	source peer
//					, std::get<0>(tpl)	//	cluster sequence
//					, std::get<3>(tpl)	//	requests
//					, std::get<4>(tpl)	//	ws tag
//					, server			//	server
//					, name
//					, pwd));
//#else
//			auto res = find_peer(std::get<1>(tpl), tbl_session, tbl_gw);
//			if (std::get<0>(res) != nullptr) {
//				const auto name = cyng::value_cast<std::string>(std::get<1>(res)["userName"], "");
//				const auto pwd = cyng::value_cast<std::string>(std::get<1>(res)["userPwd"], "");
//				std::get<0>(res)->vm_.async_run(node::client_req_query_gateway(std::get<3>(res)
//					, std::get<2>(tpl)	//	source peer
//					, std::get<0>(tpl)	//	cluster sequence
//					, std::get<3>(tpl)	//	requests
//					, std::get<4>(tpl)	//	ws tag
//					, std::get<2>(res)	//	server
//					, name
//					, pwd));
//
//#endif
//
//			}
//
//			}	, cyng::store::read_access("_Session")
//				, cyng::store::read_access("TGateway"));
//		}

	//void cluster::bus_res_query_gateway(cyng::context& ctx)
	//{
		//	[9e3dda5d-7c0b-4a8d-b25d-eea3a113a856,16,0fc100f8-0f1d-4c36-a131-6351bcf467d7,00:15:3b:02:29:80,op-log-status-word,%(("status.word":00072202))]
		//	[9e3dda5d-7c0b-4a8d-b25d-eea3a113a856,17,0fc100f8-0f1d-4c36-a131-6351bcf467d7,00:15:3b:01:ec:46,root-device-id,%(("active":false),("firmware":INSTALLATION [1]),("number":00000004),("version":VARIOMUC-ETHERNET-1.406_14132___18X022e))]
		//	[9e3dda5d-7c0b-4a8d-b25d-eea3a113a856,22,534af696-c4f4-4304-a5af-3b3bdb5779a9,00:15:3b:02:29:7e,root-active-devices,%(("class":---),("meter":0ad00001),("number":0005),("timestamp":2017-01-19 08:56:53.00000000),("type":0000000a))]
		//	[9e3dda5d-7c0b-4a8d-b25d-eea3a113a856,22,534af696-c4f4-4304-a5af-3b3bdb5779a9,00:15:3b:02:29:7e,root-active-devices,%(("class":---),("meter":01-e61e-13090016-3c-07),("number":0009),("timestamp":2018-11-07 12:04:46.00000000),("type":00000000))]
		//
		//	* source
		//	* bus sequence
		//	* tag_ws
		//	* server id
		//	* OBIS name
		//	* parameters
		//
		//const cyng::vector_t frame = ctx.get_frame();
		//CYNG_LOG_INFO(logger_, "bus.res.query.gateway " << cyng::io::to_str(frame));

		//auto const tpl = cyng::tuple_cast<
		//	boost::uuids::uuid,		//	[0] source
		//	std::uint64_t,			//	[1] sequence
		//	boost::uuids::uuid,		//	[2] websocket tag
		//	std::string,			//	[3] server id (key)
		//	std::string,			//	[4] OBIS name
		//	cyng::param_map_t		//	[5] params
		//>(frame);

		//
		//	routing back
		//
	//	db_.access([&](const cyng::store::table* tbl_cluster)->void {

	//		auto rec = tbl_cluster->lookup(cyng::table::key_generator(std::get<0>(tpl)));
	//		if (!rec.empty()) {
	//			auto peer = cyng::object_cast<session>(rec["self"]);
	//			if (peer != nullptr) {

	//				CYNG_LOG_INFO(logger_, "bus.res.query.gateway - send to peer "
	//					<< cyng::value_cast<std::string>(rec["class"], "")
	//					<< '@'
	//					<< peer->vm_.tag());

	//				//
	//				//	forward data
	//				//
	//				peer->vm_.async_run(node::bus_res_query_gateway(std::get<0>(tpl)
	//					, std::get<1>(tpl)		//	sequence
	//					, std::get<2>(tpl)		//	websocket tag
	//					, std::get<3>(tpl)		//	server id
	//					, std::get<4>(tpl)		//	OBIS name
	//					, std::get<5>(tpl)));	//	params
	//			}
	//		}
	//		else {
	//			CYNG_LOG_WARNING(logger_, "bus.res.query.gateway - peer not found " << std::get<0>(tpl));
	//		}
	//	}, cyng::store::read_access("_Cluster"));

	//	if (boost::algorithm::equals(std::get<4>(tpl), "root-active-devices")) {

	//		//
	//		//	update TMeter table
	//		//

	//		//	%(("class":---),("meter":01-e61e-13090016-3c-07),("number":0009),("timestamp":2018-11-07 12:04:46.00000000),("type":00000000))
	//		const auto ident = cyng::find_value<std::string>(std::get<5>(tpl), "ident", "");
	//		const auto meter = cyng::find_value<std::string>(std::get<5>(tpl), "meter", "");
	//		const auto class_name = cyng::find_value<std::string>(std::get<5>(tpl), "class", "A");
	//		const auto type = cyng::find_value<std::uint32_t>(std::get<5>(tpl), "type", 0u);
	//		const auto age = cyng::find_value(std::get<5>(tpl), "timestamp", std::chrono::system_clock::now());

	//		if (is_catch_meters(global_configuration_) && (type < 2))
	//		{
	//			CYNG_LOG_TRACE(logger_, "update TMeter " << ident);

	//			db_.access([&](cyng::store::table* tbl_meter)->void {
	//				bool found{ false };
	//				tbl_meter->loop([&](cyng::table::record const& rec) -> bool {

	//					const auto rec_ident = cyng::value_cast<std::string>(rec["ident"], "");
	//					if (boost::algorithm::equals(meter, rec_ident)) {
	//						//	abort loop
	//						found = true;
	//					}

	//					//	continue loop
	//					return found;
	//				});

	//				if (!found) {
	//					const auto tag = uidgen_();
	//					CYNG_LOG_INFO(logger_, "insert TMeter " << tag << " : " << meter);
	//					tbl_meter->insert(cyng::table::key_generator(tag)
	//						, cyng::table::data_generator(ident, meter, "", age, "", "", "", "", class_name, std::get<0>(tpl))
	//						, 0
	//						, std::get<0>(tpl));
	//				}
	//			}, cyng::store::write_access("TMeter"));
	//		}
	//	}
	//	else if (boost::algorithm::equals(std::get<4>(tpl), "root-wMBus-status")) {

	//		//
	//		//	update TGateway table
	//		//
	//		//	"id" contains the "mbus" ID
	//		cyng::buffer_t tmp;
	//		const auto id = cyng::find_value(std::get<5>(tpl), "id", tmp);
	//		CYNG_LOG_INFO(logger_, "Update M-Bus ID: " << cyng::io::to_hex(id));
	//	}
	//}

//	void cluster::bus_req_modify_gateway(cyng::context& ctx)
//	{
//		//	[1,ipt,[dca135f3-ff2b-4bf7-8371-a9904c74522b],9e3dda5d-7c0b-4a8d-b25d-eea3a113a856,%(("serverId":0500153B02297E),("smf-gw-ipt-host-1":192.168.1.21),("smf-gw-ipt-host-2":192.168.1.21),("smf-gw-ipt-local-1":68ee),("smf-gw-ipt-local-2":68ef),("smf-gw-ipt-name-1":LSMTest5),("smf-gw-ipt-name-2":werwer),("smf-gw-ipt-pwd-1":LSMTest5),("smf-gw-ipt-pwd-2":LSMTest5),("smf-gw-ipt-remote-1":0),("smf-gw-ipt-remote-2":0))]
//		//
//		//	* cluster seq
//		//	* section
//		//	* gateway key
//		//	* source tag
//		//	* parameters
//		//
//		const cyng::vector_t frame = ctx.get_frame();
//		CYNG_LOG_INFO(logger_, "bus.req.modify.gateway " << cyng::io::to_str(frame));
//
//		auto const tpl = cyng::tuple_cast<
//			std::uint64_t,			//	[0] sequence
//			std::string,			//	[1] section
//			cyng::table::key_type,	//	[2] session key
//			boost::uuids::uuid,		//	[3] source tag
//			cyng::param_map_t		//	[4] params
//		>(frame);
//		//
//		//	test session key
//		//
//		BOOST_ASSERT(!std::get<1>(tpl).empty());
//
//		//
//		//	send process parameter request to an gateway
//		//
//		db_.access([&](const cyng::store::table* tbl_session, const cyng::store::table* tbl_gw)->void {
//
////#if _MSC_VER || (__GNUC__ > 6)
//
//
//			//
//			//	C++17 feature
//			//
//			auto[peer, rec, server, tag] = find_peer(std::get<2>(tpl), tbl_session, tbl_gw);
//			if (peer != nullptr) {
//
//				const auto name = cyng::value_cast<std::string>(rec["userName"], "");
//				const auto pwd = cyng::value_cast<std::string>(rec["userPwd"], "");
//
//				//	("serverId":0500153B01EC46) == server
//				cyng::buffer_t serverId;
//				serverId = cyng::value_cast(std::get<4>(tpl).at("serverId"), serverId);
//				BOOST_ASSERT_MSG(serverId == server, "inconsistent server ID");
//
//				peer->vm_.async_run(node::client_req_modify_gateway(tag	//	ipt session tag
//					, std::get<3>(tpl)	//	source peer
//					, std::get<0>(tpl)	//	cluster sequence
//					, std::get<1>(tpl)
//					, std::get<4>(tpl)	//	requests
//					, server			//	server
//					, name
//					, pwd));
//
//			}
//		}	, cyng::store::read_access("_Session")
//			, cyng::store::read_access("TGateway"));
//	}

	void cluster::bus_res_attention_code(cyng::context& ctx)
	{
		//	[f83e12c6-b22c-4775-ad83-b180d3d2a8e0,38,4ac545a0-2b60-41b4-9d32-81eacec5bd94,00:15:3b:02:29:7e,8181C7C7FD00]
		//
		//	* [uuid] source
		//	* [u64] cluster seq
		//	* [uuid] ws tag
		//	* [string] server id
		//	* [buffer] attention code
		//
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_INFO(logger_, "bus.res.attention.code " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] source
			std::uint64_t,			//	[1] sequence
			boost::uuids::uuid,		//	[2] ws tag
			std::string,			//	[3] server id
			cyng::buffer_t,			//	[4] attention code (OBIS)
			std::string				//	[5] msg
		>(frame);

		insert_msg(db_
			, cyng::logging::severity::LEVEL_INFO
			, ("attention message from " + std::get<3>(tpl) + ": " + std::get<5>(tpl))
			, std::get<0>(tpl));

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