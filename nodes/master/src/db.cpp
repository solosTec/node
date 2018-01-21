/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 


#include "db.h"
#include <cyng/table/meta.hpp>
#include <cyng/intrinsics/traits/tag.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace node 
{
	void init(cyng::logging::log_ptr logger, cyng::store::db& db, boost::uuids::uuid tag)
	{
		CYNG_LOG_INFO(logger, "initialize database as node " << tag);

		//
		//	TDevice table
		//
		//	(1) tag (UUID) - pk
		//	+-----------------
		//	(2) name [std::string]
		//	(3) number [std::string]
		//	(4) description [std::string]
		//	(5) identifier [std::string] (device identifier/type)
		//	(6) firmware version [std::string]
		//	(7) enabled [bool] (only login allowed)
		//	(8) age [std::chrono::system_clock::time_stamp]
		//	(9) config [cyng::param_map_t]

		//if (!db.create_table(cyng::store::make_meta_table<1, 8>("TDevice",
		//	{ "pk", "name", "number", "descr", "id", "vFirmware", "enabled", "created", "config" },
		//	{ cyng::TC_UUID, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_BOOL, cyng::TC_TIME_POINT, cyng::TC_PARAM_MAP },
		//	{ 36, 128, 128, 512, 64, 64, 0, 0, 1024 })))
		if (!db.create_table(cyng::table::make_meta_table<1, 7>("TDevice",
			{ "pk", "name", "number", "descr", "id", "vFirmware", "enabled", "created" },
			{ cyng::TC_UUID, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_BOOL, cyng::TC_TIME_POINT },
			{ 36, 128, 128, 512, 64, 64, 0, 0 })))
		{
			CYNG_LOG_FATAL(logger, "cannot create table TDevice");
		}
		else
		{
			//db.insert("TDevice", cyng::store::key_generator(tag), cyng::store::data_generator("name", "number", "descr", "id", "vFirmware", true, std::chrono::system_clock::now(), cyng::param_map_t()), 72);
			db.insert("TDevice", cyng::table::key_generator(tag), cyng::table::data_generator("name", "number", "descr", "id", "vFirmware", true, std::chrono::system_clock::now()), 72);

		}

		//	(1) tag (UUID) - pk
		//	+-----------------
		//	(1) id (server/gateway ID) - unique
		//	(2) manufacturer (i.e. EMH)
		//	(3) Typbezeichnung (i.e. Variomuc ETHERNET)
		//	(4) production date/time
		//	(5) firmwareversion (i.e. 11600000)
		//	(6) fabrik nummer (i.e. 06441734)
		//	(7) MAC of service interface
		//	(8) MAC od data interface
		//	(9) Default PW
		//	(10) root PW
		//	(11) W-Mbus ID (i.e. A815408943050131)
		//	(12) source (UUID) - usefull to detect multiple configuration uploads
		if (!db.create_table(cyng::table::make_meta_table<1, 13>("TGateway", { "pk"	//	primary key
				, "id"	//	(1) Server-ID (i.e. 0500153B02517E)
				, "manufacturer"	//	(2) manufacturer (i.e. EMH)
				, "model"	//	(3) Typbezeichnung (i.e. Variomuc ETHERNET)
				, "made"	//	(4) production date
				, "vFirmware"	//	(5) firmwareversion (i.e. 11600000)
				, "factoryNr"	//	(6) fabrik nummer (i.e. 06441734)
				, "ifService"	//	(7) MAC of service interface
				, "ifData"	//	(8) MAC od data interface
				, "pwdDef"	//	(9) Default PW
				, "pwdRoot"	//	(10)
				, "mbus"	//	(11)
				, "userName"	//	(12)
				, "userPwd"	//	(13)
			},
			{ cyng::TC_UUID, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_TIME_POINT, cyng::TC_STRING, cyng::TC_MAC48, cyng::TC_MAC48, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING },
			{ 36, 23, 64, 64, 0, 8, 18, 18, 32, 32, 16, 32, 32 })))
		{
			CYNG_LOG_FATAL(logger, "cannot create table TGateway");
		}


	}

}



