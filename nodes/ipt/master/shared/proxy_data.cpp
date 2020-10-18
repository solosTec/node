/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "proxy_data.h"
#include <smf/sml/srv_id_io.h>
#include <smf/sml/parser/srv_id_parser.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/intrinsics/obis_factory.hpp>

#include <cyng/vector_cast.hpp>
#include <cyng/set_cast.h>

#include <cyng/dom/reader.h>
#include <cyng/parser/buffer_parser.h>
#include <cyng/factory/factory.hpp>
#include <cyng/numeric_cast.hpp>

#include <boost/algorithm/string.hpp>

namespace node
{
	proxy_data::proxy_data(
		boost::uuids::uuid tag,		//	[0] ident tag
		boost::uuids::uuid source,	//	[1] source tag
		std::uint64_t seq,			//	[2] cluster seq
		boost::uuids::uuid origin,	//	[3] ws tag (origin)

		std::string msg_type,		//	[4] SML message type
		sml::obis_path_t path,				//	[5] OBIS root code
		cyng::vector_t gw,			//	[6] TGateway/TDevice PK
		cyng::tuple_t params,		//	[7] parameters

		cyng::buffer_t srv,			//	[8] server id
		std::string name,			//	[9] name
		std::string	pwd,			//	[10] pwd
		bool job					//	job - true if running as job
	)
	: tag_(tag)
		, source_(source)	//	source tag
		, seq_(seq)			//	cluster seq
		, origin_(origin)	//	ws tag

		, msg_type_(sml::messages::get_msg_code(msg_type))	//	SML message type (explicit type conversion)
		, path_(path)			//	OBIS root code (implicit type conversion)
		, gw_(gw)				//	TGateway key
		, params_(params)		//	parameters

		, srv_(srv)			//	server id
		, name_(name)		//	name
		, pwd_(pwd)			//	pwd
		, job_(job)
	{}

	proxy_data::proxy_data(
		boost::uuids::uuid tag,		//	[0] ident tag
		boost::uuids::uuid source,	//	[1] source tag
		std::uint64_t seq,			//	[2] cluster seq
		boost::uuids::uuid origin,	//	[3] ws tag (origin)

		sml::message_e msg_code,	//	[4] SML message type
		sml::obis_path_t path,				//	[5] OBIS root code
		cyng::vector_t gw,			//	[6] TGateway/TDevice PK
		cyng::tuple_t params,		//	[7] parameters

		cyng::buffer_t srv,			//	[8] server id
		std::string name,			//	[9] name
		std::string	pwd,			//	[10] pwd
		bool job					//	job - true if running as job
	)
		: tag_(tag)
		, source_(source)	//	source tag
		, seq_(seq)			//	cluster seq
		, origin_(origin)	//	ws tag

		, msg_type_(msg_code)	//	SML message type (explicit type conversion)
		, path_(path)			//	OBIS root code (implicit type conversion)
		, gw_(gw)				//	TGateway key
		, params_(params)		//	parameters

		, srv_(srv)			//	server id
		, name_(name)		//	name
		, pwd_(pwd)			//	pwd
		, job_(job)
	{}

	sml::message_e proxy_data::get_msg_code() const
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

	sml::obis proxy_data::get_root() const
	{
		BOOST_ASSERT(!path_.empty());
		return path_.front();
	}

	sml::obis_path_t proxy_data::get_path() const
	{
		return path_;
	}

	bool proxy_data::is_job() const
	{
		return job_;
	}


	//
	//	helper functions:
	//


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


	proxy_data finalize_get_proc_parameter_request(proxy_data&& pd)
	{
		auto const root = pd.get_root();
		auto const reader = cyng::make_reader(pd.get_params());

		if (sml::OBIS_ROOT_ACCESS_RIGHTS == root) {
			auto const obj_path = cyng::to_vector(reader.get("path"));
			if (obj_path.size() == 3) {

				auto const role = cyng::numeric_cast<std::uint8_t>(obj_path.at(0), 1);
				auto const user = cyng::numeric_cast<std::uint8_t>(obj_path.at(1), 1);
				auto const meter = cyng::numeric_cast<std::uint8_t>(obj_path.at(2), 1);

				return proxy_data(pd.get_tag_ident()
					, pd.get_tag_source()
					, pd.get_sequence()
					, pd.get_tag_origin()
					, pd.get_msg_code()
					, sml::obis_path_t({ root
						, sml::make_obis(0x81, 0x81, 0x81, 0x60, role, 0xFF)
						, sml::make_obis(0x81, 0x81, 0x81, 0x60, role, user)
						, sml::make_obis(0x81, 0x81, 0x81, 0x64, 0x01, meter) })
					, pd.get_key_gw()
					, pd.get_params()
					, pd.get_srv()
					, pd.get_user()
					, pd.get_pwd()
					, pd.is_job());
			}
		}
		else if (sml::OBIS_ROOT_SENSOR_PARAMS == root
			|| sml::OBIS_ROOT_DATA_COLLECTOR == root
			|| sml::OBIS_ROOT_PUSH_OPERATIONS == root) {

			auto const meter = cyng::value_cast<std::string>(reader.get("meter"), "");
			auto const r = sml::parse_srv_id(meter);
			if (r.second) {

				return proxy_data(pd.get_tag_ident()
					, pd.get_tag_source()
					, pd.get_sequence()
					, pd.get_tag_origin()
					, pd.get_msg_code()
					, sml::obis_path_t({ root })
					, pd.get_key_gw()
					, pd.get_params()
					, r.first	//	instead of server
					, pd.get_user()
					, pd.get_pwd()
					, pd.is_job());
			}
		}
		return pd;
	}

	proxy_data finalize_set_proc_parameter_request(proxy_data&& pd)
	{
		auto const root = pd.get_root();
		auto const reader = cyng::make_reader(pd.get_params());

		if (sml::OBIS_ACTIVATE_DEVICE == root) {
			//
			//	activate meter device
			//
			auto const nr = cyng::numeric_cast<std::uint8_t>(reader.get("nr"), 0u);
			auto const meter = cyng::value_cast<std::string>(reader.get("meter"), "");
			auto const r = cyng::parse_hex_string(meter);

			if (r.second) {
				return proxy_data(pd.get_tag_ident()
					, pd.get_tag_source()
					, pd.get_sequence()
					, pd.get_tag_origin()
					, pd.get_msg_code()
					, sml::obis_path_t({ root
						, sml::make_obis(0x81, 0x81, 0x11, 0x06, 0xFB, nr)
						, sml::OBIS_SERVER_ID })
					, pd.get_key_gw()
					, pd.get_params()
					, r.first	//	server id
					, pd.get_user()
					, pd.get_pwd()
					, pd.is_job());
			}
		}
		else if (sml::OBIS_DEACTIVATE_DEVICE == root) {

			//
			//	deactivate meter device
			//
			auto const nr = cyng::numeric_cast<std::uint8_t>(reader.get("nr"), 0u);
			auto const meter = cyng::value_cast<std::string>(reader.get("meter"), "");
			auto const r = cyng::parse_hex_string(meter);
			if (r.second) {
				return proxy_data(pd.get_tag_ident()
					, pd.get_tag_source()
					, pd.get_sequence()
					, pd.get_tag_origin()
					, pd.get_msg_code()
					, sml::obis_path_t({ root
						, sml::make_obis(0x81, 0x81, 0x11, 0x06, 0xFC, nr)
						, sml::OBIS_SERVER_ID })
					, pd.get_key_gw()
					, pd.get_params()
					, r.first	//	server id
					, pd.get_user()
					, pd.get_pwd()
					, pd.is_job());
			}
		}
		else if (sml::OBIS_DELETE_DEVICE == root) {

			//
			//	remove a meter device from list of visible meters
			//
			auto const nr = cyng::numeric_cast<std::uint8_t>(reader.get("nr"), 0u);
			auto const meter = cyng::value_cast<std::string>(reader.get("meter"), "");
			auto const r = cyng::parse_hex_string(meter);
			if (r.second) {
				return proxy_data(pd.get_tag_ident()
					, pd.get_tag_source()
					, pd.get_sequence()
					, pd.get_tag_origin()
					, pd.get_msg_code()
					, sml::obis_path_t({ root
						, sml::make_obis(0x81, 0x81, 0x11, 0x06, 0xFD, nr)
						, sml::OBIS_SERVER_ID })
					, pd.get_key_gw()
					, pd.get_params()
					, r.first	//	server id
					, pd.get_user()
					, pd.get_pwd()
					, pd.is_job());
			}
		}
		return pd;
	}

	proxy_data finalize(proxy_data&& pd)
	{
		switch (pd.get_msg_code()) {
		case sml::message_e::GET_PROC_PARAMETER_REQUEST:
			return finalize_get_proc_parameter_request(std::move(pd));
		case sml::message_e::SET_PROC_PARAMETER_REQUEST:
			return pd;
		default:
			break;
		}
		return pd;
	}

}


