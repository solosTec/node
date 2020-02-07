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
#include <smf/shared/db_cfg.h>

#include <cyng/tuple_cast.hpp>
#include <cyng/parser/buffer_parser.h>
#include <cyng/dom/algorithm.h>
#include <cyng/io/io_buffer.h>

#include <boost/uuid/nil_generator.hpp>

namespace node 
{
	cluster::cluster(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, cache& cfg)
	: mux_(mux)
		, logger_(logger)
		, cache_(cfg)
		, uidgen_()
	{}

	void cluster::register_this(cyng::context& ctx)
	{
		ctx.queue(cyng::register_function("bus.req.gateway.proxy", 7, std::bind(&cluster::bus_req_com_sml, this, std::placeholders::_1)));
		ctx.queue(cyng::register_function("bus.res.gateway.proxy", 9, std::bind(&cluster::bus_res_com_sml, this, std::placeholders::_1)));
		ctx.queue(cyng::register_function("bus.res.attention.code", 7, std::bind(&cluster::bus_res_attention_code, this, std::placeholders::_1)));
		ctx.queue(cyng::register_function("bus.req.com.task", 7, std::bind(&cluster::bus_req_com_task, this, std::placeholders::_1)));
		ctx.queue(cyng::register_function("bus.req.com.node", 7, std::bind(&cluster::bus_req_com_node, this, std::placeholders::_1)));
	}

	void cluster::bus_req_com_sml(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_INFO(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));
		
		//	[]
		//
		//	* source
		//	* bus sequence
		//	* web-socket tag (origin)
		//	* channel
		//	* OBIS root 
		//	* TGateway/TDevice key
		//	* sections
		//	* params

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] source
			std::uint64_t,			//	[1] sequence
			boost::uuids::uuid,		//	[2] websocket tag (origin)
			std::string,			//	[3] channel (SML message type)
			cyng::buffer_t,			//	[4] OBIS root 
			cyng::vector_t,			//	[5] TGateway/TDevice key
			cyng::tuple_t			//	[6] optional parameters
		>(frame);

		//
		//	test channel and TGateway key
		//
		BOOST_ASSERT_MSG(!std::get<3>(tpl).empty(), "no SML message type specified");
		BOOST_ASSERT(!std::get<5>(tpl).empty());
		
		//
		//	send process parameter request to an gateway
		//
		cache_.read_tables("_Session", "TGateway", [&](const cyng::store::table* tbl_session, const cyng::store::table* tbl_gw)->void {

			//
			//	Since TGateway and TDevice share the same primary key, we can search for a session 
			//	of that device/gateway. Additionally we get the required access data like
			//	server ID, user and password.
			//
#if defined __CPP_SUPPORT_P0217R3
			auto const[peer, rec, server, tag] = find_peer(std::get<5>(tpl), tbl_session, tbl_gw);
#else            
            std::tuple<session const*, cyng::table::record, cyng::buffer_t, boost::uuids::uuid> r = find_peer(std::get<5>(tpl), tbl_session, tbl_gw);
            session const* peer = std::get<0>(r);
            cyng::table::record const rec = std::get<1>(r);
            cyng::buffer_t const server = std::get<2>(r);
            boost::uuids::uuid const tag =  std::get<3>(r);
#endif

			if (peer != nullptr && !rec.empty()) {

				CYNG_LOG_INFO(logger_, "bus.req.gateway.proxy " << cyng::io::to_str(frame));

				const auto name = cyng::value_cast<std::string>(rec["userName"], "operator");
				const auto pwd = cyng::value_cast<std::string>(rec["userPwd"], "operator");

				peer->vm_.async_run(node::client_req_gateway_proxy(tag	//	ipt session tag
					, std::get<0>(tpl)	//	source tag
					, std::get<1>(tpl)	//	cluster sequence
					, std::get<2>(tpl)	//	websocket tag (origin)
					, std::get<3>(tpl)	//	channel (SML message type)
					, std::get<4>(tpl)	//	OBIS root 
					, std::get<5>(tpl)	//	TGateway/TDevice key
					, std::get<6>(tpl)	//	optional parameters
					, server			//	server
					, name
					, pwd));
			}
			else {

				//
				//	peer/node not found
				//
				CYNG_LOG_WARNING(logger_, "bus.req.gateway.proxy - peer/node not found " 
					<< cyng::io::to_str(std::get<5>(tpl))
					<< ", channel: "
					<< std::get<3>(tpl));
			}

		});
	}

	void cluster::bus_res_com_sml(cyng::context& ctx)
	{
		//
		//	[de554679-66c1-41d3-9975-952f3f69e40c,9f773865-e4af-489a-8824-8f78a2311278,11,[31cad258-45f1-4b1d-b131-8a55eb671bb1],98ac9841-335d-4440-8df1-64b0994e3f75,
		//	get.proc.param,00:ff:b0:4b:94:f8,root-active-devices,
		//	%(("class":---),("ident":01-ec4d-01000010-3c-02),("maker":),("meter":01-ec4d-01000010-3c-02),("meterId":01EC4D010000103C02),("number":0003),("timestamp":2019-08-13 13:41:04.00000000),("type":00000009))]
		//
		//	* [uuid] ident tag
		//	* [uuid] source tag
		//	* [u64] cluster seq
		//	* [vec] gateway key
		//	* [uuid] web-socket tag
		//	* [str] channel (e.g. "GetProcParameterRequest")
		//	* [str] server id (e.g. "00:15:3b:02:29:7e")
		//	* [str] "OBIS code" as text
		//	* [param_map_t] params
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_INFO(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		auto tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] ident
			boost::uuids::uuid,		//	[1] source
			std::uint64_t,			//	[2] sequence
			cyng::vector_t,			//	[3] gw key
			boost::uuids::uuid,		//	[4] websocket tag (origin)
			std::string,			//	[5] channel (message type)
			std::string,			//	[6] server id
			std::string,			//	[7] "OBIS code" as text (see obis_db.cpp)
			cyng::param_map_t		//	[8] params
		>(frame);

		//
		//	get the tag from the TGateway table key
		//
		BOOST_ASSERT_MSG(!std::get<3>(tpl).empty(), "no TGateway key");
		if (std::get<3>(tpl).size() != 1)	return;

		//
		//	update specific data in TGateway and TMeter table (81 81 11 06 FF FF == OBIS_ROOT_ACTIVE_DEVICES)
		//	We don't include <smf/sml/obis_db.h> otherwise we had to link agains the whole
		//	SML library.
		//
		if (boost::algorithm::equals(std::get<7>(tpl), "81811006FFFF") 
			|| boost::algorithm::equals(std::get<7>(tpl), "81811106FFFF")) {

			routing_back_meters(std::get<0>(tpl)
				, std::get<1>(tpl)
				, std::get<2>(tpl)
				, std::get<3>(tpl)
				, std::get<4>(tpl)
				, std::get<5>(tpl)
				, std::get<6>(tpl)
				, std::get<7>(tpl)
				, std::get<8>(tpl));

			//
			//	re-routing already done
			//

		}
		//	81, 06, 0F, 06, 00, FF, ROOT_W_MBUS_STATUS
		else if (boost::algorithm::equals(std::get<7>(tpl), "81060F0600FF")) {

			//
			//	update TGateway table
			//
			//	"id" contains the "mbus" ID
			cyng::buffer_t tmp;
			const auto id = cyng::find_value(std::get<8>(tpl), "id", tmp);
			CYNG_LOG_INFO(logger_, "Update M-Bus ID: " << cyng::io::to_hex(id));

			cache_.write_table("TGateway", [&](cyng::store::table* tbl_gw)->void {
				//	update "mbus" W-Mbus ID
				tbl_gw->modify(std::get<3>(tpl), cyng::param_factory("mbus", id), std::get<1>(tpl));
			});

			//
			//	routing back to sender (cluster member)
			//
			routing_back(std::get<0>(tpl)
				, std::get<1>(tpl)
				, std::get<2>(tpl)
				, std::get<3>(tpl)
				, std::get<4>(tpl)
				, std::get<5>(tpl)
				, std::get<6>(tpl)
				, std::get<7>(tpl)
				, std::get<8>(tpl));
		}
		else {

			//
			//	routing back to sender (cluster member)
			//
			routing_back(std::get<0>(tpl)
				, std::get<1>(tpl)
				, std::get<2>(tpl)
				, std::get<3>(tpl)
				, std::get<4>(tpl)
				, std::get<5>(tpl)
				, std::get<6>(tpl)
				, std::get<7>(tpl)
				, std::get<8>(tpl));
		}
	}

	void cluster::routing_back(boost::uuids::uuid ident
		, boost::uuids::uuid source
		, std::uint64_t sequence
		, cyng::vector_t gw_key
		, boost::uuids::uuid ws
		, std::string channel
		, std::string server_id
		, std::string code
		, cyng::param_map_t& params)
	{
		cache_.read_table("_Cluster", [&](cyng::store::table const* tbl_cluster)->void {

			auto rec = tbl_cluster->lookup(cyng::table::key_generator(source));
			if (!rec.empty()) {
				auto peer = cyng::object_cast<session>(rec["self"]);
				if (peer != nullptr) {

					CYNG_LOG_INFO(logger_, "bus.res.gateway.proxy - send to peer "
						<< cyng::value_cast<std::string>(rec["class"], "")
						<< '@'
						<< peer->vm_.tag()
						<< " ["
						<< channel
						<< "]");


					//
					//	forward data
					//
					peer->vm_.async_run(node::bus_res_com_sml(
						  ident	
						, source
						, sequence
						, gw_key
						, ws
						, channel
						, server_id
						, code
						, params));	//	params						
				}
			}
			else {
				CYNG_LOG_WARNING(logger_, "bus.res.query.gateway - peer not found " << source);
			}
		});
	}

	void cluster::routing_back_meters(boost::uuids::uuid pk
		, boost::uuids::uuid source
		, std::uint64_t sequence
		, cyng::vector_t gw_key
		, boost::uuids::uuid ws
		, std::string channel
		, std::string server_id
		, std::string code
		, cyng::param_map_t& meters)
	{
		//
		//	preconditions are
		//
		//	* configuration flag for auto insert meters is true
		//	* meter type is SRV_MBUS or SRV_SERIAL - see node::sml::get_srv_type()
		//	* TGateway key has correct size (and is not empty)
		//
		//	Additionally:
		//	* meters from gateways are unique by meter number ("meter") AND gateway (gw)
		//
		auto const gw_tag = cyng::value_cast(gw_key.at(0), boost::uuids::nil_uuid());

		//
		//	Use the incoming active meter devices to populate TMeter table
		//
		for (auto pos = meters.begin(); pos != meters.end(); ) {
		//for (auto & meter : meters) {

			auto data_ptr = const_cast<cyng::param_map_t*>(cyng::object_cast<cyng::param_map_t>(pos->second));
			BOOST_ASSERT(data_ptr != nullptr);
			if (data_ptr == nullptr)	continue;

			CYNG_LOG_INFO(logger_, "meter configuration - " << cyng::io::to_str(*data_ptr));

			const auto ident = cyng::find_value(*data_ptr, "8181C78204FF", std::string{});
			const auto type = cyng::find_value<std::uint32_t>(*data_ptr, "type", 0u);

			if (type < 4)	//	 meter type is SRV_MBUS_WIRED, SRV_MBUS_RADIO, SRV_W_MBUS or SRV_SERIAL
			{
				CYNG_LOG_TRACE(logger_, "lookup TMeter for "
					<< ident
					<< " with "
					<< cache_.db_.size("TMeter")
					<< " record(s)");

				cache_.db_.access([&](cyng::store::table* tbl_meter, cyng::store::table const* tbl_cfg) -> void {

					//
					//	search for meter on this gateway
					//
					auto const rec = lookup_meter(tbl_meter, ident, gw_tag);

					//
					//	insert meter if not found
					//
					if (rec.empty()) {

						//
						//	no entry in TMeter table - create one
						//
						const auto tag = uidgen_();

						const auto meter = cyng::find_value<std::string>(*data_ptr, "serial", "");
						const auto maker = cyng::find_value<std::string>(*data_ptr, "maker", "");
						const auto class_name = cyng::find_value<cyng::buffer_t>(*data_ptr, "8181C78202FF", cyng::buffer_t{});
						const auto age = cyng::find_value(*data_ptr, "010000090B00", std::chrono::system_clock::now());

						//
						//	generate meter code
						//
						std::stringstream ss;
						ss
							<< cyng::value_cast<std::string>(get_config(tbl_cfg, "country-code"), "AU")
							<< std::setfill('0')
							<< std::setw(5)
							<< 0	//	1 .. 5
							<< +tag.data[15]	//	6
							<< +tag.data[14]	//	7
							<< +tag.data[13]	//	8
							<< +tag.data[12]	//	9
							<< +tag.data[11]	//	10
							<< +tag.data[10]	//	11
							<< std::setw(20)
							<< meter
							;
						const auto mc = ss.str();

						auto const key = cyng::table::key_generator(tag);

						if (cache_.is_catch_meters()) {

							CYNG_LOG_INFO(logger_, "auto insert TMeter " << tag << " : " << meter << " mc: " << mc);

							//
							//	catch meters automatically
							//
							tbl_meter->insert(key
								, cyng::table::data_generator(ident, meter, mc, maker, age, "", "", "", "", class_name, gw_tag)
								, 0
								, source);
						}

						CYNG_LOG_TRACE(logger_, "bus.res.query.gateway - meter pk " << tag);
						data_ptr->emplace("pk", cyng::make_object(key));
						data_ptr->emplace("mc", cyng::make_object(mc));	//	metering code

					}
					else {

						auto const code = cyng::value_cast<std::string>(rec["code"], "");
						CYNG_LOG_TRACE(logger_, "bus.res.query.gateway - found pk " 
							<< cyng::io::to_str(rec.key())
							<< " for meter "
							<< ident
							<< ", mc: "
							<< code);
						data_ptr->emplace("pk", cyng::make_object(rec.key()));
						if (code.empty()) {
							data_ptr->emplace("mc", cyng::make_object("MC00000000000000000000000000"));	//	metering code
						}
						else {
							data_ptr->emplace("mc", rec["code"]);	//	metering code
						}
					}

				}	, cyng::store::write_access("TMeter")
					, cyng::store::read_access("_Config"));

				//
				//	next device
				//
				++pos;
			}
			else {

				//
				//	dont't forward unknown device types
				//
				CYNG_LOG_WARNING(logger_, "bus.res.query.gateway - skip meter " 
					<< ident
					<< " of type "
					<< type);

				pos = meters.erase(pos);
			}
		}	//	for(...


		routing_back(pk
			, source
			, sequence
			, gw_key
			, ws
			, channel
			, server_id
			, code
			, meters);

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
		CYNG_LOG_INFO(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

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
		cache_.insert_msg(cyng::logging::severity::LEVEL_INFO
			, ("attention message from " + std::get<4>(tpl) + ": " + std::get<6>(tpl))
			, std::get<1>(tpl));

		//
		//	routing back - forward to receiver
		//
		cache_.read_table("_Cluster", [&](cyng::store::table const* tbl_cluster)->void {

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
		});
	}

	void cluster::bus_req_com_task(cyng::context& ctx)
	{
		//	[9f773865-e4af-489a-8824-8f78a2311278,3,7a7f89be-5440-4e57-94f1-88483c022d19,09be403a-aa78-4946-9cc0-0f510441717c,req.config,[monitor,influxdb,lineProtocol,db,multiple,single],[tsdb]]
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_INFO(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] ident
			std::uint64_t,			//	[1] cluster sequence
			boost::uuids::uuid,		//	[2] task key
			boost::uuids::uuid,		//	[3] source (web socket)
			std::string,			//	[4] channel
			cyng::vector_t,			//	[5] sections
			cyng::vector_t			//	[6] params
		>(frame);

		//
		//	ToDo: search session and forward request
		//
	}

	void cluster::bus_req_com_node(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_INFO(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		//
		//	ToDo: search session and forward request
		//
	}


	std::tuple<session const*, cyng::table::record, cyng::buffer_t, boost::uuids::uuid> cluster::find_peer(cyng::table::key_type const& key_gw
		, const cyng::store::table* tbl_session
		, const cyng::store::table* tbl_gw)
	{
#ifdef _DEBUG
		CYNG_LOG_TRACE(logger_, "find_peer("
			<< cyng::value_cast(key_gw.at(0), boost::uuids::nil_uuid())
			<< " in "
			<< tbl_session->size()
			<< " sessions)");
#endif

		//
		//	get gateway record
		//
		auto rec_gw = tbl_gw->lookup(key_gw);
		BOOST_ASSERT(!rec_gw.empty());
		if (!rec_gw.empty())
		{
			//
			//	gateway and (associated) device share the same key
			//
			auto rec_session = tbl_session->find_first(cyng::param_t("device", key_gw.at(0)));
			if (!rec_session.empty())
			{
				auto peer = cyng::object_cast<session>(rec_session["local"]);
				if (peer && (key_gw.size() == 1))
				{
					//
					//	get remote session tag
					//
					auto const tag = cyng::value_cast(rec_session.key().at(0), boost::uuids::nil_uuid());
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
		}
		else
		{
			CYNG_LOG_ERROR(logger_, "find_peer(gateway "
				<< cyng::value_cast(key_gw.at(0), boost::uuids::nil_uuid())
				<< " not found, "
				<< tbl_gw->size()
				<< " gateways configured)");
		}

#ifdef _DEBUG
		//std::size_t counter{ 0 };
		//tbl_session->loop([&](cyng::table::record const& rec) -> bool {

		//	CYNG_LOG_TRACE(logger_, "session #"
		//		<< counter
		//		<< " - "
		//		<< cyng::io::to_str(rec.convert()));

		//	counter++;
		//	return true;
		//});
#endif

		//
		//	not found - peer is a null pointer
		//
		return std::make_tuple(nullptr, rec_gw, cyng::buffer_t{}, boost::uuids::nil_uuid());
	}


}
