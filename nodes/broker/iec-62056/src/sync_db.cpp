/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2020 Sylko Olzscher 
 * 
 */ 

#include "sync_db.h"
#include <smf/shared/db_schemes.h>
#include <smf/cluster/generator.h>

#include <cyng/table/meta.hpp>
#include <cyng/io/serializer.h>
#include <cyng/tuple_cast.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/uuid/nil_generator.hpp>

namespace node 
{
	void create_cache(cyng::logging::log_ptr logger, cyng::store::db& db)
	{
		CYNG_LOG_TRACE(logger, "create "
			<< db_sync::tables_.size()
			<< " cache tables");

		for (auto const& tbl : db_sync::tables_) {
			if (!tbl.custom_) {
				if (!create_table(db, tbl.name_)) {
					CYNG_LOG_FATAL(logger, "cannot create table: " << tbl.name_);
				}
				else {
					CYNG_LOG_DEBUG(logger, "create table: " << tbl.name_);
				}
			}
		}

		//
		//	all tables created
		//
		CYNG_LOG_INFO(logger, db.size() << " tables created");
	}

	void clear_cache(cyng::store::db& db, boost::uuids::uuid tag)
	{
		for (auto const& tbl : db_sync::tables_) {
				db.clear(tbl.name_, tag);
		}
	}

	db_sync::db_sync(cyng::logging::log_ptr logger, cyng::store::db& db)
		: logger_(logger)
		, db_(db)
	{}

	void db_sync::register_this(cyng::controller& vm)
	{
		vm.register_function("db.res.insert", 4, std::bind(&db_sync::db_res_insert, this, std::placeholders::_1));
		vm.register_function("db.res.remove", 2, std::bind(&db_sync::db_res_remove, this, std::placeholders::_1));
		vm.register_function("db.res.modify.by.attr", 3, std::bind(&db_sync::db_res_modify_by_attr, this, std::placeholders::_1));
		vm.register_function("db.res.modify.by.param", 3, std::bind(&db_sync::db_res_modify_by_param, this, std::placeholders::_1));
		vm.register_function("db.req.insert", 4, std::bind(&db_sync::db_req_insert, this, std::placeholders::_1));
		vm.register_function("db.req.remove", 3, std::bind(&db_sync::db_req_remove, this, std::placeholders::_1));
		vm.register_function("db.req.modify.by.param", 5, std::bind(&db_sync::db_req_modify_by_param, this, std::placeholders::_1));
		vm.register_function("bus.res.subscribe", 6, std::bind(&db_sync::res_subscribe, this, std::placeholders::_1));

	}

	void db_sync::db_res_insert(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		//
		//	[TDevice,[32f1a373-83c9-4f24-8fac-b13103bc7466],[00000006,2018-03-11 18:35:33.61302590,true,,,comment #10,1010000,secret,device-10000],0]
		//
		//	* table name
		//	* record key
		//	* record data
		//	* generation
		//	
		CYNG_LOG_TRACE(logger_, "db.res.insert - " << cyng::io::to_str(frame));

		auto tpl = cyng::tuple_cast<
			std::string,			//	[0] table name
			cyng::table::key_type,	//	[1] table key
			cyng::table::data_type,	//	[2] record
			std::uint64_t			//	[3] generation
		>(frame);

		//
		//	assemble a record
		//
		std::reverse(std::get<1>(tpl).begin(), std::get<1>(tpl).end());
		std::reverse(std::get<2>(tpl).begin(), std::get<2>(tpl).end());

		db_insert(ctx
			, std::get<0>(tpl)		//	[0] table name
			, std::get<1>(tpl)		//	[1] table key
			, std::get<2>(tpl)		//	[2] record
			, std::get<3>(tpl)		//	[3] generation
			, ctx.tag());	//	[4] origin	
	}

	void db_sync::db_req_insert(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		//
		//	[*Session,[2ce46726-6bca-44b6-84ed-0efccb67774f],[00000000-0000-0000-0000-000000000000,2018-03-12 17:56:27.10338240,f51f2ae7,data-store,eaec7649-80d5-4b71-8450-3ee2c7ef4917,94aa40f9-70e8-4c13-987e-3ed542ecf7ab,null,session],1]
		//
		//	* table name
		//	* record key
		//	* record data
		//	* generation
		//	* source
		//	
		CYNG_LOG_TRACE(logger_, "db.req.insert - " << cyng::io::to_str(frame));

		//
		//	note: this cast is different from the cast in the store_domain (CYNG).
		//
		auto tpl = cyng::tuple_cast<
			std::string,			//	[0] table name
			cyng::table::key_type,	//	[1] table key
			cyng::table::data_type,	//	[2] record
			std::uint64_t,			//	[3] generation
			boost::uuids::uuid		//	[4] source - explicit
		>(frame);

		//
		//	assemble a record
		//
		std::reverse(std::get<1>(tpl).begin(), std::get<1>(tpl).end());
		std::reverse(std::get<2>(tpl).begin(), std::get<2>(tpl).end());

		db_insert(ctx
			, std::get<0>(tpl)		//	[0] table name
			, std::get<1>(tpl)		//	[1] table key
			, std::get<2>(tpl)		//	[2] record
			, std::get<3>(tpl)		//	[3] generation
			, std::get<4>(tpl));	//	[4] origin	

	}

	void db_sync::db_req_remove(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		//
		//	[*Session,[e72bc048-cb37-4a86-b156-d07f22608476]]
		//
		//	* table name
		//	* record key
		//	* source
		//	
		CYNG_LOG_TRACE(logger_, "db.req.remove - " << cyng::io::to_str(frame));

		auto tpl = cyng::tuple_cast<
			std::string,			//	[0] table name
			cyng::table::key_type,	//	[1] table key
			boost::uuids::uuid		//	[2] source
		>(frame);

		//
		//	reordering table key
		//	
		std::reverse(std::get<1>(tpl).begin(), std::get<1>(tpl).end());

		//node::db_req_remove(logger_
		//	, db_
		//	, std::get<0>(tpl)		//	[0] table name
		//	, std::get<1>(tpl)		//	[1] table key
		//	, std::get<2>(tpl));	//	[2] source

	}

	void db_sync::db_res_remove(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		//
		//	[TDevice,[def8e1ef-4a67-49ff-84a9-fda31509dd8e]]
		//
		//	* table name
		//	* record key
		//	
		CYNG_LOG_TRACE(logger_, "db.res.remove - " << cyng::io::to_str(frame));

		auto tpl = cyng::tuple_cast<
			std::string,			//	[0] table name
			cyng::table::key_type	//	[1] table key
		>(frame);

		//
		//	reordering table key
		//	
		std::reverse(std::get<1>(tpl).begin(), std::get<1>(tpl).end());

		//node::db_res_remove(logger_
		//	, db_
		//	, std::get<0>(tpl)		//	[0] table name
		//	, std::get<1>(tpl)		//	[1] table key
		//	, ctx.tag());			//	[2] source
	}

	void db_sync::db_res_modify_by_attr(cyng::context& ctx)
	{
		//	[TDevice,[0950f361-7800-4d80-b3bc-c6689f159439],(1:secret)]
		//
		//	* table name
		//	* record key
		//	* attr [column,value]
		//	
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "db.res.modify.by.attr - " << cyng::io::to_str(frame));

		auto tpl = cyng::tuple_cast<
			std::string,			//	[0] table name
			cyng::table::key_type,	//	[1] table key
			cyng::attr_t			//	[2] attribute
		>(frame);

		//
		//	reordering table key
		//	
		std::reverse(std::get<1>(tpl).begin(), std::get<1>(tpl).end());

		//node::db_res_modify_by_attr(logger_
		//	, db_
		//	, std::get<0>(tpl)		//	[0] table name
		//	, std::get<1>(tpl)		//	[1] table key
		//	, std::move(std::get<2>(tpl))		//	[2] attribute
		//	, ctx.tag());			//	[3] source
	}

	void db_sync::db_res_modify_by_param(cyng::context& ctx)
	{
		//	
		//
		//	* table name
		//	* record key
		//	* param [column,value]
		//	
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "db.res.modify.by.param - " << cyng::io::to_str(frame));

		auto tpl = cyng::tuple_cast<
			std::string,			//	[0] table name
			cyng::table::key_type,	//	[1] table key
			cyng::param_t			//	[2] parameter
		>(frame);

		//node::db_res_modify_by_param(logger_
		//	, db_
		//	, std::get<0>(tpl)		//	[0] table name
		//	, std::get<1>(tpl)		//	[1] table key
		//	, std::move(std::get<2>(tpl))		//	[2] parameter
		//	, ctx.tag());			//	[3] source
	}

	void db_sync::db_req_modify_by_param(cyng::context& ctx)
	{
		//	
		//	[*Session,[35d1d76d-56c3-4df7-b1ff-b7ad374d2e8f],("rx":33344),327,35d1d76d-56c3-4df7-b1ff-b7ad374d2e8f]
		//	[*Cluster,[1e4527b3-6479-4b2c-854b-e4793f40d864],("ping":00:00:0.003736),4,1e4527b3-6479-4b2c-854b-e4793f40d864]
		//
		//	* table name
		//	* record key
		//	* param [column,value]
		//	* generation
		//	* source
		//	
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "db.req.modify.by.param - " << cyng::io::to_str(frame));

		auto tpl = cyng::tuple_cast<
			std::string,			//	[0] table name
			cyng::table::key_type,	//	[1] table key
			cyng::param_t,			//	[2] parameter
			std::uint64_t,			//	[3] generation
			boost::uuids::uuid		//	[4] source
		>(frame);

		//
		//	reordering table key
		//	
		std::reverse(std::get<1>(tpl).begin(), std::get<1>(tpl).end());

		//node::db_req_modify_by_param(logger_
		//	, db_
		//	, std::get<0>(tpl)		//	[0] table name
		//	, std::get<1>(tpl)		//	[1] table key
		//	, std::move(std::get<2>(tpl))		//	[2] parameter
		//	, std::get<3>(tpl)		//	[3] generation
		//	, std::get<4>(tpl));	//	[4] source
	}

	bool db_sync::db_insert(cyng::context& ctx
		, std::string const& table		//	[0] table name
		, cyng::table::key_type key		//	[1] table key
		, cyng::table::data_type data	//	[2] record
		, std::uint64_t	gen
		, boost::uuids::uuid origin)
	{
		//
		//	insert new record
		//
		if (!db_.insert(table	//	table name
			, key	//	table key
			, data	//	table data
			, gen	//	generation
			, origin))
		{
			CYNG_LOG_WARNING(logger_, "res.insert failed "
				<< table		// table name
				<< " - "
				<< cyng::io::to_str(key));
			return false;
		}

		if (boost::algorithm::equals(table, "TIECBridge")) {
			//
			//	if connection is outgoin - start a new task
			//
			auto const rec = db_.lookup("TIECBridge", key);
			if (!rec.empty()) {
				auto direction = cyng::value_cast(rec["direction"], false);
				if (!direction) {

					auto const tpl = rec.convert();
					CYNG_LOG_INFO(logger_, "start client task for "
						<< cyng::io::to_str(tpl));



				}
			}
		}
		return true;
	}

	void db_sync::res_subscribe(cyng::context& ctx)
	{
		auto const frame = ctx.get_frame();
		//
		//	* table name
		//	* record key
		//	* record data
		//	* generation
		//	* origin session id
		//	* optional task id
		//	
		//CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		auto tpl = cyng::tuple_cast<
			std::string,			//	[0] table name
			cyng::table::key_type,	//	[1] table key
			cyng::table::data_type,	//	[2] record
			std::uint64_t,			//	[3] generation
			boost::uuids::uuid,		//	[4] origin session id
			std::size_t				//	[5] optional task id
		>(frame);

		CYNG_LOG_TRACE(logger_, ctx.get_name()
			<< ": "
			<< std::get<0>(tpl)		// table name
			<< " - "
			<< cyng::io::to_str(std::get<1>(tpl)));

		//
		//	reorder vectors
		//
		std::reverse(std::get<1>(tpl).begin(), std::get<1>(tpl).end());
		std::reverse(std::get<2>(tpl).begin(), std::get<2>(tpl).end());

		//
		//	insert new record
		//
		if (!db_insert(ctx
			, std::get<0>(tpl)	//	table name
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

	/**
	 * Initialize all used table names
	 */
	const std::array<db_sync::tbl_descr, 3>	db_sync::tables_ =
	{
		tbl_descr{"TMeter", false},
		tbl_descr{"TIECBridge", false},
		tbl_descr{"_Broker", false},	//	broker
	};

}
