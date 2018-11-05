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

#include <boost/uuid/nil_generator.hpp>

namespace node 
{
	cluster::cluster(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, cyng::store::db& db)
	: mux_(mux)
		, logger_(logger)
		, db_(db)
	{
	}

	void cluster::register_this(cyng::context& ctx)
	{
		ctx.attach(cyng::register_function("bus.req.reboot.client", 4, std::bind(&cluster::bus_req_reboot_client, this, std::placeholders::_1)));
		ctx.attach(cyng::register_function("bus.req.query.gateway", 5, std::bind(&cluster::bus_req_query_gateway, this, std::placeholders::_1)));
		ctx.attach(cyng::register_function("bus.res.query.gateway", 6, std::bind(&cluster::bus_res_query_gateway, this, std::placeholders::_1)));

		//ctx.attach(cyng::register_function("bus.res.query.status.word", 5, std::bind(&cluster::bus_res_query_status_word, this, std::placeholders::_1)));
		//ctx.attach(cyng::register_function("bus.res.query.srv.visible", 8, std::bind(&cluster::bus_res_query_srv_visible, this, std::placeholders::_1)));
		//ctx.attach(cyng::register_function("bus.res.query.srv.active", 8, std::bind(&cluster::bus_res_query_srv_active, this, std::placeholders::_1)));
		//ctx.attach(cyng::register_function("bus.res.query.firmware", 8, std::bind(&cluster::bus_res_query_firmware, this, std::placeholders::_1)));
		//ctx.attach(cyng::register_function("bus.res.query.memory", 6, std::bind(&cluster::bus_res_query_memory, this, std::placeholders::_1)));
		//ctx.attach(cyng::register_function("bus.res.query.wmbus.status", 0, std::bind(&cluster::bus_res_query_wmbus_status, this, std::placeholders::_1)));
		ctx.attach(cyng::register_function("bus.res.attention.code", 6, std::bind(&cluster::bus_res_attention_code, this, std::placeholders::_1)));

	}

	void cluster::bus_req_reboot_client(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_INFO(logger_, "bus.req.reboot.client " << cyng::io::to_str(frame));

		//	bus.req.reboot.client not found 
		//	[1,[dca135f3-ff2b-4bf7-8371-a9904c74522b],430681d9-a735-47be-a26c-c9cac94d6729]
		//
		//	* bus sequence
		//	* session key
		//	* source
		//	* ws tag

		auto const tpl = cyng::tuple_cast<
			std::uint64_t,			//	[0] sequence
			cyng::table::key_type,	//	[1] session key
			boost::uuids::uuid,		//	[2] source
			boost::uuids::uuid		//	[3] ws tag
		>(frame);

		//
		//	test session key
		//
		BOOST_ASSERT(!std::get<1>(tpl).empty());

		//	reboot a gateway (client)
		//
		db_.access([&](const cyng::store::table* tbl_session, const cyng::store::table* tbl_gw)->void {

#if _MSC_VER || (__GNUC__ > 6)
			//
			//	C++17 feature: structured binding
			//
			//	peer - 
			//	tag - target session tag
			auto[peer, rec, server, tag] = find_peer(std::get<1>(tpl), tbl_session, tbl_gw);
			if (peer != nullptr) {
				const auto name = cyng::value_cast<std::string>(rec["userName"], "");
				const auto pwd = cyng::value_cast<std::string>(rec["userPwd"], "");
				peer->vm_.async_run(client_req_reboot(tag	//	ipt session tag
					, ctx.tag()			//	source peer
					, std::get<0>(tpl)	//	cluster sequence
					, std::get<3>(tpl)	//	ws tag
					, server			//	server
					, name
					, pwd));
			}
#else
			auto res = find_peer(std::get<1>(tpl), tbl_session, tbl_gw);
			if (std::get<0>(res) != nullptr) {
				const auto name = cyng::value_cast<std::string>(std::get<1>(res)["userName"], "");
				const auto pwd = cyng::value_cast<std::string>(std::get<1>(res)["userPwd"], "");
				std::get<0>(res)->vm_.async_run(client_req_reboot(std::get<3>(res)	//	ipt session tag
					, ctx.tag()			//	source peer
					, std::get<0>(tpl)	//	cluster sequence
					, std::get<3>(tpl)	//	ws tag
					, std::get<2>(res)	//	server
					, name
					, pwd));
			}
#endif


		}, cyng::store::read_access("_Session")
			, cyng::store::read_access("TGateway"));
	}

	void cluster::bus_req_query_gateway(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_INFO(logger_, "bus.req.query.gateway " << cyng::io::to_str(frame));

		//	[4,[54c15b9e-858a-431c-9c2c-8b654c7d7651],dfc987b3-3853-4e30-a6ac-7c4ab52bb33f,[firmware,srv:active,srv:visible,status:word],7ba6186d-3f0d-428e-b5c0-59d1833f01a1]
		//
		//	* bus sequence
		//	* session key
		//	* source
		//	* tag_ws

		auto const tpl = cyng::tuple_cast<
			std::uint64_t,			//	[0] sequence
			cyng::vector_t,			//	[1] session key
			boost::uuids::uuid,		//	[2] source
			cyng::vector_t,			//	[3] requests (strings)
			boost::uuids::uuid		//	[4] websocket tag
		>(frame);

		//
		//	test session key
		//
		BOOST_ASSERT(!std::get<1>(tpl).empty());

		//
		//	send process parameter request to an gateway
		//
		db_.access([&](const cyng::store::table* tbl_session, const cyng::store::table* tbl_gw)->void {

#if _MSC_VER || (__GNUC__ > 6)

			//
			//	C++17 feature
			//
			auto[peer, rec, server, tag] = find_peer(std::get<1>(tpl), tbl_session, tbl_gw);
			if (peer != nullptr) {
				const auto name = cyng::value_cast<std::string>(rec["userName"], "");
				const auto pwd = cyng::value_cast<std::string>(rec["userPwd"], "");
				peer->vm_.async_run(node::client_req_query_gateway(tag	//	ipt session tag
					, std::get<2>(tpl)	//	source peer
					, std::get<0>(tpl)	//	cluster sequence
					, std::get<3>(tpl)	//	requests
					, std::get<4>(tpl)	//	ws tag
					, server			//	server
					, name
					, pwd));
#else
			auto res = find_peer(std::get<1>(tpl), tbl_session, tbl_gw);
			if (std::get<0>(res) != nullptr) {
				const auto name = cyng::value_cast<std::string>(std::get<1>(res)["userName"], "");
				const auto pwd = cyng::value_cast<std::string>(std::get<1>(res)["userPwd"], "");
				std::get<0>(res)->vm_.async_run(node::client_req_query_gateway(std::get<3>(res)
					, std::get<2>(tpl)	//	source peer
					, std::get<0>(tpl)	//	cluster sequence
					, std::get<3>(tpl)	//	requests
					, std::get<4>(tpl)	//	ws tag
					, std::get<2>(res)	//	server
					, name
					, pwd));

#endif

			}

			}, cyng::store::read_access("_Session")
				, cyng::store::read_access("TGateway"));
		}

	void cluster::bus_res_query_gateway(cyng::context& ctx)
	{
		//	[9e3dda5d-7c0b-4a8d-b25d-eea3a113a856,16,0fc100f8-0f1d-4c36-a131-6351bcf467d7,00:15:3b:02:29:80,op-log-status-word,%(("status.word":00072202))]
		//	[9e3dda5d-7c0b-4a8d-b25d-eea3a113a856,17,0fc100f8-0f1d-4c36-a131-6351bcf467d7,00:15:3b:01:ec:46,root-device-id,%(("active":false),("firmware":INSTALLATION [1]),("number":00000004),("version":VARIOMUC-ETHERNET-1.406_14132___18X022e))]
		//
		//	* source
		//	* bus sequence
		//	* tag_ws
		//	* server id
		//	* OBIS name
		//	* parameters
		//
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_INFO(logger_, "bus.res.query.gateway " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] source
			std::uint64_t,			//	[1] sequence
			boost::uuids::uuid,		//	[2] websocket tag
			std::string,			//	[3] server id (key)
			std::string,			//	[4] OBIS name
			cyng::param_map_t		//	[5] params
		>(frame);

		db_.access([&](const cyng::store::table* tbl_cluster)->void {

			auto rec = tbl_cluster->lookup(cyng::table::key_generator(std::get<0>(tpl)));
			if (!rec.empty()) {
				auto peer = cyng::object_cast<session>(rec["self"]);
				if (peer != nullptr) {

					CYNG_LOG_INFO(logger_, "bus.res.query.gateway - send to peer "
						<< cyng::value_cast<std::string>(rec["class"], "")
						<< '@'
						<< peer->vm_.tag());

					//
					//	forward data
					//
					peer->vm_.async_run(node::bus_res_query_gateway(std::get<0>(tpl)
						, std::get<1>(tpl)		//	sequence
						, std::get<2>(tpl)		//	websocket tag
						, std::get<3>(tpl)		//	server id
						, std::get<4>(tpl)		//	OBIS name
						, std::get<5>(tpl)));	//	params
				}
			}
			else {
				CYNG_LOG_WARNING(logger_, "bus.res.query.gateway - peer not found " << std::get<0>(tpl));
			}
		}, cyng::store::read_access("_Cluster"));
	}

	//void cluster::bus_res_query_srv_visible(cyng::context& ctx)
	//{
	//	//	[6b9c001a-4390-40b5-9abc-25c6180cc8b9,21,00:15:3b:02:29:7e,0005,01-e61e-29436587-bf-03,---,2018-08-29 19:26:29.00000000]
	//	const cyng::vector_t frame = ctx.get_frame();
	//	CYNG_LOG_INFO(logger_, "bus.res.query.srv.visible " << cyng::io::to_str(frame));

	//	auto const tpl = cyng::tuple_cast<
	//		boost::uuids::uuid,		//	[0] source
	//		std::uint64_t,			//	[1] sequence
	//		boost::uuids::uuid,		//	[2] websocket tag
	//		std::uint16_t,			//	[3] list number
	//		std::string,			//	[4] server id (key)
	//		std::string,			//	[5] meter
	//		std::string,			//	[6] device class
	//		std::chrono::system_clock::time_point,	//	[7] last status update
	//		std::uint32_t			//	[8] server type
	//	>(frame);


	//	db_.access([&](const cyng::store::table* tbl_cluster)->void {

	//		auto rec = tbl_cluster->lookup(cyng::table::key_generator(std::get<0>(tpl)));
	//		if (!rec.empty()) {
	//			auto peer = cyng::object_cast<session>(rec["self"]);
	//			if (peer != nullptr) {

	//				CYNG_LOG_INFO(logger_, "bus.res.query.srv.visible - send to peer "
	//					<< cyng::value_cast<std::string>(rec["class"], "")
	//					<< '@'
	//					<< peer->vm_.tag());

	//				//
	//				//	forward data
	//				//
	//				peer->vm_.async_run(node::bus_res_query_srv_visible(std::get<0>(tpl)
	//					, std::get<1>(tpl)		//	sequence
	//					, std::get<2>(tpl)		//	websocket tag
	//					, std::get<3>(tpl)		//	list number
	//					, std::get<4>(tpl)		//	server id
	//					, std::get<5>(tpl)		//	meter
	//					, std::get<6>(tpl)		//	device class
	//					, std::get<7>(tpl)		//	last status update
	//					, std::get<8>(tpl)));
	//			}
	//		}
	//		else {
	//			CYNG_LOG_WARNING(logger_, "bus.res.query.srv.visible - peer not found " << std::get<0>(tpl));
	//		}
	//	}, cyng::store::read_access("_Cluster"));
	//}

	//void cluster::bus_res_query_srv_active(cyng::context& ctx)
	//{
	//	const cyng::vector_t frame = ctx.get_frame();
	//	CYNG_LOG_INFO(logger_, "bus.res.query.srv.active " << cyng::io::to_str(frame));
	//	auto const tpl = cyng::tuple_cast<
	//		boost::uuids::uuid,		//	[0] source
	//		std::uint64_t,			//	[1] sequence
	//		boost::uuids::uuid,		//	[2] websocket tag
	//		std::uint16_t,			//	[3] list number
	//		std::string,			//	[4] server id (key)
	//		std::string,			//	[5] meter
	//		std::string,			//	[6] device class
	//		std::chrono::system_clock::time_point,	//	[7] last status update
	//		std::uint32_t			//	[8] server type
	//	>(frame);


	//	db_.access([&](const cyng::store::table* tbl_cluster)->void {
	//		auto rec = tbl_cluster->lookup(cyng::table::key_generator(std::get<0>(tpl)));
	//		if (!rec.empty()) {
	//			auto peer = cyng::object_cast<session>(rec["self"]);
	//			if (peer != nullptr) {

	//				CYNG_LOG_INFO(logger_, "bus.res.query.srv.active - send to peer "
	//					<< cyng::value_cast<std::string>(rec["class"], "")
	//					<< '@'
	//					<< peer->vm_.tag());

	//				//
	//				//	forward data
	//				//
	//				peer->vm_.async_run(node::bus_res_query_srv_active(std::get<0>(tpl)
	//					, std::get<1>(tpl)	//	sequence
	//					, std::get<2>(tpl)		//	websocket tag
	//					, std::get<3>(tpl)		//	list number
	//					, std::get<4>(tpl)		//	server id
	//					, std::get<5>(tpl)		//	meter
	//					, std::get<6>(tpl)		//	device class
	//					, std::get<7>(tpl)		//	last status update
	//					, std::get<8>(tpl)));
	//			}
	//		}
	//		else {
	//			CYNG_LOG_WARNING(logger_, "bus.res.query.srv.active - peer not found " << std::get<0>(tpl));
	//		}
	//	}, cyng::store::read_access("_Cluster"));
	//}

	//void cluster::bus_res_query_firmware(cyng::context& ctx)
	//{
	//	const cyng::vector_t frame = ctx.get_frame();
	//	CYNG_LOG_INFO(logger_, "bus.res.query.firmware " << cyng::io::to_str(frame));
	//	auto const tpl = cyng::tuple_cast<
	//		boost::uuids::uuid,		//	[0] source
	//		std::uint64_t,			//	[1] sequence
	//		boost::uuids::uuid,		//	[2] websocket tag
	//		std::uint32_t,			//	[3] list number
	//		std::string,			//	[4] server id (key)
	//		std::string,			//	[5] section
	//		std::string,			//	[6] version
	//		bool					//	[7] active/inactive
	//	>(frame);


	//	db_.access([&](const cyng::store::table* tbl_cluster)->void {
	//		auto rec = tbl_cluster->lookup(cyng::table::key_generator(std::get<0>(tpl)));
	//		if (!rec.empty()) {
	//			auto peer = cyng::object_cast<session>(rec["self"]);
	//			if (peer != nullptr) {

	//				CYNG_LOG_INFO(logger_, "bus.res.query.firmware - send to peer "
	//					<< cyng::value_cast<std::string>(rec["class"], "")
	//					<< '@'
	//					<< peer->vm_.tag()
	//					<< " - "
	//					<< std::get<6>(tpl));

	//				//
	//				//	forward data
	//				//
	//				peer->vm_.async_run(node::bus_res_query_firmware(std::get<0>(tpl)
	//					, std::get<1>(tpl)	//	sequence
	//					, std::get<2>(tpl)		//	websocket tag
	//					, std::get<3>(tpl)		//	list number
	//					, std::get<4>(tpl)		//	server id
	//					, std::get<5>(tpl)		//	section
	//					, std::get<6>(tpl)		//	version
	//					, std::get<7>(tpl)));	//	active/inactive
	//			}
	//		}
	//		else {
	//			CYNG_LOG_WARNING(logger_, "bus.res.query.firmware - peer not found " << std::get<0>(tpl));
	//		}
	//	}, cyng::store::read_access("_Cluster"));
	//}

	//void cluster::bus_res_query_memory(cyng::context& ctx)
	//{
	//	const cyng::vector_t frame = ctx.get_frame();
	//	CYNG_LOG_INFO(logger_, "bus.res.query.memory " << cyng::io::to_str(frame));
	//	auto const tpl = cyng::tuple_cast<
	//		boost::uuids::uuid,		//	[0] source
	//		std::uint64_t,			//	[1] sequence
	//		boost::uuids::uuid,		//	[2] websocket tag
	//		std::string,			//	[3] server id (key)
	//		std::uint8_t,			//	[4] mirror
	//		std::uint8_t			//	[5] tmp
	//	>(frame);


	//	db_.access([&](const cyng::store::table* tbl_cluster)->void {
	//		auto rec = tbl_cluster->lookup(cyng::table::key_generator(std::get<0>(tpl)));
	//		if (!rec.empty()) {
	//			auto peer = cyng::object_cast<session>(rec["self"]);
	//			if (peer != nullptr) {

	//				CYNG_LOG_INFO(logger_, "bus.res.query.memory - send to peer "
	//					<< cyng::value_cast<std::string>(rec["class"], "")
	//					<< '@'
	//					<< peer->vm_.tag()
	//					<< " - "
	//					<< std::get<4>(tpl)
	//					<< "% - "
	//					<< std::get<5>(tpl)
	//					<< "%");

	//				//
	//				//	forward data
	//				//
	//				peer->vm_.async_run(node::bus_res_query_memory(std::get<0>(tpl)
	//					, std::get<1>(tpl)	//	sequence
	//					, std::get<2>(tpl)		//	websocket tag
	//					, std::get<3>(tpl)		//	server id
	//					, std::get<4>(tpl)		//	mirror
	//					, std::get<5>(tpl)));		//	tmp
	//			}
	//		}
	//		else {
	//			CYNG_LOG_WARNING(logger_, "bus.res.query.memory - peer not found " << std::get<0>(tpl));
	//		}
	//	}, cyng::store::read_access("_Cluster"));
	//}

	//void cluster::bus_res_query_wmbus_status(cyng::context& ctx)
	//{
	//	//	[9e3dda5d-7c0b-4a8d-b25d-eea3a113a856,4,0fc100f8-0f1d-4c36-a131-6351bcf467d7,00:ff:b0:0b:ca:ae,solos::Tec,a8 15 34 83 40 04 01 31,0.6.2018305,2.00]
	//	const cyng::vector_t frame = ctx.get_frame();
	//	CYNG_LOG_INFO(logger_, "bus.res.query.wmbus.status " << cyng::io::to_str(frame));

	//}

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

	std::tuple<session const*, cyng::table::record, cyng::buffer_t, boost::uuids::uuid> cluster::find_peer(cyng::table::key_type const& key_session
		, const cyng::store::table* tbl_session
		, const cyng::store::table* tbl_gw)
	{
		auto rec_gw = tbl_gw->lookup(key_session);
		auto rec_session = tbl_session->find_first(cyng::param_t("device", key_session.at(0)));
		if (!rec_gw.empty() && !rec_session.empty())
		{
			auto peer = cyng::object_cast<session>(rec_session["local"]);
			if (peer && (key_session.size() == 1))
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