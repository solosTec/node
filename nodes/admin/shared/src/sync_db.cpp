/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#include "sync_db.h"
#include "tables.h"
#include <smf/shared/db_schemes.h>
#include <smf/cluster/generator.h>

#include <cyng/table/meta.hpp>
#include <cyng/io/serializer.h>
#include <cyng/io/io_chrono.hpp>
#include <cyng/tuple_cast.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/uuid/nil_generator.hpp>

namespace node 
{
	void create_cache(cyng::logging::log_ptr logger
		, cyng::store::db& db)
	{
		CYNG_LOG_TRACE(logger, "create "
			<< tables::list_.size()
			<< " cache tables");

		for (auto const& tbl : tables::list_) {
			if (!tbl.custom_) {
				if (!create_table(db, tbl.name_, tbl.pass_trough_)) {
					CYNG_LOG_FATAL(logger, "cannot create table: " << tbl.name_);
				}
				else {
					CYNG_LOG_DEBUG(logger, "create table: " << tbl.name_);
				}
			}
		}

		//
		//	Has more columns than original TGateway definition
		//
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
			//	since here columns specific for dash/s and not part of the original table scheme
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
			, cyng::TC_INT32	//	on/offline state
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
			, 0		//	online/offline state
			}), false))
		{
			CYNG_LOG_FATAL(logger, "cannot create table TGateway");
		}

		if (!db.create_table(cyng::table::make_meta_table<1, 16>("TMeter", { "pk"
				, "ident"		//	ident nummer (i.e. 1EMH0006441734, 01-e61e-13090016-3c-07)
				, "meter"		//	meter number (i.e. 16000913) 4 bytes 
				, "code"		//	metering code - changed at 2019-01-31
				, "maker"		//	manufacturer
				, "tom"			//	time of manufacture
				, "vFirmware"	//	firmwareversion (i.e. 11600000)
				, "vParam"		//	parametrierversion (i.e. 16A098828.pse)
				, "factoryNr"	//	fabrik nummer (i.e. 06441734)
				, "item"		//	ArtikeltypBezeichnung = "NXT4-S20EW-6N00-4000-5020-E50/Q"
				, "mClass"		//	Metrological Class: A, B, C, Q3/Q1, ...
				, "gw"			//	optional gateway pk
				, "protocol"	//	[string] data protocol (IEC, M-Bus, COSEM, ...)
				//	-- additional columns
				, "serverId"	//	optional gateway server ID
				, "online"		//	gateway online state (1,2,3)
				, "region"		//	general location
				, "address"		//	street, city

			},
			{ cyng::TC_UUID
			, cyng::TC_STRING		//	ident
			, cyng::TC_STRING		//	meter
			, cyng::TC_STRING		//	code
			, cyng::TC_STRING		//	maker
			, cyng::TC_TIME_POINT	//	tom
			, cyng::TC_STRING		//	vFirmware
			, cyng::TC_STRING		//	vParam
			, cyng::TC_STRING		//	factoryNr
			, cyng::TC_STRING		//	item
			, cyng::TC_STRING		//	mClass
			, cyng::TC_UUID			//	gw
			, cyng::TC_STRING		//	protocol
			, cyng::TC_STRING		//	serverID
			, cyng::TC_INT32		//	on/offline state
			, cyng::TC_STRING		//	region
			, cyng::TC_STRING		//	address
			},
			{ 36
			, 24	//	ident
			, 8		//	meter
			, 33	//	code - country[2], ident[11], number[22]
			, 64	//	maker
			, 0		//	tom
			, 64	//	vFirmware
			, 64	//	vParam
			, 32	//	factoryNr
			, 128	//	item
			, 8		//	mClass 
			, 36	//	gw
			, 32	//	protocol
			, 23 	//	serverId
			, 0		//	on/offline state
			, 32	//	region
			, 64	//	address
			}), false))
		{
			CYNG_LOG_FATAL(logger, "cannot create table TMeter");
		}


		//
		//	TBridge is supplementary table to TMeter.
		//
		if (!db.create_table(cyng::table::make_meta_table<1, 6>("TBridge",
			{ "pk"		//	same key as in TMeter table
			, "address"		//	[ip address] incoming/outgoing IP connection
			, "port"		//	[u16] port
			, "direction"	//	[bool] incoming/outgoing
			, "interval"	//	[seconds] pull cycle
			//	-- additional columns
			, "meter"
			, "protocol"
			},
			{ cyng::TC_UUID			//	pk
			, cyng::TC_IP_ADDRESS	//	host address
			, cyng::TC_UINT16		//	TCP/IP port
			, cyng::TC_BOOL
			, cyng::TC_SECOND
			, cyng::TC_STRING
			, cyng::TC_STRING
			},
			{ 36
			, 0	//	address
			, 0	//	port
			, 0	//	direction
			, 0
			, 8
			, 32
			}), false))
		{
			CYNG_LOG_FATAL(logger, "cannot create table TBridge");
		}

		//
		//	OUI contains the registered organisation names for MACs
		//
		if (!db.create_table(cyng::table::make_meta_table<1, 1>("_OUI",
			{ "oui"		//	same key as in TMeter table
			, "org"		//	[ip address] incoming/outgoing IP connection
			},
			{ cyng::TC_BUFFER	//	oui
			, cyng::TC_STRING	//	organisation
			},
			{ 3
			, 128	//	organisation
			}), false))
		{
			CYNG_LOG_FATAL(logger, "cannot create table _OUI");
		}

		//
		//	all tables created
		//
		CYNG_LOG_INFO(logger, db.size() << " tables created");
	}

	void clear_cache(cyng::store::db& db, boost::uuids::uuid tag)
	{
		for (auto const& tbl : tables::list_) {
			if (!tbl.local_) {
				db.clear(tbl.name_, tag);
			}
		}
		db.insert("_Config", cyng::table::key_generator("cpu:load"), cyng::table::data_generator(0.0), 0, tag);
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
		if (key.empty()) {
			CYNG_LOG_FATAL(logger, "res_subscribe("
				<< table
				<< ") has no key");
			return;
		}

		//
		//	Boost gateway records with additional data from TDevice and _Session table
		//
		if (boost::algorithm::equals(table, "TGateway"))
		{
			if (!complete_data_gw(db, key, data)) {

				CYNG_LOG_WARNING(logger, "gateway"
					<< cyng::io::to_str(key)
					<< " has no associated device");
			}
		}
		
		//
		//	Boost meter records with additional data from TGateway and TLocation
		//
		else if (boost::algorithm::equals(table, "TMeter"))
		{
			if (!complete_data_meter(db, key, data)) {

				CYNG_LOG_WARNING(logger, "meter"
					<< cyng::io::to_str(key)
					<< " has no associated gateway");
			}
		}

		//
		//	Boost IEC records with additional data from TMeter
		//
		else if (boost::algorithm::equals(table, "TBridge"))
		{
//#ifdef _DEBUG
			//
			//	hunting a bug on linux: IP address is NULL
			//
			//CYNG_LOG_DEBUG(logger, "Bridge meter "
			//	<< cyng::io::to_str(key)
			//	<< " - "
			//	<< cyng::io::to_str(data));

//#endif

			if (!complete_data_bridge(db, key, data)) {

				CYNG_LOG_WARNING(logger, "IEC device "
					<< cyng::io::to_str(key)
					<< " has no associated meter");
			}
		}

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
		else
		{
			if (boost::algorithm::equals(table, "_Session"))
			{
				//
				//	mark gateways as online
				//
				update_data_online_status(db, key, data, origin);
			}
			else if (boost::algorithm::equals(table, "TLocation"))
			{
				update_data_meter(db, key, data, origin);
			}
		}
	}

	db_sync::db_sync(cyng::logging::log_ptr logger
		, cyng::store::db& db)
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

		auto const tpl = cyng::tuple_cast<
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
			if (!complete_data_gw(db_, key, data)) {

				CYNG_LOG_WARNING(logger_, "gateway"
					<< cyng::io::to_str(key)
					<< " has no associated device");
			}
		}

		else if (boost::algorithm::equals(table, "TMeter"))
		{
			if (!complete_data_meter(db_, key, data)) {

				CYNG_LOG_WARNING(logger_, "meter"
					<< cyng::io::to_str(key)
					<< " has no associated gateway");
			}
		}
		else if (boost::algorithm::equals(table, "TBridge"))
		{
			if (!complete_data_bridge(db_, key, data)) {

				CYNG_LOG_WARNING(logger_, "bridged meter "
					<< cyng::io::to_str(key)
					<< " has no associated meter");
			}
		}

		//
		//	insert
		//
		if (!db_.insert(table
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
			//
			//	update online state of meters and gateways
			//
			if (boost::algorithm::equals(table, "_Session"))
			{
				update_data_online_status(db_, key, data, origin);
			}
			else if (boost::algorithm::equals(table, "TLocation"))
			{
				update_data_meter(db_, key, data, origin);
			}
		}
	}

	void update_data_online_status(cyng::store::db& db
		, cyng::table::key_type const& key
		, cyng::table::data_type& data
		, boost::uuids::uuid origin)
	{
		db.access([&](cyng::store::table* tbl_gw, cyng::store::table* tbl_meter, const cyng::store::table* tbl_ses) {

			//
			//	[*Session,[2ce46726-6bca-44b6-84ed-0efccb67774f],[00000000-0000-0000-0000-000000000000,2018-03-12 17:56:27.10338240,f51f2ae7,data-store,eaec7649-80d5-4b71-8450-3ee2c7ef4917,94aa40f9-70e8-4c13-987e-3ed542ecf7ab,null,session],1]
			//	Gateway and Device table share the same table key
			//
			auto rec = tbl_ses->lookup(key);
			if (rec.empty()) {
				//	set online state
				tbl_gw->modify(cyng::table::key_generator(rec["device"]), cyng::param_factory("online", 0), origin);
				tbl_meter->modify(cyng::table::key_generator(rec["device"]), cyng::param_factory("online", 0), origin);
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
				auto const meters = collect_meter_of_gw(tbl_meter, gw_tag);

				//
				//	Update all found meters
				//
				for (auto const& item : meters) {
					tbl_meter->modify(item, cyng::param_factory("online", state), origin);
				}

			}
		}	, cyng::store::write_access("TGateway")
			, cyng::store::write_access("TMeter")
			, cyng::store::read_access("_Session"));

	}

	void update_data_meter(cyng::store::db& db
		, cyng::table::key_type const& key
		, cyng::table::data_type& data
		, boost::uuids::uuid origin)
	{
		//
		//	Update values of TMeter
		//
		db.access([&](cyng::store::table* tbl_meter) {
			if (tbl_meter->exist(key)) {

				tbl_meter->modify(key, cyng::param_t("region", data.at(2)), origin);
				tbl_meter->modify(key, cyng::param_t("address", data.at(3)), origin);
			}
		}, cyng::store::write_access("TMeter"));

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

		node::res_subscribe(logger_
			, db_
			, std::get<0>(tpl)	//	[0] table name
			, std::get<1>(tpl)	//	[1] table key
			, std::get<2>(tpl)	//	[2] record
			, std::get<3>(tpl)	//	[3] generation
			, std::get<4>(tpl)	//	[4] origin session id
			, std::get<5>(tpl));
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
			case 4:
				//	change "id"
				db.modify("TGateway", key, cyng::param_factory("model", attr.second), origin);
				break;
			case 5:
				//	change "vFirmware"
				db.modify("TGateway", key, cyng::param_factory("vFirmware", attr.second), origin);
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

			BOOST_ASSERT(key.size() == 1);

			if (boost::algorithm::equals(param.first, "rtag")) {

				auto const rtag = cyng::value_cast(param.second, boost::uuids::nil_uuid());

				//
				//	mark gateways as online
				//
				db.access([&](cyng::store::table* tbl_gw, cyng::store::table* tbl_meter, const cyng::store::table* tbl_ses) {

					//
					//	Gateway and Device table share the same table key.
					//	If the remote key (rtag) is the same as the session tag, then there is an internal connection 
					//	established (e.g. with a task).
					//
					auto rec = tbl_ses->lookup(key);
					auto const tag = cyng::value_cast(key.at(0), boost::uuids::nil_uuid());	//	session tag
					std::uint32_t const online_state = (rec.empty())
						? 0
						: (rtag.is_nil() ? 1 
							: ((rtag == tag) ? 3 : 2))
						;
					tbl_gw->modify(cyng::table::key_generator(rec["device"]), cyng::param_factory("online", online_state), origin);

					//
					//	update TMeter
					//	Lookup if TMeter has a gateway with this key
					//	This method is inherently slow.
					//	ToDo: optimize
					//
					auto const gw_tag = cyng::value_cast(rec["device"], boost::uuids::nil_uuid());
					auto const meters = collect_meter_of_gw(tbl_meter, gw_tag);
					//
					//	Update all found meters
					//
					for (auto const& item : meters) {
						tbl_meter->modify(item, cyng::param_factory("online", online_state), origin);
					}
				}	, cyng::store::write_access("TGateway")
					, cyng::store::write_access("TMeter")
					, cyng::store::read_access("_Session"));
			}
		}
		else if (boost::algorithm::equals(table, "TDevice")) {
			BOOST_ASSERT(key.size() == 1);
			if (boost::algorithm::equals(param.first, "id")) {
				db.modify("TGateway", key, cyng::param_factory("model", param.second), origin);
			}
			else if (boost::algorithm::equals(param.first, "vFirmware")) {
				db.modify("TGateway", key, cyng::param_factory("vFirmware", param.second), origin);
			}
		}
		else if (boost::algorithm::equals(table, "TLocation")) {
			BOOST_ASSERT(key.size() == 1);
			if (boost::algorithm::equals(param.first, "region")) {
				db.modify("TMeter", key, cyng::param_factory(param.first, param.second), origin);
			}
			else if (boost::algorithm::equals(param.first, "address")) {
				db.modify("TMeter", key, cyng::param_factory(param.first, param.second), origin);
			}
		}

		//
		//	generic
		//
		if (!db.modify(table, key, param, origin))
		{
			CYNG_LOG_WARNING(logger, "db.req.modify.by.param failed "
				<< table		// table name
				<< " - "
				<< cyng::io::to_str(key)
				<< " => "
				<< param.first
				<< " := "
				<< cyng::io::to_str(param.second));
		}
	}

	cyng::table::key_list_t collect_meter_of_gw(cyng::store::table const* tbl_meter, boost::uuids::uuid gw_tag)
	{
		cyng::table::key_list_t result;
		tbl_meter->loop([&](cyng::table::record const& rec) -> bool {

			auto const tag = cyng::value_cast(rec["gw"], boost::uuids::nil_uuid());
			if (tag == gw_tag) {

				//
				//	The gateway of this meter is online/connected
				//
				result.push_back(rec.key());
			}
			return true;	//	continue
		});
		return result;
	}

	bool complete_data_bridge(cyng::store::db& db
		, cyng::table::key_type const& key
		, cyng::table::data_type& data)
	{
		bool rc{ false };

		//
		//	Additional values for TBridge from TMeter
		//
		db.access([&](const cyng::store::table* tbl_meter) {
				
			//
			//	TBridge shares the primary key with TMeter table
			//
			auto rec = tbl_meter->lookup(key);
				
			//
			//	set meter ID
			//
			if (!rec.empty())
			{
				data.push_back(rec["meter"]);
				data.push_back(rec["protocol"]);
				rc = true;
			}
			else
			{
				data.push_back(cyng::make_object("00000000"));
				data.push_back(cyng::make_object("any"));
			}
		}, cyng::store::read_access("TMeter"));

		return rc;
	}

	bool complete_data_meter(cyng::store::db& db
		, cyng::table::key_type const& key
		, cyng::table::data_type& data)
	{
		bool rc{ false };

		//
		//	Additional values for TMeter
		//
		db.access([&](cyng::store::table const* tbl_gw, cyng::store::table const* tbl_loc) {

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
				rc = true;
			}
			else
			{
				data.push_back(cyng::make_object("00000000000000"));
				data.push_back(cyng::make_object(-1));
			}

			//
			//	get data from TLocation
			//
			auto const rec = tbl_loc->lookup(key); 
			if (!rec.empty()) {
				data.push_back(rec["region"]);
				data.push_back(rec["address"]);
			}
			else {
				// "region"		- general location
				// "address"	- street, city
				data.push_back(cyng::make_object(""));
				data.push_back(cyng::make_object(""));
			}

		}	, cyng::store::read_access("TGateway")
			, cyng::store::read_access("TLocation"));

		return rc;
	}

	bool complete_data_gw(cyng::store::db& db
		, cyng::table::key_type const& key
		, cyng::table::data_type& data)
	{
		bool rc{ false };

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

				rc = true;
			}

		}	, cyng::store::read_access("TDevice")
			, cyng::store::read_access("_Session"));

		//
		//	lookup mac manufacturer (0500153B02517E)
		//
		//auto const srv = cyng::value_cast<std::string>(data.at(0), "");
		//auto const m = sml::to_mac48(srv);
		//std::string org = lookup_org(m);

		return rc;
	}


}
