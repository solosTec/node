/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 

#include "sync_db.h"
#include "../../../nodes/shared/db/db_schemes.h"
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
		CYNG_LOG_TRACE(logger, "create cache tables");

		//
		//	time series
		//
		if (!create_table(db, "_TimeSeries"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table _TimeSeries");
		}

		//
		//	all tables created
		//
		CYNG_LOG_INFO(logger, db.size() << " tables created");
	}

	void clear_cache(cyng::store::db& db, boost::uuids::uuid tag)
	{
		db.clear("_TimeSeries", tag);
	}

	void res_subscribe(cyng::logging::log_ptr logger
		, cyng::store::db& db
		, std::string const& table		//	[0] table name
		, cyng::table::key_type key		//	[1] table key
		, cyng::table::data_type data	//	[2] record
		, std::uint64_t	gen				//	[3] generation
		, boost::uuids::uuid origin		//	[4] origin session id
		, std::size_t tsk)
	{
		//
		//	insert new record
		//
		if (!db.insert(table	//	table name
			, key	//	table key
			, data	//	table data
			, gen	//	generation
			, origin))
		{
			CYNG_LOG_WARNING(logger, "res.subscribe failed "
				<< table		// table name
				<< " - "
				<< cyng::io::to_str(key));
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
		//	
		CYNG_LOG_TRACE(logger_, "db.req.insert - " << cyng::io::to_str(frame));

		auto tpl = cyng::tuple_cast<
			std::string,			//	[0] table name
			cyng::table::key_type,	//	[1] table key
			cyng::table::data_type,	//	[2] record
			std::uint64_t,			//	[3] generation
			boost::uuids::uuid		//	[4] source
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

		node::db_req_remove(logger_
			, db_
			, std::get<0>(tpl)		//	[0] table name
			, std::get<1>(tpl)		//	[1] table key
			, std::get<2>(tpl));	//	[2] source

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

		node::db_res_remove(logger_
			, db_
			, std::get<0>(tpl)		//	[0] table name
			, std::get<1>(tpl)		//	[1] table key
			, ctx.tag());			//	[2] source
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

		node::db_res_modify_by_attr(logger_
			, db_
			, std::get<0>(tpl)		//	[0] table name
			, std::get<1>(tpl)		//	[1] table key
			, std::move(std::get<2>(tpl))		//	[2] attribute
			, ctx.tag());			//	[3] source
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

		node::db_res_modify_by_param(logger_
			, db_
			, std::get<0>(tpl)		//	[0] table name
			, std::get<1>(tpl)		//	[1] table key
			, std::move(std::get<2>(tpl))		//	[2] parameter
			, ctx.tag());			//	[3] source
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

		node::db_req_modify_by_param(logger_
			, db_
			, std::get<0>(tpl)		//	[0] table name
			, std::get<1>(tpl)		//	[1] table key
			, std::move(std::get<2>(tpl))		//	[2] parameter
			, std::get<3>(tpl)		//	[3] generation
			, std::get<4>(tpl));	//	[4] source
	}

	void db_sync::db_insert(cyng::context& ctx
		, std::string const& table		//	[0] table name
		, cyng::table::key_type key		//	[1] table key
		, cyng::table::data_type data	//	[2] record
		, std::uint64_t	gen
		, boost::uuids::uuid origin)
	{
		if (boost::algorithm::equals(table, "TGateway"))
		{
			db_.access([&](cyng::store::table* tbl_gw, const cyng::store::table* tbl_dev, const cyng::store::table* tbl_ses) {

				//
				//	search session with this device/GW tag
				//
				auto dev_rec = tbl_dev->lookup(key);

				//
				//	set device name
				//	set model
				//	set vFirmware
				//	set online state
				//
				if (!dev_rec.empty())
				{
					data.push_back(dev_rec["name"]);
					data.push_back(dev_rec["descr"]);
					data.push_back(dev_rec["id"]);
					data.push_back(dev_rec["vFirmware"]);

					//
					//	get online state
					//
					auto ses_rec = tbl_ses->find_first(cyng::param_t("device", key.at(0)));
					if (ses_rec.empty()) {
						data.push_back(cyng::make_object(0));
					}
					else {
						const auto rtag = cyng::value_cast(ses_rec["rtag"], boost::uuids::nil_uuid());
						data.push_back(cyng::make_object(rtag.is_nil() ? 1 : 2));
					}

					if (!tbl_gw->insert(key, data, gen, origin))
					{

						CYNG_LOG_WARNING(logger_, ctx.get_name()
							<< " failed "
							<< table		// table name
							<< " - "
							<< cyng::io::to_str(key)
							<< " => "
							<< cyng::io::to_str(data));

					}
				}
				else {

					std::stringstream ss;
					ss
						<< "gateway "
						<< cyng::io::to_str(key)
						<< " has no associated device"
						;

					CYNG_LOG_WARNING(logger_, ss.str());

					ctx.queue(bus_insert_msg(cyng::logging::severity::LEVEL_WARNING, ss.str()));

				}
			}	, cyng::store::write_access("TGateway")
				, cyng::store::read_access("TDevice")
				, cyng::store::read_access("_Session"));
		}
		else if (boost::algorithm::equals(table, "TMeter"))
		{
			//
			//	Additional values for TMeter
			//
			db_.access([&](cyng::store::table* tbl_meter, cyng::store::table const* tbl_gw) {

				//
				//	TMeter contains an optional reference to TGateway table
				//
				auto const key_gw = cyng::table::key_generator(data.at(10));
				auto const dev_gw = tbl_gw->lookup(key_gw);

				//
				//	set serverId
				//
				if (!dev_gw.empty())
				{
					data.push_back(dev_gw["serverId"]);
					data.push_back(dev_gw["online"]);
				}
				else
				{
					data.push_back(cyng::make_object("00000000000000"));
					data.push_back(cyng::make_object(-1));

					CYNG_LOG_WARNING(logger_, "res.subscribe - meter"
						<< cyng::io::to_str(key)
						<< " has no associated gateway");
				}

				if (!tbl_meter->insert(key, data, gen, origin))
				{

					CYNG_LOG_WARNING(logger_, ctx.get_name()
						<< " failed "
						<< table		// table name
						<< " - "
						<< cyng::io::to_str(key)
						<< " => "
						<< cyng::io::to_str(data));

				}

			}	, cyng::store::write_access("TMeter")
				, cyng::store::read_access("TGateway"));

		}
		else if (!db_.insert(table
			, key
			, data
			, gen
			, origin))
		{
			CYNG_LOG_WARNING(logger_, ctx.get_name()
				<< " failed "
				<< table		// table name
				<< " - "
				<< cyng::io::to_str(key)
				<< " => "
				<< cyng::io::to_str(data));

			//
			//	dump record data
			//
			std::size_t idx{ 0 };
			for (auto const& obj : data) {
				CYNG_LOG_TRACE(logger_, "data ["
					<< idx++
					<< "] "
					<< obj.get_class().type_name()
					<< ": "
					<< cyng::io::to_str(obj))
					;
			}

		}
		else
		{
			if (boost::algorithm::equals(table, "_Session"))
			{
				db_.access([&](cyng::store::table* tbl_gw, cyng::store::table* tbl_meter, const cyng::store::table* tbl_ses) {

					//
					//	[*Session,[2ce46726-6bca-44b6-84ed-0efccb67774f],[00000000-0000-0000-0000-000000000000,2018-03-12 17:56:27.10338240,f51f2ae7,data-store,eaec7649-80d5-4b71-8450-3ee2c7ef4917,94aa40f9-70e8-4c13-987e-3ed542ecf7ab,null,session],1]
					//	Gateway and Device table share the same table key
					//
					auto rec = tbl_ses->lookup(key);
					if (rec.empty()) {
						//	set online state
						tbl_gw->modify(cyng::table::key_generator(rec["device"]), cyng::param_factory("online", 0), origin);
					}
					else {
						auto const rtag = cyng::value_cast(rec["rtag"], boost::uuids::nil_uuid());
						auto const state = rtag.is_nil() ? 1 : 2;
						tbl_gw->modify(cyng::table::key_generator(rec["device"]), cyng::param_factory("online", state), origin);

						//
						//	update TMeter
						//	Lookup if TMeter has a gateway with this key
						//	This method is inherently slow.
						//	ToDo: optimize
						//

						std::map<cyng::table::key_type, int>	result;
						auto const gw_tag = cyng::value_cast(rec["device"], boost::uuids::nil_uuid());
						tbl_meter->loop([&](cyng::table::record const& rec) -> bool {

							auto const tag = cyng::value_cast(rec["gw"], boost::uuids::nil_uuid());
							if (tag == gw_tag) {

								//
								//	The gateway of this meter is online/connected
								//
								result.emplace(rec.key(), state);
							}
							return true;	//	continue
						});

						//
						//	Update all found meters
						//
						for (auto const& item : result) {
							tbl_meter->modify(item.first, cyng::param_factory("online", item.second), origin);
						}

					}
				}	, cyng::store::write_access("TGateway")
					, cyng::store::write_access("TMeter")
					, cyng::store::read_access("_Session"));
			}
		}
	}

	void db_req_remove(cyng::logging::log_ptr logger
		, cyng::store::db& db
		, std::string const& table		//	[0] table name
		, cyng::table::key_type	key		//	[1] table key
		, boost::uuids::uuid origin)
	{
		//
		//	we have to query session data before session will be removed
		//
		if (boost::algorithm::equals(table, "_Session"))
		{
			db.access([&](cyng::store::table* tbl_gw, cyng::store::table* tbl_meter, const cyng::store::table* tbl_ses) {

				//
				//	Gateway and Device table share the same table key
				//
				auto rec = tbl_ses->lookup(key);
				if (!rec.empty())
				{
					//	set online state
					auto const gw_tag = cyng::value_cast(rec["device"], boost::uuids::nil_uuid());
					tbl_gw->modify(cyng::table::key_generator(gw_tag), cyng::param_factory("online", 0), origin);

					//
					//	find meters of this gateway and set offline
					//
					std::set<cyng::table::key_type>	result;
					tbl_meter->loop([&](cyng::table::record const& rec) -> bool {

						auto const tag = cyng::value_cast(rec["gw"], boost::uuids::nil_uuid());
						if (tag == gw_tag) {

							//
							//	The gateway of this meter is offline
							//
							result.emplace(rec.key());
						}

						return true;	//	continue
					});

					//
					//	Update all found meters
					//
					for (auto const& item : result) {
						tbl_meter->modify(item, cyng::param_factory("online", 0), origin);
					}

				}
			}	, cyng::store::write_access("TGateway")
				, cyng::store::write_access("TMeter")
				, cyng::store::read_access("_Session"));
		}

		//
		//	remove record
		//
		if (!db.erase(table, key, origin))
		{
			CYNG_LOG_WARNING(logger, "db.req.remove failed "
				<< table		// table name
				<< " - "
				<< cyng::io::to_str(key));

		}
	}

	void db_res_remove(cyng::logging::log_ptr logger
		, cyng::store::db& db
		, std::string const& table		//	[0] table name
		, cyng::table::key_type	key		//	[1] table key
		, boost::uuids::uuid origin)
	{
		if (!db.erase(table, key, origin))
		{
			CYNG_LOG_WARNING(logger, "db.res.remove failed "
				<< table		// table name
				<< " - "
				<< cyng::io::to_str(key));
		}
	}

	void db_res_modify_by_attr(cyng::logging::log_ptr logger
		, cyng::store::db& db
		, std::string const& table		//	[0] table name
		, cyng::table::key_type key		//	[1] table key
		, cyng::attr_t attr		//	[2] attribute
		, boost::uuids::uuid origin)
	{
		//
		//	distribute some changes to other tables too
		//
		if (boost::algorithm::equals(table, "TDevice")) {
			switch (attr.first) {
			case 0:
				//	change name
				db.modify("TGateway", key, cyng::param_factory("name", attr.second), origin);
				break;
			case 3:
				//	change description
				db.modify("TGateway", key, cyng::param_factory("descr", attr.second), origin);
				break;
			default:
				break;
			}
		}
		else if (boost::algorithm::equals(table, "TGateway")) {
			//
			//	distribute changes from TGateway to TMeter
			//
			switch (attr.first) {
			case 0:
				//	modified serverID 
				db.access([&](cyng::store::table* tbl_meter) {
					auto const gw_tag = cyng::value_cast(key.at(0), boost::uuids::nil_uuid());
					std::set<cyng::table::key_type> result;
					tbl_meter->loop([&](cyng::table::record const& rec) -> bool {

						auto const tag = cyng::value_cast(rec["gw"], boost::uuids::nil_uuid());
						if (tag == gw_tag) {
							result.insert(rec.key());
						}
						return true;	//	continue
					});
					for (auto const& item : result) {
						tbl_meter->modify(item, cyng::param_factory("serverId", attr.second), origin);
					}
				}, cyng::store::write_access("TMeter"));
				break;
			default:
				break;
			}
		}

		//
		//	update cache
		//
		if (!db.modify(table, key, std::move(attr), origin))
		{
			CYNG_LOG_WARNING(logger, "db.res.modify.by.attr failed "
				<< table		// table name
				<< " - "
				<< cyng::io::to_str(key));
		}
	}

	void db_res_modify_by_param(cyng::logging::log_ptr logger
		, cyng::store::db& db
		, std::string const& table		//	[0] table name
		, cyng::table::key_type key		//	[1] table key
		, cyng::param_t	param			//	[2] parameter
		, boost::uuids::uuid origin)
	{
		if (!db.modify(table, key, param, origin))
		{
			CYNG_LOG_WARNING(logger, "db.res.modify.by.param failed "
				<< table		// table name
				<< " - "
				<< cyng::io::to_str(key));
		}
	}

	void db_req_modify_by_param(cyng::logging::log_ptr logger
		, cyng::store::db& db
		, std::string const& table	//	[0] table name
		, cyng::table::key_type	key //	[1] table key
		, cyng::param_t param		//	[2] parameter
		, std::uint64_t	gen			//	[3] generation
		, boost::uuids::uuid origin)
	{
		if (boost::algorithm::equals(table, "_Session")) {
			if (boost::algorithm::equals(param.first, "rtag")) {

				const auto rtag = cyng::value_cast(param.second, boost::uuids::nil_uuid());

				//
				//	mark gateways as online
				//
				db.access([&](cyng::store::table* tbl_gw, const cyng::store::table* tbl_ses) {

					//
					//	[*Session,[2ce46726-6bca-44b6-84ed-0efccb67774f],[00000000-0000-0000-0000-000000000000,2018-03-12 17:56:27.10338240,f51f2ae7,data-store,eaec7649-80d5-4b71-8450-3ee2c7ef4917,94aa40f9-70e8-4c13-987e-3ed542ecf7ab,null,session],1]
					//	Gateway and Device table share the same table key
					//
					auto rec = tbl_ses->lookup(key);
					if (rec.empty()) {
						//	set online state
						tbl_gw->modify(cyng::table::key_generator(rec["device"]), cyng::param_factory("online", 0), origin);
					}
					else {
						tbl_gw->modify(cyng::table::key_generator(rec["device"]), cyng::param_factory("online", rtag.is_nil() ? 1 : 2), origin);
					}
				}	, cyng::store::write_access("TGateway")
					, cyng::store::read_access("_Session"));
			}
		}
		if (!db.modify(table, key, param, origin))
		{
			CYNG_LOG_WARNING(logger, "db.req.modify.by.param failed "
				<< table		// table name
				<< " - "
				<< cyng::io::to_str(key));
		}

	}

}
