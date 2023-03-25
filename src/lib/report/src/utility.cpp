
#include <smf/report/utility.h>

#include <smf/config/schemes.h>
#include <smf/mbus/server_id.h>
#include <smf/mbus/units.h>
#include <smf/obis/defs.h>
#include <smf/obis/profile.h>

#include <cyng/db/storage.h>
#include <cyng/io/ostream.h>
#include <cyng/io/serialize.h>
#include <cyng/obj/buffer_cast.hpp>
#include <cyng/obj/intrinsics/date.h>
#include <cyng/sql/dsl.h>
#include <cyng/sql/dsl/placeholder.h>
#include <cyng/sql/sql.hpp>

#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#ifdef _DEBUG
#include <fstream>
#endif

#include <cmath>
#include <iostream>

namespace smf {
    std::map<cyng::obis, sml_data> select_readout_data(cyng::db::session db, boost::uuids::uuid tag, std::uint32_t status) {

        // std::vector<sml_data> readout;
        std::map<cyng::obis, sml_data> readout;

        auto const ms = config::get_table_sml_readout_data();
        std::string const sql = "SELECT tag, register, gen, reading, type, scaler, unit FROM TSMLreadoutData WHERE tag = ?";
        auto stmt = db.create_statement();
        std::pair<int, bool> const r = stmt->prepare(sql);
        if (r.second) {
            stmt->push(cyng::make_object(tag), 0); //	tag
            while (auto res = stmt->get_result()) {
                auto const rec = cyng::to_record(ms, res);
#ifdef _DEBUG
                // using cyng::operator<<;
                // std::cout << rec.to_string() << std::endl;
#endif
                auto const code = rec.value<std::uint16_t>("type", cyng::TC_STRING);
                auto const val = rec.value("reading", "");
                auto const scaler = rec.value<std::int8_t>("scaler", 0);
                auto const reg = rec.value("register", OBIS_DATA_COLLECTOR_REGISTER);
                auto const unit = rec.value<std::uint8_t>("unit", 0u);

                // readout.emplace_back(reg, code, scaler, unit, val);
                auto const pos = readout.emplace(
                    std::piecewise_construct, std::forward_as_tuple(reg), std::forward_as_tuple(code, scaler, unit, val, status));
#ifdef _DEBUG
                // std::cout << tag << ": " << pos.first->second.restore() << ", value: " << val << ", scaler: " << +scaler
                //           << std::endl;
#endif
            }
        }

        return readout;
    }

    std::size_t loop_readout_data(cyng::db::session db, cyng::obis profile, cyng::date start, cb_loop_readout cb) {

        std::size_t counter = 0;

        //
        // Collect all data from "start" for all devices of the specified profile ordered by readout time (actTime).
        //
        // Parameters are:
        //
        // * start time
        // * profile
        //
        std::string const sql =
            "SELECT TSMLReadout.tag, TSMLReadout.meterID, datetime(actTime), TSMLReadout.status, "
            "TSMLReadoutData.register, TSMLReadoutData.reading, TSMLReadoutData.type, TSMLReadoutData.scaler, TSMLReadoutData.unit FROM "
            "TSMLReadout INNER JOIN TSMLReadoutData ON TSMLReadout.tag = TSMLReadoutData.tag "
            "WHERE TSMLReadout.actTime > julianday(?) AND TSMLReadout.profile = ? "
            "ORDER BY actTime, TSMLReadout.tag";
        auto stmt = db.create_statement();
        std::pair<int, bool> const r = stmt->prepare(sql);
        if (r.second) {
            stmt->push(cyng::make_object(start), 0);   //	start time
            stmt->push(cyng::make_object(profile), 0); //	select one profile at a time

            boost::uuids::uuid prev_tag = boost::uuids::nil_uuid();

            while (auto res = stmt->get_result()) {
                //  [1] tag,
                //  [2] meterID,
                //  [3] actTime,
                //  [4] status,
                //  [5] register,
                //  [6] reading,
                //  [7] type,
                //  [8] scaler,
                //  [9] unit
                auto const tag = cyng::value_cast(res->get(1, cyng::TC_UUID, 0), prev_tag);
                BOOST_ASSERT(!tag.is_nil());

                //
                //  true if one record is complete
                //
                bool next_record = false;
                if (tag != prev_tag) {
                    prev_tag = tag;
                    next_record = true;
                }
                ++counter;

                //  [2] meterID
                auto const meter = cyng::to_buffer(res->get(2, cyng::TC_BUFFER, 0));
                auto const id = to_srv_id(meter); //  different formats are possible

                //  [3] actTime
                auto const act_time = cyng::value_cast(res->get(3, cyng::TC_DATE, 0), start);
                BOOST_ASSERT(act_time >= start);

                //  [4] status
                auto const status = cyng::value_cast<std::uint32_t>(res->get(4, cyng::TC_UINT32, 0), 0u);
                //  [5] register
                auto const reg = cyng::value_cast(res->get(5, cyng::TC_OBIS, 0), OBIS_DATA_COLLECTOR_REGISTER);
                //  [6] reading
                auto const value = cyng::value_cast(res->get(6, cyng::TC_STRING, 0), "?");
                //  [7] type
                auto const code = cyng::value_cast<std::uint16_t>(res->get(7, cyng::TC_UINT16, 0), cyng::TC_STRING);
                //  [8] scaler
                auto const scaler = cyng::value_cast<std::int8_t>(res->get(8, cyng::TC_INT8, 0), 0);
                //  [9] unit
                auto const unit = mbus::to_unit(cyng::numeric_cast<std::uint8_t>(res->get(9, cyng::TC_UINT8, 0), 0u));

                if (!cb(next_record, tag, id, act_time, status, reg, value, code, scaler, unit)) {
                    break;
                }
            }
        }
        return counter;
    }

    std::size_t loop_readout(cyng::db::session db, cyng::obis profile, cyng::date start, cb_loop_meta cb) {

        std::size_t counter = 0;

        std::string const sql =
            "SELECT meterID, datetime(actTime) FROM TSMLReadout WHERE profile = ? AND actTime > julianday(?) ORDER BY actTime";
        auto stmt = db.create_statement();
        std::pair<int, bool> const r = stmt->prepare(sql);
        if (r.second) {
            stmt->push(cyng::make_object(profile), 0); // profile
            stmt->push(cyng::make_object(start), 0);   // start time

            while (auto res = stmt->get_result()) {
                //  [1] meterID
                auto const meter = cyng::to_buffer(res->get(1, cyng::TC_BUFFER, 0));
                auto const id = to_srv_id(meter);
                //  [2] actTime
                auto const act_time = cyng::value_cast(res->get(2, cyng::TC_DATE, 0), start);
                BOOST_ASSERT(act_time >= start);
                ++counter;

                if (!cb(id, act_time)) {
                    break;
                }
            }
        }

        return counter;
    }

    bool has_passed(cyng::obis reg, cyng::obis_path_t const &filter) {
        //
        // apply filter only if not empty
        // The filter contains allowed registers (OBIS)
        //
        if (!filter.empty()) {
            return std::find(filter.begin(), filter.end(), reg) != filter.end();
        }
        return true;
    }

    std::set<cyng::obis> collect_profiles(std::map<std::uint64_t, std::map<cyng::obis, sml_data>> const &data) {
        std::set<cyng::obis> regs;
        for (auto const d : data) {
            for (auto const &v : d.second) {
                regs.insert(v.first);
            }
        }
        return regs;
    }

    std::string get_filename(std::string prefix, cyng::obis profile, srv_id_t srv_id, cyng::date const &start) {

        // auto const d = cyng::date::make_date_from_local_time(start);
        std::stringstream ss;
        ss << prefix << sml::get_prefix(profile) << "-" << to_string(srv_id) << '_' << cyng::as_string(start, "%Y-%m-%d") << ".csv";
        return ss.str();
    }

    std::string get_filename(std::string prefix, cyng::obis profile, cyng::date const &start) {

        // auto const d = cyng::date::make_date_from_local_time(start);
        std::stringstream ss;
        ss << prefix << sml::get_prefix(profile) << "-" << cyng::as_string(start, "%Y-%m-%d") << ".csv";
        return ss.str();
    }

    // std::string get_prefix(cyng::obis profile) {
    //     //
    //     //  the prefox should be a valid file name
    //     //
    //     switch (profile.to_uint64()) {
    //     case CODE_PROFILE_1_MINUTE: return "1min";
    //     case CODE_PROFILE_15_MINUTE: return "15min";
    //     case CODE_PROFILE_60_MINUTE: return "1h";
    //     case CODE_PROFILE_24_HOUR: return "1d";
    //     case CODE_PROFILE_LAST_2_HOURS: return "2h";
    //     case CODE_PROFILE_LAST_WEEK: return "1w";
    //     case CODE_PROFILE_1_MONTH: return "1mon";
    //     case CODE_PROFILE_1_YEAR: return "1y";
    //     case CODE_PROFILE_INITIAL: return "init";
    //     default: break;
    //     }
    //     return to_string(profile);
    // }

    std::optional<lpex_customer> query_customer_data_by_meter(cyng::db::session db, cyng::buffer_t id) {
        std::string const sql = "SELECT TLPExMeter.id, TLPExMeter.gen, TLPExMeter.mc, TLPExCustomer.name, TLPExCustomer.uniqueName "
                                "FROM TLPExMeter JOIN TLPExCustomer ON TLPExMeter.id = TLPExCustomer.id "
                                "WHERE TLPExMeter.meter = ?";
        auto stmt = db.create_statement();
        std::pair<int, bool> const r = stmt->prepare(sql);
        if (r.second) {
            stmt->push(cyng::make_object(id), 9); //	meterID
            if (auto res = stmt->get_result()) {
                cyng::meta_sql const ms = get_table_virtual_customer();
                auto const rec = cyng::to_record(ms, res);
#ifdef _DEBUG
                std::cout << rec.to_string() << std::endl;
#endif
                return lpex_customer(rec.value("id", ""), rec.value("mc", ""), rec.value("name", ""), rec.value("uniqueName", ""));
            }
        }

        return std::nullopt;
    }

    //
    //  anonymous namespace for local function
    //
    namespace {
        std::set<boost::uuids::uuid> select_readout_records(cyng::db::session db, cyng::obis profile, cyng::date tp) {
            //
            //  result
            //
            std::set<boost::uuids::uuid> tags;

            //
            //  query outdated records of this profile
            //
            std::string const sql = "SELECT tag "
                                    "FROM TSMLReadout "
                                    "WHERE profile = ? "
                                    "AND actTime < julianday(?)";
            ;
            auto stmt = db.create_statement();
            std::pair<int, bool> const r = stmt->prepare(sql);
            if (r.second) {
                stmt->push(cyng::make_object(profile), 0); //	profile
                stmt->push(cyng::make_object(tp), 0);      //	actTime
                while (auto res = stmt->get_result()) {
                    auto const tag = cyng::value_cast(res->get(1, cyng::TC_UUID, 0), boost::uuids::nil_uuid());
                    tags.insert(tag);
#ifdef _DEBUG
                    // std::cout << tag << " #" << tags.size() << std::endl;
#endif
                }
            }

            return tags;
        }

        void delete_readout_data_records(cyng::db::session db, std::set<boost::uuids::uuid> const &tags) {
            std::string const sql = "DELETE FROM TSMLReadoutData "
                                    "WHERE tag = ?";
            auto stmt = db.create_statement();
            std::size_t counter = 0, prev = 0;
            std::pair<int, bool> const r = stmt->prepare(sql);
            for (auto const &tag : tags) {
                stmt->push(cyng::make_object(tag), 0); //	tag
                if (stmt->execute()) {
                    stmt->clear();
                    ++counter;
                } else {
#ifdef _DEBUG
                    std::cerr << "***warning: deletion of \"TSMLReadoutData\" record with tag = " << tag << "failed" << std::endl;
#endif
                    break; //  stop
                }
                auto const percent = static_cast<std::size_t>((static_cast<double>(counter) / tags.size()) * 100.0);
                if (prev != percent) {
                    prev = percent;
                    std::cout << percent << "% TSMLReadoutData complete" << std::endl;
                }
            }
        }

        void delete_readout_records(cyng::db::session db, std::set<boost::uuids::uuid> const &tags) {
            std::string const sql = "DELETE FROM TSMLReadout "
                                    "WHERE tag = ?";
            auto stmt = db.create_statement();
            std::size_t counter = 0, prev = 0;
            std::pair<int, bool> const r = stmt->prepare(sql);
            for (auto const &tag : tags) {
                stmt->push(cyng::make_object(tag), 0); //	tag
                if (stmt->execute()) {
                    stmt->clear();
                    ++counter;
                } else {
#ifdef _DEBUG
                    std::cerr << "***warning: deletion of \"TSMLReadout\" record with tag = " << tag << "failed" << std::endl;
#endif
                    break; //  stop
                }

                auto const percent = static_cast<std::size_t>((static_cast<double>(counter) / tags.size()) * 100.0);
                if (prev != percent) {
                    prev = percent;
                    std::cout << percent << "% TSMLReadout complete" << std::endl;
                }
            }
        }
    } // namespace

    std::size_t cleanup(cyng::db::session db, cyng::obis profile, cyng::date tp) {

#ifdef _DEBUG
        std::cout << "***info: delete all records of profile : " << obis::get_name(profile) << " prior " << tp << std::endl;
#endif

        //
        // start transaction
        // Tests with SQL JOINS weren't successful. Tried
        // DELETE FROM TSMLReadout WHERE tag IN ( SELECT TSMLReadout.tag FROM TSMLReadout a INNER JOIN TSMLReadoutData b ON (a.tag =
        // b.tag) WHERE a.actTime < julianday(?));
        // but didn't work.
        // ToDo: Consider pragma foreign_keys
        //
        // So we do the "joined delete" manually
        //
        cyng::db::transaction trx(db);

        //
        //  1. get all PKs of outdated readouts
        //
        std::set<boost::uuids::uuid> tags = select_readout_records(db, profile, tp);
#ifdef _DEBUG
        std::cout << "***info: found " << tags.size() << " outdated records of profile : " << obis::get_name(profile) << std::endl;
#endif
        //
        //  2. delete all affected records from "TSMLReadoutData"
        //
#ifdef _DEBUG
        std::cout << "***info: delete " << tags.size()
                  << " outdated TSMLReadoutData records of profile : " << obis::get_name(profile) << std::endl;
#endif
        delete_readout_data_records(db, tags);

//
//  3. delete all affected records from "TSMLReadout"
//
#ifdef _DEBUG
        std::cout << "***info: delete " << tags.size() << " outdated TSMLReadout records of profile : " << obis::get_name(profile)
                  << std::endl;
#endif
        delete_readout_records(db, tags);

        return tags.size();
    }

    void vacuum(cyng::db::session db) {
        auto stmt = db.create_statement();
        stmt->execute("vacuum");
    }

    cyng::meta_store get_store_virtual_sml_readout() {
        return cyng::meta_store(
            "virtualSMLReadoutData",
            {cyng::column("tag", cyng::TC_UUID),       // server/meter/sensor ID
                                                       // cyng::column("meterID", cyng::TC_BUFFER), // server/meter/sensor ID
             cyng::column("meterID", cyng::TC_BUFFER), // server/meter/sensor ID
             cyng::column("register", cyng::TC_OBIS),  // OBIS code (data type)
             //   -- body
             cyng::column("status", cyng::TC_UINT32),
             cyng::column("actTime", cyng::TC_TIME_POINT),
             cyng::column("reading", cyng::TC_STRING), // value as string
             cyng::column("type", cyng::TC_UINT16),    // data type code
             cyng::column("scaler", cyng::TC_INT8),    // decimal place
             cyng::column("unit", cyng::TC_UINT8)},
            3);
    }
    cyng::meta_sql get_table_virtual_sml_readout() {
        return cyng::to_sql(get_store_virtual_sml_readout(), {0, 9, 0, 256, 0, 0, 0, 0, 0});
    }

    cyng::meta_store get_store_virtual_customer() {
        //
        // SELECT LPExMeter.id, LPExMeter.gen, LPExMeter.mc, LPExCustomer.name, LPExCustomer.uniqueName
        //
        return cyng::meta_store(
            "virtualCustomer",
            {cyng::column("id", cyng::TC_STRING), // server/meter/sensor ID
                                                  //   -- body
             cyng::column("mc", cyng::TC_STRING),
             cyng::column("name", cyng::TC_STRING),
             cyng::column("uniqueName", cyng::TC_STRING)},
            1);
    }
    cyng::meta_sql get_table_virtual_customer() { return cyng::to_sql(get_store_virtual_customer(), {8, 34, 64, 64}); }

    namespace data {
        typename readout_t::value_type make_readout(
            std::int64_t slot,
            std::uint16_t code,
            std::int8_t scaler,
            std::uint8_t unit,
            std::string reading,
            std::uint32_t status) {
            return {slot, {code, scaler, unit, reading, status}};
        }

        typename values_t::value_type make_value(cyng::obis reg, readout_t::value_type value) { return {reg, {value}}; }
        typename data_set_t::value_type make_set(smf::srv_id_t id, values_t::value_type value) { return {id, {value}}; }

        void clear(data_set_t &data_set) {
            for (auto &data : data_set) {
                std::cout << "> clear " << data.second.size() << " data records of meter " << to_string(data.first) << std::endl;
                data.second.clear();
            }
        }

        void update(
            data_set_t &data_set, // to update
            smf::srv_id_t id,
            cyng::obis reg,
            std::uint64_t slot,
            std::uint16_t code,
            std::int8_t scaler,
            std::uint8_t unit,
            std::string value,
            std::uint32_t status) {
            auto pos = data_set.find(id);
            if (pos != data_set.end()) {
                //
                // meter has already entries
                // lookup for register
                //
                auto pos_reg = pos->second.find(reg);
                if (pos_reg != pos->second.end()) {
                    //
                    //  register has already entries
                    //
                    pos_reg->second.insert(make_readout(slot, code, scaler, unit, value, status));
                } else {
                    //
                    //  new register
                    //
                    // std::cout << "> new register #" << pos->second.size() << ": " << reg << " of meter " << to_string(id)
                    //          << std::endl;
                    pos->second.insert(make_value(reg, make_readout(slot, code, scaler, unit, value, status)));
                }

            } else {
                //
                //  new meter found
                //
                // std::cout << "> new meter #" << data_set.size() << ": " << to_string(id) << std::endl;
                // std::cout << "> new register #0: " << reg << " of meter " << to_string(id) << std::endl;

                //
                //  create a record of readout data
                //
                data_set.insert(make_set(id, make_value(reg, make_readout(slot, code, scaler, unit, value, status))));
            }
        }

    } // namespace data

    namespace gap {
        typename slot_date_t::value_type make_slot(std::uint64_t slot, cyng::date tp) { return {slot, tp}; }

        typename readout_t::value_type make_readout(srv_id_t id, slot_date_t::value_type val) { return {id, {val}}; }

    } // namespace gap

    void dump_readout(cyng::db::session db, cyng::date now, std::chrono::hours backlog, std::ostream &os) {

#ifdef _DEBUG
        std::cout << "Now       : " << cyng::as_string(now) << std::endl;
        std::cout << "Backlog   : " << backlog << std::endl;
        std::cout << "Start time: " << cyng::as_string(now - backlog) << std::endl;
#endif
        //  statistics
        std::set<srv_id_t> server;
        int day = 0;

        std::string const sql =
            "SELECT TSMLReadout.tag, TSMLReadout.meterID, datetime(actTime), TSMLReadoutData.register, reading, unit "
            "FROM TSMLReadout "
            "INNER JOIN TSMLReadoutData ON TSMLReadout.tag = TSMLReadoutData.tag "
            "WHERE TSMLReadout.actTime > julianday(?) "
            "ORDER BY actTime, TSMLReadout.tag";
        auto stmt = db.create_statement();
        std::pair<int, bool> const r = stmt->prepare(sql);
        if (r.second) {
            stmt->push(cyng::make_object(now - backlog), 0); //	backlog

            std::size_t idx_readout = 0;
            std::size_t idx_value = 0;
            cyng::date ro_start = cyng::make_utc_date();
            cyng::date ro_end = now - backlog;
            boost::uuids::uuid prev_tag = boost::uuids::nil_uuid();
            while (auto res = stmt->get_result()) {
                //
                //  readout tag
                //
                auto const tag = cyng::value_cast(res->get(1, cyng::TC_UUID, 0), prev_tag);

                //
                //  meta data
                //
                auto const meter = cyng::to_buffer(res->get(2, cyng::TC_BUFFER, 0));
                auto const id = to_srv_id(meter);
                server.insert(id);

                auto const act_time = cyng::value_cast(res->get(3, cyng::TC_DATE, 0), now);
                if (ro_start > act_time) {
                    ro_start = act_time;
                }
                if (ro_end < act_time) {
                    ro_end = act_time;
                }

                int const dom = cyng::day(act_time);
                if (dom != day) {
                    day = dom;
                    os << "day   : " << cyng::day(act_time) << std::endl;
                }

                if (tag != prev_tag) {
                    os << std::endl;
                    prev_tag = tag;
                    idx_value = 1;
                    idx_readout++;
                    os << std::setw(4) << idx_readout << ": " << tag << ", " << to_string(id) << ", "
                       << cyng::as_string(act_time, "%Y-%m-%d %H:%M:%S") << std::endl;
                } else {
                    idx_value++;
                    auto const reg = cyng::value_cast(res->get(4, cyng::TC_OBIS, 0), OBIS_DATA_COLLECTOR_REGISTER);
                    auto const value = cyng::value_cast(res->get(5, cyng::TC_STRING, 0), "?");
                    auto const unit = mbus::to_unit(cyng::numeric_cast<std::uint8_t>(res->get(6, cyng::TC_UINT8, 0), 0u));
                    os << '#' << std::setw(3) << idx_value << ", " << reg << ": " << value << ' ' << mbus::get_name(unit)
                       << std::endl;
                }
            }

            os << std::endl;
            os << "statistics:" << std::endl;

            auto const start_utc = now;
            auto const start = now - backlog;
            auto const end = now;
            auto ro_range = ro_end.sub<std::chrono::minutes>(ro_start);

            os << std::fixed << std::setprecision(2);
            os << "UTC time  : " << cyng::as_string(start_utc) << std::endl;
            os << "time span : " << cyng::as_string(start) << " to " << cyng::as_string(end) << " = " << backlog.count()
               << "h (target)" << std::endl;
            os << "time span : " << cyng::as_string(ro_start) << " to " << cyng::as_string(ro_end) << " = " << ro_range.count()
               << " minutes (actual)" << std::endl;
            os << "readouts  : " << idx_readout << " (in total)" << std::endl;
            //  backlog in hours
            os << "frequency : " << static_cast<float>(idx_readout) / backlog.count() << " readout(s) per hour (target)"
               << std::endl;
            os << "ro freq.  : " << static_cast<float>(idx_readout * 60) / ro_range.count() << " readout(s) per hour (actual)"
               << std::endl;
            os << "server    : " << server.size();

            bool initial = true;
            for (auto const &srv : server) {
                if (initial) {
                    os << " - ";
                    // os << "          : ";
                    initial = false;
                } else {
                    os << ", ";
                }
                os << to_string(srv);
            }
            os << std::endl;

            {
                //  total number of entries
                std::string const sql = "SELECT count(*) FROM TSMLReadout";
                auto stmt = db.create_statement();
                std::pair<int, bool> const r = stmt->prepare(sql);
                if (r.second) {
                    if (auto res = stmt->get_result(); res) {
                        auto const count = cyng::numeric_cast<std::uint64_t>(res->get(1, cyng::TC_UINT64, 0), 0u);
                        os << "TSMLReadout contains " << count << " records" << std::endl;
                    }
                }
            }
            {
                //  total number of entries
                std::string const sql = "SELECT count(*) FROM TSMLReadoutData";
                auto stmt = db.create_statement();
                std::pair<int, bool> const r = stmt->prepare(sql);
                if (r.second) {
                    if (auto res = stmt->get_result(); res) {
                        auto const count = cyng::numeric_cast<std::uint64_t>(res->get(1, cyng::TC_UINT64, 0), 0u);
                        os << "TSMLReadoutData contains " << count << " records" << std::endl;
                    }
                }
            }
        }
    }

    bool sanitize_path(std::string path) {
        if (!std::filesystem::exists(path)) {
            std::error_code ec;
            return std::filesystem::create_directories(path, ec);
        }
        return true;
    }

} // namespace smf
