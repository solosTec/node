#include <tasks/sml_db_writer.h>

#include <smf/config/schemes.h>
#include <smf/obis/db.h>
#include <smf/obis/defs.h>
#include <smf/obis/profile.h>

#include <cyng/io/serialize.h>
#include <cyng/log/record.h>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/util.hpp>
#include <cyng/parse/string.h>
#include <cyng/sql/sql.hpp>
#include <cyng/task/channel.h>

#include <iostream>

#include <boost/algorithm/string.hpp>

namespace smf {

    sml_db_writer::sml_db_writer(cyng::channel_weak wp, cyng::controller &ctl, cyng::logger logger, cyng::db::session db)
        : sigs_{
        std::bind(&sml_db_writer::open_response, this, std::placeholders::_1, std::placeholders::_2), // open_response
        std::bind(&sml_db_writer::close_response, this), // close_response
        std::bind(&sml_db_writer::get_profile_list_response, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6), //
        std::bind(&sml_db_writer::get_proc_parameter_response, this), // get_proc_parameter_response
        std::bind(&sml_db_writer::stop, this, std::placeholders::_1)
        }
        , channel_(wp)
        , ctl_(ctl)
        , logger_(logger)
        , db_(db)
        , uidgen_() {

        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"open.response", "close.response", "get.profile.list.response", "get.proc.parameter.response"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    bool sml_db_writer::init_storage(cyng::db::session db) {
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

    void sml_db_writer::stop(cyng::eod) {}
    void sml_db_writer::open_response(cyng::buffer_t, cyng::buffer_t) {}
    void sml_db_writer::close_response() {}
    void sml_db_writer::get_profile_list_response(
        std::string trx,
        cyng::buffer_t server_id,
        cyng::object act_time,
        std::uint32_t status,
        cyng::obis_path_t path,
        cyng::param_map_t values) {

        if (path.size() == 1) {
            auto const &profile = path.front();
            if (sml::is_profile(profile)) {
                CYNG_LOG_TRACE(logger_, "[sml.db] get_profile_list_response #" << values.size() << ": " << obis::get_name(profile));
                store(trx, to_srv_id(server_id), profile, act_time, status, values);
            } else {
                CYNG_LOG_WARNING(
                    logger_,
                    "[sml.db] get_profile_list_response with unsupported profile: "
                        << profile << " (" << obis::get_name(profile) << ") from server " << srv_id_to_str(server_id));
                //  assume 15 min profile
                store(trx, to_srv_id(server_id), OBIS_PROFILE_15_MINUTE, act_time, status, values);
            }

        } else {
            CYNG_LOG_WARNING(logger_, "[sml.db] get_profile_list_response - invalid path size: " << path);
        }
    }

    void sml_db_writer::store(
        std::string trx,
        srv_id_t &&srv,
        cyng::obis profile,
        cyng::object act_time,
        std::uint32_t status,
        cyng::param_map_t const &values) {

        auto const now = std::chrono::system_clock::now();
        // auto const now = cyng::make_utc_date();

        //
        //  check act_time
        //
        BOOST_ASSERT(now > sml::offset_);
        if (now < sml::offset_) {
            CYNG_LOG_WARNING(logger_, "[sml.db] invalid \"act_time\" " << act_time << " at transaction " << trx);
            return;
        }

        //
        //  meter id
        //
        auto const id = get_id(srv);
        auto const meter = to_buffer(srv);

        //
        //  primary key
        //
        auto const tag = uidgen_();

        //
        //  write to database
        //
        try {

            //
            //	start transaction
            //
            cyng::db::transaction transaction(db_);

            //
            //	prepare statement
            //
            auto const m = config::get_table_sml_readout();
            auto const sql = cyng::sql::insert(db_.get_dialect(), m).bind_values(m)();

            auto stmt = db_.create_statement();
            auto const r = stmt->prepare(sql);
            if (r.second) {

                stmt->push(cyng::make_object(tag), 0); //	tag

                stmt->push(cyng::make_object(0), 0); //	gen

                stmt->push(cyng::make_object(meter), 9);   //	meterID
                stmt->push(cyng::make_object(profile), 0); //	profile
                stmt->push(cyng::make_object(trx), 21);    //	trx
                stmt->push(cyng::make_object(status), 0);  //	status
                //
                //  overwrite actTime (01 00 00 09 0b 00)
                //  OBIS_CURRENT_UTC
                //  The actTime is stored as TC_TIME but will be read as TC_DATE
                auto const pos = values.find("010000090b00");
                if (pos != values.end()) {
                    auto const c = cyng::container_cast<cyng::param_map_t>(pos->second);
                    auto const idx = c.find("value");
                    if (idx != c.end()) {
                        auto const prev = act_time.clone();
                        act_time = idx->second.clone();
                        stmt->push(idx->second, 0); // actTime
                        stmt->push(prev, 0);        // original actTime

                        auto const unboxed = cyng::value_cast(act_time, now);
                        CYNG_LOG_TRACE(
                            logger_,
                            "[sml.db] update actTime of data record "
                                << tag << " from " << prev << " to " << act_time
                                << ", UNIX time: " << std::chrono::system_clock::to_time_t(unboxed));
                    } else {
                        CYNG_LOG_ERROR(logger_, "[sml.db] missing column \"value\" in data record " << tag);
                    }
                } else {
                    stmt->push(act_time, 0);               //	received
                    stmt->push(cyng::make_object(now), 0); //	actTime
                }

                if (stmt->execute()) {
                    stmt->clear();
                    CYNG_LOG_TRACE(
                        logger_,
                        "[sml.db] data record of " << to_string(srv) << " successful stored in table " << m.get_name()
                                                   << " by key: " << tag);
                } else {
                    CYNG_LOG_WARNING(logger_, "[sml.db] insert into " << m.get_name() << " failed");
                }

                //
                //  write data
                //
                store(tag, values);

            } else {
                CYNG_LOG_WARNING(logger_, "[sml.db] internal error: " << sql);
            }

        } catch (std::exception const &ex) {
            CYNG_LOG_WARNING(logger_, "[sml.db]: " << ex.what());
        }
    }

    void sml_db_writer::store(boost::uuids::uuid tag, cyng::param_map_t const &data) {
        auto const m = config::get_table_sml_readout_data();
        auto const sql = cyng::sql::insert(db_.get_dialect(), m).bind_values(m)();

        auto stmt = db_.create_statement();
        auto const r = stmt->prepare(sql);
        if (r.second) {

            for (auto const &value : data) {
                auto const reader = cyng::make_reader(value.second);
                auto const scaler = reader.get<std::int8_t>("scaler", 0); //  signed!
                auto const unit = reader.get<std::uint8_t>("unit", 0u);
                auto const reading = cyng::io::to_plain(reader.get("value")); //  string
                auto const obj = reader.get("raw");
                auto const type = obj.tag(); //  u16
                auto const descr = reader.get("descr", "");

                CYNG_LOG_DEBUG(
                    logger_,
                    "[sml.db] store " << value.first << ": " << value.second << " ("
                                      << cyng::intrinsic_name_by_type_code(static_cast<cyng::type_code>(type)) << ")");
                CYNG_LOG_DEBUG(logger_, "[sml.db] store scaler : " << +scaler);
                CYNG_LOG_DEBUG(logger_, "[sml.db] store unit   : " << +unit);
                CYNG_LOG_DEBUG(
                    logger_, "[sml.db] store reading: " << reading << " (" << cyng::io::to_typed(reader.get("value")) << ")");
                CYNG_LOG_DEBUG(
                    logger_,
                    "[sml.db] store type   : " << type << " ("
                                               << cyng::intrinsic_name_by_type_code(static_cast<cyng::type_code>(type)) << ")");

                stmt->push(cyng::make_object(tag), 0); //	tag

                auto const reg = cyng::to_obis(value.first);
                stmt->push(cyng::make_object(reg), 0); //	register
                CYNG_LOG_DEBUG(logger_, "[sml.db] store reg    : " << reg);
                stmt->push(cyng::make_object(0), 0); //	gen

                //  Scaled values must be stored as integers without scaling
                if (type == cyng::TC_BUFFER) {
                    //  Buffer values should contain val.size() % 2 == 0 elements
                    if (reading.size() % 2 == 0) {
                        stmt->push(cyng::make_object(reading), 256); //	reading
                    } else {
                        stmt->push(cyng::make_object(""), 256); //	reading
                    }
                } else {
                    stmt->push(cyng::make_object(reading), 256); //	reading
                }
                stmt->push(cyng::make_object(type), 0);   //	type
                stmt->push(cyng::make_object(scaler), 0); //	scaler
                stmt->push(cyng::make_object(unit), 0);   //	unit
                if (stmt->execute()) {
                    stmt->clear();
                    CYNG_LOG_TRACE(logger_, "[sml.db] data record " << tag << " successful stored in table " << m.get_name());
                } else {
                    CYNG_LOG_WARNING(logger_, "[sml.db] insert into " << m.get_name() << " failed");
                }
            }
        } else {
            CYNG_LOG_WARNING(logger_, "[sml.db] internal error: " << sql);
        }
    }
    void sml_db_writer::get_proc_parameter_response() {}

    std::vector<cyng::meta_store> sml_db_writer::get_store_meta_data() {
        return {config::get_store_sml_readout(), config::get_store_sml_readout_data()};
    }
    std::vector<cyng::meta_sql> sml_db_writer::get_sql_meta_data() {
        return {
            config::get_table_sml_readout(),
            config::get_table_sml_readout_data(),
            config::get_table_customer_lpex(),
            config::get_table_meter_lpex()};
    }

} // namespace smf
