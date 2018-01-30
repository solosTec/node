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
//#include <cyng/vm/controller.h>
//#include <cyng/io/parser/parser.h>
#include <boost/random.hpp>

namespace node 
{
	class client
	{
		friend class connection;

	public:
		client(cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, cyng::store::db&);

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

	private:
		cyng::async::mux& mux_;
		cyng::logging::log_ptr logger_;
		cyng::store::db& db_;

		boost::random::mt19937 rng_;
		boost::random::uniform_int_distribution<std::uint32_t> distribution_;

	};

}



#endif

