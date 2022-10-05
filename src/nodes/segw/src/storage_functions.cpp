/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <storage_functions.h>

#include <cfg.h>
#include <storage.h>

#include <config/cfg_nms.h>

#include <cyng/db/details/statement_interface.h>
#include <cyng/db/storage.h>
#include <cyng/io/ostream.h>
#include <cyng/obj/algorithm/find.h>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/container_cast.hpp>
#include <cyng/obj/numeric_cast.hpp>
#include <cyng/parse/string.h>
#include <cyng/sql/sql.hpp>
#include <cyng/sys/net.h>

#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/predef.h>
#include <boost/uuid/string_generator.hpp>

namespace smf {

    cyng::meta_store get_store_cfg() {

        return cyng::meta_store(
            "cfg",
            {
                cyng::column("path", cyng::TC_STRING), //	path, '/' separated values
                cyng::column("value", cyng::TC_NULL)   //	value (data type may vary)
            },
            1);
    }

    cyng::meta_sql get_table_cfg() {

        return cyng::meta_sql(
            "TCfg",
            {
                cyng::column_sql("path", cyng::TC_STRING, 128), //	path, '/' separated values
                //	no generation column
                cyng::column_sql("value", cyng::TC_STRING, 256), //	value - note: This is hard coded in storage::cfg_update()
                cyng::column_sql("def", cyng::TC_STRING, 256),   //	default value
                cyng::column_sql("type", cyng::TC_UINT16, 0),    //	data type code (default)
                cyng::column_sql("desc", cyng::TC_STRING, 256)   //	optional description
            },
            1);
    }

    cyng::meta_store get_store_oplog() {

        return cyng::meta_store(
            "opLog",
            {
                // cyng::column("ROWID", cyng::TC_INT64), //	index - with SQLite this prevents creating a column
                // cyng::column("idx", cyng::TC_INT64), //	index - with SQLite this prevents creating a column
                cyng::column("actTime", cyng::TC_TIME_POINT),
                cyng::column("age", cyng::TC_TIME_POINT),
                cyng::column("regPeriod", cyng::TC_UINT32), //	register period
                cyng::column("valTime", cyng::TC_UINT32),   //	val time
                cyng::column("status", cyng::TC_UINT64),    //	status word
                cyng::column("event", cyng::TC_UINT32),     //	event code
                cyng::column("peer", cyng::TC_OBIS),        //	peer address (-> u64)
                cyng::column("utc", cyng::TC_TIME_POINT),   //	UTC time
                cyng::column("serverId", cyng::TC_BUFFER),  //	server ID (meter)
                cyng::column("target", cyng::TC_STRING),    //	target name
                cyng::column("pushNr", cyng::TC_UINT8),     //	operation number
                cyng::column("details", cyng::TC_STRING)    //	description (DATA_PUSH_DETAILS)
            },
            0);
    }
    cyng::meta_sql get_table_oplog() { return cyng::to_sql(get_store_oplog(), {/*0,*/ 0, 0, 0, 0, 0, 0, 0, 0, 24, 64, 0, 128}); }

    cyng::meta_store get_store_meter() {

        return cyng::meta_store(
            "meter",
            {
                cyng::column("tag", cyng::TC_UUID),     //	SAH1 hash of "meter"
                cyng::column("meter", cyng::TC_BUFFER), //	02-e61e-03197715-3c-07, or 03197715
                cyng::column("type", cyng::TC_STRING),  //	IEC, M-Bus, wM-Bus
                cyng::column("desc", cyng::TC_STRING)   //	optional description/manufacturer
            },
            1);
    }
    cyng::meta_sql get_table_meter() { return cyng::to_sql(get_store_meter(), {0, 23, 0, 128}); }

    cyng::meta_store get_store_meter_mbus() {
        return cyng::meta_store(
            "meterMBus",
            {
                cyng::column("serverID", cyng::TC_BUFFER), //	OBIS_SERVER_ID: server/meter ID <- key
                cyng::column(
                    "lastSeen",
                    cyng::TC_TIME_POINT),                //	OBIS_CURRENT_UTC: last seen - Letzter Datensatz: 20.06.2018 14:34:22"
                cyng::column("class", cyng::TC_STRING),  //	DEVICE_CLASS: device/quality class (always "---" == 2D 2D 2D)
                cyng::column("visible", cyng::TC_BOOL),  //	active/visible
                cyng::column("status", cyng::TC_UINT32), //	"Statusinformation: 00"
                //	Contains a bit mask to define the bits of the status word, that if changed
                //	will result in an entry in the log-book.
                cyng::column("mask", cyng::TC_BUFFER), //	"Bitmaske: 00 00"
                // cyng::column("interval", cyng::TC_UINT32, 0), //	Time between two data sets: 49000
                //	--- optional data
                cyng::column("pubKey", cyng::TC_BUFFER),   //	Public Key: "18 01 16 05 E6 1E 0D 02 BF 0C FA 35 7D 9E 77 03"
                cyng::column("aes", cyng::TC_AES128),      //	AES-Key
                cyng::column("secondary", cyng::TC_BUFFER) //	secondary server/meter
            },
            1);
    }
    cyng::meta_sql get_table_meter_mbus() {
        //
        //  SELECT hex(serverID), hex(secondary), datetime(lastSeen), visible, status, hex(aes) FROM TMeterMBus;
        //
        return cyng::to_sql(get_store_meter_mbus(), {9, 0, 16, 0, 0, 0, 16, 32, 9});
    }

    cyng::meta_store get_store_meter_iec() {
        return cyng::meta_store(
            "meterIEC",
            {
                cyng::column("meterID", cyng::TC_BUFFER), //	max. 32 bytes (8181C7930AFF)
                cyng::column("address", cyng::TC_BUFFER), //	mostly the same as meterID (8181C7930CFF)
                cyng::column("descr", cyng::TC_STRING),
                cyng::column("baudrate", cyng::TC_UINT32), //	9600, ... (in opening sequence) (8181C7930BFF)
                cyng::column("p1", cyng::TC_STRING),       //	login password (8181C7930DFF)
                                                           //, "p2"		//	login password
                cyng::column("w5", cyng::TC_STRING)        //	W5 password (reserved for national applications) (8181C7930EFF)
            },
            1);
    }
    cyng::meta_sql get_table_meter_iec() { return cyng::to_sql(get_store_meter_iec(), {32, 32, 128, 0, 32, 32}); }

    cyng::meta_store get_store_push_ops() {
        return cyng::meta_store(
            "pushOps",
            {
                cyng::column("meterID", cyng::TC_BUFFER), // server/meter/sensor ID
                cyng::column("nr", cyng::TC_UINT8),       // index - starts with 1
                // -- body
                cyng::column("interval", cyng::TC_SECOND), // (81 81 C7 8A 02 FF - PUSH_INTERVAL) push interval in seconds
                cyng::column("delay", cyng::TC_SECOND),    // (81 81 C7 8A 04 FF - PUSH_DELAY) delay in seconds
                //	* 81 81 C7 8A 42 FF == profile (PUSH_SOURCE_PROFILE)
                //	* 81 81 C7 8A 43 FF == installation parameters (PUSH_SOURCE_INSTALL)
                //	* 81 81 C7 8A 44 FF == list of visible sensors/actors (PUSH_SOURCE_SENSOR_LIST)
                cyng::column("source", cyng::TC_OBIS),      // (81 81 c7 8a 04 ff - PUSH_SOURCE) source type - mostly
                                                            // PUSH_SOURCE_SENSOR_LIST (81 81 C7 8A 44 FF)
                cyng::column("target", cyng::TC_STRING),    // (81 47 17 07 00 FF - PUSH_TARGET) target name
                cyng::column("service", cyng::TC_OBIS),     // (81 49 00 00 10 FF - PUSH_SERVICE) push service (= PUSH_SERVICE_IPT)
                cyng::column("profile", cyng::TC_OBIS),     // (81 81 C7 8A 83 FF - PROFILE) 5, 15, 60, 1440 minutes
                cyng::column("task", cyng::TC_UINT64),      // push task id
                cyng::column("offset", cyng::TC_TIME_POINT) // last push time
            },
            2);
    }
    cyng::meta_sql get_table_push_ops() {
        //
        // SELECT hex(meterID), nr, printf('%012X', service) FROM TPushOps;
        //
        // SELECT hex(TPushOps.meterID), TPushOps.target, TPushOps.nr, TPushRegister.register FROM TPushOps INNER JOIN TPushRegister
        // ON TPushRegister.meterID = TPushOps.meterID AND TPushRegister.nr = TPushOps.nr;
        //
        return cyng::to_sql(get_store_push_ops(), {9, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    }

    cyng::meta_store get_store_push_register() {
        return cyng::meta_store(
            "pushRegister",
            {
                cyng::column("meterID", cyng::TC_BUFFER), // server/meter/sensor ID
                cyng::column("nr", cyng::TC_UINT8),       // index - starts with 1
                cyng::column("idx", cyng::TC_UINT8),      // The register codes have an index by it's own
                // -- body
                cyng::column("register", cyng::TC_OBIS) // OBIS code
            },
            3);
    }
    cyng::meta_sql get_table_push_register() {

        //
        //  select hex(meterID), nr, idx, printf('%012X', register) from TPushRegister;
        //
        return cyng::to_sql(get_store_push_register(), {9, 0, 0, 0});
    }

    cyng::meta_store get_store_data_collector() {
        //
        //  The index allows multiple entries of the same meter id.
        //
        return cyng::meta_store(
            "dataCollector",
            {
                cyng::column("meterID", cyng::TC_BUFFER), // server/meter/sensor ID
                cyng::column("nr", cyng::TC_UINT8),       // index - starts with 1
                //  -- body
                cyng::column("profile", cyng::TC_OBIS),   // type 1min, 15min, 1h, ... (OBIS_PROFILE)
                cyng::column("active", cyng::TC_BOOL),    // turned on / off(OBIS_DATA_COLLECTOR_ACTIVE)
                cyng::column("maxSize", cyng::TC_UINT32), // max entry count (OBIS_DATA_COLLECTOR_SIZE)
                cyng::column(
                    "regPeriod",
                    cyng::TC_SECOND) //	register period - if 0, recording is event-driven (OBIS_DATA_REGISTER_PERIOD)
            },
            2);
    }
    cyng::meta_sql get_table_data_collector() {
        //
        //  SELECT hex(meterID), nr, printf('%012X', profile), active FROM TDataCollector;
        //
        return cyng::to_sql(get_store_data_collector(), {9, 0, 0, 0, 0, 0});
    }

    cyng::meta_store get_store_data_mirror() {
        return cyng::meta_store(
            "dataMirror",
            {
                cyng::column("meterID", cyng::TC_BUFFER), // server/meter/sensor ID
                cyng::column("nr", cyng::TC_UINT8),       // index - starts with 1
                cyng::column("idx", cyng::TC_UINT8),      // The register codes have an index by it's own
                //   -- body
                cyng::column("register", cyng::TC_OBIS) // OBIS code
            },
            3);
    }
    cyng::meta_sql get_table_data_mirror() {
        //
        //  SELECT hex(meterID), nr, idx, printf('%012X', register) FROM TDataMirror;
        //
        return cyng::to_sql(get_store_data_mirror(), {9, 0, 0, 0});
    }

    cyng::meta_store get_store_readout() {
        return cyng::meta_store(
            "readout",
            {
                cyng::column("meterID", cyng::TC_BUFFER), // server/meter/sensor ID
                //   -- body
                cyng::column("payload", cyng::TC_BUFFER),      // raw (encrypted)
                cyng::column("ci", cyng::TC_UINT8),            // frame type (CI field - M-Bus only)
                cyng::column("received", cyng::TC_TIME_POINT), // receiving time
                cyng::column("secIdx", cyng::TC_UINT32),       // seconds index (gateway)
                cyng::column("decrypted", cyng::TC_BOOL)       // if true readout data available
            },
            1);
    }

    cyng::meta_store get_store_readout_data() {
        return cyng::meta_store(
            "readoutData",
            {
                cyng::column("meterID", cyng::TC_BUFFER), // server/meter/sensor ID
                cyng::column("register", cyng::TC_OBIS),  // OBIS code,
                                                          //   -- body
                cyng::column("reading", cyng::TC_STRING), // value as string
                cyng::column("type", cyng::TC_UINT16),    // data type code
                cyng::column("scaler", cyng::TC_INT8),    // decimal place
                cyng::column("unit", cyng::TC_UINT8),     // physical unit
                cyng::column("status", cyng::TC_UINT32)   // status
            },
            2);
    }

    cyng::meta_store get_store_mirror() {
        return cyng::meta_store(
            "mirror",
            {
                cyng::column("meterID", cyng::TC_BUFFER), // server/meter/sensor ID
                cyng::column("profile", cyng::TC_OBIS),   // load profile
                cyng::column("idx", cyng::TC_INT64),      // time stamp index (with offset)
                //   -- body
                cyng::column("payload", cyng::TC_BUFFER),      // raw (encrypted)
                cyng::column("ci", cyng::TC_UINT8),            // frame type (CI field - M-Bus only)
                cyng::column("received", cyng::TC_TIME_POINT), // receiving time
                cyng::column("secIdx", cyng::TC_UINT32),       // seconds index (gateway)
                cyng::column("decrypted", cyng::TC_BOOL)       // if true readout data available
            },
            3);
    }

    cyng::meta_sql get_table_mirror() {
        //
        //  SELECT hex(meterID), profile, idx, datetime(received), secIdx FROM TMirror;
        //
        return cyng::to_sql(get_store_mirror(), {9, 0, 0, 0, 0, 0, 0, 0});
    }

    cyng::meta_store get_store_mirror_data() {
        return cyng::meta_store(
            "mirrorData",
            {
                cyng::column("meterID", cyng::TC_BUFFER), // server/meter/sensor ID
                cyng::column("profile", cyng::TC_OBIS),   // load profile
                cyng::column("idx", cyng::TC_INT64),      // time stamp index (with offset)
                cyng::column("register", cyng::TC_OBIS),  // OBIS code (data type)
                //   -- body
                cyng::column("reading", cyng::TC_STRING), // value as string
                cyng::column("type", cyng::TC_UINT16),    // data type code
                cyng::column("scaler", cyng::TC_INT8),    // decimal place
                cyng::column("unit", cyng::TC_UINT8),     // physical unit
                cyng::column("status", cyng::TC_UINT32)   // status
            },
            4);
    }
    cyng::meta_sql get_table_mirror_data() {
        //
        // SELECT hex(TMirrorData.meterID), TMirrorData.profile, TMirrorData.register, TMirrorData.reading FROM TMirrorData;
        //
        // SELECT hex(TMirrorData.meterID), datetime(TMirror.received), TMirrorData.profile, TMirrorData.register,
        // TMirrorData.reading FROM TMirrorData INNER JOIN TMirror ON TMirror.meterID = TMirrorData.meterID AND TMirror.profile =
        // TMirrorData.profile AND TMirror.idx = TMirrorData.idx ORDER BY TMirror.received;
        //
        return cyng::to_sql(get_store_mirror_data(), {9, 0, 0, 0, 256, 0, 0, 0, 0});
    }

    cyng::meta_store get_store_mbus_cache() {
        return cyng::meta_store(
            "mbusCache",
            {
                cyng::column("meterID", cyng::TC_BUFFER), // server/meter/sensor ID
                //   -- body
                cyng::column("payload", cyng::TC_BUFFER),     // raw (encrypted)
                cyng::column("ci", cyng::TC_UINT8),           // frame type (CI field - paket type)
                cyng::column("flag", cyng::TC_UINT16),        // manufacturer code
                cyng::column("version", cyng::TC_UINT8),      // version
                cyng::column("medium", cyng::TC_UINT8),       // medium
                cyng::column("received", cyng::TC_TIME_POINT) // receiving time
            },
            1);
    }
    cyng::meta_sql get_table_mbus_cache() { return cyng::to_sql(get_store_mbus_cache(), {9, 512, 0, 0, 0, 0, 0}); }

    std::vector<cyng::meta_store> get_store_meta_data() {
        return {
            get_store_cfg(),            // cfg
            get_store_oplog(),          // opLog
            get_store_meter_mbus(),     // meterMBus
            get_store_meter_iec(),      // meterIEC
            get_store_data_collector(), // dataCollector
            get_store_data_mirror(),    // dataMirror
            get_store_push_ops(),       // pushOps
            get_store_push_register(),  // pushRegister
            get_store_readout(),        // readout - ephemeral
            get_store_readout_data(),   // readoutData - ephemeral
            get_store_mirror(),         // mirror
            get_store_mirror_data(),    // mirrorData
            get_store_mbus_cache()      // mbusCache
        };
    }

    std::vector<cyng::meta_sql> get_sql_meta_data() {

        return {
            get_table_cfg(),            // TCfg
            get_table_oplog(),          // TOpLog
            get_table_meter_mbus(),     //  TMeterMBus
            get_table_meter_iec(),      // TMeterIEC
            get_table_data_collector(), // TDataCollector
            get_table_data_mirror(),    // TDataMirror
            get_table_push_ops(),       // TPushOps
            get_table_push_register(),  // TPushRegister
            get_table_mirror(),         // Tmirror
            get_table_mirror_data(),    // TMirrorData
            get_table_mbus_cache()      // TMbusCache
        };
    }

    cyng::meta_sql get_sql_meta_data(std::string name) {
        if (boost::algorithm::equals(name, "meterMBus")) {
            return get_table_meter_mbus();
        } else if (boost::algorithm::equals(name, "meterIEC")) {
            return get_table_meter_iec();
        } else if (boost::algorithm::equals(name, "opLog")) {
            return get_table_oplog();
        } else if (boost::algorithm::equals(name, "dataCollector")) {
            return get_table_data_collector();
        } else if (boost::algorithm::equals(name, "dataMirror")) {
            return get_table_data_mirror();
        } else if (boost::algorithm::equals(name, "pushOps")) {
            return get_table_push_ops();
        } else if (boost::algorithm::equals(name, "pushRegister")) {
            return get_table_push_register();
        } else if (boost::algorithm::equals(name, "mirror")) {
            return get_table_mirror();
        } else if (boost::algorithm::equals(name, "mirrorData")) {
            return get_table_mirror_data();
        }
        BOOST_ASSERT_MSG(false, "table not found");
        return cyng::meta_sql(name, {}, 0);
    }

    bool init_storage(cyng::db::session &db) {
        BOOST_ASSERT(db.is_alive());

        try {

            //
            //	start transaction
            //
            cyng::db::transaction trx(db);

            auto const vec = get_sql_meta_data();
            for (auto const &m : vec) {
                auto const sql = cyng::sql::create(db.get_dialect(), m).to_string();
                if (!db.execute(sql)) {
                    std::cerr << "**error: " << sql << std::endl;
                } else {
                    std::cerr << "**info: " << sql << std::endl;
                }
            }

            return true;
        } catch (std::exception const &ex) {
            boost::ignore_unused(ex);
            std::cerr << "**error: " << ex.what() << std::endl;
        }
        return false;
    }

    void clear_config(cyng::db::session &db) {
        BOOST_ASSERT(db.is_alive());

        auto const m = get_table_cfg();
        auto const sql = cyng::sql::remove(db.get_dialect(), m)();
        db.execute(sql);
    }

    std::string get_section(std::string path, std::size_t idx) {

        auto const vec = cyng::split(path, "/");
        return (idx < vec.size()) ? vec.at(idx) : path;
    }

    void list_config(cyng::db::session &db) {
        BOOST_ASSERT(db.is_alive());

        auto const ms = get_table_cfg();
        auto const sql = cyng::sql::select(db.get_dialect(), ms).all(ms, false).from().order_by("path")();
        auto stmt = db.create_statement();
        stmt->prepare(sql);

        //
        //	read all results
        //
        std::string section{"000000000000"};

        while (auto res = stmt->get_result()) {
            auto const rec = cyng::to_record(ms, res);
            // std::cout << rec.to_tuple() << std::endl;
            auto const path = rec.value("path", "");
            auto const val = rec.value("value", "");
            auto const def = rec.value("def", "");
            auto const type = rec.value("type", static_cast<std::uint16_t>(0));
            auto const desc = rec.value("desc", "");
            try {
                auto const obj = cyng::restore(val, type);

                //	ToDo:
                // auto const opath = sml::translate_obis_path(path);

                //
                //	insert split lines between sections
                //
                auto const this_section = get_section(path);
                if (!boost::algorithm::equals(section, this_section)) {

                    //
                    //	update section name
                    //
                    section = this_section;

                    //
                    //  print new section name
                    //
                    std::cout << std::string(42, '-') << "  [" << section << ']' << std::endl;
                }

                std::cout << std::setfill('.') << std::setw(42) << std::left << path << ": ";

                //
                //	list value (optional default value)
                //
                if (boost::algorithm::equals(val, def)) {
                    std::cout << val;
                } else {
                    std::cout << '[' << val << '/' << def << ']';
                }

                std::cout << " (" << obj << ':' << obj.rtti().type_name() << ") - " << desc;
            } catch (std::exception const &ex) {
                std::cout << std::setfill('.') << std::setw(42) << std::left << path << ": ***error " << ex.what();
            }

            //
            //	complete
            //
            std::cout << std::endl;
        }
    }

    bool set_config_value(cyng::db::session &db, std::string const &path, std::string const &value, std::string const &type) {

        storage store(db);

        auto const obj = cyng::make_object(path);

        if (boost::algorithm::equals(type, "bool") || boost::algorithm::equals(type, "b") ||
            boost::algorithm::equals(type, "boolean")) {
            return store.cfg_update(obj, cyng::make_object(boost::algorithm::equals(value, "true")));
        } else if (boost::algorithm::equals(type, "u8")) {
            return store.cfg_update(obj, cyng::make_object(static_cast<std::uint8_t>(std::stoul(value))));
        } else if (boost::algorithm::equals(type, "u16")) {
            return store.cfg_update(obj, cyng::make_object(static_cast<std::uint16_t>(std::stoul(value))));
        } else if (boost::algorithm::equals(type, "u32")) {
            return store.cfg_update(obj, cyng::make_object(static_cast<std::uint32_t>(std::stoul(value))));
        } else if (boost::algorithm::equals(type, "u64")) {
            return store.cfg_update(obj, cyng::make_object(static_cast<std::uint64_t>(std::stoull(value))));
        } else if (boost::algorithm::equals(type, "i8")) {
            return store.cfg_update(obj, cyng::make_object(static_cast<std::int8_t>(std::stol(value))));
        } else if (boost::algorithm::equals(type, "i16")) {
            return store.cfg_update(obj, cyng::make_object(static_cast<std::int16_t>(std::stol(value))));
        } else if (boost::algorithm::equals(type, "i32")) {
            return store.cfg_update(obj, cyng::make_object(static_cast<std::int32_t>(std::stol(value))));
        } else if (boost::algorithm::equals(type, "i64")) {
            return store.cfg_update(obj, cyng::make_object(static_cast<std::int64_t>(std::stoll(value))));
        } else if (boost::algorithm::equals(type, "s")) {
            return store.cfg_update(obj, cyng::make_object(value));
        } else if (boost::algorithm::equals(type, "chrono:sec")) {
            return store.cfg_update(obj, cyng::make_object(std::chrono::seconds(std::stoull(value))));
        } else if (boost::algorithm::equals(type, "chrono:min")) {
            return store.cfg_update(obj, cyng::make_object(std::chrono::minutes(std::stoull(value))));
        } else if (boost::algorithm::equals(type, "ip:address")) {
            boost::system::error_code ec;
            return store.cfg_update(obj, cyng::make_object(boost::asio::ip::make_address(value, ec)));
        } else {
            std::cerr << "supported data types: [bool] [u8] [u16] [u32] [u64] [i8] [i16] [i32] [i64] [s] [chrono:sec] [chrono:min] "
                         "[ip:address]"
                      << std::endl;
        }

        return false;
    }

    bool add_config_value(cyng::db::session &db, std::string const &path, std::string const &value, std::string const &type) {

        storage store(db);

        auto const obj = cyng::make_object(path);

        if (boost::algorithm::equals(type, "bool") || boost::algorithm::equals(type, "b") ||
            boost::algorithm::equals(type, "boolean")) {
            return store.cfg_insert(obj, cyng::make_object(boost::algorithm::equals(value, "true")));
        } else if (boost::algorithm::equals(type, "u8")) {
            return store.cfg_insert(obj, cyng::make_object(static_cast<std::uint8_t>(std::stoul(value))));
        } else if (boost::algorithm::equals(type, "u16")) {
            return store.cfg_insert(obj, cyng::make_object(static_cast<std::uint16_t>(std::stoul(value))));
        } else if (boost::algorithm::equals(type, "u32")) {
            return store.cfg_insert(obj, cyng::make_object(static_cast<std::uint32_t>(std::stoul(value))));
        } else if (boost::algorithm::equals(type, "u64")) {
            return store.cfg_insert(obj, cyng::make_object(static_cast<std::uint64_t>(std::stoull(value))));
        } else if (boost::algorithm::equals(type, "i8")) {
            return store.cfg_insert(obj, cyng::make_object(static_cast<std::int8_t>(std::stol(value))));
        } else if (boost::algorithm::equals(type, "i16")) {
            return store.cfg_insert(obj, cyng::make_object(static_cast<std::int16_t>(std::stol(value))));
        } else if (boost::algorithm::equals(type, "i32")) {
            return store.cfg_insert(obj, cyng::make_object(static_cast<std::int32_t>(std::stol(value))));
        } else if (boost::algorithm::equals(type, "i64")) {
            return store.cfg_insert(obj, cyng::make_object(static_cast<std::int64_t>(std::stoll(value))));
        } else if (boost::algorithm::equals(type, "s")) {
            return store.cfg_insert(obj, cyng::make_object(value));
        } else if (boost::algorithm::equals(type, "chrono:sec")) {
            return store.cfg_insert(obj, cyng::make_object(std::chrono::seconds(std::stoull(value))));
        } else if (boost::algorithm::equals(type, "chrono:min")) {
            return store.cfg_insert(obj, cyng::make_object(std::chrono::minutes(std::stoull(value))));
        } else if (boost::algorithm::equals(type, "ip:address")) {
            boost::system::error_code ec;
            return store.cfg_insert(obj, cyng::make_object(boost::asio::ip::make_address(value, ec)));
        } else {
            std::cerr << "supported data types: [bool] [u8] [u16] [u32] [u64] [i8] [i16] [i32] [i64] [s] [chrono:sec] [chrono:min] "
                         "[ip:address]"
                      << std::endl;
        }

        return false;
    }

    bool del_config_value(cyng::db::session &db, std::string const &path) {
        storage store(db);

        auto const obj = cyng::make_object(path);
        return store.cfg_remove(obj);
    }

    bool alter_table(cyng::db::session &db, std::string const &name) {
        storage store(db);
        try {
            //
            //	start transaction
            //
            cyng::db::transaction trx(db);

            //
            //	search specified table
            //
            auto const vec = get_sql_meta_data();
            auto const pos = std::find_if(std::begin(vec), std::end(vec), [&](cyng::meta_sql const &m) {
                return boost::algorithm::equals(m.get_name(), name);
            });
            if (pos != vec.end()) {
                recreate_table(db, *pos);
            } else {
                //
                //  ToDo: take credit of non-cached tables
                //
                std::cerr << "**error: " << name << " not found" << std::endl;
            }

        } catch (std::exception const &ex) {
            boost::ignore_unused(ex);
            std::cerr << "**error: " << ex.what() << std::endl;
        }
        return false;
    }

    bool recreate_table(cyng::db::session &db, cyng::meta_sql const &m) {
        //
        //  drop table
        //
        auto const sql_drop = cyng::sql::drop(db.get_dialect(), m).to_string();
        if (!db.execute(sql_drop)) {
            std::cerr << "**error: " << sql_drop << std::endl;
        } else {
            std::cout << m.get_name() << " - " << sql_drop << std::endl;

            //
            //  create table
            //
            auto const sql_create = cyng::sql::create(db.get_dialect(), m).to_string();
            if (!db.execute(sql_create)) {
                std::cerr << "**error: " << sql_create << std::endl;
            } else {
                std::cout << m.get_name() << " - " << sql_create << std::endl;
                return true;
            }
        }
        return false;
    }

    bool set_nms_mode(cyng::db::session &db, std::string const &mode) {
        storage store(db);

        //  nms/nic..............: Ethernet (Ethernet:s) - NMS: nic
        //  nms/nic-index........: 9 (9:i64) - NMS: nic-index
        //  nms/nic-ipv4.........: 192.168.1.82 (192.168.1.82:ip:address) - IPv4 address of NMS adapter
        //  nms/nic-linklocal....: fe80::1566:fadd:d93e:2576 (fe80::1566:fadd:d93e:2576:ip:address) - link-local address
        //  nms/port.............: 1d8a (1d8a:u16) - default NMS listener port (7562)
        if (boost::algorithm::starts_with(mode, "prod")) {
            auto const nic_def = get_nic();
            auto const nic = cyng::value_cast(store.cfg_read(cyng::make_object("nms/nic"), cyng::make_object(nic_def)), nic_def);
            auto const linklocal = get_ipv6_linklocal(nic); //  std::pair<boost::asio::ip::address, std::uint32_t>

            //
            //  read "nms/nic-linklocal" and "nms/nic-index"
            //
            auto obj = store.cfg_read(cyng::make_object("nms/nic-linklocal"), cyng::make_object(linklocal.first));
            auto const address = cyng::value_cast<boost::asio::ip::address>(obj, linklocal.first);
            BOOST_ASSERT_MSG(address.is_v6(), "not an IPv6 address");

            //
            //  build a link-local address
            //
            auto const link_local_addr = cyng::sys::make_link_local_address(address, linklocal.second);
#ifdef _DEBUG
            std::cout << "nms/address = " << link_local_addr << std::endl;
#endif

            //
            //  set "nms/nic-index" to new index
            //
            store.cfg_update(cyng::make_object("nms/nic-index"), cyng::make_object(linklocal.second));

            //
            //  set "nms/address" to link local address
            //
            store.cfg_update(cyng::make_object("nms/mode"), cyng::make_object("production"));
            return store.cfg_update(cyng::make_object("nms/address"), cyng::make_object(link_local_addr));

        } else if (boost::algorithm::equals(mode, "test")) {
            //
            //  set "nms/address" to "0.0.0.0"
            //
            store.cfg_update(cyng::make_object("nms/mode"), cyng::make_object(mode));
            boost::system::error_code ec;
            return store.cfg_update(
                cyng::make_object("nms/address"), cyng::make_object(boost::asio::ip::make_address("0.0.0.0", ec)));
        } else if (boost::algorithm::equals(mode, "local")) {
            //
            //  set "nms/address" to local IPv4 address
            //
            auto const nic_def = get_nic();
            auto const nic = cyng::value_cast(store.cfg_read(cyng::make_object("nms/nic"), cyng::make_object(nic_def)), nic_def);
            auto const address = get_ipv4_address(nic); //  boost::asio::ip::address
            BOOST_ASSERT_MSG(address.is_v4(), "not an IPv4 address");
#ifdef _DEBUG
            std::cout << "nms/address = " << address << std::endl;
#endif
            //
            //  set "nms/address" to local IPv4 address
            //
            store.cfg_update(cyng::make_object("nms/mode"), cyng::make_object(mode));
            return store.cfg_update(cyng::make_object("nms/address"), cyng::make_object(address));
        }
        return false;
    }

    bool insert_config_record(
        cyng::db::statement_ptr stmt_insert,
        cyng::db::statement_ptr stmt_select,
        std::string key,
        cyng::object obj,
        std::string desc) {
        //
        //	use already prepared statements
        //
        if (key.empty()) {
            std::cerr << "***error  : empty key" << std::endl;
            return false;
        } else {
            // std::cout << "***info   : pk = " << key << std::endl;
        }
        {
            //  key must be cloned
            stmt_select->push(cyng::make_object(key.substr()), 128); //	pk
            auto res = stmt_select->get_result();
            if (res) {
                //"value"
                auto const val = cyng::value_cast(res->get(2, cyng::TC_STRING, 256), "");
                // std::cout << "previous value is: " << val_prev << std::endl;
                std::cout << "***warning: " << key;
                std::cout << std::setfill('.') << std::setw((key.size() < 42) ? (42 - key.size()) : 1) << "= ";
                std::cout << val << " already in use" << std::endl;
                return false;
            } else {
                // std::cout << "***info   : " << key << " is not used" << std::endl;
            }
            stmt_select->clear();
        }

        {
            auto const val = cyng::make_object(cyng::to_string(obj));

            //
            //  logging
            //
            std::cout << key;
            std::cout << std::setfill('.') << std::setw((key.size() < 42) ? (42 - key.size()) : 1) << ": ";
            std::cout << val;
            if (!desc.empty()) {

                std::cout << " (" << desc << ")";
            }
            std::cout << std::endl;

            //  no "gen" column
            //  key will be moved
            stmt_insert->push(cyng::make_object(key), 128);     //	pk
            stmt_insert->push(val, 256);                        //	val
            stmt_insert->push(val, 256);                        //	def
            stmt_insert->push(cyng::make_object(obj.tag()), 0); //	type
            stmt_insert->push(cyng::make_object(desc), 256);    //	desc
            if (stmt_insert->execute()) {
                stmt_insert->clear();
                return true;
            }
        }
        return false;
    }

    bool insert_config_record(cyng::db::statement_ptr stmt, cyng::object const &key, cyng::object obj) {

        auto const val = cyng::make_object(cyng::to_string(obj));

        stmt->push(key, 128);                                       //	pk
        stmt->push(val, 256);                                       //	val
        stmt->push(val, 256);                                       //	def
        stmt->push(cyng::make_object(obj.tag()), 0);                //	type
        stmt->push(cyng::make_object(obj.rtti().type_name()), 256); //	desc
        if (stmt->execute()) {
            stmt->clear();
            return true;
        }

        return false;
    }

    bool update_config_record(cyng::db::statement_ptr stmt, cyng::object const &key, cyng::object obj) {
        auto const val = cyng::make_object(cyng::to_string(obj));
        stmt->push(val, 256); //	val
        stmt->push(key, 128); //	pk
        if (stmt->execute()) {
            stmt->clear();
            return true;
        }
        return false;
    }

    bool remove_config_record(cyng::db::statement_ptr stmt, cyng::object const &key) {
        stmt->push(key, 128); //	pk
        if (stmt->execute()) {
            stmt->clear();
            return true;
        }
        return false;
    }

    cyng::object read_config_record(cyng::db::statement_ptr stmt, cyng::object const &key) {
        stmt->push(key, 128); //	pk
        auto const res = stmt->get_result();
        if (res) {
            auto const ms = get_table_cfg();
            auto const rec = cyng::to_record(ms, res);
            auto const val = rec.value("value", "");
            auto const type = rec.value("type", static_cast<std::uint16_t>(0));
            return cyng::restore(val, type);
        }
        return cyng::make_object();
    }

} // namespace smf
