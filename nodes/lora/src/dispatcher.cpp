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
		//
		//	data handling
		//
		vm.register_function("db.trx.start", 0, [this](cyng::context& ctx) {
			CYNG_LOG_TRACE(logger_, ctx.get_name());
			});
		vm.register_function("db.trx.commit", 1, [this](cyng::context& ctx) {
			auto const frame = ctx.get_frame();
			CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));
			});
		vm.register_function("bus.res.subscribe", 6, std::bind(&dispatcher::res_subscribe, this, std::placeholders::_1));
	}

	void dispatcher::subscribe()
	{
		cache_.get_listener("TLoRaDevice"
			, std::bind(&dispatcher::sig_ins, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
			, std::bind(&dispatcher::sig_del, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			, std::bind(&dispatcher::sig_clr, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&dispatcher::sig_mod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
		cache_.get_listener("_Config"
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
		CYNG_LOG_DEBUG(logger_, "sig.ins - "
			<< tbl->meta().get_name());

		cyng::table::record rec(tbl->meta_ptr(), key, data, gen);

		if (boost::algorithm::equals(tbl->meta().get_name(), "TLoRaDevice"))
		{
			//	
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_Config"))
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
		CYNG_LOG_DEBUG(logger_, "sig.del - "
			<< tbl->meta().get_name());

		if (boost::algorithm::equals(tbl->meta().get_name(), "TLoRaDevice"))
		{
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_SysMsg"))
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
		if (boost::algorithm::equals(tbl->meta().get_name(), "TLoRaDevice"))
		{
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_SysMsg"))
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
		CYNG_LOG_DEBUG(logger_, "sig.mod - "
			<< tbl->meta().get_name());

		//
		//	convert attribute to parameter (as map)
		//
		auto pm = tbl->meta().to_param_map(attr);

		if (boost::algorithm::equals(tbl->meta().get_name(), "TLoRaDevice"))
		{
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_Config"))
		{
		}
		else
		{
			CYNG_LOG_WARNING(logger_, "sig.mod - unknown table "
				<< tbl->meta().get_name());

		}
	}

	//void dispatcher::update_channel(std::string const& channel, std::size_t size)
	//{
	//	auto tpl = cyng::tuple_factory(
	//		cyng::param_factory("cmd", std::string("update")),
	//		cyng::param_factory("channel", channel),
	//		cyng::param_factory("value", size));

	//	auto msg = cyng::json::to_string(tpl);
	//	connection_manager_.push_event(channel, msg);
	//}


	void dispatcher::subscribe(std::string table, std::string const& channel, boost::uuids::uuid tag)
	{
	}

	void dispatcher::res_subscribe(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		auto tpl = cyng::tuple_cast<
			std::string,			//	[0] table name
			cyng::table::key_type,	//	[1] table key
			cyng::table::data_type,	//	[2] record
			std::uint64_t,			//	[3] generation
			boost::uuids::uuid,		//	[4] origin session id
			std::size_t				//	[5] optional task id
		>(frame);

		CYNG_LOG_TRACE(logger_, "res.subscribe "
			<< std::get<0>(tpl)		// table name
			<< " - "
			<< cyng::io::to_str(std::get<1>(tpl)));

		//
		//	reorder vectors
		//
		std::reverse(std::get<1>(tpl).begin(), std::get<1>(tpl).end());
		std::reverse(std::get<2>(tpl).begin(), std::get<2>(tpl).end());

#ifdef _DEBUG
		if (boost::algorithm::equals(std::get<0>(tpl), "TLoRaDevice")) {
			BOOST_ASSERT_MSG(std::get<2>(tpl).at(0).get_class().tag() == cyng::TC_MAC64, "DevEUI has wrong data type");
		}
#endif

		if (!cache_.insert(std::get<0>(tpl)	//	table name
			, std::get<1>(tpl)	//	table key
			, std::get<2>(tpl)	//	table data
			, std::get<3>(tpl)	//	generation
			, std::get<4>(tpl)))
		{
			CYNG_LOG_WARNING(logger_, "res.subscribe failed "
				<< std::get<0>(tpl)		// table name
				<< " - "
				<< cyng::io::to_str(std::get<1>(tpl)));
		}
	}

}
