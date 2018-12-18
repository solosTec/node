/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#include "sync_db.h"
#include "../../shared/db/db_schemes.h"
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

		if (!create_table_device(db)) {
			CYNG_LOG_FATAL(logger, "cannot create table TDevice");
		}

		if (!db.create_table(cyng::table::make_meta_table<1, 16>("TGateway", { "pk"	//	primary key
			, "serverId"	//	(1) Server-ID (i.e. 0500153B02517E)
			, "manufacturer"	//	(2) manufacturer (i.e. EMH)
			, "made"	//	(4) production date
			, "factoryNr"	//	(6) fabrik nummer (i.e. 06441734)
			, "ifService"	//	(7) MAC of service interface
			, "ifData"	//	(8) MAC od data interface
			, "pwdDef"	//	(9) Default PW
			, "pwdRoot"	//	(10) root PW
			, "mbus"	//	(11) W-Mbus ID (i.e. A815408943050131)
			, "userName"	//	(12)
			, "userPwd"	//	(13)
			, "name"	//	IP-T/device name
			, "descr"	//	IP-T/device description
			, "model"	//	(3) Typbezeichnung (i.e. Variomuc ETHERNET)
			, "vFirmware"	//	(5) firmware version (i.e. 11600000)
			, "online"	//	(14)
			},
			{ cyng::TC_UUID		//	pk
			, cyng::TC_STRING	//	server id
			, cyng::TC_STRING	//	manufacturer
			, cyng::TC_TIME_POINT	//	production data
			, cyng::TC_STRING	//	Fabriknummer/serial number (i.e. 06441734)
			, cyng::TC_MAC48	//	MAC of service interface
			, cyng::TC_MAC48	//	MAC od data interface
			, cyng::TC_STRING	//	Default PW
			, cyng::TC_STRING	//	root PW
			, cyng::TC_STRING	//	W-Mbus ID (i.e. A815408943050131)
			, cyng::TC_STRING	//	operator
			, cyng::TC_STRING	//	operator
								//	--- dynamic part:
			, cyng::TC_STRING	//	IP-T/device name
			, cyng::TC_STRING	//	model
			, cyng::TC_STRING	//	vFirmware
			, cyng::TC_BOOL		//	on/offline
			},
			{ 36	//	pk
			, 23	//	server id
			, 64	//	manufacturer
			, 0		//	production date
			, 8		//	serial
			, 18	//	MAC
			, 18	//	MAC
			, 32	//	PW
			, 32	//	PW
			, 16	//	M-Bus
			, 32	//	PW
			, 32	//	PW
			, 128	//	IP-T/device name
			, 64	//	model
			, 64	//	vFirmware
			, 0		//	bool online/offline
			})))
		{
			CYNG_LOG_FATAL(logger, "cannot create table TGateway");
		}

		//	https://www.thethingsnetwork.org/docs/lorawan/address-space.html#devices
		//	DevEUI - 64 bit end-device identifier, EUI-64 (unique)
		//	DevAddr - 32 bit device address (non-unique)
		if (!create_table_lora_device(db))
		{
			CYNG_LOG_FATAL(logger, "cannot create table TLoRaDevice");
		}

// 		if (!create_table_meter(db))
// 		{
// 			CYNG_LOG_FATAL(logger, "cannot create table TMeter");
// 		}
		if (!db.create_table(cyng::table::make_meta_table<1, 11>("TMeter", { "pk"
			, "ident"		//	ident nummer (i.e. 1EMH0006441734, 01-e61e-13090016-3c-07)
			, "meter"		//	meter number (i.e. 16000913) 4 bytes 
			, "maker"		//	manufacturer
			, "tom"			//	time of manufacture
			, "vFirmare"	//	firmwareversion (i.e. 11600000)
			, "vParam"		//	parametrierversion (i.e. 16A098828.pse)
			, "factoryNr"	//	fabrik nummer (i.e. 06441734)
			, "item"		//	ArtikeltypBezeichnung = "NXT4-S20EW-6N00-4000-5020-E50/Q"
			, "mClass"		//	Metrological Class: A, B, C, Q3/Q1, ...
			, "gw"			//	optional gateway pk
			, "serverId"	//	optional gateway server ID
			},
			{ cyng::TC_UUID
			, cyng::TC_STRING		//	ident
			, cyng::TC_STRING		//	meter
			, cyng::TC_STRING		//	maker
			, cyng::TC_TIME_POINT	//	tom
			, cyng::TC_STRING		//	vFirmare
			, cyng::TC_STRING		//	vParam
			, cyng::TC_STRING		//	factoryNr
			, cyng::TC_STRING		//	item
			, cyng::TC_STRING		//	mClass
			, cyng::TC_UUID			//	gw
			, cyng::TC_STRING		//	serverID
			},
			{ 36
			, 24	//	ident
			, 8		//	meter
			, 64	//	maker
			, 0		//	tom
			, 64	//	vFirmare
			, 64	//	vParam
			, 32	//	factoryNr
			, 128	//	item
			, 8		//	mClass 
			, 36	//	gw
			, 23 	//	serverId
		})))
		{
			CYNG_LOG_FATAL(logger, "cannot create table TMeter");
		}
		
		if (!create_table_session(db))
		{
			CYNG_LOG_FATAL(logger, "cannot create table _Session");
		}

		if (!create_table_target(db))
		{
			CYNG_LOG_FATAL(logger, "cannot create table _Target");
		}

		if (!create_table_connection(db))
		{
			CYNG_LOG_FATAL(logger, "cannot create table _Connection");
		}

		if (!create_table_cluster(db))
		{
			CYNG_LOG_FATAL(logger, "cannot create table _Cluster");
		}

		if (!create_table_config(db))
		{
			CYNG_LOG_FATAL(logger, "cannot create table _Config");
		}
		else
		{
			//
			//	set initial value
			//
			//db.insert("_Config", cyng::table::key_generator("cpu:load"), cyng::table::data_generator(0.0), 0, bus_->vm_.tag());
		}

		if (!create_table_sys_msg(db))
		{
			CYNG_LOG_FATAL(logger, "cannot create table *SysMsg");
		}

		if (!create_table_csv(db))
		{
			CYNG_LOG_FATAL(logger, "cannot create table _CSV");
		}

		//
		//	all tables created
		//
		CYNG_LOG_INFO(logger, db.size() << " tables created");
	}

	void clear_cache(cyng::store::db& db, boost::uuids::uuid tag)
	{

		db.clear("TDevice", tag);
		db.clear("TGateway", tag);
		db.clear("TMeter", tag);
		db.clear("_Session", tag);
		db.clear("_Target", tag);
		db.clear("_Connection", tag);
		db.clear("_Cluster", tag);
		db.clear("_Config", tag);
		db.insert("_Config", cyng::table::key_generator("cpu:load"), cyng::table::data_generator(0.0), 0, tag);
		//cache_.clear("_SysMsg", bus_->vm_.tag());
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
		//	Boost gateway records with additional data from TDevice and _Session table
		//
		if (boost::algorithm::equals(table, "TGateway"))
		{
			//
			//	Additional values for TGateway
			//
			db.access([&](const cyng::store::table* tbl_dev, const cyng::store::table* tbl_ses) {

				//
				//	Gateway and Device table share the same table key
				//	look for a session of this device
				//
				auto dev_rec = tbl_dev->lookup(key);
				auto ses_rec = tbl_ses->find_first(cyng::param_t("device", key.at(0)));

				//
				//	set device name
				//	set model
				//	set firmware
				//	set online state
				//
				if (!dev_rec.empty())
				{
					data.push_back(dev_rec["name"]);
					data.push_back(dev_rec["descr"]);
					data.push_back(dev_rec["id"]);
					data.push_back(dev_rec["vFirmware"]);
					if (ses_rec.empty()) {
						data.push_back(cyng::make_object(0));
					}
					else {
						const auto peer = cyng::value_cast(ses_rec["rtag"], boost::uuids::nil_uuid());
						data.push_back(cyng::make_object(peer.is_nil() ? 1 : 2));
					}
				}
				else
				{
					CYNG_LOG_WARNING(logger, "res.subscribe - gateway"
						<< cyng::io::to_str(key)
						<< " has no associated device");
				}
			}	, cyng::store::read_access("TDevice")
				, cyng::store::read_access("_Session"));
		}
		
		//
		//	Boost meter records with additional data from TGateway
		//
		if (boost::algorithm::equals(table, "TMeter"))
		{
			//
			//	Additional values for TMeter
			//
			db.access([&](const cyng::store::table* tbl_gw) {
				
				//
				//	TMeter contains an optional reference to TGateway table
				//
				auto key = cyng::table::key_generator(data.at(9));
				auto dev_gw = tbl_gw->lookup(key);
				
				//
				//	set serverId
				//
				if (!dev_gw.empty())
				{
					data.push_back(dev_gw["serverId"]);
				}
				else
				{
					data.push_back(cyng::make_object("05000000000000"));
					
					CYNG_LOG_WARNING(logger, "res.subscribe - meter"
					<< cyng::io::to_str(key)
					<< " has no associated gateway");
				}
			}, cyng::store::read_access("TGateway"));
		}

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
		else
		{
			if (boost::algorithm::equals(table, "_Session"))
			{
				//
				//	mark gateways as online
				//
				db.access([&](cyng::store::table* tbl_gw, const cyng::store::table* tbl_ses) {

					//
					//	[*Session,[2ce46726-6bca-44b6-84ed-0efccb67774f],[00000000-0000-0000-0000-000000000000,2018-03-12 17:56:27.10338240,f51f2ae7,data-store,eaec7649-80d5-4b71-8450-3ee2c7ef4917,94aa40f9-70e8-4c13-987e-3ed542ecf7ab,null,session],1]
					//	Gateway and Device table share the same table key
					//
					auto rec = tbl_ses->lookup(key);
					if (rec.empty())	{
						//	set online state
						tbl_gw->modify(cyng::table::key_generator(rec["device"]), cyng::param_factory("online", 0), origin);
					}
					else {
						const auto rtag = cyng::value_cast(rec["rtag"], boost::uuids::nil_uuid());
						tbl_gw->modify(cyng::table::key_generator(rec["device"]), cyng::param_factory("online", rtag.is_nil() ? 1 : 2), origin);
					}
				}	, cyng::store::write_access("TGateway")
					, cyng::store::read_access("_Session"));
			}
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

		node::db_res_insert(logger_
			, db_
			, std::get<0>(tpl)		//	[0] table name
			, std::get<1>(tpl)		//	[1] table key
			, std::get<2>(tpl)		//	[2] record
			, std::get<3>(tpl)		//	[3] generation
			, ctx.tag());

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

		node::db_req_insert(logger_
			, db_
			, std::get<0>(tpl)		//	[0] table name
			, std::get<1>(tpl)		//	[1] table key
			, std::get<2>(tpl)		//	[2] record
			, std::get<3>(tpl)		//	[3] generation
			, std::get<4>(tpl));
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


	void db_res_insert(cyng::logging::log_ptr logger
		, cyng::store::db& db
		, std::string const& table		//	[0] table name
		, cyng::table::key_type key		//	[1] table key
		, cyng::table::data_type data	//	[2] record
		, std::uint64_t	gen
		, boost::uuids::uuid origin)
	{
		cyng::table::record rec(db.meta(table), key, data, gen);

		if (!db.insert(table
			, key
			, data
			, gen
			, origin))	//	self
		{
			CYNG_LOG_WARNING(logger, "db.res.insert failed "
				<< table		// table name
				<< " - "
				<< cyng::io::to_str(key));
		}
	}

	void db_req_insert(cyng::logging::log_ptr logger
		, cyng::store::db& db
		, std::string const& table		//	[0] table name
		, cyng::table::key_type key		//	[1] table key
		, cyng::table::data_type data	//	[2] record
		, std::uint64_t	gen
		, boost::uuids::uuid origin)
	{
		//cyng::table::record rec(cache_.meta(table), key, data, gen);

		if (boost::algorithm::equals(table, "TGateway"))
		{
			db.access([&](cyng::store::table* tbl_gw, const cyng::store::table* tbl_dev, const cyng::store::table* tbl_ses) {

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

						CYNG_LOG_WARNING(logger, "db.req.insert failed "
							<< table		// table name
							<< " - "
							<< cyng::io::to_str(key)
							<< " => "
							<< cyng::io::to_str(data));

					}
				}
				else {
					CYNG_LOG_WARNING(logger, "gateway "
						<< cyng::io::to_str(key)
						<< " has no associated device");
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
			db.access([&](cyng::store::table* tbl_meter, cyng::store::table const* tbl_gw) {

				//
				//	TMeter contains an optional reference to TGateway table
				//
				auto key = cyng::table::key_generator(data.at(9));
				auto dev_gw = tbl_gw->lookup(key);

				//
				//	set serverId
				//
				if (!dev_gw.empty())
				{
					data.push_back(dev_gw["serverId"]);
				}
				else
				{
					data.push_back(cyng::make_object("00000000000000"));

					CYNG_LOG_WARNING(logger, "res.subscribe - meter"
						<< cyng::io::to_str(key)
						<< " has no associated gateway");
				}

				if (!tbl_meter->insert(key, data, gen, origin))
				{

					CYNG_LOG_WARNING(logger, "db.req.insert failed "
						<< table		// table name
						<< " - "
						<< cyng::io::to_str(key)
						<< " => "
						<< cyng::io::to_str(data));

				}

			}	, cyng::store::write_access("TMeter")
				, cyng::store::read_access("TGateway"));

		}
		else if (!db.insert(table
			, key
			, data
			, gen
			, origin))
		{
			CYNG_LOG_WARNING(logger, "db.req.insert failed "
				<< table		// table name
				<< " - "
				<< cyng::io::to_str(key)
				<< " => "
				<< cyng::io::to_str(data));
		}
		else
		{
			if (boost::algorithm::equals(table, "_Session"))
			{
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
						const auto rtag = cyng::value_cast(rec["rtag"], boost::uuids::nil_uuid());
						tbl_gw->modify(cyng::table::key_generator(rec["device"]), cyng::param_factory("online", rtag.is_nil() ? 1 : 2), origin);
					}
				}	, cyng::store::write_access("TGateway")
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
			db.access([&](cyng::store::table* tbl_gw, const cyng::store::table* tbl_ses) {

				//
				//	Gateway and Device table share the same table key
				//
				auto rec = tbl_ses->lookup(key);
				if (!rec.empty())
				{
					//	set online state
					tbl_gw->modify(cyng::table::key_generator(rec["device"]), cyng::param_factory("online", 0), origin);
				}
			}	, cyng::store::write_access("TGateway")
				, cyng::store::read_access("_Session"));
		}

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
			//	ToDo: distribute modified serverID to TMeter
			//
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
