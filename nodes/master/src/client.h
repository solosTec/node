/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_MASTER_CLIENT_H
#define NODE_MASTER_CLIENT_H

#include <cyng/async/mux.h>
#include <cyng/log.h>
#include <cyng/store/db.h>
#include <cyng/table/key.hpp>
#include <boost/random.hpp>
#include <boost/uuid/random_generator.hpp>

namespace node 
{
	std::pair<cyng::table::key_list_t, std::uint16_t> collect_matching_targets(const cyng::store::table* tbl_target
		, boost::uuids::uuid
		, std::string const& name);

	std::uint32_t get_source_channel(const cyng::store::table* tbl_session, boost::uuids::uuid);
	std::size_t remove_targets_by_tag(cyng::store::table* tbl_target, boost::uuids::uuid);
	cyng::table::key_list_t get_targets_by_peer(cyng::store::table const* tbl_target, std::size_t hash);
	cyng::table::key_list_t get_channels_by_peer(cyng::store::table const* tbl_channel, std::size_t hash);

	class client
	{
		friend class connection;

	public:
		client(cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, cyng::store::db&
			, bool connection_auto_login
			, bool connection_superseed);

		client(client const&) = delete;
		client& operator=(client const&) = delete;

		cyng::vector_t req_login(boost::uuids::uuid,		//	[0] remote client tag
			boost::uuids::uuid,		//	[1] peer tag
			std::uint64_t,			//	[2] sequence number
			std::string,			//	[3] name
			std::string,			//	[4] pwd/credential
			std::string,			//	[5] authorization scheme
			cyng::param_map_t const&,		//	[6] bag)
			cyng::object session);

		cyng::vector_t req_close(boost::uuids::uuid,		//	[0] remote client tag
			boost::uuids::uuid,		//	[1] peer tag
			std::uint64_t,			//	[2] sequence number
			int);

		cyng::vector_t req_open_connection(boost::uuids::uuid,		//	[0] remote client tag
			boost::uuids::uuid,		//	[1] peer tag
			std::uint64_t,			//	[2] sequence number
			std::string,			//	[3] number
			cyng::param_map_t const&);		//	[4] bag)

		cyng::vector_t res_open_connection(boost::uuids::uuid,		//	[0] origin client tag
			boost::uuids::uuid,		//	[1] peer tag
			std::uint64_t,			//	[2] sequence number
			bool,					//	[3] success
			cyng::param_map_t const&,		//	[4] options
			cyng::param_map_t const&);		//	[5] bag);

		void req_transmit_data(boost::uuids::uuid,		//	[0] origin client tag
			boost::uuids::uuid,		//	[1] peer tag
			std::uint64_t,			//	[2] sequence number
			cyng::param_map_t const&,		//	[5] bag
			cyng::object);	//	data

		cyng::vector_t req_open_push_channel(boost::uuids::uuid,		//	[0] remote client tag
			boost::uuids::uuid,		//	[1] peer tag
			std::uint64_t,			//	[2] sequence number
			std::string,			//	[3] target name
			std::string,			//	[4] device name
			std::string,			//	[5] device number
			std::string,			//	[6] device software version
			std::string,			//	[7] device id
			std::chrono::seconds,	//	[8] timeout
			cyng::param_map_t const&);		//	[9] bag

		cyng::vector_t req_close_push_channel(boost::uuids::uuid tag,
			boost::uuids::uuid peer,
			std::uint64_t seq,			//	[2] sequence number
			std::uint32_t channel,		//	[3] channel id
			cyng::param_map_t const& bag);

		cyng::vector_t req_transfer_pushdata(boost::uuids::uuid,		//	[0] origin client tag
			boost::uuids::uuid,		//	[1] peer tag
			std::uint64_t,			//	[2] sequence number
			std::uint32_t,			//	[3] channel
			std::uint32_t,			//	[4] source
			cyng::param_map_t const&,		//	[5] bag
			cyng::object);			//	data

		cyng::vector_t req_register_push_target(boost::uuids::uuid,		//	[0] remote client tag
			boost::uuids::uuid,		//	[1] peer tag
			std::uint64_t,			//	[2] sequence number
			std::string,			//	[3] target name
			cyng::param_map_t const&);

		cyng::vector_t update_attr(boost::uuids::uuid,		//	[0] origin client tag
			boost::uuids::uuid,		//	[1] peer tag
			std::uint64_t,			//	[2] sequence number
			std::string,			//	[3] name
			cyng::object,			//	[4] value
			cyng::param_map_t);

	private:
		cyng::vector_t req_open_push_channel_empty(boost::uuids::uuid tag,
			std::uint64_t seq, 
			cyng::param_map_t const& bag);

	private:
		cyng::async::mux& mux_;
		cyng::logging::log_ptr logger_;
		cyng::store::db& db_;
		const bool connection_auto_login_;
		const bool connection_superseed_;

		boost::random::mt19937 rng_;
		boost::random::uniform_int_distribution<std::uint32_t> distribution_;
		boost::uuids::random_generator uuid_gen_;

	};

}



#endif

