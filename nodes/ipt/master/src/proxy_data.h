/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_IPT_MASTER_PROXY_DATA_H
#define NODE_IPT_MASTER_PROXY_DATA_H

#include <cyng/intrinsics/buffer.h>
#include <cyng/object.h>
#include <cyng/intrinsics/sets.h>
#include <boost/uuid/uuid.hpp>

namespace node 
{
	namespace ipt
	{
		class proxy_data 
		{
		public:
			proxy_data(
				boost::uuids::uuid,		//	[0] ident tag
				boost::uuids::uuid,		//	[1] source tag
				std::uint64_t,			//	[2] cluster seq
				cyng::vector_t,			//	[3] TGateway key
				boost::uuids::uuid,		//	[4] ws tag
				std::string channel,	//	[5] channel
				cyng::vector_t,			//	[6] sections
				cyng::vector_t,			//	[7] parameters
				cyng::buffer_t,			//	[8] server id
				std::string,			//	[9] name
				std::string,			//	[10] pwd
				std::size_t
			);

			/** 
			 * @return a vector containing all affected sections
			 */
			std::vector<std::string> get_section_names() const;

			/**
			 * Walks over parameter vector and build a parameter map
			 */
			cyng::vector_t get_params(std::string const& section) const;

			std::string const& get_channel() const;
			cyng::buffer_t const& get_srv() const;
			std::string const& get_user() const;
			std::string const& get_pwd() const;
			boost::uuids::uuid get_ident_tag() const;
			boost::uuids::uuid get_source_tag() const;
			boost::uuids::uuid get_ws_tag() const;
			std::size_t get_sequence() const;
			cyng::vector_t get_key() const;

			/**
			 * @return true if data not in processing state
			 */
			bool is_waiting() const;

			/**
			 * transit to next state
			 */
			void next();

		private:
			boost::uuids::uuid const tag_;		//	ident tag
			boost::uuids::uuid const source_;	//	source tag
			std::uint64_t const seq_;			//	cluster seq
			cyng::vector_t const key_;			//	TGateway key
			boost::uuids::uuid const ws_tag_;	//	ws tag
			std::string const channel_;
			cyng::vector_t const sections_;		//	sections
			cyng::vector_t const params_;		//	parameters
			cyng::buffer_t const srv_;			//	server id
			std::string const name_;			//	name
			std::string const pwd_;				//	pwd
			enum {
				STATE_PROCESSING_,
				STATE_WAITING_,
			} state_;
		};

		/**
		 * Convert obj to string and parse it as an server ID
		 */
		cyng::object read_server_id(cyng::object obj);
		cyng::object parse_server_id(cyng::object obj);
	}
}


#endif

