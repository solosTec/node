/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#include "forwarder.h"
#include "../../shared/db/db_schemes.h"
#include <smf/cluster/generator.h>
#include <cyng/table/key.hpp>
#include <cyng/io/serializer.h>
#include <cyng/vector_cast.hpp>
#include <cyng/tuple_cast.hpp>
#include <cyng/parser/buffer_parser.h>
#include <cyng/parser/mac_parser.h>
#include <cyng/xml.h>
#include <cyng/csv.h>

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
				auto key = cyng::table::key_generator(vec.at(0));
				ctx.queue(bus_req_db_remove("TDevice", key, ctx.tag()));
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
				auto key = cyng::table::key_generator(vec.at(0));
				ctx.queue(bus_req_db_remove("TGateway", key, ctx.tag()));
			}
			catch (std::exception const& ex) {
				CYNG_LOG_ERROR(logger, "ws.read - delete channel [" 
					<< channel 
					<< "] failed: "
					<< ex.what());
			}
		}
		else if (boost::algorithm::starts_with(channel, "config.meter"))
		{
			//
			//	TDevice key is of type UUID
			//
			try {
				cyng::vector_t vec;
				vec = cyng::value_cast(reader["key"].get("tag"), vec);
				BOOST_ASSERT_MSG(vec.size() == 1, "TMeter key has wrong size");
				auto key = cyng::table::key_generator(vec.at(0));
				ctx.queue(bus_req_db_remove("TMeter", key, ctx.tag()));
			}
			catch (std::exception const& ex) {
				CYNG_LOG_ERROR(logger, "ws.read - delete channel ["
					<< channel
					<< "] failed: "
					<< ex.what());
			}
		}
		else if (boost::algorithm::starts_with(channel, "config.lora"))
		{
			//
			//	TLoRaDevice key is of type UUID
			//
			try {
				cyng::vector_t vec;
				vec = cyng::value_cast(reader["key"].get("tag"), vec);
				BOOST_ASSERT_MSG(vec.size() == 1, "TLoRaDevice key has wrong size");
				auto key = cyng::table::key_generator(vec.at(0));
				ctx.queue(bus_req_db_remove("TLoRaDevice", key, ctx.tag()));
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

				//const std::string str = cyng::value_cast<std::string>(vec.at(0), "");
				//CYNG_LOG_DEBUG(logger, "TDevice key [" << str << "]");
				//auto key = cyng::table::key_generator(boost::uuids::string_generator()(str));
				auto key = cyng::table::key_generator(vec.at(0));

				cyng::tuple_t tpl;
				tpl = cyng::value_cast(reader["rec"].get("data"), tpl);
				for (auto p : tpl)
				{
					cyng::param_t param;
					ctx.queue(bus_req_db_modify("TDevice"
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
				auto key = cyng::table::key_generator(vec.at(0));

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
						ctx.queue(bus_req_db_modify("TGateway"
							, key
							, cyng::param_factory("serverId", server_id)
							, 0
							, ctx.tag()));
					}
					else {
						ctx.queue(bus_req_db_modify("TGateway"
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
			ctx.queue(bus_req_db_modify("_Config"
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

			ctx.queue(bus_insert_msg(cyng::logging::severity::LEVEL_WARNING, ss.str()));

		}
		else if (boost::algorithm::starts_with(channel, "config.meter"))
		{
			try {

				cyng::vector_t vec;
				vec = cyng::value_cast(reader["rec"].get("key"), vec);
				BOOST_ASSERT_MSG(vec.size() == 1, "TMeter key has wrong size");

				//const std::string str = cyng::value_cast<std::string>(vec.at(0), "");
				//CYNG_LOG_DEBUG(logger, "TDevice key [" << str << "]");
				//auto key = cyng::table::key_generator(boost::uuids::string_generator()(str));
				auto key = cyng::table::key_generator(vec.at(0));

				cyng::tuple_t tpl;
				tpl = cyng::value_cast(reader["rec"].get("data"), tpl);
				for (auto p : tpl)
				{
					cyng::param_t param;
					ctx.queue(bus_req_db_modify("TMeter"
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
		else if (boost::algorithm::starts_with(channel, "config.lora")) 
		{
			try {

				//	smf-form-lora-pk=96c24827-2847-4612-9982-ab5cb1e8dee7&smf-form-lora-address=&smf-form-lora-appuid=&smf-form-lora-gwuid=&smf-form-lora-DevEUI=0100%3A0302%3A0504%3A0706&smf-form-lora-aes=0000000000000000000000000000000000000000000000000000000000000000&smf-form-lora-driver%20=raw&smf-form-lora-activation=on&smf-form-lora-devaddr=&smf-form-lora-appeui=&smf-form-lora-gweui=
				//	 {"cmd":"modify","channel":"config.lora","rec":{"key":["96c24827-2847-4612-9982-ab5cb1e8dee7"],"data":{"DevEUI":"0100:0302:0504:0706","AESKey":"0000000000000000000000000000000000000000000000000000000000000000","driver":"raw","activation":"ABP","DevAddr":"20","AppEUI":"0100:0302:0604:0807","GatewayEUI":"0100:0302:0604:0807"}}}

				cyng::vector_t vec;
				vec = cyng::value_cast(reader["rec"].get("key"), vec);
				BOOST_ASSERT_MSG(vec.size() == 1, "TLoRaDevice key has wrong size");

				auto key = cyng::table::key_generator(vec.at(0));

				cyng::tuple_t tpl;
				tpl = cyng::value_cast(reader["rec"].get("data"), tpl);
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

						ctx.queue(bus_req_db_modify("TLoRaDevice"
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
							ctx.queue(bus_req_db_modify("TLoRaDevice"
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
							ctx.queue(bus_req_db_modify("TLoRaDevice"
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
							ctx.queue(bus_req_db_modify("TLoRaDevice"
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
						ctx.queue(bus_req_db_modify("TLoRaDevice"
							, key
							, param
							, 0
							, ctx.tag()));
					}
				}
			}
			catch (std::exception const& ex) {
				CYNG_LOG_ERROR(logger, "ws.read - modify channel ["
					<< channel <<
					"] failed:"
					<< ex.what());
			}
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

				//const std::string str = cyng::value_cast<std::string>(vec.at(0), "");
				//CYNG_LOG_DEBUG(logger, "*Session key [" << str << "]");
				//auto key = cyng::table::key_generator(boost::uuids::string_generator()(str));
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

	void fwd_config_gateway(cyng::logging::log_ptr logger
		, cyng::context& ctx
		, boost::uuids::uuid tag_ws
		, std::string const& channel
		, cyng::reader<cyng::object> const& reader)
	{
		//	{"cmd":"config:gateway","channel":"get.proc.param","key":["fce4fe15-0756-4ae4-91e7-28dee59f07e6"],"params":[{"section":["op-log-status-word","root-visible-devices","root-active-devices","root-device-id","memory","root-wMBus-status","IF_wMBUS","root-ipt-state","root-ipt-param"]}]}
		//	{"cmd":"config:gateway","channel":"set.proc.param","key":["fce4fe15-0756-4ae4-91e7-28dee59f07e6"],"params":[{"name":"smf-form-gw-ipt-srv","value":"0500153B0223B3"},{"name":"smf-gw-ipt-host-1","value":"waiting..."},{"name":"smf-gw-ipt-local-1","value":""},{"name":"smf-gw-ipt-remote-1","value":""},{"name":"smf-gw-ipt-name-1","value":"waiting..."},{"name":"smf-gw-ipt-pwd-1","value":""},{"name":"smf-gw-ipt-host-2","value":"waiting..."},{"name":"smf-gw-ipt-local-2","value":""},{"name":"smf-gw-ipt-remote-2","value":""},{"name":"smf-gw-ipt-name-2","value":"waiting..."},{"name":"smf-gw-ipt-pwd-2","value":""},{"section":["ipt"]}]}
		//	{"cmd":"config:gateway","channel":"set.proc.param","key":["31cad258-45f1-4b1d-b131-8a55eb671bb1"],"params":[{"name":"meter-tag","value":"1a38ed66-bd25-4e34-827c-b1a62f09abf4"},{"name":"meter-id","value":"01-a815-74314505-01-02"}],"section":["query"]}
		cyng::vector_t key;
		key = cyng::value_cast(reader.get("key"), key);
		CYNG_LOG_INFO(logger, "ws tag: " << tag_ws << " - TGateway key" << cyng::io::to_str(key));
		BOOST_ASSERT_MSG(!key.empty(), "TGateway key is empty");
		if (!key.empty()) {

			cyng::vector_t vec;
			ctx.queue(bus_req_gateway_proxy(key	//	key into TGateway and TDevice table
				, tag_ws	//	web-socket tag
				, channel
				, cyng::value_cast(reader.get("section"), vec)
				, cyng::value_cast(reader.get("params"), vec)));	//	parameters, requests, commands

		}
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

		vm.register_function("cfg.download.devices", 2, std::bind(&forward::cfg_download_devices, this, std::placeholders::_1));
		vm.register_function("cfg.download.gateways", 2, std::bind(&forward::cfg_download_gateways, this, std::placeholders::_1));
		vm.register_function("cfg.download.meters", 2, std::bind(&forward::cfg_download_meters, this, std::placeholders::_1));
		vm.register_function("cfg.download.messages", 2, std::bind(&forward::cfg_download_messages, this, std::placeholders::_1));
		vm.register_function("cfg.download.LoRa", 2, std::bind(&forward::cfg_download_LoRa, this, std::placeholders::_1));
		vm.register_function("cfg.download.uplink", 2, std::bind(&forward::cfg_download_uplink, this, std::placeholders::_1));

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
		auto merge = cyng::value_cast<std::string>(cyng::find(std::get<1>(tpl), "smf-upload-config-device-merge"), "insert");

		//
		//	get pointer to XML data
		//
		pugi::xml_document doc;
		const pugi::xml_parse_result result = doc.load_file(file_name.c_str());
		if (result) {

			if (boost::algorithm::equals(version, "v3.2")) {
				read_device_configuration_3_2(ctx, doc, boost::algorithm::equals(merge, "insert"));
			}
			else {
				read_device_configuration_5_x(ctx, doc, boost::algorithm::equals(merge, "insert"));
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

		auto const file_name = cyng::value_cast<std::string>(cyng::find(std::get<1>(tpl), "gw-conf-0"), "");

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

				CYNG_LOG_INFO(logger_, "session "
					<< ctx.tag()
					<< " - insert gateway #"
					<< counter
					<< " "
					<< cyng::value_cast(rec["pk"], boost::uuids::nil_uuid())
					<< " - "
					<< cyng::value_cast<std::string>(rec["serverId"], ""));

				ctx.queue(bus_req_db_insert("TGateway", rec.key(), rec.data(), rec.get_generation(), ctx.tag()));
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

		auto const file_name = cyng::value_cast<std::string>(cyng::find(std::get<1>(tpl), "gw-conf-0"), "");

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

				CYNG_LOG_INFO(logger_, "session "
					<< ctx.tag()
					<< " - insert meter #"
					<< counter
					<< " "
					<< cyng::value_cast(rec["pk"], boost::uuids::nil_uuid())
					<< " - "
					<< cyng::value_cast<std::string>(rec["serverId"], ""));

				ctx.queue(bus_req_db_insert("TMeter", rec.key(), rec.data(), rec.get_generation(), ctx.tag()));
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


		auto const file_name = cyng::value_cast<std::string>(cyng::find(std::get<1>(tpl), "gw-conf-0"), "");

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

				CYNG_LOG_INFO(logger_, "session "
					<< ctx.tag()
					<< " - insert LoRa device #"
					<< counter
					<< " "
					<< cyng::value_cast(rec["pk"], boost::uuids::nil_uuid())
					<< " - "
					<< cyng::value_cast<std::string>(rec["serverId"], ""));

				ctx.queue(bus_req_db_insert("TLoRaDevice", rec.key(), rec.data(), rec.get_generation(), ctx.tag()));
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
			//		, reader["device"].get_object("ver")	//	root-device-id 
			//		, reader["device"].get_object("enabled")
			//		, reader["device"].get_object("age")

			//		//	configuration as parameter map
			//		, reader["device"].get_object("config")
			//		, reader["device"].get_object("source")));

			//ctx.queue(bus_req_db_insert("TDevice"
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
		ctx.queue(bus_insert_msg(cyng::logging::severity::LEVEL_INFO, ss.str()));

	}

	void forward::read_device_configuration_5_x(cyng::context& ctx, pugi::xml_document const& doc, bool insert)
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

			ctx.queue(bus_req_db_insert("TDevice", rec.key(), rec.data(), rec.get_generation(), ctx.tag()));
		}
	}

	void forward::cfg_download_devices(cyng::context& ctx)
	{
		//
		//	example
		//	[%(("smf-procedure":cfg.download.devices),("smf-upload-config-device-version":v5.0),("target":/api/download/config/device))]
		//
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "cfg.download.devices - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,	//	[0] session tag
			cyng::param_map_t	//	[1] variables
		>(frame);

		auto const reader = cyng::make_reader(std::get<1>(tpl));
		auto const fmt = cyng::value_cast<std::string>(reader.get("smf-download-device-format"), "XML");
		if (boost::algorithm::equals("CSV", fmt)) {
			trigger_download_csv(std::get<0>(tpl), "TDevice", "device.csv");
		}
		else {
			trigger_download_xml(std::get<0>(tpl), "TDevice", "device.xml");
		}
	}

	void forward::cfg_download_gateways(cyng::context& ctx)
	{
		//
		//	example
		//	[fd003d7f-43a5-4b3b-9dca-71c7584092a5,%(("smf-download-gw-format":CSV),("smf-procedure":cfg.download.gateways),("target":/config.download.html))]
		//
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "cfg.download.gateways - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,	//	[0] session tag
			cyng::param_map_t	//	[1] variables
		>(frame);

		auto const reader = cyng::make_reader(std::get<1>(tpl));
		auto const fmt = cyng::value_cast<std::string>(reader.get("smf-download-gw-format"), "XML");
		if (boost::algorithm::equals("CSV", fmt)) {
			trigger_download_csv(std::get<0>(tpl), "TGateway", "gateway.csv");
		}
		else {
			trigger_download_xml(std::get<0>(tpl), "TGateway", "gateway.xml");
		}
	}

	void forward::cfg_download_meters(cyng::context& ctx)
	{
		//
		//	example
		//	[%(("smf-procedure":cfg.download.devices),("smf-upload-config-device-version":v5.0),("target":/api/download/config/device))]
		//
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "cfg.download.meters - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,	//	[0] session tag
			cyng::param_map_t	//	[1] variables
		>(frame);

		auto const reader = cyng::make_reader(std::get<1>(tpl));
		auto const fmt = cyng::value_cast<std::string>(reader.get("smf-download-meter-format"), "XML");
		if (boost::algorithm::equals("CSV", fmt)) {
			trigger_download_csv(std::get<0>(tpl), "TMeter", "meter.csv");
		}
		else {
			trigger_download_xml(std::get<0>(tpl), "TMeter", "meter.xml");
		}
	}

	void forward::cfg_download_messages(cyng::context& ctx)
	{
		//
		//	example
		//	[%(("smf-procedure":cfg.download.devices),("smf-upload-config-device-version":v5.0),("target":/api/download/config/device))]
		//
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "cfg.download.messages - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,	//	[0] session tag
			cyng::param_map_t	//	[1] variables
		>(frame);

		auto const reader = cyng::make_reader(std::get<1>(tpl));
		auto const fmt = cyng::value_cast<std::string>(reader.get("smf-download-msg-format"), "XML");
		if (boost::algorithm::equals("CSV", fmt)) {
			trigger_download_csv(std::get<0>(tpl), "_SysMsg", "messages.csv");
		}
		else {
			trigger_download_xml(std::get<0>(tpl), "_SysMsg", "messages.xml");
		}
	}

	void forward::cfg_download_LoRa(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "cfg.download.LoRa - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,	//	[0] session tag
			cyng::param_map_t	//	[1] variables
		>(frame);

		trigger_download_xml(std::get<0>(tpl), "TLoRaDevice", "LoRa.xml");
	}

	void forward::cfg_download_uplink(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,	//	[0] session tag
			cyng::param_map_t	//	[1] variables
		>(frame);

		auto const reader = cyng::make_reader(std::get<1>(tpl));
		auto const fmt = cyng::value_cast<std::string>(reader.get("smf-download-uplink-format"), "XML");
		if (boost::algorithm::equals("CSV", fmt)) {
			trigger_download_csv(std::get<0>(tpl), "_LoRaUplink", "LoRaUplink.csv");
		}
		else {
			trigger_download_xml(std::get<0>(tpl), "_LoRaUplink", "LoRaUplink.xml");
		}
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
			CYNG_LOG_INFO(logger_, tbl->size() << ' ' << tbl->meta().get_name() << " records sent");

		}, cyng::store::read_access(table));

		//
		//	write XML file
		//
		auto out = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path("record-%%%%-%%%%-%%%%-%%%%.xml");
		if (doc_.save_file(out.c_str(), PUGIXML_TEXT("  "))) {

			//
			//	trigger download
			//
			connection_manager_.trigger_download(tag, out.string(), filename);
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
		auto out = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path("record-%%%%-%%%%-%%%%-%%%%.csv");

		db_.access([&](cyng::store::table const* tbl) {

			std::ofstream of(out.string(), std::ios::binary | std::ios::trunc | std::ios::out);

			if (of.is_open()) {
				auto vec = tbl->convert(true);
				cyng::csv::write(of, make_object(vec));
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

}
