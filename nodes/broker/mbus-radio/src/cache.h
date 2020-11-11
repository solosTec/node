/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2020 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_IEC_CACHE_H
#define NODE_IEC_CACHE_H

#include <cyng/log.h>
#include <cyng/store/db.h>
#include <cyng/intrinsics/aes_keys.hpp>

namespace node 
{
	class cache
	{
	public:
		cache(cyng::logging::log_ptr, cyng::store::db&);

		cyng::table::record lookup_meter_by_ident(std::string const& id);
		cyng::crypto::aes_128_key lookup_aes(cyng::table::key_type const&);

	private:

	private:
		cyng::logging::log_ptr logger_;
		cyng::store::db& db_;

	};
}

#endif
