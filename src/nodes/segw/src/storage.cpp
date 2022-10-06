/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <storage.h>
#include <storage_functions.h>

#include <cyng/db/storage.h>
#include <cyng/sql/sql.hpp>
#include <cyng/task/controller.h>

namespace smf {

    storage::storage(cyng::db::session db)
        : db_(db) {}

    bool storage::cfg_insert(cyng::object const &key, cyng::object const &obj) {

        //
        //	start transaction
        //
        cyng::db::transaction trx(db_);

        //
        //	prepare statement
        //
        auto const m = get_table_cfg();
        auto const sql = cyng::sql::insert(db_.get_dialect(), m).bind_values(m)();

        auto stmt = db_.create_statement();
        std::pair<int, bool> const r = stmt->prepare(sql);
        return (r.second) ? insert_config_record(stmt, key, obj) : false;
    }

    bool storage::cfg_update(cyng::object const &key, cyng::object const &obj) {

        //
        //	start transaction
        //
        cyng::db::transaction trx(db_);

        //
        //	prepare statement
        //
        auto const m = get_table_cfg();
        // auto const sql = cyng::sql::update(db_.get_dialect(), m).set_placeholder(m)();
        auto const sql = "UPDATE TCfg SET value = ? WHERE path = ?";

        auto stmt = db_.create_statement();
        std::pair<int, bool> const r = stmt->prepare(sql);
        return (r.second) ? update_config_record(stmt, key, obj) : false;
    }

    bool storage::cfg_remove(cyng::object const &key) {
        //
        //	start transaction
        //
        cyng::db::transaction trx(db_);

        //
        //	prepare statement
        //
        // auto const m = get_table_cfg();
        // auto const sql = cyng::sql::remove(db_.get_dialect(), m)()
        auto const sql = "DELETE FROM TCfg WHERE path = ?";

        auto stmt = db_.create_statement();
        std::pair<int, bool> const r = stmt->prepare(sql);
        return (r.second) ? remove_config_record(stmt, key) : false;
    }

    cyng::object storage::cfg_read(cyng::object const &key, cyng::object def) {
        // auto const sql = "SELECT val FROM TCfg WHERE path = ?";
        auto const m = get_table_cfg();
        auto sql = cyng::sql::select(db_.get_dialect(), m).all(m, false).from().where(cyng::sql::pk())();

        auto stmt = db_.create_statement();
        std::pair<int, bool> const r = stmt->prepare(sql);
        return (r.second) ? read_config_record(stmt, key) : def;
    }

    void storage::generate_op_log(
        std::uint64_t status,
        std::uint32_t evt,
        cyng::obis peer,
        cyng::buffer_t srv,
        std::string target,
        std::uint8_t nr,
        std::string details) {
        //
        //	start transaction
        //
        cyng::db::transaction trx(db_);

        //
        //	prepare statement
        //
        auto const m = get_table_oplog();
        auto const sql = cyng::sql::insert(db_.get_dialect(), m).bind_values(m)();

        auto stmt = db_.create_statement();
        std::pair<int, bool> const r = stmt->prepare(sql);

        if (r.second) {
            BOOST_ASSERT(r.first == 13);
            auto const now = std::chrono::system_clock::now();

            //	--skip ROWID
            stmt->push(cyng::make_object(1u), 0);                                            //	gen
            stmt->push(cyng::make_object(now), 0);                                           //	actTime
            stmt->push(cyng::make_object(now), 0);                                           //	age
            stmt->push(cyng::make_object<std::uint32_t>(0u), 0);                             //	regPeriod
            stmt->push(cyng::make_object<std::uint32_t>(now.time_since_epoch().count()), 0); //	valTime
            stmt->push(cyng::make_object(status), 0);                                        //	status
            stmt->push(cyng::make_object(evt), 0);                                           //	event
            stmt->push(cyng::make_object(peer), 0);                                          //	peer
            stmt->push(cyng::make_object(now), 0);                                           //	utc
            stmt->push(cyng::make_object(std::move(srv)), 23);                               //	serverId
            stmt->push(cyng::make_object(target), 64);                                       //	target
            stmt->push(cyng::make_object(nr), 0);                                            //	pushNr
            stmt->push(cyng::make_object(details), 128);                                     //	details

            stmt->execute();
        }
    }

    void storage::loop_oplog(
        std::chrono::system_clock::time_point const &begin,
        std::chrono::system_clock::time_point const &end,
        std::function<bool(cyng::record &&)> cb) {

        //
        //	start transaction
        //
        cyng::db::transaction trx(db_);

        auto const ms = get_table_oplog();
        auto const sql = cyng::sql::select(db_.get_dialect(), ms)
                             .all(ms, true)
                             .from()
                             .where(std::string("utc between julianday(?) and julianday(?)"))
                             .to_string();
        // std::string const sql =
        //     "select gen, datetime(actTime), datetime(age), regPeriod, valTime, status, event, peer, datetime(utc), serverId,
        //     target, pushNr, details from TOpLog where utc between julianday(?) and julianday(?) order by utc;";
        // auto const m = get_table_oplog();

        auto stmt = db_.create_statement();
        std::pair<int, bool> const r = stmt->prepare(sql);

        if (r.second) {
            stmt->push(cyng::make_object(begin), 0); //	from
            stmt->push(cyng::make_object(end), 0);   //	to
            //
            //	read all results
            //
            while (auto res = stmt->get_result()) {

                //
                //	Convert SQL result to record
                //	false terminates the loop
                //
                if (!cb(cyng::to_record(ms, res)))
                    break;
            }
        }

        // cyng::db::storage s(db_);
        // s.loop(get_table_oplog(), [&](cyng::record const &rec) -> bool {
        //   [ROWID: 00000000000005bd][actTime: 1969-12-31T23:59:59+0100][age: 2022-05-10T05:53:33+0100][regPeriod:
        //   00000000][valTime: 1969-12-31T23:59:59+0100][status: 0000000000022202][event: 00100023][peer: null][utc:
        //   2022-05-10T05:53:33+0100][serverId: 0500155d85dcfb][target: ][pushNr: 0][details: power return]
        //  std::cout << rec.to_string() << std::endl;
        //    return true;
        //});
    }

} // namespace smf
