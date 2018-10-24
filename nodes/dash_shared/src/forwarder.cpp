/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#include "forwarder.h"
#include <smf/cluster/generator.h>
#include <cyng/table/key.hpp>
#include <cyng/io/serializer.h>
#include <cyng/vector_cast.hpp>
#include <cyng/tuple_cast.hpp>
#include <cyng/xml.h>

#include <boost/algorithm/string.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

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
			ctx.attach(bus_req_db_insert("TDevice"
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

	void fwd_delete(cyng::logging::log_ptr logger
		, cyng::context& ctx
		, cyng::reader<cyng::object> const& reader)
	{
		//	[ce28c0c4-1914-45d1-bcf1-22bcbe461855,{("cmd":delete),("channel":config.device),("key":{("tag":a8b3d691-6ea9-40d3-83aa-b1e4dfbcb2f1)})}]
		const std::string channel = cyng::value_cast<std::string>(reader.get("channel"), "");
		CYNG_LOG_TRACE(logger, "ws.read - delete channel [" << channel << "]");
		if (boost::algorithm::starts_with(channel, "config.device"))
		{
			//
			//	TDevice key is of type UUID
			//
			try {
				cyng::vector_t vec;
				vec = cyng::value_cast(reader["key"].get("tag"), vec);
				BOOST_ASSERT_MSG(vec.size() == 1, "TDevice key has wrong size");
				const std::string str = cyng::value_cast<std::string>(vec.at(0), "");
				CYNG_LOG_DEBUG(logger, "TDevice key [" << str << "]");
				auto key = cyng::table::key_generator(boost::uuids::string_generator()(str));

				ctx.attach(bus_req_db_remove("TDevice", key, ctx.tag()));
			}
			catch (std::exception const& ex) {
				CYNG_LOG_ERROR(logger, "ws.read - delete channel [" 
					<< channel 
					<< "] failed: "
					<< ex.what());
			}
		}
		else if (boost::algorithm::starts_with(channel, "config.gateway"))
		{
			//
			//	TGateway key is of type UUID
			//
			try {
				cyng::vector_t vec;
				vec = cyng::value_cast(reader["key"].get("tag"), vec);
				BOOST_ASSERT_MSG(vec.size() == 1, "TGateway key has wrong size");
				const std::string str = cyng::value_cast<std::string>(vec.at(0), "");
				CYNG_LOG_DEBUG(logger, "TGateway key [" << str << "]");
				auto key = cyng::table::key_generator(boost::uuids::string_generator()(str));

				ctx.attach(bus_req_db_remove("TGateway", key, ctx.tag()));
			}
			catch (std::exception const& ex) {
				CYNG_LOG_ERROR(logger, "ws.read - delete channel [" 
					<< channel 
					<< "] failed: "
					<< ex.what());
			}
		}
		else
		{
			CYNG_LOG_WARNING(logger, "ws.read - unknown delete channel [" << channel << "]");
		}
	}

	void fwd_modify(cyng::logging::log_ptr logger
		, cyng::context& ctx
		, cyng::reader<cyng::object> const& reader)
	{
		const std::string channel = cyng::value_cast<std::string>(reader.get("channel"), "");
		CYNG_LOG_TRACE(logger, "ws.read - modify channel [" << channel << "]");
		if (boost::algorithm::starts_with(channel, "config.device"))
		{
			//
			//	TDevice key is of type UUID
			//	all values have the same key
			//
			try {

				cyng::vector_t vec;
				vec = cyng::value_cast(reader["rec"].get("key"), vec);
				BOOST_ASSERT_MSG(vec.size() == 1, "TDevice key has wrong size");

				const std::string str = cyng::value_cast<std::string>(vec.at(0), "");
				CYNG_LOG_DEBUG(logger, "TDevice key [" << str << "]");
				auto key = cyng::table::key_generator(boost::uuids::string_generator()(str));

				cyng::tuple_t tpl;
				tpl = cyng::value_cast(reader["rec"].get("data"), tpl);
				for (auto p : tpl)
				{
					cyng::param_t param;
					ctx.attach(bus_req_db_modify("TDevice"
						, key
						, cyng::value_cast(p, param)
						, 0
						, ctx.tag()));
				}
			}
			catch (std::exception const& ex) {
				CYNG_LOG_ERROR(logger, "ws.read - modify channel [" 
					<< channel << 
					"] failed:"
					<< ex.what());
			}
		}
		else if (boost::algorithm::starts_with(channel, "config.gateway"))
		{
			//
			//	TGateway key is of type UUID
			//	all values have the same key
			//
			try {

				cyng::vector_t vec;
				vec = cyng::value_cast(reader["rec"].get("key"), vec);
				BOOST_ASSERT_MSG(vec.size() == 1, "TGateway key has wrong size");

				const std::string str = cyng::value_cast<std::string>(vec.at(0), "");
				CYNG_LOG_DEBUG(logger, "TGateway key [" << str << "]");
				auto key = cyng::table::key_generator(boost::uuids::string_generator()(str));

				cyng::tuple_t tpl;
				tpl = cyng::value_cast(reader["rec"].get("data"), tpl);
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
						ctx.attach(bus_req_db_modify("TGateway"
							, key
							, cyng::param_factory("serverId", server_id)
							, 0
							, ctx.tag()));

					}
					else {
						ctx.attach(bus_req_db_modify("TGateway"
							, key
							, cyng::value_cast(p, param)
							, 0
							, ctx.tag()));
					}
				}
			}
			catch (std::exception const& ex) {
				CYNG_LOG_ERROR(logger, "ws.read - modify channel [" 
					<< channel 
					<< "] failed: "
					<< ex.what());
			}
		}
		else if (boost::algorithm::starts_with(channel, "config.system"))
		{
			//	{"cmd":"modify","channel":"config.system","rec":{"key":{"name":"connection-auto-login"},"data":{"value":true}}}
			const std::string name = cyng::value_cast<std::string>(reader["rec"]["key"].get("name"), "?");
			const cyng::object value = reader["rec"]["data"].get("value");
			ctx.attach(bus_req_db_modify("_Config"
				//	generate new key
				, cyng::table::key_generator(reader["rec"]["key"].get("name"))
				//	build parameter
				, cyng::param_t("value", value)
				, 0
				, ctx.tag()));

			std::stringstream ss;
			ss
				<< "system configuration changed "
				<< name
				<< " ["
				<< value.get_class().type_name()
				<< "] = "
				<< cyng::io::to_str(value)
				;

			ctx.attach(bus_insert_msg(cyng::logging::severity::LEVEL_WARNING, ss.str()));

		}
		else
		{
			CYNG_LOG_WARNING(logger, "ws.read - unknown modify channel [" << channel << "]");
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

				cyng::vector_t vec;
				vec = cyng::value_cast(reader.get("key"), vec);
				BOOST_ASSERT_MSG(vec.size() == 1, "*Session key has wrong size");

				const std::string str = cyng::value_cast<std::string>(vec.at(0), "");
				CYNG_LOG_DEBUG(logger, "*Session key [" << str << "]");
				auto key = cyng::table::key_generator(boost::uuids::string_generator()(str));

				ctx.attach(bus_req_stop_client(key, ctx.tag()));

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

	void fwd_reboot(cyng::logging::log_ptr logger
		, cyng::context& ctx
		, boost::uuids::uuid tag_ws
		, cyng::reader<cyng::object> const& reader)
	{
		const std::string channel = cyng::value_cast<std::string>(reader.get("channel"), "");
		CYNG_LOG_TRACE(logger, "ws.read - reboot channel [" << channel << "]");
		if (boost::algorithm::starts_with(channel, "config.gateway"))
		{
			//	{"cmd":"reboot","channel":"config.gateway","key":["dca135f3-ff2b-4bf7-8371-a9904c74522b"]}
			cyng::vector_t vec;
			vec = cyng::value_cast(reader.get("key"), vec);
			CYNG_LOG_INFO(logger, "reboot gateway " << cyng::io::to_str(vec));
			BOOST_ASSERT_MSG(!vec.empty(), "TGateway key is empty");

			if (!vec.empty()) {
				const std::string str = cyng::value_cast<std::string>(vec.at(0), "");
				CYNG_LOG_DEBUG(logger, "TGateway key [" << str << "]");
				auto key = cyng::table::key_generator(boost::uuids::string_generator()(str));

				ctx.attach(bus_req_reboot_client(key, ctx.tag(), tag_ws));
			}
		}
		else
		{
			CYNG_LOG_WARNING(logger, "ws.read - unknown reboot channel [" << channel << "]");
		}
	}

	void fwd_query_gateway(cyng::logging::log_ptr logger
		, cyng::context& ctx
		, boost::uuids::uuid tag_ws
		, cyng::reader<cyng::object> const& reader)
	{
		const std::string channel = cyng::value_cast<std::string>(reader.get("channel"), "");
		CYNG_LOG_TRACE(logger, "ws.read - query:gateway channel [" << channel << "]");
		if (boost::algorithm::starts_with(channel, "config.gateway"))
		{
			cyng::vector_t vec;
			vec = cyng::value_cast(reader.get("key"), vec);
			CYNG_LOG_INFO(logger, "query:gateway " << cyng::io::to_str(vec));

			BOOST_ASSERT_MSG(!vec.empty(), "TGateway key is empty");
			if (!vec.empty()) {
				const std::string str = cyng::value_cast<std::string>(vec.at(0), "");
				CYNG_LOG_DEBUG(logger, "TGateway key [" << str << "]");
				auto key = cyng::table::key_generator(boost::uuids::string_generator()(str));

#ifdef _DEBUG
				auto params = cyng::vector_cast<std::string>(reader.get("params"), "");
				for (auto const& p : params) {
					CYNG_LOG_DEBUG(logger, p);

				}
#endif
				ctx.attach(bus_req_query_gateway(key, ctx.tag(), cyng::value_cast(reader.get("params"), vec), tag_ws));
			}
		}
	}

	forward::forward(cyng::logging::log_ptr logger, cyng::store::db& db)
		: logger_(logger)
		, db_(db)
	{}

	void forward::register_this(cyng::controller& vm)
	{
		vm.register_function("cfg.upload.devices", 2, std::bind(&forward::cfg_upload_devices, this, std::placeholders::_1));
		vm.register_function("cfg.upload.gateways", 2, std::bind(&forward::cfg_upload_gateways, this, std::placeholders::_1));
		vm.register_function("cfg.upload.meter", 2, std::bind(&forward::cfg_upload_meter, this, std::placeholders::_1));
	}

	void forward::cfg_upload_devices(cyng::context& ctx)
	{
		//	
		//	[181f86c7-23e5-4e01-a4d9-f6c9855962bf,
		//	%(
		//		("devConf_0":text/xml),
		//		("devices.xml":C:\\Users\\Pyrx\\AppData\\Local\\Temp\\smf-dash-ea2e-1fe1-6628-93ff.tmp),
		//		("smf-procedure":cfg.upload.devices),
		//		("smf-upload-config-device-version":v5.0),
		//		("target":/upload/config/device/)
		//	)]
		//
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "cfg.upload.device - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,	//	[0] session tag
			cyng::param_map_t	//	[1] variables
		>(frame);

		auto file_name = cyng::value_cast<std::string>(cyng::find(std::get<1>(tpl), "dev-conf-0"), "");
		auto version = cyng::value_cast<std::string>(cyng::find(std::get<1>(tpl), "smf-upload-config-device-version"), "v0.5");

		//
		//	get pointer to XML data
		//
		pugi::xml_document doc;
		const pugi::xml_parse_result result = doc.load_file(file_name.c_str());
		if (result) {

			if (boost::algorithm::equals(version, "v3.2")) {
				read_device_configuration_3_2(ctx, doc);
			}
			else {
				read_device_configuration_5_x(ctx, doc);
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

			ctx.attach(bus_insert_msg(cyng::logging::severity::LEVEL_WARNING, ss.str()));
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

		auto file_name = cyng::value_cast<std::string>(cyng::find(std::get<1>(tpl), "gw-conf-0"), "");

		//
		//	get pointer to XML data
		//
		pugi::xml_document doc;
		const pugi::xml_parse_result result = doc.load_file(file_name.c_str());
		if (result) {

			std::size_t counter{ 0 };
			pugi::xpath_node_set data = doc.select_nodes("TGateway/record");
			auto meta = db_.meta("TGateway");
			for (pugi::xpath_node_set::const_iterator it = data.begin(); it != data.end(); ++it)
			{
				counter++;
				pugi::xml_node node = it->node();

				auto rec = cyng::xml::read(node, meta);

				CYNG_LOG_TRACE(logger_, "session "
					<< ctx.tag()
					<< " - insert gateway #"
					<< counter
					<< " "
					<< cyng::value_cast(rec["pk"], boost::uuids::nil_uuid())
					<< " - "
					<< cyng::value_cast<std::string>(rec["name"], ""));

				ctx.attach(bus_req_db_insert("TGateway", rec.key(), rec.data(), rec.get_generation(), ctx.tag()));
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

			ctx.attach(bus_insert_msg(cyng::logging::severity::LEVEL_WARNING, ss.str()));
		}
	}

	void forward::cfg_upload_meter(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "cfg.upload.meter - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,	//	[0] session tag
			cyng::param_map_t	//	[1] variables
		>(frame);
	}

	void forward::read_device_configuration_3_2(cyng::context& ctx, pugi::xml_document const& doc)
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
			const std::string pk = node.child_value();

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
			ctx.attach(bus_req_db_insert("TDevice"
				//	generate new key
				, cyng::table::key_generator(boost::uuids::string_generator()(pk))
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

		std::stringstream ss;
		ss
			<< counter
			<< " device records (v3.2) uploaded"
			;
		ctx.attach(bus_insert_msg(cyng::logging::severity::LEVEL_INFO, ss.str()));

	}

	void forward::read_device_configuration_4_0(cyng::context& ctx, pugi::xml_document const& doc)
	{
		std::size_t counter{ 0 };
		pugi::xpath_node_set data = doc.select_nodes("configuration/records/device");
		for (pugi::xpath_node_set::const_iterator it = data.begin(); it != data.end(); ++it)
		{
			counter++;
			pugi::xpath_node node = *it;
			const std::string name = node.node().child_value("name");

			CYNG_LOG_TRACE(logger_, "session "
				<< ctx.tag()
				<< " - insert device "
				<< name);

			//auto obj = cyng::traverse(node.node());
			//CYY_LOG_TRACE(logger_, "session "
			//	<< cyy::uuid_short_format(vm_.tag())
			//	<< " - "
			//	<< cyy::to_literal(obj, cyx::io::custom));

			//	("device":
			//	%(("age":"2016.11.24 14:48:09.00000000"ts),
			//	("config":%(("pwd":"LUsregnP"),("scheme":"plain"))),
			//	("descr":"-[0008]-"),
			//	("enabled":true),
			//	("id":""),
			//	("name":"oJtrQQfSCRrF"),
			//	("number":"609971066"),
			//	("revision":0u32),
			//	("source":"d365c4c7-e89d-4187-86ae-a143d54f4cfe"uuid),
			//	("tag":"e29938f5-71a9-4860-97e2-0a78097a6858"uuid),
			//	("ver":"")))

			//cyy::object_reader reader(obj);

			//
			//	collect data
			//	forward to process bus task
			//
			//insert("TDevice"
			//	, cyy::store::key_generator(reader["device"].get_uuid("tag"))
			//	, cyy::store::data_generator(reader["device"].get_object("revision")
			//		, reader["device"].get_object("name")
			//		, reader["device"].get_object("number")
			//		, reader["device"].get_object("descr")
			//		, reader["device"].get_object("id")	//	device ID
			//		, reader["device"].get_object("ver")	//	firmware 
			//		, reader["device"].get_object("enabled")
			//		, reader["device"].get_object("age")

			//		//	configuration as parameter map
			//		, reader["device"].get_object("config")
			//		, reader["device"].get_object("source")));

			//ctx.attach(bus_req_db_insert("TDevice"
			//	//	generate new key
			//	, cyng::table::key_generator(boost::uuids::string_generator()(pk))
			//	//	build data vector
			//	, cyng::table::data_generator(name
			//		, node.attribute("pwd").value()
			//		, node.attribute("number").value()
			//		, node.attribute("description").value()
			//		, std::string("")	//	model
			//		, std::string("")	//	version
			//		, node.attribute("enabled").as_bool()
			//		, std::chrono::system_clock::now()
			//		, node.attribute("query").as_uint())
			//	, 0
			//	, ctx.tag()));
		}

		std::stringstream ss;
		ss
			<< counter
			<< " device records (v4.0) uploaded"
			;
		ctx.attach(bus_insert_msg(cyng::logging::severity::LEVEL_INFO, ss.str()));

	}

	void forward::read_device_configuration_5_x(cyng::context& ctx, pugi::xml_document const& doc)
	{
		std::size_t counter{ 0 };
		pugi::xpath_node_set data = doc.select_nodes("TDevice/record");
		auto meta = db_.meta("TDevice");
		for (pugi::xpath_node_set::const_iterator it = data.begin(); it != data.end(); ++it)
		{
			counter++;
			pugi::xml_node node = it->node();

			auto rec = cyng::xml::read(node, meta);

			CYNG_LOG_TRACE(logger_, "session "
				<< ctx.tag()
				<< " - insert device #"
				<< counter
				<< " "
				<< cyng::value_cast(rec["pk"], boost::uuids::nil_uuid())
				<< " - "
				<< cyng::value_cast<std::string>(rec["name"], ""));

			ctx.attach(bus_req_db_insert("TDevice", rec.key(), rec.data(), rec.get_generation(), ctx.tag()));
		}
	}

}
