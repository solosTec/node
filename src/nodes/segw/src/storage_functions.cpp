/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <storage_functions.h>
#include <smf/obis/defs.h>
#include <smf/ipt/config.h>
#include <smf/ipt/scramble_key_format.h>

#include <cyng/sql/sql.hpp>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/algorithm/find.h>
#include <cyng/obj/container_cast.hpp>
#include <cyng/obj/numeric_cast.hpp>
#include <cyng/obj/vector_cast.hpp>
#include <cyng/io/ostream.h>
#include <cyng/db/details/statement_interface.h>

#include <iostream>

#include <boost/core/ignore_unused.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/algorithm/string.hpp>

namespace smf {

	cyng::meta_store get_store_cfg() {

		return cyng::meta_store("cfg"
			, {
				cyng::column("path", cyng::TC_STRING),	//	path, '/' separated values
				cyng::column("val", cyng::TC_NULL)	//	value (data type may vary)
				//cyng::column("def", cyng::TC_STRING),	//	default value
				//cyng::column("type", cyng::TC_UINT16),	//	data type code (default)
				//cyng::column("desc", cyng::TC_STRING)	//	optional description
			}
		, 1);
	}

	cyng::meta_sql get_table_cfg() {

		return cyng::meta_sql("TCfg"
			, {
				cyng::column_sql("path", cyng::TC_STRING, 128),	//	path, '/' separated values
				//	generation
				cyng::column_sql("val", cyng::TC_STRING, 256),	//	value
				cyng::column_sql("def", cyng::TC_STRING, 256),	//	default value
				cyng::column_sql("type", cyng::TC_UINT16, 0),	//	data type code (default)
				cyng::column_sql("desc", cyng::TC_STRING, 256)	//	optional description
			}
		, 1);
	}

	cyng::meta_store get_store_oplog() {

		return cyng::meta_store("opLog"
			, {
				cyng::column("id", cyng::TC_INT64),
				cyng::column("actTime", cyng::TC_TIME_POINT),
				cyng::column("age", cyng::TC_TIME_POINT),
				cyng::column("regPeriod", cyng::TC_UINT32),		//	register period
				cyng::column("valTime", cyng::TC_TIME_POINT),	//	val time
				cyng::column("status", cyng::TC_UINT64),		//	status word
				cyng::column("event", cyng::TC_UINT32),			//	event code
				cyng::column("peer", cyng::TC_BUFFER),			//	peer address
				cyng::column("utc", cyng::TC_TIME_POINT),		//	UTC time
				cyng::column("serverId", cyng::TC_BUFFER),		//	server ID (meter)
				cyng::column("target", cyng::TC_STRING),		//	target name
				cyng::column("pushNr", cyng::TC_UINT8),			//	operation number
				cyng::column("details", cyng::TC_STRING)		//	description (DATA_PUSH_DETAILS)
			}
		, 1);
	}

	cyng::meta_sql get_table_oplog() {
		return cyng::to_sql(get_store_oplog(), { 0, 0, 0, 0, 0, 0, 0, 13, 0, 23, 64, 0, 128 });
	}

	std::vector< cyng::meta_store > get_store_meta_data() {
		return {
			get_store_cfg(),
			get_store_oplog()
		};
	}

	std::vector< cyng::meta_sql > get_sql_meta_data() {

		return {
			get_table_cfg(),
			get_table_oplog(),

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
		catch(std::exception const& ex)	{
			boost::ignore_unused(ex);
		}
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
		//	prepare statement
		//
		auto const m = get_table_cfg();
		auto const sql = cyng::sql::insert(db.get_dialect(), m).bind_values(m)();

		auto stmt = db.create_statement();
		std::pair<int, bool> const r = stmt->prepare(sql);
		if (r.second) {


			//
			//	transfer some global entries
			//		
			insert_config_record(stmt
				, cyng::to_path('/', "country-code")
				, reader["country-code"].get()
				, "country code");

			insert_config_record(stmt
				, cyng::to_path('/', "language-code")
				, reader["language-code"].get()
				, "language code");

			insert_config_record(stmt
				, cyng::to_path('/', "generate-profile")
				, reader["generate-profile"].get()
				, "generate profiles");

			auto const obj = reader["tag"].get();
			if (cyng::is_of_type<cyng::TC_STRING>(obj)) {
				auto const str = cyng::value_cast(obj, "00000000-0000-0000-0000-000000000000");
				if (str.size() == 36 && str.at(8) == '-' && str.at(13) == '-' && str.at(18) == '-' && str.at(23) == '-') {

					//
					//	convert string into UUID
					//
					boost::uuids::string_generator sgen;
					insert_config_record(stmt
						, cyng::to_path('/', "tag")
						, cyng::make_object(sgen(str))
						, "unique app id");
				}
				else {
#ifdef _DEBUG_SEGW
					std::cerr
						<< "**warning: invalid tag: "
						<< str
						<< std::endl;
#endif
				}
			}

			//
			//	transfer network/server configuration
			//
			transfer_net(stmt, cyng::container_cast<cyng::param_map_t>(reader["net"].get()));

			//
			//	transfer IP-T configuration
			//
			transfer_ipt_config(stmt, cyng::container_cast<cyng::vector_t>(reader["ipt"]["config"].get()));

			//
			//	transfer IP-T parameter
			//
			transfer_ipt_params(stmt, cyng::container_cast<cyng::param_map_t>(reader["ipt"]["param"].get()));

			//
			//	transfer SML server configuration
			//	%(("accept-all-ids":false),("account":operator),("address":0.0.0.0),("discover":5798),("enabled":true),("pwd":operator),("service":7259))
			//
			transfer_sml(stmt, cyng::container_cast<cyng::param_map_t>(reader["sml"].get()));

			//
			//	transfer NMS server configuration
			//
			transfer_nms(stmt, cyng::container_cast<cyng::param_map_t>(reader["nms"].get()));

			//
			//	transfer LMN configuration
			//
			transfer_lnm(stmt, cyng::container_cast<cyng::vector_t>(reader["lmn"].get()));

			//
			//	transfer GPIO configuration
			//
			transfer_gpio(stmt, cyng::container_cast<cyng::param_map_t>(reader["gpio"].get()));
		}
		else {

			std::cerr
				<< "**error: prepare statement failed"
				<< std::endl;
		}
	}

	void transfer_net(cyng::db::statement_ptr stmt, cyng::param_map_t&& pmap) {

		insert_config_record(stmt
			, cyng::to_path('/', "net", "mac")
			, cyng::make_object(cyng::find_value(pmap, std::string("mac"), std::string()))
			, "HAN MAC");

		insert_config_record(stmt
			, cyng::to_path('/', OBIS_SERVER_ID)
			, cyng::make_object(cyng::find_value(pmap, cyng::to_str(OBIS_SERVER_ID), std::string()))
			, "Server ID");

		auto const vec = cyng::vector_cast<std::string>(cyng::find(pmap, "local-links"), std::string("0000::0000:0000:0000:0000"));
		std::size_t counter{ 0 };
		for (auto link : vec) {

			insert_config_record(stmt
				, cyng::to_path('/', "net", "local-link", std::to_string(++counter))
				, cyng::make_object(link)
				, "local link (computed)");
		}

	}

	void transfer_ipt_config(cyng::db::statement_ptr stmt, cyng::vector_t&& vec) {

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
			//std::cout << "IP-T: " << cfg << std::endl;
			auto const srv = ipt::read_config(cyng::container_cast<cyng::param_map_t>(cfg));

			insert_config_record(stmt
				, cyng::to_path('/', OBIS_ROOT_IPT_PARAM,
					cyng::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
					cyng::make_obis(0x81, 0x49, 0x17, 0x07, 0x00, idx))
				, cyng::make_object(srv.host_)
				, "IP-T server address");

			insert_config_record(stmt
				, cyng::to_path('/', OBIS_ROOT_IPT_PARAM,
					cyng::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
					cyng::make_obis(0x81, 0x49, 0x1A, 0x07, 0x00, idx))
				, cyng::make_object(srv.service_)
				, "IP-T server target port");

			insert_config_record(stmt
				, cyng::to_path('/', OBIS_ROOT_IPT_PARAM,
					cyng::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
					cyng::make_obis(0x81, 0x49, 0x19, 0x07, 0x00, idx))
				, cyng::make_object(static_cast<std::uint16_t>(0u))
				, "IP-T server source port");

			insert_config_record(stmt
				, cyng::to_path('/', OBIS_ROOT_IPT_PARAM,
					cyng::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
					cyng::make_obis(0x81, 0x49, 0x63, 0x3C, 0x01, idx))
				, cyng::make_object(srv.account_)
				, "login account");

			insert_config_record(stmt
				, cyng::to_path('/', OBIS_ROOT_IPT_PARAM,
					cyng::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
					cyng::make_obis(0x81, 0x49, 0x63, 0x3C, 0x02, idx))
				, cyng::make_object(srv.pwd_)
				, "login password");

			insert_config_record(stmt
				, cyng::to_path('/', OBIS_ROOT_IPT_PARAM,
					cyng::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
					cyng::make_obis(0x81, 0x49, 0x63, 0x3C, 0x03, idx))
				, cyng::make_object(srv.scrambled_)
				, "scrambled login");

			insert_config_record(stmt
				, cyng::to_path('/', OBIS_ROOT_IPT_PARAM,
					cyng::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
					"sk")
				, cyng::make_object(ipt::to_string(srv.sk_))
				, "scramble key");

			idx++;
		}
	}

	void transfer_ipt_params(cyng::db::statement_ptr stmt, cyng::param_map_t&& pmap) {
		std::cout << "IP-T parameter: " << pmap << std::endl;

		//cyng::make_param("enabled", true)
		insert_config_record(stmt
			, cyng::to_path('/', OBIS_ROOT_IPT_PARAM,
				"enabled")
			, cyng::make_object(cyng::find_value(pmap, std::string("enabled"), true))
			, "IP-T enabled");

		//cyng::make_param(OBIS_TCP_WAIT_TO_RECONNECT, 1u),	//	minutes
		auto const tcp_wait_to_reconnect = cyng::numeric_cast<std::uint32_t>(cyng::find(pmap, cyng::to_str(OBIS_TCP_WAIT_TO_RECONNECT)), 1u);
		insert_config_record(stmt
			, cyng::to_path('/', OBIS_ROOT_IPT_PARAM,
				OBIS_TCP_WAIT_TO_RECONNECT)
			, cyng::make_object(std::chrono::seconds(tcp_wait_to_reconnect))
			, "reconnect timer (seconds)");

		//cyng::make_param(OBIS_TCP_CONNECT_RETRIES, 20u),
		auto const obis_tcp_connect_retries = cyng::numeric_cast<std::uint32_t>(cyng::find(pmap, cyng::to_str(OBIS_TCP_CONNECT_RETRIES)), 20u);
		insert_config_record(stmt
			, cyng::to_path('/', OBIS_ROOT_IPT_PARAM,
				OBIS_TCP_CONNECT_RETRIES)
			, cyng::make_object(obis_tcp_connect_retries)
			, "reconnect counter");

		//cyng::make_param(OBIS_HAS_SSL_CONFIG, 0u),	//	has SSL configuration
		auto const obis_has_ssl_config = cyng::value_cast(cyng::find(pmap, cyng::to_str(OBIS_HAS_SSL_CONFIG)), false);
		insert_config_record(stmt
			, cyng::to_path('/', OBIS_ROOT_IPT_PARAM,
				OBIS_ROOT_IPT_PARAM)
			, cyng::make_object(obis_has_ssl_config)
			, "SSL support");
	}

	void transfer_sml(cyng::db::statement_ptr stmt, cyng::param_map_t&& pmap) {
		//std::cout << "SML: " << pmap << std::endl;

		insert_config_record(stmt
			, cyng::to_path('/', "sml",
				"enabled")
			, cyng::make_object(cyng::find_value(pmap, std::string("enabled"), true))
			, "SML enabled");

		insert_config_record(stmt
			, cyng::to_path('/', "sml",
				"address")
			, cyng::make_object(cyng::find_value(pmap, std::string("address"), std::string("0.0.0.0")))
			, "SML bind address");

		insert_config_record(stmt
			, cyng::to_path('/', "sml",
				"service")
			, cyng::make_object(cyng::find_value(pmap, std::string("service"), std::string("7259")))
			, "SML service port");

		insert_config_record(stmt
			, cyng::to_path('/', "sml",
				"discover")
			, cyng::make_object(cyng::find_value(pmap, std::string("discover"), std::string("5798")))
			, "SML discover port");

		insert_config_record(stmt
			, cyng::to_path('/', "sml",
				"account")
			, cyng::make_object(cyng::find_value(pmap, std::string("account"), std::string("")))
			, "SML login name");

		insert_config_record(stmt
			, cyng::to_path('/', "sml",
				"pwd")
			, cyng::make_object(cyng::find_value(pmap, std::string("pwd"), std::string("")))
			, "SML login password");

		insert_config_record(stmt
			, cyng::to_path('/', "sml",
				"accept-all-ids")
			, cyng::make_object(cyng::find_value(pmap, std::string("accept-all-ids"), false))
			, "check server IDs");

	}

	void transfer_nms(cyng::db::statement_ptr stmt, cyng::param_map_t&& pmap) {
		//std::cout << "NMS: " << pmap << std::endl;

		insert_config_record(stmt
			, cyng::to_path('/', "nms",
				"enabled")
			, cyng::make_object(cyng::find_value(pmap, std::string("enabled"), true))
			, "NMS enabled");

		insert_config_record(stmt
			, cyng::to_path('/', "nms",
				"address")
			, cyng::make_object(cyng::find_value(pmap, std::string("address"), std::string("0.0.0.0")))
			, "NMS bind address");

		auto const nms_port = cyng::numeric_cast<std::uint16_t>(cyng::find(pmap, std::string("port")), 7261);
		insert_config_record(stmt
			, cyng::to_path('/', "nms",
				"port")
			, cyng::make_object(nms_port)
			, "NMS listener port (" + std::to_string(nms_port) + ")");

		insert_config_record(stmt
			, cyng::to_path('/', "nms",
				"account")
			, cyng::make_object(cyng::find_value(pmap, std::string("account"), std::string("")))
			, "NMS login name");

		insert_config_record(stmt
			, cyng::to_path('/', "nms",
				"pwd")
			, cyng::make_object(cyng::find_value(pmap, std::string("pwd"), std::string("")))
			, "NMS login password");

		std::filesystem::path const script_path =  cyng::find_value(pmap, std::string("script-path"), std::string(""));
		insert_config_record(stmt
			, cyng::to_path('/', "nms",
				"script-path")
			, cyng::make_object(script_path)
			, "path to update script");

	}

	void transfer_lnm(cyng::db::statement_ptr stmt, cyng::vector_t&& vec) {
		std::size_t counter{ 0 };
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
			//std::cout << "LMN: " << cfg << std::endl;

			auto const pmap = cyng::container_cast<cyng::param_map_t>(cfg);
			for (auto const& param : pmap) {

				if (boost::algorithm::equals(param.first, "databits")) {

					auto const databits = cyng::numeric_cast<std::uint8_t>(cyng::find(cfg, param.first), 8);
					insert_config_record(stmt
						, cyng::to_path('/', "lmn", std::to_string(counter),param.first)
						, cyng::make_object(databits)
						, "databits (7, 8)");
				}
				else if (boost::algorithm::equals(param.first, "speed")) {

					auto const speed = cyng::numeric_cast<std::uint32_t>(cyng::find(cfg, param.first), 2400);
					insert_config_record(stmt
						, cyng::to_path('/', "lmn", std::to_string(counter), param.first)
						, cyng::make_object(speed)
						, "speed in bauds (" + std::to_string(speed) + ")");
				}
				else if (boost::algorithm::equals(param.first, "broker-timeout")) {
					auto const reconnect = cyng::numeric_cast<std::uint32_t>(cyng::find(cfg, param.first), 12);
					insert_config_record(stmt
						, cyng::to_path('/', "lmn", std::to_string(counter), param.first)
						, cyng::make_object(std::chrono::seconds(reconnect))
						, "reconnect time in seconds (" + std::to_string(reconnect) + ")");
				}
				else if (boost::algorithm::equals(param.first, "broker")) {

					//
					//	multiple broker possible
					//
					auto const vec = cyng::container_cast<cyng::vector_t>(param.second);
					std::size_t broker_index{ 0 };
					for (auto const& obj : vec) {

						auto const broker = cyng::container_cast<cyng::param_map_t>(obj);

						auto pos = broker.find("address");
						if (pos != broker.end()) {
							insert_config_record(stmt
								, cyng::to_path('/', "broker", std::to_string(counter), std::to_string(broker_index), pos->first)
								, pos->second
								, "broker address");
						}

						pos = broker.find("port");
						if (pos != broker.end()) {
							auto const broker_port = cyng::numeric_cast<std::uint16_t>(pos->second, 12000 + broker_index);

							insert_config_record(stmt
								, cyng::to_path('/', "broker", std::to_string(counter), std::to_string(broker_index), pos->first)
								, cyng::make_object(broker_port)
								, "broker port " + std::to_string(broker_port));
						}

						pos = broker.find("account");
						if (pos != broker.end()) {
							insert_config_record(stmt
								, cyng::to_path('/', "broker", std::to_string(counter), std::to_string(broker_index), pos->first)
								, pos->second
								, "login name");
						}

						pos = broker.find("pwd");
						if (pos != broker.end()) {
							insert_config_record(stmt
								, cyng::to_path('/', "broker", std::to_string(counter), std::to_string(broker_index), pos->first)
								, pos->second
								, "login password");
						}

						broker_index++;
					}

					insert_config_record(stmt
						, cyng::to_path('/', "broker", std::to_string(counter), "count")
						, cyng::make_object(broker_index)
						, "broker count");

				}
				else if (boost::algorithm::equals(param.first, "blocklist")) {

					//	FixMe: incomplete
					auto const broker = cyng::container_cast<cyng::param_map_t>(param.second);

					auto pos = broker.find("enabled");
					if (pos != broker.end()) {
						insert_config_record(stmt
							, cyng::to_path('/', "blocklist", std::to_string(counter), pos->first)
							, pos->second
							, "enable/disable blocklist");
					}
					pos = broker.find("mode");
					if (pos != broker.end()) {
						insert_config_record(stmt
							, cyng::to_path('/', "blocklist", std::to_string(counter), pos->first)
							, pos->second
							, "drop mode (drop/accept)");
					}
					pos = broker.find("period");
					if (pos != broker.end()) {
						auto const period = cyng::numeric_cast<std::uint32_t>(cyng::find(cfg, param.first), 30);
						insert_config_record(stmt
							, cyng::to_path('/', "blocklist", std::to_string(counter), pos->first)
							, cyng::make_object(std::chrono::seconds(period))
							, "period in seconds");
					}
				}
				else {
					insert_config_record(stmt
						, cyng::to_path('/', "lmn", std::to_string(counter), param.first)
						, cyng::find(cfg, param.first)
						, param.first);
				}
			}

			//
			//	next LMN
			//
			counter++;
		}

		insert_config_record(stmt
			, cyng::to_path('/', "lmn", "count")
			, cyng::make_object(counter)
			, "LMN count");

	}

	void transfer_gpio(cyng::db::statement_ptr stmt, cyng::param_map_t&& pmap) {
		std::cout << "GPIO: " << pmap << std::endl;

		insert_config_record(stmt
			, cyng::to_path('/', "gpio", "enabled")
			, cyng::find(pmap, std::string("enabled"))
			, "GPIO enabled");

		//
		//	treat as filesystem path
		//
		std::filesystem::path p = cyng::find_value(pmap, "path", "/sys/class/gpio");
		insert_config_record(stmt
			, cyng::to_path('/', "gpio",
				"path")
			, cyng::make_object(std::move(p))
			, "virtual path");

		auto const vec = cyng::vector_cast<std::int64_t>(cyng::find(pmap, "list"), 0);
		std::size_t counter{ 0 };
		for (auto pin : vec) {

			insert_config_record(stmt
				, cyng::to_path('/', "gpio", "pin", std::to_string(++counter))
				, cyng::make_object(pin)
				, "GPIO pin");
		}
	}


	void clear_config(cyng::db::session& db) {
		BOOST_ASSERT(db.is_alive());

		auto const m = get_table_cfg();
		auto const sql = cyng::sql::remove(db.get_dialect(), m)();
		db.execute(sql);
	}

	bool insert_config_record(cyng::db::statement_ptr stmt, std::string key, cyng::object obj, std::string desc)
	{
		//
		//	use already prepared statements
		//

		auto const val = cyng::make_object(cyng::to_string(obj));

		stmt->push(cyng::make_object(key), 128);	//	pk
		//stmt->push(cyng::make_object(1), 0);	//	gen
		stmt->push(val, 256);	//	val
		stmt->push(val, 256);	//	def
		stmt->push(cyng::make_object(obj.rtti().tag()), 0);	//	type
		stmt->push(cyng::make_object(desc), 256);	//	desc
		if (stmt->execute()) {
			stmt->clear();
			return true;
		}

		return false;
	}

}