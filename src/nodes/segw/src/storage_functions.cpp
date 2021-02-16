/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <storage_functions.h>
#include <smf/obis/defs.h>

#include <cyng/sql/sql.hpp>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/container_cast.hpp>
#include <cyng/io/ostream.h>
#include <cyng/db/details/statement_interface.h>

#include <iostream>

namespace smf {

	cyng::meta_sql get_table_cfg() {

		return cyng::meta_sql("TCfg"
			, {
				cyng::column_sql("path", cyng::TC_STRING, 128),	//	path, '/' separated values
				cyng::column_sql("val", cyng::TC_STRING, 256),	//	value
				cyng::column_sql("def", cyng::TC_STRING, 256),	//	default value
				cyng::column_sql("type", cyng::TC_UINT32, 0),	//	data type code (default)
				cyng::column_sql("desc", cyng::TC_STRING, 0)	//	optional description
			}
		, 1);
	}

	std::vector< cyng::meta_sql > get_sql_meta_data() {

		return {
			get_table_cfg(),

			cyng::meta_sql("TOpLog"
				, {
					cyng::column_sql("id", cyng::TC_INT64, 0),
					cyng::column_sql("actTime", cyng::TC_TIME_POINT, 0),
					cyng::column_sql("age", cyng::TC_TIME_POINT, 0),
					cyng::column_sql("regPeriod", cyng::TC_UINT32, 0),		//	register period
					cyng::column_sql("valTime", cyng::TC_TIME_POINT, 0),	//	val time
					cyng::column_sql("status", cyng::TC_UINT64, 0),			//	status word
					cyng::column_sql("event", cyng::TC_UINT32, 0),			//	event code
					cyng::column_sql("peer", cyng::TC_BUFFER, 13),			//	peer address
					cyng::column_sql("utc", cyng::TC_TIME_POINT, 0),		//	UTC time
					cyng::column_sql("serverId", cyng::TC_BUFFER, 23),		//	server ID (meter)
					cyng::column_sql("target", cyng::TC_STRING, 64),		//	target name
					cyng::column_sql("pushNr", cyng::TC_UINT8, 0),			//	operation number
					cyng::column_sql("details", cyng::TC_STRING, 128)		//	description (DATA_PUSH_DETAILS)
				}
			, 1),

			cyng::meta_sql("TMeter"
				, {
					cyng::column_sql("id", cyng::TC_UINT64, 0),		//	hash of "meter"
					cyng::column_sql("meter", cyng::TC_STRING, 23),	//	02-e61e-03197715-3c-07, or 03197715
					cyng::column_sql("type", cyng::TC_STRING, 0),	//	IEC, M-Bus, wM-Bus
					cyng::column_sql("activity", cyng::TC_TIME_POINT, 0),	//	last activity
					cyng::column_sql("enabled", cyng::TC_BOOL, 0),			//	active
					cyng::column_sql("desc", cyng::TC_STRING, 128)	//	optional description
				}
			, 1),

			cyng::meta_sql("TMeterMBus"
				, {
					cyng::column_sql("serverID", cyng::TC_BUFFER, 9),		//	server/meter ID
					cyng::column_sql("lastSeen", cyng::TC_TIME_POINT, 0),	//	last seen - Letzter Datensatz: 20.06.2018 14:34:22"
					cyng::column_sql("class", cyng::TC_STRING, 16),			//	device class (always "---" == 2D 2D 2D)
					cyng::column_sql("active", cyng::TC_BOOL, 0),			//	active
					cyng::column_sql("descr", cyng::TC_STRING, 128),		//	manufacturer/description
					//	---
					cyng::column_sql("status", cyng::TC_UINT32, 0),			//	"Statusinformation: 00"
					//	Contains a bit mask to define the bits of the status word, that if changed
					//	will result in an entry in the log-book.
					cyng::column_sql("mask", cyng::TC_BUFFER, 0),			//	"Bitmaske: 00 00"
					cyng::column_sql("interval", cyng::TC_UINT32, 0),		//	"Zeit zwischen zwei Datensätzen: 49000"
					//	--- optional data
					cyng::column_sql("pubKey", cyng::TC_BUFFER, 16),		//	Public Key: 18 01 16 05 E6 1E 0D 02 BF 0C FA 35 7D 9E 77 03"
					cyng::column_sql("aes", cyng::TC_AES128, 32),			//	AES-Key
					cyng::column_sql("user", cyng::TC_STRING, 32),
					cyng::column_sql("pwd", cyng::TC_STRING, 32)

				}
			, 1),

			cyng::meta_sql("TMeterIEC"
				, {
					cyng::column_sql("nr", cyng::TC_UINT8, 0),			//	number
					cyng::column_sql("meterID", cyng::TC_BUFFER, 32),	//	max. 32 bytes (8181C7930AFF)
					cyng::column_sql("address", cyng::TC_BUFFER, 32),	//	mostly the same as meterID (8181C7930CFF)
					cyng::column_sql("descr", cyng::TC_STRING, 128),
					cyng::column_sql("baudrate", cyng::TC_UINT32, 0),	//	9600, ... (in opening sequence) (8181C7930BFF)
					cyng::column_sql("p1", cyng::TC_STRING, 32),		//	login password (8181C7930DFF)
					//, "p2"		//	login password
					cyng::column_sql("w5", cyng::TC_STRING, 32)			//	W5 password (reserved for national applications) (8181C7930EFF)
				}
			, 1)
		};

	}

	bool init_storage(cyng::db::session& db) {
		BOOST_ASSERT(db.is_alive());

		try {
			//
			//	start transaction
			//
			cyng::db::transaction	trx(db);

			auto const vec = get_sql_meta_data();
			for (auto const& m : vec) {
				auto const sql = cyng::sql::create(db.get_dialect(), m).to_str();
				if (!db.execute(sql)) {
					std::cerr
						<< "**error: "
						<< sql
						<< std::endl;
				}
			}

			return true;
		}
		catch(std::exception const& ex)	{}
		return false;
	}

	void transfer_config(cyng::db::session& db, cyng::object&& cfg) {
		BOOST_ASSERT(db.is_alive());
		auto const reader = cyng::make_reader(std::move(cfg));

		//
		//	start transaction
		//
		cyng::db::transaction	trx(db);

		//
		//	transfer IP-T configuration
		//
		transfer_ipt(db, cyng::container_cast<cyng::vector_t>(reader["ipt-config"].get()));

		//
		//	transfer SML server configuration
		//	%(("accept-all-ids":false),("account":operator),("address":0.0.0.0),("discover":5798),("enabled":true),("pwd":operator),("service":7259))
		//
		transfer_sml(db, cyng::container_cast<cyng::param_map_t>(reader["sml"].get()));

		//
		//	transfer NMS server configuration
		//
		transfer_nms(db, cyng::container_cast<cyng::param_map_t>(reader["nms"].get()));

		//
		//	transfer LMN configuration
		//
		transfer_lnm(db, cyng::container_cast<cyng::vector_t>(reader["lmn"].get()));

		//
		//	transfer GPIO configuration
		//

	}

	void transfer_ipt(cyng::db::session& db, cyng::vector_t&& vec) {

		std::uint8_t idx{ 1 };
		for (auto const& cfg : vec) {
			//
			//	%(("account":segw),
			//	("def-sk":0102030405060708090001020304050607080900010203040506070809000001),
			//	("host":127.0.0.1),
			//	("pwd":NODE_PWD),
			//	("scrambled":true),
			//	("service":26862))
			//
			std::cout << "IP-T: " << cfg << std::endl;
			std::cout << cyng::to_path('/', OBIS_ROOT_IPT_PARAM,
				cyng::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
				cyng::make_obis(0x81, 0x49, 0x17, 0x07, 0x00, idx)) << std::endl;
			insert_config_record(db
				, cyng::to_path('/', OBIS_ROOT_IPT_PARAM,
					cyng::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
					cyng::make_obis(0x81, 0x49, 0x17, 0x07, 0x00, idx))
				, cyng::make_object("example")
				, "description");
			idx++;
		}
	}

	void transfer_sml(cyng::db::session&, cyng::param_map_t&& pmap) {
		std::cout << "SML: " << pmap << std::endl;

	}

	void transfer_nms(cyng::db::session&, cyng::param_map_t&& pmap) {
		std::cout << "NMS: " << pmap << std::endl;

	}

	void transfer_lnm(cyng::db::session&, cyng::vector_t&& vec) {
		for (auto const& cfg : vec) {
			//
			//	%(
			//	("HCI":CP210x),
			//	("blocklist":%(("enabled":true),("list":[00684279,12345678]),("mode":drop),("period":30))),
			//	("broker":[%(("address":segw.ch),("port":12001))]),
			//	("broker-enabled":false),
			//	("broker-login":false),
			//	("databits":8),
			//	("enabled":false),
			//	("flow-control":none),
			//	("parity":none),
			//	("port":COM3),
			//	("speed":57600),
			//	("stopbits":one),
			//	("type":wireless))
			// 
			std::cout << "LMN: " << cfg << std::endl;
		}
	}

	void clear_config(cyng::db::session& db) {
		BOOST_ASSERT(db.is_alive());

		auto const m = get_table_cfg();
		auto const sql = cyng::sql::remove(db.get_dialect(), m)();
		db.execute(sql);
	}

	bool insert_config_record(cyng::db::session& db, std::string key, cyng::object obj, std::string desc)
	{
		//
		//	ToDo: use already prepared statements
		//
		auto const m = get_table_cfg();
		auto const sql = cyng::sql::insert(db.get_dialect(), m).bind_values(m)();

		auto stmt = db.create_statement();
		std::pair<int, bool> r = stmt->prepare(sql);
		if (r.second) {

			auto const val = cyng::make_object(cyng::to_string(obj));

			stmt->push(cyng::make_object(key), 128);	//	pk
			//stmt->push(cyng::make_object(1u), 0);	//	gen
			stmt->push(val, 256);	//	val
			stmt->push(val, 256);	//	def
			stmt->push(cyng::make_object(obj.rtti().tag()), 0);	//	type
			if (stmt->execute())	return true;

		}

		//cyng::table::meta_table_ptr meta = storage::mm_.at("TCfg");
		//cyng::sql::command cmd(meta, s.get_dialect());
		//auto const sql = cmd.insert()();
		//auto stmt = s.create_statement();
		//std::pair<int, bool> r = stmt->prepare(sql);
		//if (r.second) {
		//	//	repackaging as string
		//	auto const val = cyng::make_object(cyng::io::to_str(obj));
		//	stmt->push(cyng::make_object(key), 128);	//	pk
		//	stmt->push(cyng::make_object(1u), 0);	//	gen
		//	stmt->push(val, 256);	//	val
		//	stmt->push(val, 256);	//	def
		//	stmt->push(cyng::make_object(obj.get_class().tag()), 0);	//	type
		//	if (stmt->execute())	return true;
		//}
		return false;
	}

}