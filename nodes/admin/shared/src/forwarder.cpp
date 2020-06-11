/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#include "forwarder.h"
#include "tables.h"
#include <smf/shared/db_schemes.h>
#include <smf/cluster/generator.h>

#include <cyng/table/key.hpp>
#include <cyng/io/serializer.h>
#include <cyng/tuple_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/parser/buffer_parser.h>
#include <cyng/parser/mac_parser.h>
#include <cyng/json/json_parser.h>
#include <cyng/dom/reader.h>
#include <cyng/xml.h>
#include <cyng/csv.h>
#include <cyng/json.h>

#include <boost/algorithm/string.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/core/ignore_unused.hpp>

namespace node
{
	void fwd_insert(cyng::logging::log_ptr logger
		, cyng::context& ctx
		, cyng::reader<cyng::object> const& reader
		, boost::uuids::uuid tag)
	{
		//	{"cmd":"insert","channel":"config.device","rec":{"key":{"pk":"0b5c2a64-5c48-48f1-883b-e5be3a3b1e3d"},"data":{"name":"demo","msisdn":"demo","descr":"comment #55","enabled":true,"pwd":"secret","age":"2018-02-04T14:31:34.000Z"}}}
		const std::string channel = cyng::value_cast<std::string>(reader.get("channel"), "");
		CYNG_LOG_TRACE(logger, "ws.read - insert channel [" << channel << "]");
		if (boost::algorithm::starts_with(channel, "config.device"))
		{
			ctx.queue(bus_req_db_insert("TDevice"
				//	generate new key
				, cyng::table::key_generator(tag)
				//	build data vector
				, cyng::vector_t{ reader["rec"]["data"].get("name")
				, reader["rec"]["data"].get("pwd")
				, reader["rec"]["data"].get("msisdn")
				, reader["rec"]["data"].get("descr")
				, cyng::make_object("")	//	model
				, cyng::make_object("")	//	version
				, reader["rec"]["data"].get("enabled")
				, cyng::make_now()
				, cyng::make_object(static_cast<std::uint32_t>(6)) }
				, 0
				, ctx.tag()));
		}
		else
		{
			CYNG_LOG_WARNING(logger, "ws.read - unknown insert channel [" << channel << "]");
		}
	}

	void fwd_delete_device(cyng::logging::log_ptr logger
		, cyng::context& ctx
		, cyng::reader<cyng::object> const& reader
		, std::string const& tbl_name)
	{
		BOOST_ASSERT(tbl_name == "TDevice");

		//
		//	TDevice key is of type UUID
		//
		cyng::vector_t const vec = cyng::to_vector(reader["key"].get("tag"));
		BOOST_ASSERT_MSG(vec.size() == 1, "TDevice key has wrong size");
		auto key = cyng::table::key_generator(vec.at(0));
		ctx.queue(bus_req_db_remove(tbl_name, key, ctx.tag()));
	}

	void fwd_delete_gateway(cyng::logging::log_ptr logger
		, cyng::context& ctx
		, cyng::reader<cyng::object> const& reader
		, std::string const& tbl_name)
	{
		BOOST_ASSERT(tbl_name == "TGateway");

		//
		//	TGateway key is of type UUID
		//
		cyng::vector_t const vec = cyng::to_vector(reader["key"].get("tag"));
		BOOST_ASSERT_MSG(vec.size() == 1, "TGateway key has wrong size");
		auto key = cyng::table::key_generator(vec.at(0));
		ctx.queue(bus_req_db_remove(tbl_name, key, ctx.tag()));
	}

	void fwd_delete_meter(cyng::logging::log_ptr logger
		, cyng::context& ctx
		, cyng::reader<cyng::object> const& reader
		, std::string const& tbl_name)
	{
		BOOST_ASSERT(tbl_name == "TMeter");

		//
		//	TDevice key is of type UUID
		//
		cyng::vector_t const vec = cyng::to_vector(reader["key"].get("tag"));
		BOOST_ASSERT_MSG(vec.size() == 1, "TMeter key has wrong size");
		auto key = cyng::table::key_generator(vec.at(0));
		ctx.queue(bus_req_db_remove(tbl_name, key, ctx.tag()));
	}

	void fwd_delete_iec(cyng::logging::log_ptr logger
		, cyng::context& ctx
		, cyng::reader<cyng::object> const& reader
		, std::string const& tbl_name)
	{
		BOOST_ASSERT(tbl_name == "TIECBridge");

		//
		//	TIECBridge key is of type UUID
		//
		cyng::vector_t const vec = cyng::to_vector(reader["key"].get("tag"));
		BOOST_ASSERT_MSG(vec.size() == 1, "TIECBridge key has wrong size");
		auto key = cyng::table::key_generator(vec.at(0));
		ctx.queue(bus_req_db_remove(tbl_name, key, ctx.tag()));
	}

	void fwd_delete_lora(cyng::logging::log_ptr logger
		, cyng::context& ctx
		, cyng::reader<cyng::object> const& reader
		, std::string const& tbl_name)
	{
		BOOST_ASSERT(tbl_name == "TLoRaDevice");
		//
		//	TLoRaDevice key is of type UUID
		//
		cyng::vector_t vec;
		vec = cyng::value_cast(reader["key"].get("tag"), vec);
		BOOST_ASSERT_MSG(vec.size() == 1, "TLoRaDevice key has wrong size");
		auto key = cyng::table::key_generator(vec.at(0));
		ctx.queue(bus_req_db_remove(tbl_name, key, ctx.tag()));
	}

	void fwd_delete(cyng::logging::log_ptr logger
		, cyng::context& ctx
		, cyng::reader<cyng::object> const& reader)
	{
		//	[ce28c0c4-1914-45d1-bcf1-22bcbe461855,{("cmd":delete),("channel":config.device),("key":{("tag":a8b3d691-6ea9-40d3-83aa-b1e4dfbcb2f1)})}]
		const std::string channel = cyng::value_cast<std::string>(reader.get("channel"), "");
		CYNG_LOG_TRACE(logger, "ws.read - delete channel [" << channel << "]");
		try {

			auto const rel = channel::find_rel_by_channel(channel);

			if (boost::algorithm::starts_with(channel, "config.device"))
			{
				fwd_delete_device(logger, ctx, reader, rel.table_);
			}
			else if (boost::algorithm::starts_with(channel, "config.gateway"))
			{
				fwd_delete_gateway(logger, ctx, reader, rel.table_);
			}
			else if (boost::algorithm::starts_with(channel, "config.meter"))
			{
				fwd_delete_meter(logger, ctx, reader, rel.table_);
			}
			else if (boost::algorithm::starts_with(channel, "config.iec"))
			{
				fwd_delete_iec(logger, ctx, reader, rel.table_);
			}
			else if (boost::algorithm::starts_with(channel, "config.lora"))
			{
				fwd_delete_lora(logger, ctx, reader, rel.table_);
			}
			else
			{
				CYNG_LOG_WARNING(logger, "ws.read - unknown delete channel [" << channel << "]");
			}
		}
		catch (std::exception const& ex) {
			CYNG_LOG_ERROR(logger, "ws.read - delete channel ["
				<< channel
				<< "] failed: "
				<< ex.what());
		}
	}

	void fwd_modify_device(cyng::logging::log_ptr logger
		, cyng::context& ctx
		, cyng::reader<cyng::object> const& reader
		, std::string const& tbl_name)
	{
		BOOST_ASSERT(tbl_name == "TDevice");

		//
		//	TDevice key is of type UUID
		//	all values have the same key
		//
		auto const vec = cyng::to_vector(reader["rec"].get("key"));
		BOOST_ASSERT_MSG(vec.size() == 1, "TDevice key has wrong size");

		auto key = cyng::table::key_generator(vec.at(0));

		auto const tpl = cyng::to_tuple(reader["rec"].get("data"));
		for (auto p : tpl)
		{
			cyng::param_t param;
			ctx.queue(bus_req_db_modify(tbl_name
				, key
				, cyng::value_cast(p, param)
				, 0
				, ctx.tag()));
		}
	}

	void fwd_modify_gateway(cyng::logging::log_ptr logger
		, cyng::context& ctx
		, cyng::reader<cyng::object> const& reader
		, std::string const& tbl_name)
	{
		BOOST_ASSERT(tbl_name == "TGateway");

		//
		//	TGateway key is of type UUID
		//	all values have the same key
		//
		auto const vec = cyng::to_vector(reader["rec"].get("key"));
		BOOST_ASSERT_MSG(vec.size() == 1, "TGateway key has wrong size");
		auto key = cyng::table::key_generator(vec.at(0));

		auto const tpl = cyng::to_tuple(reader["rec"].get("data"));
		for (auto p : tpl)
		{
			cyng::param_t param;
			param = cyng::value_cast(p, param);

			if (boost::algorithm::equals(param.first, "serverId")) {
				//
				//	to uppercase
				//
				std::string server_id;
				server_id = cyng::value_cast(param.second, server_id);
				boost::algorithm::to_upper(server_id);
				ctx.queue(bus_req_db_modify(tbl_name
					, key
					, cyng::param_factory("serverId", server_id)
					, 0
					, ctx.tag()));
			}
			else {
				ctx.queue(bus_req_db_modify(tbl_name
					, key
					, cyng::value_cast(p, param)
					, 0
					, ctx.tag()));
			}
		}
	}

	void fwd_modify_system(cyng::logging::log_ptr logger
		, cyng::context& ctx
		, cyng::reader<cyng::object> const& reader
		, std::string const& tbl_name)
	{
		BOOST_ASSERT(tbl_name == "_Config");

		//	{"cmd":"modify","channel":"config.system","rec":{"key":{"name":"connection-auto-login"},"data":{"value":true}}}
		std::string const name = cyng::value_cast<std::string>(reader["rec"]["key"].get("name"), "?");
		cyng::object const value = reader["rec"]["data"].get("value");
		ctx.queue(bus_req_db_modify(tbl_name
			//	generate new key
			, cyng::table::key_generator(reader["rec"]["key"].get("name"))
			//	build parameter
			, cyng::param_t("value", value)
			, 0
			, ctx.tag()));

		std::stringstream ss;
		ss
			<< "system configuration for "
			<< name
			<< " has changed ["
			<< value.get_class().type_name()
			<< "] = "
			<< cyng::io::to_str(value)
			;

		ctx.queue(bus_insert_msg(cyng::logging::severity::LEVEL_WARNING, ss.str()));
	}

	void fwd_modify_meter(cyng::logging::log_ptr logger
		, cyng::context& ctx
		, cyng::reader<cyng::object> const& reader
		, std::string const& tbl_name)
	{
		BOOST_ASSERT(tbl_name == "TMeter");

		auto const vec = cyng::to_vector(reader["rec"].get("key"));
		BOOST_ASSERT_MSG(vec.size() == 1, "TMeter key has wrong size");

		auto const key = cyng::table::key_generator(vec.at(0));

		auto const tpl = cyng::to_tuple(reader["rec"].get("data"));
		for (auto p : tpl)
		{
			cyng::param_t param;
			ctx.queue(bus_req_db_modify(tbl_name
				, key
				, cyng::value_cast(p, param)
				, 0
				, ctx.tag()));
		}
	}

	void fwd_modify_iec(cyng::logging::log_ptr logger
		, cyng::context& ctx
		, cyng::reader<cyng::object> const& reader
		, std::string const& tbl_name)
	{
		BOOST_ASSERT(tbl_name == "TIECBridge");

		auto const vec = cyng::to_vector(reader["rec"].get("key"));
		BOOST_ASSERT_MSG(vec.size() == 1, "TIECBridge key has wrong size");

		auto key = cyng::table::key_generator(vec.at(0));

		auto const tpl = cyng::to_tuple(reader["rec"].get("data"));
		for (auto p : tpl)
		{
			cyng::param_t param;
			ctx.queue(bus_req_db_modify(tbl_name
				, key
				, cyng::value_cast(p, param)
				, 0
				, ctx.tag()));
		}
	}

	void fwd_modify_lora(cyng::logging::log_ptr logger
		, cyng::context& ctx
		, cyng::reader<cyng::object> const& reader
		, std::string const& channel
		, std::string const& tbl_name)
	{
		BOOST_ASSERT(tbl_name == "TLoRaDevice");

		cyng::vector_t vec;
		vec = cyng::value_cast(reader["rec"].get("key"), vec);
		BOOST_ASSERT_MSG(vec.size() == 1, "TLoRaDevice key has wrong size");

		auto key = cyng::table::key_generator(vec.at(0));

		auto const tpl = cyng::to_tuple(reader["rec"].get("data"));
		for (auto p : tpl)
		{
			cyng::param_t param;
			param = cyng::value_cast(p, param);

			if (boost::algorithm::equals(param.first, "activation")) {

				//
				//	convert value from radion button into bool
				//	OTAA == true
				//	ABP == false
				//
				auto const activation = cyng::value_cast<std::string>(reader["rec"]["data"].get("activation"), "OTAA");

				ctx.queue(bus_req_db_modify(tbl_name
					, key
					, cyng::param_factory(param.first, boost::algorithm::equals(activation, "OTAA"))
					, 0
					, ctx.tag()));

			}
			else if (boost::algorithm::equals(param.first, "DevEUI")) {

				//
				//	convert value to mac64
				//
				auto const eui = cyng::value_cast<std::string>(reader["rec"]["data"].get(param.first), "0000:0000:0000:0000");
				std::pair<cyng::mac64, bool > r = cyng::parse_mac64(eui);
				if (r.second) {
					ctx.queue(bus_req_db_modify(tbl_name
						, key
						, cyng::param_factory(param.first, r.first)
						, 0
						, ctx.tag()));
				}
				else {
					CYNG_LOG_WARNING(logger, "modify channel ["
						<< channel <<
						"] with invalid "
						<< param.first
						<< ": "
						<< eui);
				}
			}
			else if (boost::algorithm::equals(param.first, "AppEUI")) {

				//
				//	convert value to mac64
				//
				auto const eui = cyng::value_cast<std::string>(reader["rec"]["data"].get(param.first), "0000:0000:0000:0000");
				std::pair<cyng::mac64, bool > r = cyng::parse_mac64(eui);
				if (r.second) {
					ctx.queue(bus_req_db_modify(tbl_name
						, key
						, cyng::param_factory(param.first, r.first)
						, 0
						, ctx.tag()));
				}
				else {
					CYNG_LOG_WARNING(logger, "modify channel ["
						<< channel <<
						"] with invalid "
						<< param.first
						<< ": "
						<< eui);
				}
			}
			else if (boost::algorithm::equals(param.first, "GatewayEUI")) {

				//
				//	convert value to mac64
				//
				auto const eui = cyng::value_cast<std::string>(reader["rec"]["data"].get(param.first), "0000:0000:0000:0000");
				std::pair<cyng::mac64, bool > r = cyng::parse_mac64(eui);
				if (r.second) {
					ctx.queue(bus_req_db_modify(tbl_name
						, key
						, cyng::param_factory(param.first, r.first)
						, 0
						, ctx.tag()));
				}
				else {
					CYNG_LOG_WARNING(logger, "modify channel ["
						<< channel <<
						"] with invalid "
						<< param.first
						<< ": "
						<< eui);
				}
			}
			else {
				ctx.queue(bus_req_db_modify(tbl_name
					, key
					, param
					, 0
					, ctx.tag()));
			}
		}
	}

	void fwd_modify_web(cyng::logging::log_ptr logger
		, cyng::context& ctx
		, cyng::reader<cyng::object> const& reader)
	{
		const std::string name = cyng::value_cast<std::string>(reader["rec"]["key"].get("name"), "?");
		const cyng::object value = reader["rec"]["data"].get("value");
		ctx.queue(cyng::generate_invoke("log.msg.info", "modify web configuration", name, value));

		ctx.queue(cyng::generate_invoke("modify.web.config", name, value));

	}

	void fwd_modify(cyng::logging::log_ptr logger
		, cyng::context& ctx
		, cyng::reader<cyng::object> const& reader)
	{
		const std::string channel = cyng::value_cast<std::string>(reader.get("channel"), "");
		CYNG_LOG_TRACE(logger, "ws.read - modify channel [" << channel << "]");
		try {

			auto const rel = channel::find_rel_by_channel(channel);

			if (boost::algorithm::starts_with(channel, "config.device"))
			{
				fwd_modify_device(logger, ctx, reader, rel.table_);
			}
			else if (boost::algorithm::starts_with(channel, "config.gateway"))
			{
				fwd_modify_gateway(logger, ctx, reader, rel.table_);
			}
			else if (boost::algorithm::starts_with(channel, "config.meter"))
			{
				fwd_modify_meter(logger, ctx, reader, rel.table_);
			}
			else if (boost::algorithm::starts_with(channel, "config.iec"))
			{
				fwd_modify_iec(logger, ctx, reader, rel.table_);
			}
			else if (boost::algorithm::starts_with(channel, "config.lora"))
			{
				fwd_modify_lora(logger, ctx, reader, channel, rel.table_);
			}
			else if (boost::algorithm::starts_with(channel, "config.system"))
			{
				fwd_modify_system(logger, ctx, reader, rel.table_);
			}
			else if (boost::algorithm::starts_with(channel, "config.web"))
			{
				fwd_modify_web(logger, ctx, reader);
			}
			else
			{
				CYNG_LOG_WARNING(logger, "ws.read - unknown modify channel [" << channel << "]");
			}
		}
		catch (std::exception const& ex) {
			CYNG_LOG_ERROR(logger, "ws.read - modify channel ["
				<< channel <<
				"] failed:"
				<< ex.what());
		}
	}

	void fwd_stop(cyng::logging::log_ptr logger
		, cyng::context& ctx
		, cyng::reader<cyng::object> const& reader)
	{
		const std::string channel = cyng::value_cast<std::string>(reader.get("channel"), "");
		CYNG_LOG_TRACE(logger, "ws.read - stop channel [" << channel << "]");
		if (boost::algorithm::starts_with(channel, "status.session"))
		{
			//
			//	TDevice key is of type UUID
			//	all values have the same key
			//
			try {

				auto const vec = cyng::to_vector(reader.get("key"));
				BOOST_ASSERT_MSG(vec.size() == 1, "*Session key has wrong size");

				//
				//	This should be effectively the same as vec
				//	(key == vec)
				//	
				auto key = cyng::table::key_generator(vec.at(0));
				ctx.queue(bus_req_stop_client(key, ctx.tag()));

			}
			catch (std::exception const& ex) {
				CYNG_LOG_ERROR(logger, "ws.read - modify channel [" 
					<< channel << 
					"] failed: "
					<< ex.what());
			}
		}
		else
		{
			CYNG_LOG_WARNING(logger, "ws.read - unknown stop channel [" << channel << "]");
		}
	}

	void fwd_com_sml(cyng::logging::log_ptr logger
		, cyng::context& ctx
		, boost::uuids::uuid tag_ws
		, std::string const& channel
		, cyng::reader<cyng::object> const& reader)
	{
        //	msgType: [string: SML message type: getProfileList, getProcParameter, getList, getProfilePack, ...]
        //	channel: [string/OBIS]
        //	gw: [vector: gateway PK]
        //	params: [tuple: optional params]

		auto const gw = cyng::to_vector(reader.get("gw"));
		if (!gw.empty()) {

			auto const channel = cyng::value_cast<std::string>(reader.get("channel"), "");
			auto const section = cyng::value_cast<std::string>(reader.get("section"), "");
			auto const r = cyng::parse_hex_string(section);
			if (r.second) {

				auto const params = cyng::to_tuple(reader.get("params"));

#ifdef _DEBUG
				CYNG_LOG_DEBUG(logger, "\"sml:com\" ws: " 
					<< tag_ws 
					<< ", section " 
					<< section
					<< " ==> " 
					<< cyng::io::to_str(params));

#endif
				//	"bus.req.proxy.gateway"
				ctx.queue(bus_req_com_sml(tag_ws	//	web-socket tag (origin)
					, channel
					, r.first	//	OBIS root code
					, gw	//	key into TGateway and TDevice table
					, params));	//	parameters, requests, commands
			}
			else {
				CYNG_LOG_ERROR(logger, "\"sml:com\" from ws: " << tag_ws << " with invalid section " << section);
			}
		}
		else {
			CYNG_LOG_ERROR(logger, "\"sml:com\" from ws: " << tag_ws << " with empty gateway PK");
		}
	}

	void fwd_com_proxy(cyng::logging::log_ptr logger
		, cyng::context& ctx
		, boost::uuids::uuid tag_ws
		, std::string const& job
		, cyng::reader<cyng::object> const& reader)
	{
		auto const gw = cyng::to_vector(reader.get("gw"));
		if (!gw.empty()) {

			auto const sections = cyng::to_vector(reader.get("sections"));
#ifdef _DEBUG
			CYNG_LOG_DEBUG(logger, "\"sml:proxy\" ws: "
				<< tag_ws
				<< " job "
				<< job
				<< " ==> "
				<< cyng::io::to_str(sections));

#endif
			//	"bus.req.proxy.job"
			ctx.queue(bus_req_com_proxy(tag_ws	//	web-socket tag (origin)
				, job
				, gw
				, sections));	

		}
		else {
			CYNG_LOG_ERROR(logger, "\"sml:proxy\" from ws: " << tag_ws << " with empty gateway PK");
		}
	}

	void fwd_com_task(cyng::logging::log_ptr logger
		, cyng::context& ctx
		, boost::uuids::uuid tag_ws
		, std::string const& channel
		, cyng::reader<cyng::object> const& reader)
	{
		auto const tag = cyng::value_cast(reader.get("key"), boost::uuids::nil_uuid());
		CYNG_LOG_WARNING(logger, "ws tag: " << tag_ws << " - TASK key" << tag);

		ctx.queue(bus_req_com_task(tag	//	key 
			, tag_ws	//	web-socket tag
			, channel
			, cyng::to_vector(reader.get("section"))
			, cyng::to_vector(reader.get("params"))));
	}

	void fwd_com_node(cyng::logging::log_ptr logger
		, cyng::context& ctx
		, boost::uuids::uuid tag_ws
		, std::string const& channel
		, cyng::reader<cyng::object> const& reader)
	{
		auto const tag = cyng::value_cast(reader.get("key"), boost::uuids::nil_uuid());
		CYNG_LOG_WARNING(logger, "ws tag: " << tag_ws << " - NODE key" << tag);

		ctx.queue(bus_req_com_node(tag	//	key
			, tag_ws	//	web-socket tag
			, channel
			, cyng::to_vector(reader.get("section"))
			, cyng::to_vector(reader.get("params"))));
	}



	forward::forward(cyng::logging::log_ptr logger
		, cyng::store::db& db
		, connection_manager_interface& cm)
	: logger_(logger)
		, db_(db)
		, connection_manager_(cm)
		, uidgen_()
	{}

	void forward::register_this(cyng::controller& vm)
	{
		vm.register_function("cfg.upload.devices", 2, std::bind(&forward::cfg_upload_devices, this, std::placeholders::_1));
		vm.register_function("cfg.upload.gateways", 2, std::bind(&forward::cfg_upload_gateways, this, std::placeholders::_1));
		vm.register_function("cfg.upload.meter", 2, std::bind(&forward::cfg_upload_meter, this, std::placeholders::_1));
		vm.register_function("cfg.upload.LoRa", 2, std::bind(&forward::cfg_upload_LoRa, this, std::placeholders::_1));

		vm.register_function("http.post.json", 5, std::bind(&forward::cfg_post_json, this, std::placeholders::_1));
		vm.register_function("http.post.form.urlencoded", 5, std::bind(&forward::cfg_post_form_urlencoded, this, std::placeholders::_1));
	}

	void forward::cfg_upload_devices(cyng::context& ctx)
	{
		//
		//	[cbc55458-5e42-406b-bc7e-95e4fb69c7f1,
		//	%(
		//		("data":C:\Users\Pyrx\AppData\Local\Temp\smf-dash-e0ee-17ae-63c8-b2dc.tmp),
		//		("devices-2.xml":text/xml),
		//		("policy":append),
		//		("target":/config/upload.devices),
		//		("version":v50))]
		//
		auto const frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "cfg.upload.device - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,	//	[0] session tag
			cyng::param_map_t	//	[1] variables
		>(frame);

		auto const file_name = cyng::value_cast<std::string>(cyng::find(std::get<1>(tpl), "file"), "");
		auto const version = cyng::value_cast<std::string>(cyng::find(std::get<1>(tpl), "version"), "v0.8");
		auto const policy = cyng::value_cast<std::string>(cyng::find(std::get<1>(tpl), "policy"), "append");

		//
		//	get pointer to XML data
		//
		pugi::xml_document doc;
		const pugi::xml_parse_result result = doc.load_file(file_name.c_str());
		if (result) {

			if (boost::algorithm::equals(version, "v3.2")) {
				read_device_configuration_3_2(ctx, doc, boost::algorithm::equals(policy, "append"));
			}
			else if (boost::algorithm::equals(version, "v4.0")) {
				read_device_configuration_4_0(ctx, doc, boost::algorithm::equals(policy, "append"));
			}
			else {
				read_device_configuration_5_x(ctx, doc, cyng::table::to_policy(policy));
			}
		}
		else {
			std::stringstream ss;
			ss
				<< "XML ["
				<< file_name
				<< "] parsed with errors: ["
				<< result.description()
				<< "]\n"
				<< "Error offset: "
				<< result.offset
				;

			ctx.queue(bus_insert_msg(cyng::logging::severity::LEVEL_WARNING, ss.str()));
		}
	}

	void forward::cfg_upload_gateways(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "cfg.upload.gateways - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,	//	[0] session tag
			cyng::param_map_t	//	[1] variables
		>(frame);

		auto const file_name = cyng::value_cast<std::string>(cyng::find(std::get<1>(tpl), "data"), "");
		auto const version = cyng::value_cast<std::string>(cyng::find(std::get<1>(tpl), "version"), "v0.5");
		auto const policy = cyng::table::to_policy(cyng::value_cast<std::string>(cyng::find(std::get<1>(tpl), "policy"), "append"));

		//
		//	get pointer to XML data
		//
		pugi::xml_document doc;
		const pugi::xml_parse_result result = doc.load_file(file_name.c_str());
		if (result) {

			std::size_t counter{ 0 };
			pugi::xpath_node_set data = doc.select_nodes("/TGateway/record");

			//
			//	target scheme required
			//
			auto meta = create_meta("TGateway");
			for (pugi::xpath_node_set::const_iterator it = data.begin(); it != data.end(); ++it)
			{
				counter++;
				pugi::xml_node node = it->node();

				auto rec = cyng::xml::read(node, meta);
				if (!rec.empty()) {

					CYNG_LOG_INFO(logger_, "session "
						<< ctx.tag()
						<< " - insert gateway #"
						<< counter
						<< " "
						<< cyng::value_cast(rec["pk"], boost::uuids::nil_uuid())
						<< " - "
						<< cyng::value_cast<std::string>(rec["serverId"], ""));

					switch (policy) {
					case cyng::table::POLICY_MERGE:
						ctx.queue(bus_req_db_merge("TGateway", rec.key(), rec.data(), rec.get_generation(), ctx.tag()));
						break;
					case cyng::table::POLICY_SUBSTITUTE:
						ctx.queue(bus_req_db_update("TGateway", rec.key(), rec.data(), rec.get_generation(), ctx.tag()));
						break;
					default:
						ctx.queue(bus_req_db_insert("TGateway", rec.key(), rec.data(), rec.get_generation(), ctx.tag()));
						break;
					}
				}
				else {

					CYNG_LOG_ERROR(logger_, "session "
						<< ctx.tag()
						<< " - TGateway record #"
						<< counter
						<< " is empty");
				}
			}
		}
		else {
			std::stringstream ss;
			ss
				<< "XML ["
				<< file_name
				<< "] parsed with errors: ["
				<< result.description()
				<< "]\n"
				<< "Error offset: "
				<< result.offset
				;

			ctx.queue(bus_insert_msg(cyng::logging::severity::LEVEL_WARNING, ss.str()));
		}
	}

	void forward::cfg_upload_meter(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,	//	[0] session tag
			cyng::param_map_t	//	[1] variables
		>(frame);

		auto const file_name = cyng::value_cast<std::string>(cyng::find(std::get<1>(tpl), "data"), "");
		auto const version = cyng::value_cast<std::string>(cyng::find(std::get<1>(tpl), "version"), "v0.5");
		auto const policy = cyng::table::to_policy(cyng::value_cast<std::string>(cyng::find(std::get<1>(tpl), "policy"), "append"));

		//
		//	get pointer to XML data
		//
		pugi::xml_document doc;
		const pugi::xml_parse_result result = doc.load_file(file_name.c_str());
		if (result) {

			std::size_t counter{ 0 };
			pugi::xpath_node_set data = doc.select_nodes("/TMeter/record");

			//
			//	target scheme required
			//
			auto meta = create_meta("TMeter");
			for (pugi::xpath_node_set::const_iterator it = data.begin(); it != data.end(); ++it)
			{
				counter++;
				pugi::xml_node node = it->node();

				auto rec = cyng::xml::read(node, meta);
				if (!rec.empty()) {

					CYNG_LOG_INFO(logger_, "session "
						<< ctx.tag()
						<< " - insert meter #"
						<< counter
						<< " "
						<< cyng::value_cast(rec["pk"], boost::uuids::nil_uuid())
						<< " - "
						<< cyng::value_cast<std::string>(rec["serverId"], ""));

					switch (policy) {
					case cyng::table::POLICY_MERGE:
						ctx.queue(bus_req_db_merge("TMeter", rec.key(), rec.data(), rec.get_generation(), ctx.tag()));
						break;
					case cyng::table::POLICY_SUBSTITUTE:
						ctx.queue(bus_req_db_update("TMeter", rec.key(), rec.data(), rec.get_generation(), ctx.tag()));
						break;
					default:
						ctx.queue(bus_req_db_insert("TMeter", rec.key(), rec.data(), rec.get_generation(), ctx.tag()));
						break;
					}
				}
				else {

					CYNG_LOG_ERROR(logger_, "session "
						<< ctx.tag()
						<< " - TMeter record #"
						<< counter
						<< " is empty");
				}
			}

		}
		else {
			std::stringstream ss;
			ss
				<< "XML ["
				<< file_name
				<< "] parsed with errors: ["
				<< result.description()
				<< "]\n"
				<< "Error offset: "
				<< result.offset
				;

			ctx.queue(bus_insert_msg(cyng::logging::severity::LEVEL_WARNING, ss.str()));
		}
	}

	void forward::cfg_upload_LoRa(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,	//	[0] session tag
			cyng::param_map_t	//	[1] variables
		>(frame);


		auto const file_name = cyng::value_cast<std::string>(cyng::find(std::get<1>(tpl), "data"), "");
		auto const version = cyng::value_cast<std::string>(cyng::find(std::get<1>(tpl), "version"), "v0.5");
		auto const policy = cyng::table::to_policy(cyng::value_cast<std::string>(cyng::find(std::get<1>(tpl), "policy"), "append"));

		//
		//	get pointer to XML data
		//
		pugi::xml_document doc;
		const pugi::xml_parse_result result = doc.load_file(file_name.c_str());
		if (result) {

			std::size_t counter{ 0 };
			pugi::xpath_node_set data = doc.select_nodes("/TLoRaDevice/record");

			//
			//	target scheme required
			//
			auto meta = create_meta("TLoRaDevice");
			for (pugi::xpath_node_set::const_iterator it = data.begin(); it != data.end(); ++it)
			{
				counter++;
				pugi::xml_node node = it->node();

				auto rec = cyng::xml::read(node, meta);
				if (!rec.empty()) {

					CYNG_LOG_INFO(logger_, "session "
						<< ctx.tag()
						<< " - insert LoRa device #"
						<< counter
						<< " "
						<< cyng::value_cast(rec["pk"], boost::uuids::nil_uuid())
						<< " - "
						<< cyng::value_cast<std::string>(rec["serverId"], ""));

					switch (policy) {
					case cyng::table::POLICY_MERGE:
						ctx.queue(bus_req_db_merge("TLoRaDevice", rec.key(), rec.data(), rec.get_generation(), ctx.tag()));
						break;
					case cyng::table::POLICY_SUBSTITUTE:
						ctx.queue(bus_req_db_update("TLoRaDevice", rec.key(), rec.data(), rec.get_generation(), ctx.tag()));
						break;
					default:
						ctx.queue(bus_req_db_insert("TLoRaDevice", rec.key(), rec.data(), rec.get_generation(), ctx.tag()));
						break;
					}
				}
				else {

					CYNG_LOG_ERROR(logger_, "session "
						<< ctx.tag()
						<< " - TLoRaDevice record #"
						<< counter
						<< " is empty");
				}
			}
		}
		else {
			std::stringstream ss;
			ss
				<< "XML ["
				<< file_name
				<< "] parsed with errors: ["
				<< result.description()
				<< "]\n"
				<< "Error offset: "
				<< result.offset
				;

			ctx.queue(bus_insert_msg(cyng::logging::severity::LEVEL_WARNING, ss.str()));
		}
	}

	void forward::read_device_configuration_3_2(cyng::context& ctx, pugi::xml_document const& doc, bool insert)
	{
		//
		//	example line:
		//	<device auto_update="false" description="BF Haus 18" enabled="true" expires="never" gid="2" latitude="0.000000" limit="no" locked="false" longitude="0.000000" monitor="00:00:12" name="a00153B018EEFen" number="20113083736" oid="2" pwd="aen_ict_1111" query="6" redirect="" watchdog="00:12:00">cc6a0256-5689-4804-a5fc-bf3599fac23a</device>
		//

		std::size_t counter{ 0 };
		pugi::xpath_node_set data = doc.select_nodes("config/units/device");
		for (pugi::xpath_node_set::const_iterator it = data.begin(); it != data.end(); ++it)
		{
			counter++;
			pugi::xml_node node = it->node();
			const std::string pk_val = node.child_value();
			const auto pk = (pk_val.size() == 36)
				? boost::uuids::string_generator()(pk_val)
				: uidgen_()
				;

			const std::string name = node.attribute("name").value();

			CYNG_LOG_TRACE(logger_, "session "
				<< ctx.tag()
				<< " - insert device "
				<< pk
				<< " - "
				<< name);

			//
			//	forward to process bus task
			//
			if (insert) {

				//
				//	merge
				//
				ctx.queue(bus_req_db_insert("TDevice"
					//	generate new key
					, cyng::table::key_generator(pk)
					//	build data vector
					, cyng::table::data_generator(name
						, node.attribute("pwd").value()
						, node.attribute("number").value()
						, node.attribute("description").value()
						, std::string("")	//	model
						, std::string("")	//	version
						, node.attribute("enabled").as_bool()
						, std::chrono::system_clock::now()
						, node.attribute("query").as_uint())
					, 0
					, ctx.tag()));
			}
			else {

				//
				//	merge
				//
				ctx.queue(bus_req_db_modify("TDevice"
					, cyng::table::key_generator(pk)
					, cyng::param_factory("pwd", node.attribute("pwd").value())
					, 0
					, ctx.tag()));
				ctx.queue(bus_req_db_modify("TDevice"
					, cyng::table::key_generator(pk)
					, cyng::param_factory("msisdn", node.attribute("number").value())
					, 0
					, ctx.tag()));
				ctx.queue(bus_req_db_modify("TDevice"
					, cyng::table::key_generator(pk)
					, cyng::param_factory("descr", node.attribute("description").value())
					, 0
					, ctx.tag()));
				ctx.queue(bus_req_db_modify("TDevice"
					, cyng::table::key_generator(pk)
					, cyng::param_factory("query", node.attribute("query").as_uint())
					, 0
					, ctx.tag()));
			}
		}

		std::stringstream ss;
		ss
			<< counter
			<< " device records (v3.2) uploaded"
			;
		ctx.queue(bus_insert_msg(cyng::logging::severity::LEVEL_INFO, ss.str()));

	}

	void forward::read_device_configuration_4_0(cyng::context& ctx, pugi::xml_document const& doc, bool insert)
	{
		std::size_t counter{ 0 };
		pugi::xpath_node_set data = doc.select_nodes("gexconfig/devices/device");
		for (pugi::xpath_node_set::const_iterator it = data.begin(); it != data.end(); ++it)
		{
			counter++;

			pugi::xml_node node = it->node();

			std::string const name = node.child_value();

			//const std::string descr = node.child_value("description");

			CYNG_LOG_TRACE(logger_, "session "
				<< ctx.tag()
				<< " - insert device "
				<< name);

			ctx.queue(bus_req_db_insert("TDevice"
				//	generate new key
				, cyng::table::key_generator(uidgen_())
				//	build data vector
				, cyng::table::data_generator(name
					, node.attribute("password").value()
					, node.attribute("number").value()
					, node.attribute("description").value()
					, std::string("")	//	model
					, std::string("")	//	version
					, node.attribute("enabled").as_bool()
					, std::chrono::system_clock::now()
					, 6u)	//	query
				, 0
				, ctx.tag()));
		}

		std::stringstream ss;
		ss
			<< counter
			<< " device records (v4.0) uploaded"
			;
		ctx.queue(bus_insert_msg(cyng::logging::severity::LEVEL_INFO, ss.str()));

	}

	void forward::read_device_configuration_5_x(cyng::context& ctx, pugi::xml_document const& doc, cyng::table::policy policy)
	{
		std::size_t counter{ 0 };
		pugi::xpath_node_set data = doc.select_nodes("TDevice/record");
		auto meta = db_.meta("TDevice");
		for (pugi::xpath_node_set::const_iterator it = data.begin(); it != data.end(); ++it)
		{
			counter++;
			pugi::xml_node node = it->node();

			auto const rec = cyng::xml::read(node, meta);
			if (!rec.empty()) {

				CYNG_LOG_TRACE(logger_, "session "
					<< ctx.tag()
					<< " - insert device #"
					<< counter
					<< " "
					<< cyng::value_cast(rec["pk"], boost::uuids::nil_uuid())
					<< " - "
					<< cyng::value_cast<std::string>(rec["name"], ""));

				switch (policy) {
				case cyng::table::POLICY_MERGE:
					ctx.queue(bus_req_db_merge("TDevice", rec.key(), rec.data(), rec.get_generation(), ctx.tag()));
					break;
				case cyng::table::POLICY_SUBSTITUTE:
					ctx.queue(bus_req_db_update("TDevice", rec.key(), rec.data(), rec.get_generation(), ctx.tag()));
					break;
				default:
					ctx.queue(bus_req_db_insert("TDevice", rec.key(), rec.data(), rec.get_generation(), ctx.tag()));
					break;
				}
			}
			else {

				CYNG_LOG_ERROR(logger_, "session "
					<< ctx.tag()
					<< " - TDevice record #"
					<< counter
					<< " is empty");
			}
		}
	}

	void forward::cfg_post_json(cyng::context& ctx)
	{
		//	[b585cb1d-5641-445b-b6cb-8b40f9859203,0000000b,/config/download.devices,30,{"type":"XML","version":"v50"}]
		//	
		//	* [uuid] session id
		//	* [uint] version
		//	* [string] URL
		//	* [uint] length of JSON string
		//	* [string] JSON payload
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,	//	[0] session tag
			std::uint32_t,		//	[1] version
			std::string,
			std::uint64_t,
			std::string
		>(frame);

		auto const r = cyng::parse_json(std::get<4>(tpl));
		if (r.second) {
			auto const reader = cyng::make_reader(r.first);
			auto const version = cyng::value_cast<std::string>(reader.get("version"), "v");
			if (boost::algorithm::equals(version, "3.2")) {
			}
			else if (boost::algorithm::equals(version, "4.0")) {
			}
			else {
				auto const type = cyng::value_cast<std::string>(reader.get("type"), "type");
				auto const fmt = cyng::value_cast<std::string>(reader.get("fmt"), "format");
				if (boost::algorithm::equals(type, "dev")) {
					if (boost::algorithm::equals(fmt, "XML")) trigger_download_xml(std::get<0>(tpl), "TDevice", "device.xml");
					else if (boost::algorithm::equals(fmt, "JSON")) trigger_download_json(std::get<0>(tpl), "TDevice", "device.json");
					else trigger_download_csv(std::get<0>(tpl), "TDevice", "device.csv");
				}
				else if (boost::algorithm::equals(type, "gw")) {
					if (boost::algorithm::equals(fmt, "XML")) trigger_download_xml(std::get<0>(tpl), "TGateway", "gateway.xml");
					else if (boost::algorithm::equals(fmt, "JSON")) trigger_download_json(std::get<0>(tpl), "TGateway", "gateway.json");
					else trigger_download_csv(std::get<0>(tpl), "TGateway", "gateway.csv");
				}
				else if (boost::algorithm::equals(type, "meter")) {
					if (boost::algorithm::equals(fmt, "XML")) trigger_download_xml(std::get<0>(tpl), "TMeter", "meter.xml");
					else if (boost::algorithm::equals(fmt, "json")) trigger_download_json(std::get<0>(tpl), "TMeter", "meter.json");
					else trigger_download_csv(std::get<0>(tpl), "TMeter", "meter.csv");
				}
				else if (boost::algorithm::equals(type, "LoRa")) {
					if (boost::algorithm::equals(fmt, "XML")) trigger_download_xml(std::get<0>(tpl), "TLoRaDevice", "LoRa.xml");
					else if (boost::algorithm::equals(fmt, "JSON")) trigger_download_json(std::get<0>(tpl), "TLoRaDevice", "LoRa.json");
					else trigger_download_csv(std::get<0>(tpl), "TLoRaDevice", "LoRa.csv");
				}
				else if (boost::algorithm::equals(type, "uplink")) {
					if (boost::algorithm::equals(fmt, "XML")) trigger_download_xml(std::get<0>(tpl), "_LoRaUplink", "LoRaUplink.xml");
					else if (boost::algorithm::equals(fmt, "JSON")) trigger_download_json(std::get<0>(tpl), "_LoRaUplink", "LoRaUplink.json");
					else trigger_download_csv(std::get<0>(tpl), "_LoRaUplink", "LoRaUplink.csv");
				}
				else if (boost::algorithm::equals(type, "msg")) {
					if (boost::algorithm::equals(fmt, "XML")) trigger_download_xml(std::get<0>(tpl), "_SysMsg", "messages.xml");
					else if (boost::algorithm::equals(fmt, "JSON")) trigger_download_json(std::get<0>(tpl), "_SysMsg", "messages.json");
					else trigger_download_csv(std::get<0>(tpl), "_SysMsg", "messages.csv");
				}
			}
		}
		else {
			CYNG_LOG_ERROR(logger_, ctx.get_name() << " JSON parse errror " << std::get<4>(tpl));
		}
	}

	void forward::cfg_post_form_urlencoded(cyng::context& ctx)
	{
		//	[44018499-1ada-458a-ac4d-d0f44fef457f,0000000b,/config/download.devices,28,type=dev&fmt=XML&version=v50]
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,	//	[0] session tag
			std::uint32_t,		//	[1] version
			std::string,
			std::uint64_t,
			std::string
		>(frame);

		trigger_download_xml(std::get<0>(tpl), "TDevice", "device.xml");
	}

	void forward::trigger_download_xml(boost::uuids::uuid tag, std::string table, std::string filename)
	{
		//
		//	generate XML download file
		//
		pugi::xml_document doc_;
		auto declarationNode = doc_.append_child(pugi::node_declaration);
		declarationNode.append_attribute("version") = "1.0";
		declarationNode.append_attribute("encoding") = "UTF-8";
		declarationNode.append_attribute("standalone") = "yes";

		pugi::xml_node root = doc_.append_child(table.c_str());
		root.append_attribute("xmlns:xsi") = "w3.org/2001/XMLSchema-instance";
		db_.access([&](cyng::store::table const* tbl) {

			const auto counter = tbl->loop([&](cyng::table::record const& rec) -> bool {

				//
				//	write record
				//
				cyng::xml::write(root.append_child("record"), rec.convert());

				//	continue
				return true;
			});
			BOOST_ASSERT(counter == 0);
			boost::ignore_unused(counter);	//	release version
			CYNG_LOG_INFO(logger_, tbl->size() << ' ' << tbl->meta().get_name() << " XML records created");

		}, cyng::store::read_access(table));

		//
		//	write XML file
		//
		auto const out = cyng::filesystem::temp_directory_path() / cyng::filesystem::unique_path("record-%%%%-%%%%-%%%%-%%%%.xml");
		if (doc_.save_file(out.c_str(), PUGIXML_TEXT("  "))) {

			//
			//	trigger download
			//
			if (!connection_manager_.trigger_download(tag, out.string(), filename)) {

				CYNG_LOG_WARNING(logger_, "HTTP session " << tag << " not found");
			}
		}
		else {
			CYNG_LOG_ERROR(logger_, "cannot open file " << out);
		}
	}

	void forward::trigger_download_csv(boost::uuids::uuid tag, std::string table, std::string filename)
	{
		//
		//	generate CSV download file
		//
		auto out = cyng::filesystem::temp_directory_path() / cyng::filesystem::unique_path("record-%%%%-%%%%-%%%%-%%%%.csv");

		db_.access([&](cyng::store::table const* tbl) {

			std::ofstream of(out.string(), std::ios::binary | std::ios::trunc | std::ios::out);

			if (of.is_open()) {
				auto vec = tbl->convert(true);
				cyng::csv::write(of, make_object(vec));
				CYNG_LOG_INFO(logger_, tbl->size() << ' ' << tbl->meta().get_name() << " CSV records created");
			}
			else {
				CYNG_LOG_ERROR(logger_, "cannot open file " << out);
			}

			//
			//	file closed implicit here
			//

		}, cyng::store::read_access(table));



		//
		//	trigger download
		//
		connection_manager_.trigger_download(tag, out, filename);
	}

	void forward::trigger_download_json(boost::uuids::uuid tag, std::string table, std::string filename)
	{
		//
		//	generate JSON download file
		//
		auto out = cyng::filesystem::temp_directory_path() / cyng::filesystem::unique_path("record-%%%%-%%%%-%%%%-%%%%.json");

		cyng::vector_t vec;
		db_.access([&](cyng::store::table const* tbl) {

			vec.reserve(tbl->size());
			const auto counter = tbl->loop([&](cyng::table::record const& rec) -> bool {

				//
				//	write record
				//
				vec.push_back(cyng::make_object(rec.convert()));

				//	continue
				return true;
			});
			BOOST_ASSERT(counter == 0);
			boost::ignore_unused(counter);	//	release version
			CYNG_LOG_INFO(logger_, tbl->size() << ' ' << tbl->meta().get_name() << " JSON records created");

		}, cyng::store::read_access(table));

		std::ofstream of(out.string(), std::ios::binary | std::ios::trunc | std::ios::out);
		if (of.is_open()) {
			cyng::json::write(of, cyng::make_object(vec));

			//
			//	trigger download
			//
			of.close();
			connection_manager_.trigger_download(tag, out, filename);
		}
		else {
			CYNG_LOG_ERROR(logger_, "cannot open file " << out);
		}
	}

}
