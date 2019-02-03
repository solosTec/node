/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_LORA_DISPATCHER_H
#define NODE_LORA_DISPATCHER_H

#include <cyng/log.h>
#include <cyng/store/db.h>
#include <cyng/vm/controller.h>

namespace node 
{
	class dispatcher
	{
	public:
		dispatcher(cyng::logging::log_ptr, cyng::store::db&);

		void register_this(cyng::controller&);

		/**
		 *	subscribe to database
		 */
		void subscribe();

	private:
		void sig_ins(cyng::store::table const*
			, cyng::table::key_type const&
			, cyng::table::data_type const&
			, std::uint64_t
			, boost::uuids::uuid);
		void sig_del(cyng::store::table const*, cyng::table::key_type const&, boost::uuids::uuid);
		void sig_clr(cyng::store::table const*, boost::uuids::uuid);
		void sig_mod(cyng::store::table const*
			, cyng::table::key_type const&
			, cyng::attr_t const&
			, std::uint64_t
			, boost::uuids::uuid);

		void subscribe(std::string table, std::string const& channel, boost::uuids::uuid tag);

		void res_subscribe(cyng::context& ctx);

	private:
		cyng::logging::log_ptr logger_;
		cyng::store::db& cache_;
	};
}

#endif
 