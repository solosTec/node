/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/sml_db_writer.h>

#include <cyng/log/record.h>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/util.hpp>
#include <cyng/sql/sql.hpp>
#include <cyng/task/channel.h>

#include <iostream>

#include <boost/algorithm/string.hpp>

namespace smf {

    sml_db_writer::sml_db_writer(cyng::channel_weak wp, cyng::controller &ctl, cyng::logger logger, cyng::db::session db)
        : sigs_{std::bind(&sml_db_writer::open_response, this, std::placeholders::_1, std::placeholders::_2), std::bind(&sml_db_writer::close_response, this), std::bind(&sml_db_writer::get_profile_list_response, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6), std::bind(&sml_db_writer::get_proc_parameter_response, this), std::bind(&sml_db_writer::stop, this, std::placeholders::_1)}
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
            auto const profile = path.front();
            CYNG_LOG_TRACE(logger_, "[sml.db] get_profile_list_response #" << values.size() << ": " << profile);

            store(trx, to_srv_id(server_id), profile, act_time, status, values);

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

        //
        //  meter id
        //
        auto const id = get_id(srv);
        auto const meter = to_buffer(srv);

        //
        //  primary key
        //
        auto const tag = uidgen_();

        auto const now = std::chrono::system_clock::now();

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
            auto const m = get_table_readout();
            auto const sql = cyng::sql::insert(db_.get_dialect(), m).bind_values(m)();

            auto stmt = db_.create_statement();
            auto const r = stmt->prepare(sql);
            if (r.second) {
                stmt->push(cyng::make_object(tag), 0); //	tag

                stmt->push(cyng::make_object(0), 0); //	gen

                stmt->push(cyng::make_object(meter), 9);    //	meterID
                stmt->push(cyng::make_object(profile), 0);  //	profile
                stmt->push(cyng::make_object(trx), 21);     //	trx
                stmt->push(cyng::make_object(status), 0);   //	status
                stmt->push(cyng::make_object(act_time), 0); //	actTime
                stmt->push(cyng::make_object(now), 0);      //	received
                if (stmt->execute()) {
                    stmt->clear();
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
        auto const m = get_table_readout_data();
        auto const sql = cyng::sql::insert(db_.get_dialect(), m).bind_values(m)();

        auto stmt = db_.create_statement();
        auto const r = stmt->prepare(sql);
        if (r.second) {

            for (auto const &value : data) {
                auto const reader = cyng::make_reader(value.second);
                auto const scaler = reader.get<std::uint8_t>("scaler", 0);
                auto const unit = reader.get<std::uint8_t>("unit", 0);
                auto const reading = reader.get("value", "0");
                auto const obj = reader.get("raw");
                auto const type = obj.tag(); //  u16
                auto const descr = reader.get("descr", "");

                CYNG_LOG_DEBUG(logger_, "[sml.db] " << value.first << ": " << value.second << " (" << descr << ")");
                // CYNG_LOG_DEBUG(logger_, "[sml.db] scaler : " << +scaler);
                // CYNG_LOG_DEBUG(logger_, "[sml.db] unit   : " << +unit);
                // CYNG_LOG_DEBUG(logger_, "[sml.db] reading: " << reading);
                // CYNG_LOG_DEBUG(logger_, "[sml.db] type   : " << type);

                stmt->push(cyng::make_object(tag), 0);         //	tag
                stmt->push(cyng::make_object(value.first), 0); //	register
                stmt->push(cyng::make_object(0), 0);           //	gen
                stmt->push(cyng::make_object(reading), 256);   //	reading
                stmt->push(cyng::make_object(type), 0);        //	type
                stmt->push(cyng::make_object(scaler), 0);      //	scaler
                stmt->push(cyng::make_object(unit), 0);        //	unit
                if (stmt->execute()) {
                    stmt->clear();
                } else {
                    CYNG_LOG_WARNING(logger_, "[sml.db] insert into " << m.get_name() << " failed");
                }
            }
        } else {
            CYNG_LOG_WARNING(logger_, "[sml.db] internal error: " << sql);
        }
    }
    void sml_db_writer::get_proc_parameter_response() {}

    std::vector<cyng::meta_store> sml_db_writer::get_store_meta_data() { return {get_store_readout(), get_store_readout_data()}; }
    std::vector<cyng::meta_sql> sml_db_writer::get_sql_meta_data() { return {get_table_readout(), get_table_readout_data()}; }

    cyng::meta_store sml_db_writer::get_store_readout() {
        return cyng::meta_store(
            "SMLreadout",
            {cyng::column("tag", cyng::TC_UUID),
             //   -- body
             cyng::column("meterID", cyng::TC_BUFFER), // server/meter/sensor ID
             cyng::column("profile", cyng::TC_OBIS),   // load profile
             cyng::column("trx", cyng::TC_STRING),     // ipt transaction
             cyng::column("status", cyng::TC_UINT32),
             cyng::column("actTime", cyng::TC_TIME_POINT),
             cyng::column("received", cyng::TC_TIME_POINT)},
            1);
    }
    cyng::meta_sql sml_db_writer::get_table_readout() { return cyng::to_sql(get_store_readout(), {0, 9, 0, 21, 0, 0, 0}); }
    cyng::meta_store sml_db_writer::get_store_readout_data() {
        return cyng::meta_store(
            "SMLreadoutData",
            {cyng::column("tag", cyng::TC_UUID),      // server/meter/sensor ID
             cyng::column("register", cyng::TC_OBIS), // OBIS code (data type)
             //   -- body
             cyng::column("reading", cyng::TC_STRING), // value as string
             cyng::column("type", cyng::TC_UINT16),    // data type code
             cyng::column("scaler", cyng::TC_UINT8),   // decimal place
             cyng::column("unit", cyng::TC_UINT8)},
            2);
    }
    cyng::meta_sql sml_db_writer::get_table_readout_data() { return cyng::to_sql(get_store_readout_data(), {0, 0, 256, 0, 0, 0}); }

} // namespace smf
