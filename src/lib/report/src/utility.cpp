
#include <smf/report/utility.h>

#include <smf/config/schemes.h>
#include <smf/mbus/server_id.h>
#include <smf/obis/defs.h>
#include <smf/obis/profile.h>

#include <cyng/db/storage.h>
#include <cyng/io/ostream.h>
#include <cyng/io/serialize.h>
#include <cyng/obj/buffer_cast.hpp>
#include <cyng/sql/dsl.h>
#include <cyng/sql/dsl/placeholder.h>
#include <cyng/sql/sql.hpp>

#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace smf {

    std::vector<cyng::buffer_t> select_meters(
        cyng::db::session db,
        cyng::obis profile,
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end) {

        BOOST_ASSERT_MSG(start < end, "invalid time range");

        //  SELECT DISTINCT hex(meterID) FROM TSMLreadout WHERE profile = '8181c78611ff' AND actTime BETWEEN julianday('2022-07-19')
        //  AND julianday('2022-07-20');
        std::string const sql =
            "SELECT DISTINCT meterID FROM TSMLReadout WHERE profile = ? AND actTime BETWEEN julianday(?) AND julianday(?)";
        auto stmt = db.create_statement();
        std::vector<cyng::buffer_t> meters;
        std::pair<int, bool> const r = stmt->prepare(sql);
        BOOST_ASSERT_MSG(r.second, "prepare SQL statement failed");
        if (r.second) {
            //  Without meta data the "real" data type is to be used
            stmt->push(cyng::make_object(profile.to_uint64()), 0); //	profile
            stmt->push(cyng::make_object(start), 0);               //	start time
            stmt->push(cyng::make_object(end), 0);                 //	end time
            while (auto res = stmt->get_result()) {
                auto const meter = cyng::to_buffer(res->get(1, cyng::TC_BUFFER, 9));
                BOOST_ASSERT(!meter.empty());
                if (!meter.empty()) {
                    meters.push_back(meter);
                }
            }
        }
        return meters;
    }

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

    std::set<cyng::obis> collect_profiles(std::map<std::uint64_t, std::map<cyng::obis, sml_data>> const &data) {
        std::set<cyng::obis> regs;
        for (auto const d : data) {
            for (auto const &v : d.second) {
                regs.insert(v.first);
            }
        }
        return regs;
    }

    std::string get_filename(std::string prefix, cyng::obis profile, srv_id_t srv_id, std::chrono::system_clock::time_point start) {

        auto tt = std::chrono::system_clock::to_time_t(start);
#ifdef _MSC_VER
        struct tm tm;
        _gmtime64_s(&tm, &tt);
#else
        auto tm = *std::gmtime(&tt);
#endif

        std::stringstream ss;
        ss << prefix << get_prefix(profile) << "-" << to_string(srv_id) << '_' << std::put_time(&tm, "%Y%m%dT%H%M") << ".csv";
        return ss.str();
    }

    std::string get_filename(
        std::string prefix,
        cyng::obis profile,
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end) {

        auto tt_start = std::chrono::system_clock::to_time_t(start);
        auto tt_end = std::chrono::system_clock::to_time_t(end);
#ifdef _MSC_VER
        struct tm tm_start, tm_end;
        _gmtime64_s(&tm_start, &tt_start);
        _gmtime64_s(&tm_end, &tt_end);
#else
        auto tm_start = *std::gmtime(&tt_start);
        auto tm_end = *std::gmtime(&tt_end);
#endif

        std::stringstream ss;
        ss << prefix << get_prefix(profile) << "-" << std::put_time(&tm_start, "%Y%m%dT%H%M") << "-"
           << std::put_time(&tm_end, "%Y%m%dT%H%M") << ".csv";
        return ss.str();
    }

    std::string get_prefix(cyng::obis profile) {
        //
        //  the prefox should be a valid file name
        //
        switch (profile.to_uint64()) {
        case CODE_PROFILE_1_MINUTE: return "1min";
        case CODE_PROFILE_15_MINUTE: return "15min";
        case CODE_PROFILE_60_MINUTE: return "1h";
        case CODE_PROFILE_24_HOUR: return "1d";
        case CODE_PROFILE_LAST_2_HOURS: return "2h";
        case CODE_PROFILE_LAST_WEEK: return "1w";
        case CODE_PROFILE_1_MONTH: return "1mon";
        case CODE_PROFILE_1_YEAR: return "1y";
        case CODE_PROFILE_INITIAL: return "init";
        default: break;
        }
        return to_string(profile);
    }

    std::map<std::uint64_t, std::map<cyng::obis, sml_data>> collect_data_by_timestamp(
        cyng::db::session db,
        cyng::obis profile,
        cyng::buffer_t id,
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end) {

        //
        //  ToDo: Incorporate the status into the return value.
        //

        std::map<std::uint64_t, std::map<cyng::obis, sml_data>> data;

        auto const ms = config::get_table_sml_readout();
        std::string const sql =
            "SELECT tag, gen, meterID, profile, trx, status, datetime(actTime), datetime(received) FROM TSMLReadout WHERE profile = ? AND meterID = ? AND received BETWEEN julianday(?) AND julianday(?) ORDER BY actTime";
        auto stmt = db.create_statement();
        std::pair<int, bool> const r = stmt->prepare(sql);
        if (r.second) {
            stmt->push(cyng::make_object(profile), 0); //	profile
            stmt->push(cyng::make_object(id), 9);      //	meterID
            stmt->push(cyng::make_object(start), 0);   //	start time
            stmt->push(cyng::make_object(end), 0);     //	end time

            while (auto res = stmt->get_result()) {
                auto const rec = cyng::to_record(ms, res);
                auto const tag = rec.value("tag", boost::uuids::nil_uuid());
                BOOST_ASSERT(!tag.is_nil());
                auto const status = rec.value<std::uint32_t>("status", 0u);

                //
                //  select readout data
                //
                //  std::map<cyng::obis, sml_data>
                auto const readouts = select_readout_data(db, tag, status);

                //
                //  calculate the time slot
                //
                auto const act_time = rec.value("actTime", start);
                auto const slot = sml::to_index(act_time, profile);
                if (slot.second) {
                    auto set_time = sml::to_time_point(slot.first, profile);

#ifdef _DEBUG
                    // using cyng::operator<<;
                    // std::cout << cyng::to_string(id) << " - slot: #" << slot.first << " (" << set_time
                    //           << "), actTime: " << rec.value("actTime", start) << " with " << readouts.size() << " readouts"
                    //           << std::endl;
#endif

                    data.emplace(slot.first, readouts);
                }
            }
        }
        return data;
    }

    gap::readout_t collect_readouts_by_time_range(
        cyng::db::session db,
        cyng::obis profile,
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end) {

        //
        //  meter => slot => actTime
        //
        gap::readout_t data;

        auto const ms = config::get_table_sml_readout();
        std::string const sql = "SELECT tag, gen, meterID, profile, trx, status, datetime(actTime), datetime(received) "
                                "FROM TSMLReadout "
                                "WHERE profile = ? AND received BETWEEN julianday(?) AND julianday(?) "
                                "ORDER BY meterId, actTime";
        auto stmt = db.create_statement();
        std::pair<int, bool> const r = stmt->prepare(sql);
        if (r.second) {
            stmt->push(cyng::make_object(profile), 0); //	profile
            stmt->push(cyng::make_object(start), 0);   //	start time
            stmt->push(cyng::make_object(end), 0);     //	end time

            while (auto res = stmt->get_result()) {
                auto const rec = cyng::to_record(ms, res);
                auto const tag = rec.value("tag", boost::uuids::nil_uuid());
                BOOST_ASSERT(!tag.is_nil());
                // auto const status = rec.value<std::uint32_t>("status", 0u);

                //
                //  calculate the time slot
                //
                auto const act_time = rec.value("actTime", start);
                auto const slot = sml::to_index(act_time, profile);
                if (slot.second) {
                    auto set_time = sml::to_time_point(slot.first, profile);
                    auto const id = rec.value("meterID", cyng::make_buffer({}));

#ifdef __DEBUG
                    using cyng::operator<<;
                    std::cout << cyng::to_string(id) << " - slot: #" << slot.first << " (" << set_time
                              << "), actTime: " << rec.value("actTime", start) << std::endl;
#endif

                    auto pos = data.find(id);
                    if (pos != data.end()) {
                        pos->second.emplace(slot.first, act_time);
                    } else {
                        data.insert(gap::make_readout(id, gap::make_slot(slot.first, act_time)));
                    }
                }
            }
        }
        return data;
    }

    data::profile_t collect_data_by_time_range(
        cyng::db::session db,
        cyng::obis profile,
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end) {

        data::profile_t data;

        //
        //  "TSMLReadout" and "TSMLReadoutData" joined by "tag"
        //
        std::string const sql =
            "SELECT TSMLReadout.tag, TSMLReadout.meterID, TSMLReadoutData.register, TSMLReadoutData.gen, TSMLReadout.status, datetime(TSMLReadout.actTime), TSMLReadoutData.reading, TSMLReadoutData.type, TSMLReadoutData.scaler, TSMLReadoutData.unit "
            "FROM TSMLReadout JOIN TSMLReadoutData ON TSMLReadout.tag = TSMLReadoutData.tag "
            "WHERE profile = ? AND received BETWEEN julianday(?) AND julianday(?) "
            "ORDER BY TSMLReadout.meterID, TSMLReadoutData.register, TSMLReadout.actTime";
        auto stmt = db.create_statement();
        std::pair<int, bool> const r = stmt->prepare(sql);
        if (r.second) {
            stmt->push(cyng::make_object(profile), 0); //	profile
            stmt->push(cyng::make_object(start), 0);   //	start time
            stmt->push(cyng::make_object(end), 0);     //	end time

            //
            // all data for the specified meter in the time range ordered by
            // 1. meter id
            // 2. register (OBIS)
            // 3. time stamp
            //
            auto const ms = get_table_virtual_sml_readout();
            while (auto res = stmt->get_result()) {

                //
                //  get virtual data record
                //
                auto const rec = cyng::to_record(ms, res);
#ifdef _DEBUG
                // std::cout << rec.to_string() << std::endl;
#endif

                //
                //  Incorporate the status into the return value.
                //
                auto const status = rec.value<std::uint32_t>("status", 0u);

                //
                //  get meter id
                //
                auto const id = rec.value("meterID", cyng::make_buffer({}));
                BOOST_ASSERT(!id.empty());

                //
                //  calculate the time slot
                //
                auto const act_time = rec.value("actTime", start);
                auto const slot = sml::to_index(act_time, profile);
                if (slot.second) {
                    auto set_time = sml::to_time_point(slot.first, profile);

                    auto const code = rec.value<std::uint16_t>("type", cyng::TC_STRING);
                    auto const reading = rec.value("reading", "");
                    auto const scaler = rec.value<std::int8_t>("scaler", 0);
                    auto const reg = rec.value("register", OBIS_DATA_COLLECTOR_REGISTER);
                    auto const unit = rec.value<std::uint8_t>("unit", 0u);

                    auto pos = data.find(id);
                    if (pos != data.end()) {
                        //
                        //  meter is already inserted - lookup register
                        //
                        auto pos_reg = pos->second.find(reg);
                        if (pos_reg != pos->second.end()) {
                            //
                            //  register is already inserted - lookup time slot
                            //
                            pos_reg->second.insert(data::make_readout(slot.first, code, scaler, unit, reading, status));
                        } else {
                            //
                            //  new register of this meter
                            //
                            pos->second.insert(
                                data::make_value(reg, data::make_readout(slot.first, code, scaler, unit, reading, status)));
                        }
                    } else {
                        //
                        //  new meter
                        //  nested initialization doesn't work, so multiple steps are necessary
                        //
                        data.insert(data::make_profile(
                            id, data::make_value(reg, data::make_readout(slot.first, code, scaler, unit, reading, status))));
                    }
                }
            }
        }
        return data;
    }

    std::map<cyng::obis, std::map<std::int64_t, sml_data>> collect_data_by_register(
        cyng::db::session db,
        cyng::obis profile,
        cyng::buffer_t id, // meter id
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end) {

        std::map<cyng::obis, std::map<std::int64_t, sml_data>> data;

        //
        //  "TSMLReadout" and "TSMLReadoutData" joined by "tag"
        //
        std::string const sql =
            "SELECT TSMLReadout.tag, TSMLReadoutData.register, TSMLReadoutData.gen, TSMLReadout.status, datetime(TSMLReadout.actTime), TSMLReadoutData.reading, TSMLReadoutData.type, TSMLReadoutData.scaler, TSMLReadoutData.unit "
            "FROM TSMLReadout JOIN TSMLReadoutData ON TSMLReadout.tag = TSMLReadoutData.tag "
            "WHERE profile = ? AND meterID = ? AND received BETWEEN julianday(?) AND julianday(?) "
            "ORDER BY TSMLReadoutData.register, TSMLReadout.actTime";
        auto stmt = db.create_statement();
        std::pair<int, bool> const r = stmt->prepare(sql);
        if (r.second) {
            stmt->push(cyng::make_object(profile), 0); //	profile
            stmt->push(cyng::make_object(id), 9);      //	meterID
            stmt->push(cyng::make_object(start), 0);   //	start time
            stmt->push(cyng::make_object(end), 0);     //	end time

            //
            // all data for the specified meter in the time range ordered by
            // 1. register
            // 2. time stamp
            //
            auto const ms = get_table_virtual_sml_readout();
            while (auto res = stmt->get_result()) {

                //
                //  get virtual data record
                //
                auto const rec = cyng::to_record(ms, res);
#ifdef _DEBUG
                // std::cout << rec.to_string() << std::endl;
#endif

                //
                //  Incorporate the status into the return value.
                //
                auto const status = rec.value<std::uint32_t>("status", 0u);

                //
                //  calculate the time slot
                //
                auto const act_time = rec.value("actTime", start);
                auto const slot = sml::to_index(act_time, profile);
                if (slot.second) {
                    auto set_time = sml::to_time_point(slot.first, profile);

                    auto const code = rec.value<std::uint16_t>("type", cyng::TC_STRING);
                    auto const reading = rec.value("reading", "");
                    auto const scaler = rec.value<std::int8_t>("scaler", 0);
                    auto const reg = rec.value("register", OBIS_DATA_COLLECTOR_REGISTER);
                    auto const unit = rec.value<std::uint8_t>("unit", 0u);

                    auto pos = data.find(reg);
                    if (pos == data.end()) {
                        //
                        //  new element
                        //  nested initialization doesn't work, so two steps are necessary
                        //
                        data.emplace(std::piecewise_construct, std::forward_as_tuple(reg), std::forward_as_tuple())
                            .first->second.emplace(
                                std::piecewise_construct,
                                std::forward_as_tuple(slot.first),
                                std::forward_as_tuple(code, scaler, unit, reading, status));
                    } else {
                        //
                        //  existing element
                        //
                        pos->second.emplace(
                            std::piecewise_construct,
                            std::forward_as_tuple(slot.first),
                            std::forward_as_tuple(code, scaler, unit, reading, status));
                    }
                }
            }
        }
        return data;
    }

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

    std::size_t cleanup(cyng::db::session db, cyng::obis profile, std::chrono::system_clock::time_point tp) {

        std::set<boost::uuids::uuid> tags;

        //
        //	start transaction
        // ToDo: Consider pragma foreign_keys = on; or a JOIN (see below)
        //
        cyng::db::transaction trx(db);

        //
        //  remove all records of this profile from table "TSMLReadout" and "TSMLReadoutData"
        //
        // 1. get all PKs of outdated readouts
        //
        {
            //  DELETE FROM TSMLReadoutData
            //  WHERE tag IN (
            //      SELECT tag FROM TSMLReadout a
            //      INNER JOIN TSMLReadoutData b
            //          ON (a.tag = b.tag)
            //      WHERE a.actTime < julianday(?)
            //  );
            std::string const sql = "SELECT tag "
                                    "FROM TSMLReadout "
                                    "WHERE profile = ? AND actTime < julianday(?)";
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
        }

        //
        //  2. delete all affected records from "TSMLReadoutData"
        //
        {
            std::string const sql = "DELETE FROM TSMLReadoutData "
                                    "WHERE tag = ?";
            auto stmt = db.create_statement();
            std::pair<int, bool> const r = stmt->prepare(sql);
            for (auto const tag : tags) {
                stmt->push(cyng::make_object(tag), 0); //	tag
                if (stmt->execute()) {
                    stmt->clear();
                } else {
#ifdef _DEBUG
                    std::cerr << "***warning: deletion of \"TSMLReadoutData\" record with tag = " << tag << "failed" << std::endl;
#endif
                    break; //  stop
                }
            }
        }

        //
        // 3. delete all affected records from "TSMLReadout"
        //
        {
            //
            //  This is more effective than deleting each record separately.
            //
            std::string const sql = "DELETE FROM TSMLReadout "
                                    "WHERE profile = ? AND actTime < julianday(?)";
            auto stmt = db.create_statement();
            std::pair<int, bool> const r = stmt->prepare(sql);
            if (r.second) {
                stmt->push(cyng::make_object(profile), 0); //	profile
                stmt->push(cyng::make_object(tp), 0);      //	actTime
                stmt->execute();
            }
        }
        return tags.size();
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

        typename profile_t::value_type make_profile(cyng::buffer_t id, values_t::value_type value) { return {id, {value}}; }

    } // namespace data

    namespace gap {
        typename slot_date_t::value_type make_slot(std::uint64_t slot, std::chrono::system_clock::time_point tp) {
            return {slot, tp};
        }

        typename readout_t::value_type make_readout(cyng::buffer_t id, slot_date_t::value_type val) { return {id, {val}}; }

    } // namespace gap

} // namespace smf
