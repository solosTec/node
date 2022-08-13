
#include <smf/report/utility.h>

#include <smf/config/schemes.h>
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

    std::map<cyng::obis, sml_data> select_readout_data(cyng::db::session db, boost::uuids::uuid tag) {

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
                using cyng::operator<<;
                std::cout << rec.to_string() << std::endl;
#endif
                auto const code = rec.value<std::uint16_t>("type", cyng::TC_STRING);
                auto const val = rec.value("reading", "");
                auto const scaler = rec.value<std::int8_t>("scaler", 0);
                auto const reg = rec.value("register", OBIS_DATA_COLLECTOR_REGISTER);
                auto const unit = rec.value<std::uint8_t>("unit", 0u);

                // readout.emplace_back(reg, code, scaler, unit, val);
                auto const pos = readout.emplace(
                    std::piecewise_construct, std::forward_as_tuple(reg), std::forward_as_tuple(code, scaler, unit, val));
#ifdef _DEBUG
                std::cout << tag << ": " << pos.first->second.restore() << ", value: " << val << ", scaler: " << +scaler
                          << std::endl;
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

        std::map<std::uint64_t, std::map<cyng::obis, sml_data>> data;

        auto const ms = config::get_table_sml_readout();
        std::string const sql =
            "SELECT tag, gen, meterID, profile, trx, status, datetime(actTime), datetime(received) FROM TSMLreadout WHERE profile = ? AND meterID = ? AND received BETWEEN julianday(?) AND julianday(?) ORDER BY actTime";
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

                //
                //  select readout data
                //
                //  std::map<cyng::obis, sml_data>
                auto const readouts = select_readout_data(db, tag);

                //
                //  calculate the time slot
                //
                auto const act_time = rec.value("actTime", start);
                auto const slot = sml::to_index(act_time, profile);
                if (slot.second) {
                    auto set_time = sml::to_time_point(slot.first, profile);

#ifdef _DEBUG
                    using cyng::operator<<;
                    std::cout << cyng::to_string(id) << " - slot: #" << slot.first << " (" << set_time
                              << "), actTime: " << rec.value("actTime", start) << " with " << readouts.size() << " readouts"
                              << std::endl;
#endif

                    data.emplace(slot.first, readouts);
                }
            }
        }
        return data;
    }

    std::map<cyng::obis, std::map<std::int64_t, sml_data>> collect_data_by_register(
        cyng::db::session db,
        cyng::obis profile,
        cyng::buffer_t id,
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end) {

        std::map<cyng::obis, std::map<std::int64_t, sml_data>> data;

        auto const ms = config::get_table_sml_readout();
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
                // auto obj = res->get(6, cyng::TC_TIME_POINT, 0);
                // std::cout << obj << std::endl;
                auto const rec = cyng::to_record(ms, res);
#ifdef _DEBUG
                std::cout << rec.to_string() << std::endl;
#endif

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
                        //
                        // sml_data val(code, scaler, unit, reading);
                        // auto obj = val.restore();
                        // std::map<std::int64_t, sml_data> tmp = {{slot.first, val}};
                        // data.emplace(std::piecewise_construct, std::forward_as_tuple(reg), std::forward_as_tuple(tmp));

                        //
                        //  nested initialization doesn't work, so two steps are necessary
                        //
                        data.emplace(std::piecewise_construct, std::forward_as_tuple(reg), std::forward_as_tuple())
                            .first->second.emplace(
                                std::piecewise_construct,
                                std::forward_as_tuple(slot.first),
                                std::forward_as_tuple(code, scaler, unit, reading));
                    } else {
                        //
                        //  existing element
                        //
                        pos->second.emplace(
                            std::piecewise_construct,
                            std::forward_as_tuple(slot.first),
                            std::forward_as_tuple(code, scaler, unit, reading));
                    }
                }
            }
        }
        return data;
    }

    cyng::meta_store get_store_virtual_sml_readout() {
        return cyng::meta_store(
            "virtualSMLReadoutData",
            {cyng::column("tag", cyng::TC_UUID),      // server/meter/sensor ID
                                                      // cyng::column("meterID", cyng::TC_BUFFER), // server/meter/sensor ID
             cyng::column("register", cyng::TC_OBIS), // OBIS code (data type)
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
        //
        //  SELECT hex(TSMLReadout.meterID), TSMLReadoutData.register, reading, unit from TSMLReadout INNER JOIN TSMLReadoutData
        //  ON TSMLReadout.tag = TSMLReadoutData.tag ORDER BY actTime;
        //
        return cyng::to_sql(get_store_virtual_sml_readout(), {0, 0, 256, 0, 0, 0, 0, 0});
    }

} // namespace smf
