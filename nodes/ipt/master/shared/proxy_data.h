/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_MASTER_PROXY_DATA_H
#define NODE_IPT_MASTER_PROXY_DATA_H

#include <smf/sml/intrinsics/obis.h>

#include <cyng/intrinsics/buffer.h>
#include <cyng/object.h>
#include <cyng/intrinsics/sets.h>

#include <boost/uuid/uuid.hpp>

namespace node
{

	class proxy_data
	{
	public:
		proxy_data(
			boost::uuids::uuid,		//	[0] ident tag (target session)
			boost::uuids::uuid,		//	[1] source tag
			std::uint64_t,			//	[2] cluster seq
			boost::uuids::uuid,		//	[3] ws tag (origin)

			std::string msg_type,	//	[4] SML message type
			sml::obis_path_t,				//	[5] OBIS root code
			cyng::vector_t,			//	[6] TGateway/Device PK
			cyng::tuple_t,			//	[7] parameters (optional)

			cyng::buffer_t,			//	[8] server id
			std::string,			//	[9] name
			std::string,			//	[10] pwd

			bool					//	job - true if running as job
		);

		proxy_data(
			boost::uuids::uuid,		//	[0] ident tag (target session)
			boost::uuids::uuid,		//	[1] source tag
			std::uint64_t,			//	[2] cluster seq
			boost::uuids::uuid,		//	[3] ws tag (origin)

			sml::message_e msg_code,	//	[4] SML message type
			sml::obis_path_t,		//	[5] OBIS root code
			cyng::vector_t,			//	[6] TGateway/Device PK
			cyng::tuple_t,			//	[7] parameters (optional)

			cyng::buffer_t,			//	[8] server id
			std::string,			//	[9] name
			std::string,			//	[10] pwd

			bool					//	job - true if running as job
		);

		/**
		 * Copy constructor
		 */
		proxy_data(proxy_data const&) = default;
		proxy_data(proxy_data&&) noexcept = default;

		//std::string get_msg_type() const;

		/**
		 * Turns the message type string into the SML message type code
		 */
		node::sml::message_e get_msg_code() const;

		cyng::buffer_t const& get_srv() const;
		std::string const& get_user() const;
		std::string const& get_pwd() const;
		boost::uuids::uuid get_tag_ident() const;

		/**
		 * @return source session cluster tag
		 */
		boost::uuids::uuid get_tag_source() const;

		/**
		 * @return origin (web-socket) tag
		 */
		boost::uuids::uuid get_tag_origin() const;

		/**
		 * @return cluster message sequence
		 */
		std::size_t get_sequence() const;

		cyng::vector_t get_key_gw() const;

		cyng::tuple_t const& get_params() const;

		/**
		 * OBIS code (root path)
		 */
		sml::obis get_root() const;
		sml::obis_path_t get_path() const;

		bool is_job() const;

	private:
		boost::uuids::uuid const tag_;		//	ident tag (target)
		boost::uuids::uuid const source_;	//	source tag
		std::uint64_t const seq_;			//	cluster seq
		boost::uuids::uuid const origin_;	//	ws tag (origin)

		sml::message_e const msg_type_;	//!<	SML message type
		sml::obis_path_t const path_;
		cyng::vector_t const gw_;			//	TGateway/TDevice PK
		cyng::tuple_t const params_;		//	parameters (optional)

		cyng::buffer_t const srv_;			//	server id
		std::string const name_;			//	name
		std::string const pwd_;				//	pwd

		bool const job_;
	};

	/**
	 * Convert obj to string and parse it as an server ID
	 */
	cyng::object read_server_id(cyng::object obj);
	cyng::object parse_server_id(cyng::object obj);

	//proxy_data finalize_get_proc_parameter_request(proxy_data&& pd);
	//proxy_data finalize_set_proc_parameter_request(proxy_data&& pd)

	/**
	 * preprocessing and cleanup of proxy data
	 */
	proxy_data finalize(proxy_data&&);
}


#endif

