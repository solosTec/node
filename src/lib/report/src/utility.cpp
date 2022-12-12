
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

    // cyng::date calculate_start_time(cyng::date now, cyng::obis profile) {
    //     switch (profile.to_uint64()) {
    //     case CODE_PROFILE_1_MINUTE: return now.get_start_of_day();
    //     default: break;
    //     }
    //     return now;
    // }

    std::vector<cyng::buffer_t> select_meters(cyng::db::session db, report_range const &rr) {

        // BOOST_ASSERT_MSG(start < end, "invalid time range");
        auto const [start, end] = rr.get_range();

        //  SELECT DISTINCT hex(meterID) FROM TSMLreadout WHERE profile = '8181c78611ff' AND actTime BETWEEN
        // julianday('2022-07-19')
        //  AND julianday('2022-07-20');
        //  Profile "8181c78611ff" translates to 142394398216703 in database
        std::string const sql =
            "SELECT DISTINCT meterID FROM TSMLReadout WHERE profile = ? AND actTime BETWEEN julianday(?) AND julianday(?)";
        auto stmt = db.create_statement();
        std::vector<cyng::buffer_t> meters;
        std::pair<int, bool> const r = stmt->prepare(sql);
        BOOST_ASSERT_MSG(r.second, "prepare SQL statement failed");
        if (r.second) {
            //  Without meta data the "real" data type is to be used
            stmt->push(cyng::make_object(rr.get_profile()), 0); //	profile
            stmt->push(cyng::make_object(start), 0);            //	start time
            stmt->push(cyng::make_object(end), 0);              //	end time
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

    std::size_t loop_readout_data(cyng::db::session db, cyng::obis profile, cyng::date start, loop_cb cb) {

        std::size_t counter = 0;
        //"SELECT TSMLReadout.tag, TSMLReadout.meterID, TSMLReadoutData.register, TSMLReadoutData.gen, TSMLReadout.status,
        // datetime(TSMLReadout.actTime), TSMLReadoutData.reading, TSMLReadoutData.type, TSMLReadoutData.scaler,
        // TSMLReadoutData.unit
        //"

        std::string const sql =
            "SELECT TSMLReadout.tag, TSMLReadout.meterID, datetime(actTime), TSMLReadout.status, "
            "TSMLReadoutData.register, TSMLReadoutData.reading, TSMLReadoutData.type, TSMLReadoutData.scaler, TSMLReadoutData.unit FROM "
            "TSMLReadout INNER JOIN TSMLReadoutData ON TSMLReadout.tag = TSMLReadoutData.tag "
            "WHERE TSMLReadout.actTime > julianday(?) AND TSMLReadout.profile = ? ORDER BY actTime";
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
                auto const id = to_srv_id(meter);
                //  [3] actTime
                auto const act_time = cyng::value_cast(res->get(3, cyng::TC_DATE, 0), start);
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
        ss << prefix << get_prefix(profile) << "-" << to_string(srv_id) << '_' << cyng::as_string(start, "%Y%m%dT%H%M") << ".csv";
        return ss.str();
    }

    std::string get_filename(std::string prefix, cyng::obis profile, cyng::date const &start) {

        // auto const d = cyng::date::make_date_from_local_time(start);
        std::stringstream ss;
        ss << prefix << get_prefix(profile) << "-" << cyng::as_string(start, "%Y%m%dT%H%M") << ".csv";
        return ss.str();
    }

    std::string get_filename(
        std::string prefix,
        cyng::obis profile,
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end) {

        BOOST_ASSERT(start < end);

        auto const d = cyng::date::make_date_from_local_time(start);
        auto const hours = std::chrono::duration_cast<std::chrono::hours>(start - end);
        std::stringstream ss;
        ss << prefix << get_prefix(profile) << cyng::as_string(d, "-%Y%m%dT%H%M-") << std::abs(hours.count()) << 'h' << ".csv";
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

    std::map<std::uint64_t, std::map<cyng::obis, sml_data>>
    collect_data_by_timestamp(cyng::db::session db, report_range const &subrr, cyng::buffer_t id) {

        //
        //  ToDo: Incorporate the status into the return value.
        //

        std::map<std::uint64_t, std::map<cyng::obis, sml_data>> data;

        //  get timestamps
        auto [start, end] = subrr.get_range();

        auto const ms = config::get_table_sml_readout();
        std::string const sql =
            "SELECT tag, gen, meterID, profile, trx, status, datetime(actTime), datetime(received) FROM TSMLReadout WHERE profile = ? AND meterID = ? AND received BETWEEN julianday(?) AND julianday(?) ORDER BY actTime";
        auto stmt = db.create_statement();
        std::pair<int, bool> const r = stmt->prepare(sql);
        if (r.second) {
            stmt->push(cyng::make_object(subrr.get_profile()), 0);         //	profile
            stmt->push(cyng::make_object(id), 9);                          //	meterID
            stmt->push(cyng::make_object(start.to_local_time_point()), 0); //	start time
            stmt->push(cyng::make_object(end.to_local_time_point()), 0);   //	end time

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
                auto const slot = sml::to_index(act_time.to_utc_time_point(), subrr.get_profile());
                if (slot.second) {
                    auto set_time = sml::restore_time_point(slot.first, subrr.get_profile());

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

    gap::readout_t
    collect_readouts_by_time_range(cyng::db::session db, gap::readout_t const &initial_data, report_range const &subrr) {

        //
        //  meter => slot => actTime
        //
        gap::readout_t data;

        //
        //  copy only meter IDs
        //
        for (auto const &init : initial_data) {
            data.insert({init.first, {}});
        }

        auto const ms = config::get_table_sml_readout();
        std::string const sql = "SELECT tag, gen, meterID, profile, trx, status, datetime(actTime), datetime(received) "
                                "FROM TSMLReadout "
                                "WHERE profile = ? AND received BETWEEN julianday(?) AND julianday(?) "
                                "ORDER BY meterId, actTime";
        auto stmt = db.create_statement();
        std::pair<int, bool> const r = stmt->prepare(sql);
        if (r.second) {

            auto const [start, end] = subrr.get_range();

#ifdef _DEBUG
            std::cout << "select ro from " << cyng::as_string(start) << " to " << cyng::as_string(end) << " ("
                      << start.to_utc_time_point() << " to " << end.to_utc_time_point() << ")" << std::endl;
#endif

            stmt->push(cyng::make_object(subrr.get_profile()), 0);       //	profile
            stmt->push(cyng::make_object(start.to_utc_time_point()), 0); //	start time
            stmt->push(cyng::make_object(end.to_utc_time_point()), 0);   //	end time

            while (auto res = stmt->get_result()) {
                auto const rec = cyng::to_record(ms, res);
                auto const tag = rec.value("tag", boost::uuids::nil_uuid());
                BOOST_ASSERT(!tag.is_nil());
                // auto const status = rec.value<std::uint32_t>("status", 0u);

                //
                //  calculate the time slot
                //
                auto const act_time = rec.value("actTime", start.to_utc_time_point()); // + offset
                auto const slot = sml::to_index(act_time, subrr.get_profile());
#ifdef _DEBUG
                // std::cout << "start: " << cyng::sys::to_string_utc(start, "%Y-%m-%dT%H:%M%z")
                //           << ", actTime: " << cyng::sys::to_string_utc(act_time, "%Y-%m-%dT%H:%M%z") << ", slot: " << slot.first
                //           << std::endl;
#endif

                if (slot.second) {
                    auto set_time = sml::restore_time_point(slot.first, subrr.get_profile());
                    auto const id = rec.value("meterID", cyng::make_buffer());

#ifdef _DEBUG
                    //  example: actTime '2022-10-07 11:00:00' from table translates to '2022-10-07T12:00:00+0200'
                    // using cyng::operator<<;
                    // std::cout << "=>start: " << cyng::sys::to_string(start, "%Y-%m-%dT%H:%M%z") << " - " << cyng::to_string(id)
                    //          << " - slot: #" << slot.first << " (" << set_time << "), actTime: " << rec.value("actTime", start)
                    //          << ", tag: " << tag << std::endl;
#endif

                    auto pos = data.find(id);
                    if (pos != data.end()) {
                        auto const res = pos->second.emplace(slot.first, act_time);
                        // BOOST_ASSERT(res.second); - duplicates possible
#ifdef __DEBUG
                        if (res.second) {
                            using cyng::operator<<;
                            std::cout << cyng::to_string(id) << " has " << pos->second.size() << " entries (" << act_time << ")"
                                      << std::endl;
                            //} else {
                            //    using cyng::operator<<;
                            //    std::cout << cyng::to_string(id) << " duplicate at " << act_time << std::endl;
                        }
#endif
                    } else {
                        data.insert(gap::make_readout(id, gap::make_slot(slot.first, act_time)));
                    }
                }
            }
        }
        return data;
    }

    data::profile_t collect_data_by_time_range(cyng::db::session db, cyng::obis_path_t const &filter, report_range const &subrr) {

        data::profile_t data;

        //
        //  "TSMLReadout" and "TSMLReadoutData" joined by "tag"
        //
        std::string const sql =
            "SELECT TSMLReadout.tag, TSMLReadout.meterID, TSMLReadoutData.register, TSMLReadoutData.gen, TSMLReadout.status, datetime(TSMLReadout.actTime), TSMLReadoutData.reading, TSMLReadoutData.type, TSMLReadoutData.scaler, TSMLReadoutData.unit "
            "FROM TSMLReadout JOIN TSMLReadoutData ON TSMLReadout.tag = TSMLReadoutData.tag "
            "WHERE profile = ? AND TSMLReadout.actTime BETWEEN julianday(?) AND julianday(?) "
            "ORDER BY TSMLReadout.meterID, TSMLReadoutData.register, TSMLReadout.actTime";
        auto stmt = db.create_statement();
        std::pair<int, bool> const r = stmt->prepare(sql);
        if (r.second) {

#ifdef _DEBUG
            std::ofstream ofdbg("LPExdebug.log", std::ios::app);
            ofdbg << subrr << std::endl;
#endif

            //  get timestamps in local time
            auto [start, end] = subrr.get_range();
            BOOST_ASSERT(start < end);

#ifdef _DEBUG
            std::cout << subrr << std::endl;
            // std::cout << start.to_utc_time_point() << " -> " << end.to_utc_time_point() << std::endl;
#endif

            stmt->push(cyng::make_object(subrr.get_profile()), 0); //	profile
            //  date and
            stmt->push(cyng::make_object(start), 0); //	start time
            stmt->push(cyng::make_object(end), 0);   //	end time

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
                ofdbg << rec.to_string() << std::endl;
#endif

                //
                //  Incorporate the status into the return value.
                //
                auto const status = rec.value<std::uint32_t>("status", 0u);

                //
                //  get meter id
                //
                auto const id = rec.value("meterID", cyng::make_buffer());
                BOOST_ASSERT(!id.empty());

                //
                //  calculate the time slot
                //
                auto const act_time = rec.value("actTime", start.to_utc_time_point());
#ifdef _DEBUG
                std::cout << start.to_utc_time_point() << ", act_time: " << act_time << ", " << end.to_utc_time_point()
                          << std::endl;
                // std::cout << rec.to_string() << std::endl;
#endif
                BOOST_ASSERT(start.to_utc_time_point() <= act_time);
                BOOST_ASSERT(act_time <= end.to_utc_time_point());

                auto const slot = sml::to_index(act_time, subrr.get_profile());
                if (slot.second) {
                    auto set_time = sml::restore_time_point(slot.first, subrr.get_profile());

                    auto const code = rec.value<std::uint16_t>("type", cyng::TC_STRING);
                    auto const reading = rec.value("reading", "");
                    auto const scaler = rec.value<std::int8_t>("scaler", 0);
                    auto const reg = rec.value("register", OBIS_DATA_COLLECTOR_REGISTER);
                    auto const unit = rec.value<std::uint8_t>("unit", 0u);

                    //
                    // apply filter only if not empty
                    // The filter contains allowed registers (OBIS)
                    //
                    if (!filter.empty()) {
                        if (std::find(filter.begin(), filter.end(), reg) != filter.end()) {
                            update_data(data, id, reg, slot.first, code, scaler, unit, reading, status);
                        } // filter
                    } else {
                        //
                        //  filter is empty - ignore it
                        //
                        update_data(data, id, reg, slot.first, code, scaler, unit, reading, status);
                    }
                }
            }
        }
        return data;
    }

    void update_data(
        data::profile_t &data,
        cyng::buffer_t id,
        cyng::obis reg,
        std::int64_t slot,
        std::uint16_t code,
        std::int8_t scaler,
        std::uint8_t unit,
        std::string const &reading,
        std::uint32_t status) {
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
                pos_reg->second.insert(data::make_readout(slot, code, scaler, unit, reading, status));
            } else {
                //
                //  new register of this meter
                //
                pos->second.insert(data::make_value(reg, data::make_readout(slot, code, scaler, unit, reading, status)));
            }
        } else {
            //
            //  new meter
            //  nested initialization doesn't work, so multiple steps are necessary
            //
            data.insert(
                data::make_profile(id, data::make_value(reg, data::make_readout(slot, code, scaler, unit, reading, status))));
        }
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
                    auto set_time = sml::restore_time_point(slot.first, profile);

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

    std::size_t cleanup(cyng::db::session db, cyng::obis profile, std::chrono::system_clock::time_point tp, std::size_t limit) {

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
                    --limit;
                    if (limit == 0) {
                        break;
                    }
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
        typename data_set_t::value_type make_set(smf::srv_id_t id, values_t::value_type value) { return {id, {value}}; }

    } // namespace data

    namespace gap {
        typename slot_date_t::value_type make_slot(std::uint64_t slot, std::chrono::system_clock::time_point tp) {
            return {slot, tp};
        }

        typename readout_t::value_type make_readout(cyng::buffer_t id, slot_date_t::value_type val) { return {id, {val}}; }

    } // namespace gap

    void dump_readout(cyng::db::session db, cyng::date now, std::chrono::hours backlog) {
        // SELECT TSMLReadout.tag, hex(TSMLReadout.meterID), datetime(actTime), TSMLReadoutData.register, reading, unit from
        // TSMLReadout INNER JOIN TSMLReadoutData ON TSMLReadout.tag = TSMLReadoutData.tag WHERE TSMLReadout.actTime >
        // julianday('2022-11-26') ORDER BY actTime;

        //  statistics
        std::set<srv_id_t> server;
        int day = 0;

        std::string const sql =
            "SELECT TSMLReadout.tag, TSMLReadout.meterID, datetime(actTime), TSMLReadoutData.register, reading, unit from "
            "TSMLReadout INNER JOIN TSMLReadoutData ON TSMLReadout.tag = TSMLReadoutData.tag "
            "WHERE TSMLReadout.actTime > julianday(?) ORDER BY actTime";
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
                auto const tag = cyng::value_cast(res->get(1, cyng::TC_UUID, 0), prev_tag);
                if (tag != prev_tag) {
                    std::cout << std::endl;
                    prev_tag = tag;
                    idx_value = 0;
                    idx_readout++;
                }
                idx_value++;

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
                    std::cout << "day   : " << cyng::day(act_time) << std::endl;
                }

                auto const reg = cyng::value_cast(res->get(4, cyng::TC_OBIS, 0), OBIS_DATA_COLLECTOR_REGISTER);
                auto const value = cyng::value_cast(res->get(5, cyng::TC_STRING, 0), "?");
                auto const unit = mbus::to_unit(cyng::numeric_cast<std::uint8_t>(res->get(6, cyng::TC_UINT8, 0), 0u));

                std::cout << std::setw(4) << idx_readout << '#' << idx_value << ": " << tag << ", " << to_string(id) << ", "
                          << cyng::as_string(act_time, "%Y-%m-%d %H:%M:%S") << ", " << reg << ": " << value << ' '
                          << mbus::get_name(unit) << std::endl;
            }

            std::cout << std::endl;
            std::cout << "statistics:" << std::endl;

            auto const start_utc = now;
            auto const start = now - backlog;
            auto const end = now;
            auto ro_range = ro_end.sub<std::chrono::minutes>(ro_start);

            std::cout << std::fixed << std::setprecision(2);
            std::cout << "UTC time  : " << cyng::as_string(start_utc) << std::endl;
            std::cout << "time span : " << cyng::as_string(start) << " to " << cyng::as_string(end) << " = " << backlog.count()
                      << "h" << std::endl;
            std::cout << "readouts  : " << idx_readout << std::endl;
            std::cout << "frequency : " << static_cast<float>(idx_readout) / backlog.count() << " readout(s) per hour" << std::endl;
            std::cout << "first ro  : " << cyng::as_string(ro_start) << std::endl;
            std::cout << "last ro   : " << cyng::as_string(ro_end) << std::endl;
            std::cout << "ro span   : " << ro_range.count() << "min" << std::endl;
            std::cout << "ro freq.  : " << static_cast<float>(idx_readout * 60) / ro_range.count() << " readout(s) per hour"
                      << std::endl;
            std::cout << "server    : " << server.size() << std::endl;
        }
    }

    report_range::report_range(cyng::obis profile, cyng::date const &start, cyng::date const &end)
        : profile_(profile)
        , start_(start)
        , end_(end) {
        BOOST_ASSERT_MSG(start_ < end_, "invalid range");
    }

    cyng::date const &report_range::get_start() const noexcept { return start_; }
    cyng::date const &report_range::get_end() const noexcept { return end_; }
    cyng::obis const &report_range::get_profile() const noexcept { return profile_; }

    std::pair<cyng::date, cyng::date> report_range::get_range() const noexcept { return {start_, end_}; }

    std::pair<std::uint64_t, std::uint64_t> report_range::get_slots() const noexcept {
        return {sml::to_index(start_.to_utc_time_point(), profile_).first, sml::to_index(end_.to_utc_time_point(), profile_).first};
    }

    std::chrono::hours report_range::get_span() const { return end_.sub<std::chrono::hours>(start_); }

    std::size_t report_range::max_readout_count() const { return sml::calculate_entry_count(profile_, get_span()); }

} // namespace smf
