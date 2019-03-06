/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 

#include "dispatcher.h"
#include <cyng/json.h>
#include <cyng/io/serializer.h>
#include <cyng/tuple_cast.hpp>
#include <cyng/sys/memory.h>

#include <boost/algorithm/string.hpp>
#include <boost/core/ignore_unused.hpp>

namespace node 
{
	dispatcher::dispatcher(cyng::logging::log_ptr logger, cyng::store::db& cache)
		: logger_(logger)
		, cache_(cache)
	{}

	void dispatcher::register_this(cyng::controller& vm)
	{
		//vm.register_function("store.relation", 2, std::bind(&dispatcher::store_relation, this, std::placeholders::_1));
		//vm.register_function("bus.res.gateway.proxy", 9, std::bind(&dispatcher::res_gateway_proxy, this, std::placeholders::_1));
		//vm.register_function("bus.res.attention.code", 7, std::bind(&dispatcher::res_attention_code, this, std::placeholders::_1));

		//vm.register_function("http.move", 2, std::bind(&dispatcher::http_move, this, std::placeholders::_1));
	}


	void dispatcher::subscribe()
	{
		cache_.get_listener("_TimeSeries"
			, std::bind(&dispatcher::sig_ins, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
			, std::bind(&dispatcher::sig_del, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			, std::bind(&dispatcher::sig_clr, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&dispatcher::sig_mod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));

		CYNG_LOG_INFO(logger_, "db has " << cache_.num_all_slots() << " connected slots");

	}

	void dispatcher::sig_ins(cyng::store::table const* tbl
		, cyng::table::key_type const& key
		, cyng::table::data_type const& data
		, std::uint64_t gen
		, boost::uuids::uuid source)
	{
		cyng::table::record rec(tbl->meta_ptr(), key, data, gen);

		if (boost::algorithm::equals(tbl->meta().get_name(), "_TimeSeries"))
		{
		}
		else
		{
			CYNG_LOG_WARNING(logger_, "sig.ins - unknown table "
				<< tbl->meta().get_name());
		}
	}

	void dispatcher::sig_del(cyng::store::table const* tbl, cyng::table::key_type const& key, boost::uuids::uuid source)
	{
		if (boost::algorithm::equals(tbl->meta().get_name(), "_TimeSeries"))
		{
		}
		else
		{
			CYNG_LOG_ERROR(logger_, "sig.del - unknown table "
				<< tbl->meta().get_name());
		}
	}

	void dispatcher::sig_clr(cyng::store::table const* tbl, boost::uuids::uuid source)
	{
		if (boost::algorithm::equals(tbl->meta().get_name(), "_TimeSeries"))
		{
		}
		else
		{
			CYNG_LOG_ERROR(logger_, "sig.clr - unknown table "
				<< tbl->meta().get_name());
		}
	}

	void dispatcher::sig_mod(cyng::store::table const* tbl
		, cyng::table::key_type const& key
		, cyng::attr_t const& attr
		, std::uint64_t gen
		, boost::uuids::uuid source)
	{
		//	to much noise
		//CYNG_LOG_DEBUG(logger_, "sig.mod - "
		//	<< tbl->meta().get_name());

		//
		//	convert attribute to parameter (as map)
		//
		auto pm = tbl->meta().to_param_map(attr);

		if (boost::algorithm::equals(tbl->meta().get_name(), "_TimeSeries"))
		{
		}
		else
		{
			CYNG_LOG_WARNING(logger_, "sig.mod - unknown table "
				<< tbl->meta().get_name());

		}
	}
}
