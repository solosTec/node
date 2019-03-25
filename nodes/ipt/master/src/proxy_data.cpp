/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#include "proxy_data.h"
#include <smf/sml/srv_id_io.h>
#include <smf/sml/parser/srv_id_parser.h>

#include <cyng/vector_cast.hpp>
#include <cyng/set_cast.h>

#include <cyng/dom/reader.h>
#include <cyng/parser/buffer_parser.h>
#include <cyng/factory/factory.hpp>

#include <boost/algorithm/string.hpp>

namespace node 
{
	namespace ipt
	{
		proxy_data::proxy_data(
			boost::uuids::uuid tag,		//	[0] ident tag
			boost::uuids::uuid source,	//	[1] source tag
			std::uint64_t seq,			//	[2] cluster seq
			cyng::vector_t key,			//	[3] TGateway key
			boost::uuids::uuid ws_tag,	//	[4] ws tag
			std::string channel,		//	[5] channel
			cyng::vector_t sections,	//	[6] sections
			cyng::vector_t params,		//	[7] parameters
			cyng::buffer_t srv,			//	[8] server id
			std::string name,			//	[9] name
			std::string	pwd	,			//	[10] pwd
			std::size_t queue_size
		)
		: tag_(tag)
			, source_(source)	//	source tag
			, seq_(seq)			//	cluster seq
			, key_(key)			//	TGateway key
			, ws_tag_(ws_tag)	//	ws tag
			, channel_(channel)
			, sections_(sections)
			, params_(params)	//	parameters
			, srv_(srv)			//	server id
			, name_(name)		//	name
			, pwd_(pwd)			//	pwd
			, state_(queue_size == 0 ? STATE_PROCESSING_ : STATE_WAITING_)
		{}

		std::vector<std::string> proxy_data::get_section_names() const
		{
			//	[{("section":[op-log-status-word,srv:visible,srv:active,firmware,memory,root-wMBus-status,IF_wMBUS,root-ipt-state,root-ipt-param])}]
			return cyng::vector_cast<std::string>(sections_, "nope");
		}

		cyng::vector_t proxy_data::get_params(std::string const& section) const
		{
			auto const reader = cyng::make_reader(params_);

			//auto const sections = get_section_names();
			if (boost::algorithm::equals(section, "root-ipt-param")) {

				if (params_.size() == 1) {
					cyng::vector_t vec;
					return cyng::value_cast(reader[0].get("ipt"), vec);
				}
			}
			else if (boost::algorithm::equals(section, "IF-wireless-mbus")) {

				if (params_.size() == 1) {
					cyng::tuple_t tpl;
					tpl = cyng::value_cast(reader[0].get("wmbus"), tpl);
					return cyng::to_vector(tpl);
				}
			}
			else if (boost::algorithm::equals(section, "IF-IEC-62505-21")) {
				if (params_.size() == 1) {
					cyng::tuple_t tpl;
					tpl = cyng::value_cast(reader[0].get("iec"), tpl);
					return cyng::to_vector(tpl);
				}
			}
			else if ((params_.size() == 3) && boost::algorithm::equals(section, "activate")) {

				//
				//	extract parameter "meterId"
				//
				return cyng::vector_t({ reader[0].get("nr"), read_server_id(reader[2].get("meterId")) });
			}
			else if ((params_.size() == 3) && boost::algorithm::equals(section, "deactivate")) {

				//
				//	extract parameter "meterId"
				//
				return cyng::vector_t({ reader[0].get("nr"), read_server_id(reader[2].get("meterId")) });
			}
			else if ((params_.size() == 3) && boost::algorithm::equals(section, "delete")) {

				//
				//	extract parameter "meterId"
				//
				return cyng::vector_t({ reader[0].get("nr"), read_server_id(reader[2].get("meterId")) });
			}

			else if ((params_.size() == 1) && boost::algorithm::equals(section, "current-data-record")) {

				//
				//	extract parameter "meterId"
				//
				return cyng::vector_t({ parse_server_id(reader[0].get("meterId")) });
			}
			if (params_.size() == 1) {
				//
				//	assuming the payload is a parameter (param_t)
				//
				return cyng::vector_t(1, reader.get(0));
			}

			//
			//	unknown format
			//
			return params_;
		}

		//cyng::param_map_t proxy_data::get_params() const
		//{
		//	cyng::param_map_t params;

		//	//	vector of tuples

		//	auto const reader = cyng::make_reader(params_);

			//for (std::size_t idx = 0; idx < params_.size(); ++idx) {

			//	auto name = cyng::value_cast<std::string>(reader[idx].get("name"), "");
			//	auto obj = reader[idx].get("value");

			//	if (boost::algorithm::equals(name, "smf-form-gw-ipt-srv")) {

			//		//
			//		//	send server/gateway ID as buffer_t type
			//		//
			//		std::string const inp = cyng::value_cast<std::string>(obj, "000000");
			//		auto const r = cyng::parse_hex_string(inp);
			//		if (r.second) {
			//			params.emplace("serverId", cyng::make_object(r.first));
			//		}
			//		else {
			//			params.emplace("serverId", cyng::make_object(cyng::make_buffer({0, 0, 0, 0, 0, 0})));
			//		}
			//	}
			//	else if (boost::algorithm::starts_with(name, "smf-gw-ipt-local-")
			//		|| boost::algorithm::starts_with(name, "smf-gw-ipt-remote-")) {

			//		//
			//		//	send port as u16 
			//		//
			//		params.emplace(name, cyng::make_object(cyng::numeric_cast<std::uint16_t>(obj, 26862)));
			//	}
			//	else if (boost::algorithm::equals(name, "parameter smf-gw-wmbus-power")
			//		|| boost::algorithm::equals(name, "smf-gw-wmbus-protocol")) {
			//		//
			//		//	send transmission power value as u8 
			//		//
			//		params.emplace(name, cyng::make_object(cyng::numeric_cast<std::uint8_t>(obj, 0)));
			//	}
			//	else if (boost::algorithm::equals(name, "smf-gw-wmbus-reboot")) {
			//		//
			//		//	send reboot cycle as u64 
			//		//
			//		params.emplace(name, cyng::make_object(cyng::numeric_cast<std::uint64_t>(obj, 0)));
			//	}
			//	else if (boost::algorithm::equals(name, "smf-gw-wmbus-adapter")) {}		//	skip
			//	else if (boost::algorithm::equals(name, "smf-gw-wmbus-firmware")) {}	//	skip
			//	else if (boost::algorithm::equals(name, "smf-gw-wmbus-hardware")) {}	//	skip
			//	else if (boost::algorithm::equals(name, "smf-gw-wmbus-type")) {}		//	skip
			//	else if (boost::algorithm::equals(name, "smf-form-gw-srv-visible-meter")
			//		|| boost::algorithm::equals(name, "smf-form-gw-srv-active-meter")) {
			//		//	get a server id as hex string and convert it into a binary buffer
			//		auto const inp = cyng::value_cast<std::string>(obj, "");
			//		auto const r = cyng::parse_hex_string(inp);
			//		if (r.second) {
			//			params.emplace(name, cyng::make_object(r.first));
			//		}
			//		else {
			//			params.emplace(name, cyng::make_object(cyng::make_buffer({ 0, 0, 0, 0, 0, 0 })));
			//		}
			//	}
			//	else {
			//		params.emplace(name, obj);
			//	}
			//}

		//	return params;
		//}


		std::string const& proxy_data::get_channel() const
		{
			return channel_;
		}

		cyng::buffer_t const& proxy_data::get_srv() const
		{
			return srv_;
		}

		std::string const& proxy_data::get_user() const
		{
			return name_;
		}

		std::string const& proxy_data::get_pwd() const
		{
			return pwd_;
		}

		boost::uuids::uuid proxy_data::get_ident_tag() const
		{
			return tag_;
		}

		boost::uuids::uuid proxy_data::get_source_tag() const
		{
			return source_;
		}

		boost::uuids::uuid proxy_data::get_ws_tag() const
		{
			return ws_tag_;
		}

		std::size_t proxy_data::get_sequence() const
		{
			return seq_;
		}

		cyng::vector_t proxy_data::get_key() const
		{
			return key_;
		}


		bool proxy_data::is_waiting() const
		{
			return STATE_WAITING_ == state_;
		}

		void proxy_data::next()
		{
			state_ = STATE_PROCESSING_;
		}

		cyng::object read_server_id(cyng::object obj)
		{
			auto const inp = cyng::value_cast<std::string>(obj, "");
			auto const r = cyng::parse_hex_string(inp);
			return (r.second)
				? cyng::make_object(r.first)
				: cyng::make_object()
				;
		}

		cyng::object parse_server_id(cyng::object obj)
		{
			auto const inp = cyng::value_cast<std::string>(obj, "");
			std::pair<cyng::buffer_t, bool> const r = node::sml::parse_srv_id(inp);
			return (r.second)
				? cyng::make_object(r.first)
				: cyng::make_object()
				;
		}

	}
}


