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

#include <boost/algorithm/string.hpp>
#include <boost/uuid/string_generator.hpp>

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
				CYNG_LOG_ERROR(logger, "ws.read - delete channel [" << channel << "] failed");
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
				CYNG_LOG_ERROR(logger, "ws.read - delete channel [" << channel << "] failed");
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
				CYNG_LOG_ERROR(logger, "ws.read - modify channel [" << channel << "] failed");
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
				CYNG_LOG_ERROR(logger, "ws.read - modify channel [" << channel << "] failed");
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
				CYNG_LOG_ERROR(logger, "ws.read - modify channel [" << channel << "] failed");
			}

		}
		else
		{
			CYNG_LOG_WARNING(logger, "ws.read - unknown stop channel [" << channel << "]");
		}
	}

	void fwd_reboot(cyng::logging::log_ptr logger
		, cyng::context& ctx
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

			const std::string str = cyng::value_cast<std::string>(vec.at(0), "");
			CYNG_LOG_DEBUG(logger, "TGateway key [" << str << "]");
			auto key = cyng::table::key_generator(boost::uuids::string_generator()(str));

			ctx.attach(bus_req_reboot_client(key, ctx.tag()));

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
}
