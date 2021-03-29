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
#include <cfg.h>
#include <storage.h>

#include <cyng/sql/sql.hpp>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/algorithm/find.h>
#include <cyng/obj/container_cast.hpp>
#include <cyng/obj/numeric_cast.hpp>
#include <cyng/obj/vector_cast.hpp>
#include <cyng/io/ostream.h>
#include <cyng/db/details/statement_interface.h>
#include <cyng/db/storage.h>
#include <cyng/parse/string.h>

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
				cyng::column("ROWID", cyng::TC_INT64),			//	index - with SQLite this prevents creating a column
				cyng::column("actTime", cyng::TC_TIME_POINT),
				cyng::column("age", cyng::TC_TIME_POINT),
				cyng::column("regPeriod", cyng::TC_UINT32),		//	register period
				cyng::column("valTime", cyng::TC_TIME_POINT),	//	val time
				cyng::column("status", cyng::TC_UINT64),		//	status word
				cyng::column("event", cyng::TC_UINT32),			//	event code
				cyng::column("peer", cyng::TC_OBIS),			//	peer address
				cyng::column("utc", cyng::TC_TIME_POINT),		//	UTC time
				cyng::column("serverId", cyng::TC_BUFFER),		//	server ID (meter)
				cyng::column("target", cyng::TC_STRING),		//	target name
				cyng::column("pushNr", cyng::TC_UINT8),			//	operation number
				cyng::column("details", cyng::TC_STRING)		//	description (DATA_PUSH_DETAILS)
			}
		, 0);
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
				, cyng::to_path(cfg::sep, "country-code")
				, reader["country-code"].get()
				, "country code");

			insert_config_record(stmt
				, cyng::to_path(cfg::sep, "language-code")
				, reader["language-code"].get()
				, "language code");

			insert_config_record(stmt
				, cyng::to_path(cfg::sep, "generate-profile")
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
						, cyng::to_path(cfg::sep, "tag")
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
			//	transfer hardware configuration
			//
			transfer_hardware(stmt, cyng::container_cast<cyng::param_map_t>(reader["hardware"].get()));

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

			//
			//	transfer virtual meter configuration
			//
			transfer_vmeter(stmt, cyng::container_cast<cyng::param_map_t>(reader["virtual-meter"].get()));

		}
		else {

			std::cerr
				<< "**error: prepare statement failed"
				<< std::endl;
		}
	}

	void transfer_net(cyng::db::statement_ptr stmt, cyng::param_map_t&& pmap) {

		insert_config_record(stmt
			, cyng::to_path(cfg::sep, "net", "mac")
			, cyng::make_object(cyng::find_value(pmap, std::string("mac"), std::string()))
			, "HAN MAC");

		insert_config_record(stmt
			, cyng::to_path(cfg::sep, OBIS_SERVER_ID)
			, cyng::make_object(cyng::find_value(pmap, cyng::to_str(OBIS_SERVER_ID), std::string()))
			, "Server ID");

		auto const vec = cyng::vector_cast<std::string>(cyng::find(pmap, "local-links"), std::string("0000::0000:0000:0000:0000"));
		std::size_t counter{ 0 };
		for (auto link : vec) {

			insert_config_record(stmt
				, cyng::to_path(cfg::sep, "net", "local-link", std::to_string(++counter))
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
				, cyng::to_path(cfg::sep, OBIS_ROOT_IPT_PARAM,
					cyng::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
					cyng::make_obis(0x81, 0x49, 0x17, 0x07, 0x00, idx))
				, cyng::make_object(srv.host_)
				, "IP-T server address");

			insert_config_record(stmt
				, cyng::to_path(cfg::sep, OBIS_ROOT_IPT_PARAM,
					cyng::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
					cyng::make_obis(0x81, 0x49, 0x1A, 0x07, 0x00, idx))
				, cyng::make_object(srv.service_)
				, "IP-T server target port");

			insert_config_record(stmt
				, cyng::to_path(cfg::sep, OBIS_ROOT_IPT_PARAM,
					cyng::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
					cyng::make_obis(0x81, 0x49, 0x19, 0x07, 0x00, idx))
				, cyng::make_object(static_cast<std::uint16_t>(0u))
				, "IP-T server source port");

			insert_config_record(stmt
				, cyng::to_path(cfg::sep, OBIS_ROOT_IPT_PARAM,
					cyng::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
					cyng::make_obis(0x81, 0x49, 0x63, 0x3C, 0x01, idx))
				, cyng::make_object(srv.account_)
				, "login account");

			insert_config_record(stmt
				, cyng::to_path(cfg::sep, OBIS_ROOT_IPT_PARAM,
					cyng::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
					cyng::make_obis(0x81, 0x49, 0x63, 0x3C, 0x02, idx))
				, cyng::make_object(srv.pwd_)
				, "login password");

			insert_config_record(stmt
				, cyng::to_path(cfg::sep, OBIS_ROOT_IPT_PARAM,
					cyng::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
					cyng::make_obis(0x81, 0x49, 0x63, 0x3C, 0x03, idx))
				, cyng::make_object(srv.scrambled_)
				, "scrambled login");

			insert_config_record(stmt
				, cyng::to_path(cfg::sep, OBIS_ROOT_IPT_PARAM,
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
			, cyng::to_path(cfg::sep, OBIS_ROOT_IPT_PARAM,
				"enabled")
			, cyng::make_object(cyng::find_value(pmap, std::string("enabled"), true))
			, "IP-T enabled");

		//cyng::make_param(OBIS_TCP_WAIT_TO_RECONNECT, 1u),	//	minutes
		auto const tcp_wait_to_reconnect = cyng::numeric_cast<std::uint32_t>(cyng::find(pmap, cyng::to_str(OBIS_TCP_WAIT_TO_RECONNECT)), 1u);
		insert_config_record(stmt
			, cyng::to_path(cfg::sep, OBIS_ROOT_IPT_PARAM,
				OBIS_TCP_WAIT_TO_RECONNECT)
			, cyng::make_object(std::chrono::seconds(tcp_wait_to_reconnect))
			, "reconnect timer (seconds)");

		//cyng::make_param(OBIS_TCP_CONNECT_RETRIES, 20u),
		auto const obis_tcp_connect_retries = cyng::numeric_cast<std::uint32_t>(cyng::find(pmap, cyng::to_str(OBIS_TCP_CONNECT_RETRIES)), 20u);
		insert_config_record(stmt
			, cyng::to_path(cfg::sep, OBIS_ROOT_IPT_PARAM,
				OBIS_TCP_CONNECT_RETRIES)
			, cyng::make_object(obis_tcp_connect_retries)
			, "reconnect counter");

		//cyng::make_param(OBIS_HAS_SSL_CONFIG, 0u),	//	has SSL configuration
		auto const obis_has_ssl_config = cyng::value_cast(cyng::find(pmap, cyng::to_str(OBIS_HAS_SSL_CONFIG)), false);
		insert_config_record(stmt
			, cyng::to_path(cfg::sep, OBIS_ROOT_IPT_PARAM,
				OBIS_ROOT_IPT_PARAM)
			, cyng::make_object(obis_has_ssl_config)
			, "SSL support");
	}

	void transfer_hardware(cyng::db::statement_ptr stmt, cyng::param_map_t&& pmap) {

		for (auto const& param : pmap) {
			if (boost::algorithm::equals(param.first, "serial")) {
				auto const serial_number = cyng::numeric_cast<std::uint32_t>(param.second, 10000000u);
				insert_config_record(stmt
					, cyng::to_path(cfg::sep, "hw", param.first)
					, cyng::make_object(serial_number)
					, "serial number " + std::to_string(serial_number));
			}
			else {

				insert_config_record(stmt
					, cyng::to_path(cfg::sep, "hw", param.first)
					, param.second
					, "hardware: " + param.first);
			}
		}
	}

	void transfer_sml(cyng::db::statement_ptr stmt, cyng::param_map_t&& pmap) {

		for (auto const& param : pmap) {
			if (boost::algorithm::equals(param.first, "address")) {

				boost::system::error_code ec;
				auto const address = boost::asio::ip::make_address(cyng::value_cast(param.second, "0.0.0.0"), ec);
				insert_config_record(stmt
					, cyng::to_path(cfg::sep, "sml", param.first)
					, cyng::make_object(address)
					, "SML bind address");

			}
			else if (boost::algorithm::equals(param.first, "port")) {

				auto const sml_port = cyng::numeric_cast<std::uint16_t>(param.second, 7259);
				insert_config_record(stmt
					, cyng::to_path(cfg::sep, "sml", param.first)
					, cyng::make_object(sml_port)
					, "default SML listener port (" + std::to_string(sml_port) + ")");
			}
			else if (boost::algorithm::equals(param.first, "discover")) {

				auto const sml_discover = cyng::numeric_cast<std::uint16_t>(param.second, 5798);
				insert_config_record(stmt
					, cyng::to_path(cfg::sep, "sml", param.first)
					, cyng::make_object(sml_discover)
					, "SML discovery port (" + std::to_string(sml_discover) + ")");
			}
			else {

				insert_config_record(stmt
					, cyng::to_path(cfg::sep, "sml", param.first)
					, param.second
					, "SML: " + param.first);

			}
		}
	}

	void transfer_nms(cyng::db::statement_ptr stmt, cyng::param_map_t&& pmap) {

		for (auto const& param : pmap) {
			if (boost::algorithm::equals(param.first, "address")) {

				boost::system::error_code ec;
				auto const address = boost::asio::ip::make_address(cyng::value_cast(param.second, "0.0.0.0"), ec);
				insert_config_record(stmt
					, cyng::to_path(cfg::sep, "nms",	param.first)
					, cyng::make_object(address)
					, "NMS bind address");

			}
			else if (boost::algorithm::equals(param.first, "port")) {

				auto const nms_port = cyng::numeric_cast<std::uint16_t>(param.second, 7261);
				insert_config_record(stmt
					, cyng::to_path(cfg::sep, "nms", param.first)
					, cyng::make_object(nms_port)
					, "default NMS listener port (" + std::to_string(nms_port) + ")");
			}
			else if (boost::algorithm::equals(param.first, "script-path")) {

				std::filesystem::path const script_path = cyng::find_value(pmap, param.first, std::string(""));
				insert_config_record(stmt
					, cyng::to_path(cfg::sep, "nms", param.first)
					, cyng::make_object(script_path)
					, "path to update script");
			}
			else {

				insert_config_record(stmt
					, cyng::to_path(cfg::sep, "nms", param.first)
					, param.second
					, "NMS: " + param.first);

			}
		}
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
						, cyng::to_path(cfg::sep, "lmn", std::to_string(counter),param.first)
						, cyng::make_object(databits)
						, "databits (7, 8)");
				}
				else if (boost::algorithm::equals(param.first, "speed")) {

					auto const speed = cyng::numeric_cast<std::uint32_t>(cyng::find(cfg, param.first), 2400);
					insert_config_record(stmt
						, cyng::to_path(cfg::sep, "lmn", std::to_string(counter), param.first)
						, cyng::make_object(speed)
						, "speed in bauds (" + std::to_string(speed) + ")");
				}
				else if (boost::algorithm::equals(param.first, "broker-timeout")) {
					auto const reconnect = cyng::numeric_cast<std::uint32_t>(cyng::find(cfg, param.first), 12);
					insert_config_record(stmt
						, cyng::to_path(cfg::sep, "lmn", std::to_string(counter), param.first)
						, cyng::make_object(std::chrono::seconds(reconnect))
						, "reconnect time in seconds (" + std::to_string(reconnect) + ")");
				}
				else if (boost::algorithm::equals(param.first, "broker")) {

					//
					//	multiple broker possible
					//
					transfer_broker(stmt, counter, cyng::container_cast<cyng::vector_t>(param.second));
				}
				else if (boost::algorithm::equals(param.first, "blocklist")) {

					//	enabled, mode, period, list[]
					transfer_blocklist(stmt, counter, cyng::container_cast<cyng::param_map_t>(param.second));

				}
				else if (boost::algorithm::equals(param.first, "listener")) {

					//
					//	One listener allowed
					//
					transfer_listener(stmt, counter, cyng::container_cast<cyng::param_map_t>(param.second));

				}
				else {
					insert_config_record(stmt
						, cyng::to_path(cfg::sep, "lmn", std::to_string(counter), param.first)
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
			, cyng::to_path(cfg::sep, "lmn", "count")
			, cyng::make_object(counter)
			, "LMN count");

	}

	void transfer_blocklist(cyng::db::statement_ptr stmt, std::size_t counter, cyng::param_map_t&& pmap) {
		//auto const broker = cyng::container_cast<cyng::param_map_t>(param.second);

		for (auto const& param : pmap) {
			if (boost::algorithm::equals(param.first, "list")) {

				auto const list = cyng::vector_cast<std::string>(param.second, "");
				std::size_t idx{ 0 };
				for (auto const& meter : list) {
					insert_config_record(stmt
						, cyng::to_path(cfg::sep, "blocklist", std::to_string(counter), "meter", idx)
						, cyng::make_object(meter)
						, "meter: " + meter);
					idx++;
				}

				BOOST_ASSERT(idx == list.size());
				insert_config_record(stmt
					, cyng::to_path(cfg::sep, "blocklist", std::to_string(counter), "size")
					, cyng::make_object(idx)
					, "blocklist size");

			}
			else if (boost::algorithm::equals(param.first, "period")) {
				//	"max-readout-frequency"
				auto const period = std::chrono::seconds(cyng::numeric_cast<std::uint32_t>(param.second, 30));
				insert_config_record(stmt
					, cyng::to_path(cfg::sep, "blocklist", std::to_string(counter), param.first)
					, cyng::make_object(period)
					, "blocklist: " + param.first);
			}
			else if (boost::algorithm::equals(param.first, "mode")) {
				auto mode = cyng::value_cast(param.second, "drop");
				std::transform(mode.begin(), mode.end(), mode.begin(), ::tolower);
				insert_config_record(stmt
					, cyng::to_path(cfg::sep, "blocklist", std::to_string(counter), param.first)
					, cyng::make_object(mode)
					, "mode: " + mode);
			}
			else {
				insert_config_record(stmt
					, cyng::to_path(cfg::sep, "blocklist", std::to_string(counter), param.first)
					, param.second
					, "blocklist: " + param.first);

			}
		}
	}

	void transfer_listener(cyng::db::statement_ptr stmt, std::size_t counter, cyng::param_map_t&& pmap) {
		//	%(("address":0.0.0.0),("port":6006))|%(("address":0.0.0.0),("port":6006))

		for (auto const& listener : pmap) {
			if (boost::algorithm::equals(listener.first, "port")) {

				auto const listener_port = cyng::numeric_cast<std::uint16_t>(listener.second, 6006);
				insert_config_record(stmt
					, cyng::to_path(cfg::sep, "listener", std::to_string(counter), listener.first)
					, cyng::make_object(listener_port)
					, "default listener port " + std::to_string(listener_port));
			}
			else if (boost::algorithm::equals(listener.first, "address")) {
				boost::system::error_code ec;
				auto const address = boost::asio::ip::make_address(cyng::value_cast(listener.second, "0.0.0.0"), ec);
				insert_config_record(stmt
					, cyng::to_path(cfg::sep, "listener", std::to_string(counter), listener.first)
					, cyng::make_object(address)
					, "listener bind address " + address.to_string());
			}
			else {
				insert_config_record(stmt
					, cyng::to_path(cfg::sep, "listener", std::to_string(counter), listener.first)
					, listener.second
					, "listener: " + listener.first);
			}
		}
	}

	void transfer_broker(cyng::db::statement_ptr stmt, std::size_t counter, cyng::vector_t&& vec) {

		std::size_t broker_index{ 0 };
		for (auto const& obj : vec) {

			auto const pmap = cyng::container_cast<cyng::param_map_t>(obj);

			for (auto const& broker : pmap) {
				if (boost::algorithm::equals(broker.first, "port")) {

					auto const broker_port = cyng::numeric_cast<std::uint16_t>(broker.second, 12000 + broker_index);
					insert_config_record(stmt
						, cyng::to_path(cfg::sep, "broker", std::to_string(counter), std::to_string(broker_index), broker.first)
						, cyng::make_object(broker_port)
						, "broker port " + std::to_string(broker_port));
				}
				else {
					insert_config_record(stmt
						, cyng::to_path(cfg::sep, "broker", std::to_string(counter), std::to_string(broker_index), broker.first)
						, broker.second
						, "broker: " + broker.first);
				}
			}

			broker_index++;
		}

		insert_config_record(stmt
			, cyng::to_path(cfg::sep, "broker", std::to_string(counter), "count")
			, cyng::make_object(broker_index)
			, "broker count");


	}

	void transfer_gpio(cyng::db::statement_ptr stmt, cyng::param_map_t&& pmap) {
		std::cout << "GPIO: " << pmap << std::endl;

		insert_config_record(stmt
			, cyng::to_path(cfg::sep, "gpio", "enabled")
			, cyng::find(pmap, std::string("enabled"))
			, "GPIO enabled");

		//
		//	treat as filesystem path
		//
		std::filesystem::path p = cyng::find_value(pmap, "path", "/sys/class/gpio");
		insert_config_record(stmt
			, cyng::to_path(cfg::sep, "gpio",
				"path")
			, cyng::make_object(std::move(p))
			, "virtual path");

		auto const vec = cyng::vector_cast<std::int64_t>(cyng::find(pmap, "list"), 0);
		std::size_t counter{ 0 };
		for (auto pin : vec) {

			insert_config_record(stmt
				, cyng::to_path(cfg::sep, "gpio", "pin", std::to_string(++counter))
				, cyng::make_object(pin)
				, "GPIO pin");
		}
	}

	void transfer_vmeter(cyng::db::statement_ptr stmt, cyng::param_map_t&& pmap) {

		for (auto const& param : pmap) {
			if (boost::algorithm::equals(param.first, "interval")) {

				auto const interval = cyng::numeric_cast<std::uint32_t>(param.second, 26000);
				insert_config_record(stmt
					, cyng::to_path(cfg::sep, "vmeter", param.first)
					, cyng::make_object(std::chrono::seconds(interval))
					, "virtual meter interval (sec) " + std::to_string(interval));
			}
			else if (boost::algorithm::equals(param.first, "port-index")) {

				auto const index = cyng::numeric_cast<std::size_t>(param.second, 1);
				insert_config_record(stmt
					, cyng::to_path(cfg::sep, "vmeter", param.first)
					, cyng::make_object(index)
					, "virtual meter port index " + std::to_string(index));
			}
			else {
				insert_config_record(stmt
					, cyng::to_path(cfg::sep, "vmeter", param.first)
					, param.second
					, "virtual meter: " + param.first);
			}
		}
	}

	void clear_config(cyng::db::session& db) {
		BOOST_ASSERT(db.is_alive());

		auto const m = get_table_cfg();
		auto const sql = cyng::sql::remove(db.get_dialect(), m)();
		db.execute(sql);
	}

	std::string get_section(std::string path, std::size_t idx) {

		auto const vec = cyng::split(path, "/");
		return (idx < vec.size())
			? vec.at(idx)
			: path
			;
	}

	void list_config(cyng::db::session& db) {
		BOOST_ASSERT(db.is_alive());

		auto const ms = get_table_cfg();
		auto const sql = cyng::sql::select(db.get_dialect()).all(ms, false).from(ms).order_by("path")();
		auto stmt = db.create_statement();
		stmt->prepare(sql);

		//cyng::column_sql("path", cyng::TC_STRING, 128),	//	path, '/' separated values
		////	generation
		//cyng::column_sql("val", cyng::TC_STRING, 256),	//	value
		//cyng::column_sql("def", cyng::TC_STRING, 256),	//	default value
		//cyng::column_sql("type", cyng::TC_UINT16, 0),	//	data type code (default)
		//cyng::column_sql("desc", cyng::TC_STRING, 256)	//	optional description

		//
		//	read all results
		//
		std::string section{ "000000000000" };

		while (auto res = stmt->get_result()) {
			auto const rec = cyng::to_record(ms, res);
			//std::cout << rec.to_tuple() << std::endl;
			auto const path = rec.value("path", "");
			auto const val = rec.value("val", "");
			auto const def = rec.value("def", "");
			auto const type = rec.value("type", static_cast<std::uint16_t>(0));
			auto const desc = rec.value("desc", "");
			auto const obj = cyng::restore(val, type);

			//	ToDo:
			//auto const opath = sml::translate_obis_path(path);

			//
			//	insert split lines between sections
			//
			auto const this_section = get_section(path);
			if (!boost::algorithm::equals(section, this_section)) {

				//
				//	update 
				//
				section = this_section;

				std::cout
					<< std::string(42, '-')
					<< "  ["
					<< section
					<< ']'
					<< std::endl;
			}

			std::cout
				<< std::setfill('.')
				<< std::setw(42)
				<< std::left
				<< path
				<< ": "
				;

			//
			//	list value (optional default value)
			//
			if (boost::algorithm::equals(val, def)) {
				std::cout << val;
			}
			else {
				std::cout
					<< '['
					<< val
					<< '/'
					<< def
					<< ']'
					;
			}

			std::cout
				<< " ("
				<< obj
				<< ':'
				<< obj.rtti().type_name()
				<< ") - "
				<< desc
				;


			//
			//	complete
			//
			std::cout << std::endl;

		}
	}

	bool set_config_value(cyng::db::session& db, std::string const& path, std::string const& value, std::string const& type) {

		storage store(db);

		auto const obj = cyng::make_object(path);

		if (boost::algorithm::equals(type, "bool") || boost::algorithm::equals(type, "b") || boost::algorithm::equals(type, "boolean")) {
			return store.cfg_update(obj, cyng::make_object(boost::algorithm::equals(value, "true")));
		}
		else if (boost::algorithm::equals(type, "u8")) {
			return store.cfg_update(obj, cyng::make_object(static_cast<std::uint8_t>(std::stoul(value))));
		}
		else if (boost::algorithm::equals(type, "u16")) {
			return store.cfg_update(obj, cyng::make_object(static_cast<std::uint16_t>(std::stoul(value))));
		}
		else if (boost::algorithm::equals(type, "u32")) {
			return store.cfg_update(obj, cyng::make_object(static_cast<std::uint32_t>(std::stoul(value))));
		}
		else if (boost::algorithm::equals(type, "u64")) {
			return store.cfg_update(obj, cyng::make_object(static_cast<std::uint64_t>(std::stoull(value))));
		}
		else if (boost::algorithm::equals(type, "i8")) {
			return store.cfg_update(obj, cyng::make_object(static_cast<std::int8_t>(std::stol(value))));
		}
		else if (boost::algorithm::equals(type, "i16")) {
			return store.cfg_update(obj, cyng::make_object(static_cast<std::int16_t>(std::stol(value))));
		}
		else if (boost::algorithm::equals(type, "i32")) {
			return store.cfg_update(obj, cyng::make_object(static_cast<std::int32_t>(std::stol(value))));
		}
		else if (boost::algorithm::equals(type, "i64")) {
			return store.cfg_update(obj, cyng::make_object(static_cast<std::int64_t>(std::stoll(value))));
		}
		else if (boost::algorithm::equals(type, "s")) {
			return store.cfg_update(obj, cyng::make_object(value));
		}
		else if (boost::algorithm::equals(type, "chrono:sec")) {
			return store.cfg_update(obj, cyng::make_object(std::chrono::seconds(std::stoull(value))));
		}
		else if (boost::algorithm::equals(type, "chrono:min")) {
			return store.cfg_update(obj, cyng::make_object(std::chrono::minutes(std::stoull(value))));
		}
		else if (boost::algorithm::equals(type, "ip:address")) {
			boost::system::error_code ec;
			return store.cfg_update(obj, cyng::make_object(boost::asio::ip::make_address(value, ec)));
		}
		else {
			std::cerr
				<< "supported data types: [bool] [u8] [u16] [u32] [u64] [i8] [i16] [i32] [i64] [s] [chrono:sec] [chrono:min] [ip:address]"
				<< std::endl;
		}

		return false;
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

	bool insert_config_record(cyng::db::statement_ptr stmt, cyng::object const& key, cyng::object obj) {

		auto const val = cyng::make_object(cyng::to_string(obj));

		stmt->push(key, 128);	//	pk
		stmt->push(val, 256);	//	val
		stmt->push(val, 256);	//	def
		stmt->push(cyng::make_object(obj.rtti().tag()), 0);	//	type
		stmt->push(cyng::make_object(obj.rtti().type_name()), 256);	//	desc
		if (stmt->execute()) {
			stmt->clear();
			return true;
		}

		return false;
	}

	bool update_config_record(cyng::db::statement_ptr stmt, cyng::object const& key, cyng::object obj)
	{
		auto const val = cyng::make_object(cyng::to_string(obj));
		stmt->push(val, 256);	//	val
		stmt->push(key, 128);	//	pk
		if (stmt->execute()) {
			stmt->clear();
			return true;
		}
		return false;
	}

	bool remove_config_record(cyng::db::statement_ptr stmt, cyng::object const& key)
	{
		stmt->push(key, 128);	//	pk
		if (stmt->execute()) {
			stmt->clear();
			return true;
		}
		return false;
	}

}