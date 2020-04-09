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
			boost::uuids::uuid origin,	//	[3] ws tag (origin)

			std::string msg_type,		//	[4] SML message type
			cyng::buffer_t code,		//	[5] OBIS root code
			cyng::vector_t gw,			//	[6] TGateway/TDevice PK
			cyng::tuple_t params,		//	[7] parameters

			cyng::buffer_t srv,			//	[8] server id
			std::string name,			//	[9] name
			std::string	pwd	,			//	[10] pwd
			bool job					//	job - true if running as job
		)
		: tag_(tag)
			, source_(source)	//	source tag
			, seq_(seq)			//	cluster seq
			, origin_(origin)	//	ws tag

			, msg_type_(node::sml::messages::get_msg_code(msg_type))	//	SML message type (explicit type conversion)
			, code_(code)			//	OBIS root code (implicit type conversion)
			, gw_(gw)				//	TGateway key
			, params_(params)		//	parameters

			, srv_(srv)			//	server id
			, name_(name)		//	name
			, pwd_(pwd)			//	pwd
			, job_(job)
		{}

		//std::string proxy_data::get_msg_type() const
		//{
		//	return msg_type_;
		//}

		node::sml::sml_message proxy_data::get_msg_code() const
		{
			//
			//	only requests expected
			//
			return msg_type_;
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

		boost::uuids::uuid proxy_data::get_tag_ident() const
		{
			return tag_;
		}

		boost::uuids::uuid proxy_data::get_tag_source() const
		{
			return source_;
		}

		boost::uuids::uuid proxy_data::get_tag_origin() const
		{
			return origin_;
		}

		std::size_t proxy_data::get_sequence() const
		{
			return seq_;
		}

		cyng::vector_t proxy_data::get_key_gw() const
		{
			return gw_;
		}

		cyng::tuple_t const& proxy_data::get_params() const
		{
			return params_;
		}

		node::sml::obis proxy_data::get_root() const
		{
			//BOOST_ASSERT(code_.size() == 6);
			return code_;
		}

		bool proxy_data::is_job() const
		{
			return job_;
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


