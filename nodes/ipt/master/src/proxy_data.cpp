/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#include "proxy_data.h"

#include <cyng/vector_cast.hpp>
#include <cyng/numeric_cast.hpp>
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

		//cyng::vector_t proxy_data::get_sections() const
		//{
		//	return sections_;
		//}

		cyng::param_map_t proxy_data::get_params() const
		{
			cyng::param_map_t params;

			//	vector of tuples

			auto const reader = cyng::make_reader(params_);

			for (std::size_t idx = 0; idx < params_.size(); ++idx) {

				auto name = cyng::value_cast<std::string>(reader[idx].get("name"), "");
				auto obj = reader[idx].get("value");

				if (boost::algorithm::equals(name, "smf-form-gw-ipt-srv")) {

					//
					//	send server/gateway ID as buffer_t type
					//
					const std::string inp = cyng::value_cast<std::string>(obj, "000000");
					const auto r = cyng::parse_hex_string(inp);
					if (r.second) {
						params.emplace("serverId", cyng::make_object(r.first));
					}
					else {
						params.emplace("serverId", cyng::make_object("000000"));
					}
				}
				//else if (boost::algorithm::starts_with(name, "smf-gw-ipt-host-")) {

				//	//
				//	//	send host name as it is (string)
				//	//
				//	try {
				//		const std::string inp = cyng::value_cast<std::string>(obj, "0.0.0.0");
				//		const auto address = boost::asio::ip::make_address(inp);
				//		params.emplace(name, cyng::make_object(address));
				//	}
				//	catch (std::runtime_error const&) {
				//		params.emplace(name, cyng::make_object(boost::asio::ip::make_address("0.0.0.0")));
				//	}
				//}
				else if (boost::algorithm::starts_with(name, "smf-gw-ipt-local-")) {

					//
					//	send port as u16 
					//
					params.emplace(name, cyng::make_object(cyng::numeric_cast<std::uint16_t>(obj, 26862)));
				}
				else if (boost::algorithm::starts_with(name, "smf-gw-ipt-remote-")) {

					//
					//	send port as u16 
					//
					params.emplace(name, cyng::make_object(cyng::numeric_cast<std::uint16_t>(obj, 0)));
				}
				else if (boost::algorithm::equals(name, "parameter smf-gw-wmbus-power")) {
					//
					//	send transmission power value as u8 
					//
					params.emplace(name, cyng::make_object(cyng::numeric_cast<std::uint8_t>(obj, 0)));
				}
				else if (boost::algorithm::equals(name, "smf-gw-wmbus-protocol")) {
					//
					//	send protocol type as u8 
					//
					params.emplace(name, cyng::make_object(cyng::numeric_cast<std::uint8_t>(obj, 0)));
				}
				else if (boost::algorithm::equals(name, "smf-gw-wmbus-reboot")) {
					//
					//	send reboot cycle as u64 
					//
					params.emplace(name, cyng::make_object(cyng::numeric_cast<std::uint64_t>(obj, 0)));
				}
				else if (boost::algorithm::equals(name, "smf-gw-wmbus-adapter")) {}		//	skip
				else if (boost::algorithm::equals(name, "smf-gw-wmbus-firmware")) {}	//	skip
				else if (boost::algorithm::equals(name, "smf-gw-wmbus-hardware")) {}	//	skip
				else if (boost::algorithm::equals(name, "smf-gw-wmbus-type")) {}		//	skip
				else {
					params.emplace(name, obj);
				}
			}

			return params;
		}


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

	}
}


