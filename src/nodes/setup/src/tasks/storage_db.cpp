/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/storage_db.h>

#include <smf.h>
#include <smf/cluster/bus.h>
#include <smf/config/persistence.h>
#include <smf/config/schemes.h>

#include <cyng/db/storage.h>
#include <cyng/io/serialize.h>
#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/sql/sql.hpp>
#include <cyng/task/channel.h>
#include <cyng/task/controller.h>

#include <smfsec/hash/sha1.h>

#include <iostream>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace smf {

    storage_db::storage_db(
        cyng::channel_weak wp,
        cyng::controller &ctl,
        bus &cluster_bus,
        cyng::store &cache,
        cyng::logger logger,
        cyng::param_map_t &&cfg,
        std::set<std::string> &&blocked_config_keys)
        : sigs_{std::bind(&storage_db::open, this), // open
        std::bind(&storage_db::load_stores, this), // load
        std::bind(&storage_db::update, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5), // update
        std::bind(&storage_db::insert, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5), // insert
        std::bind(&storage_db::remove, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), // remove
        std::bind(&storage_db::clear, this, std::placeholders::_1, std::placeholders::_2), // clear
        std::bind(&storage_db::stop, this, std::placeholders::_1) // stop
        }
        , channel_(wp)
        , ctl_(ctl)
        , cluster_bus_(cluster_bus)
        , logger_(logger)
        , db_(cyng::db::create_db_session(cfg))
        , store_(cache)
        , blocked_config_keys_(blocked_config_keys)
        , store_map_()
        , sql_map_() {

        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"open", "load", "update", "insert", "remove", "clear"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] started");
        }
    }

    void storage_db::open() {
        if (!db_.is_alive()) {
            CYNG_LOG_ERROR(logger_, "cannot open database");
        } else {
            //
            //	database is open fill cache
            //
            CYNG_LOG_INFO(logger_, "database is open");
            init_stores();
        }
    }

    void storage_db::init_stores() {
        auto const tbl_vec = get_sql_meta_data();
        for (auto const &ms : tbl_vec) {
            auto const m = cyng::to_mem(ms);
            init_store(m);

            //
            //	initialize sql map with the name of the memory table
            //
            sql_map_.emplace(m.get_name(), ms);
        }

        //
        //  init config table (without sql db)
        //
        auto const m = config::get_config();
        store_.create_table(m);
    }

    void storage_db::init_store(cyng::meta_store m) {
        store_map_.emplace(m.get_name(), m);
        if (!store_.create_table(m)) {
            CYNG_LOG_ERROR(logger_, "cannot create table: " << m.get_name());
        } else {
            CYNG_LOG_TRACE(logger_, "cache table [" << m.get_name() << "] created");
        }
    }

    void storage_db::load_stores() {

        //
        //	load data from database and subscribe it
        //
        auto const tbl_vec = get_sql_meta_data();
        for (auto const &ms : tbl_vec) {
            load_store(ms);
        }

        //
        //  load and subscribe special tables
        //
        load_config();

        //
        //  "main" node ready to start post processing
        //
        CYNG_LOG_INFO(logger_, "[storage] " << tbl_vec.size() << " tables read into the cache");
    }

    void storage_db::load_config() {

        //
        //  load config data
        //
        auto const ms = config::get_table_config();
        store_.access(
            [&](cyng::table *tbl) {
                cyng::db::storage s(db_);
                s.loop(ms, [&](cyng::record const &rec) -> bool {
                    CYNG_LOG_TRACE(logger_, "[storage] load config " << rec.to_string());

                    //
                    //  convert and transfer into "config" table
                    //
                    auto const key = rec.value("path", "");
                    auto const val = rec.value("value", "");
                    auto const type = rec.value("type", static_cast<std::uint16_t>(0));
                    auto const obj = cyng::restore(val, type);

                    tbl->insert(cyng::key_generator(key), cyng::data_generator(obj), 1, cluster_bus_.get_tag());
                    return true; //  continue
                });
            },
            cyng::access::write("config"));

        //
        //  subscribe config data
        //
        cluster_bus_.req_subscribe("config");
    }

    void storage_db::load_store(cyng::meta_sql const &ms) {
        //
        //	get in-memory meta data
        //
        auto const m = cyng::to_mem(ms);
        CYNG_LOG_INFO(logger_, "[storage] load table [" << ms.get_name() << "/" << m.get_name() << "]");

        store_.access(
            [&](cyng::table *tbl) {
                cyng::db::storage s(db_);
                s.loop(ms, [&](cyng::record const &rec) -> bool {
                    CYNG_LOG_TRACE(logger_, "[storage] load " << rec.to_string());
                    if (!tbl->insert(rec.key(), rec.data(), rec.get_generation(), cluster_bus_.get_tag())) {
                        CYNG_LOG_WARNING(
                            logger_,
                            "[storage] load table [" << ms.get_name() << "/" << m.get_name() << "]: " << rec.to_string()
                                                     << " failed");
                    }

                    return true;
                });
            },
            cyng::access::write(m.get_name()));

        CYNG_LOG_INFO(logger_, "subscribe table [" << m.get_name() << "] with " << store_.size(m.get_name()) << " entrie(s)");

        //
        //	start sync data by subscribing to this table
        //
        cluster_bus_.req_subscribe(m.get_name());
    }

    void storage_db::stop(cyng::eod) {
        db_.close();
        CYNG_LOG_WARNING(logger_, "task [" << channel_.lock()->get_name() << "] stopped");
    }

    void storage_db::update(std::string table_name, cyng::key_t key, cyng::attr_t attr, std::uint64_t gen, boost::uuids::uuid tag) {

        BOOST_ASSERT_MSG(!key.empty(), "update without key");

        if (boost::algorithm::equals(table_name, "config")) {
            //
            //  update "Config"
            //
            if (!key.empty()) {
                auto const k = cyng::io::to_plain(key.at(0));
                //  set<T>::contains is only available from C++20
                if (blocked_config_keys_.count(k) != 0) {
                    //  don't update blocked keys
                    return;
                }

                //  convert to string
                auto const v = cyng::io::to_plain(attr.second);
                CYNG_LOG_DEBUG(logger_, "[db] update config data " << key.at(0) << ": " << v);

                auto const ms = config::get_table_config();
                // auto const sql =
                // cyng::sql::update(db_.get_dialect(), ms).set_placeholder(ms, attr.first).where(ms, cyng::sql::pk())();
                auto const sql = "UPDATE TConfig SET value = ? WHERE path = ?";
                auto stmt = db_.create_statement();
                std::pair<int, bool> const r = stmt->prepare(sql);
                if (r.second) {
                    stmt->push(cyng::make_object(v), 256); //  value
                    stmt->push(key.at(0), 128);            //  key
                    if (stmt->execute()) {
                        stmt->clear();
                    } else {
                        CYNG_LOG_WARNING(logger_, "[db] update error: " << sql);
                    }
                }
            }

        } else {
            auto const pos = sql_map_.find(table_name);
            if (pos != sql_map_.end()) {

                auto const &meta = pos->second;
                //
                //  use generic function from config library
                //
                if (!config::persistent_update(meta, db_, key, attr, gen)) {
                    CYNG_LOG_ERROR(logger_, "[persistence] update " << table_name << ": " << key.at(0) << " failed");
                }

            } else {
                CYNG_LOG_WARNING(logger_, "[db] update - unknown table: " << table_name);
            }
        }
    }

    void storage_db::insert(std::string table_name, cyng::key_t key, cyng::data_t data, std::uint64_t gen, boost::uuids::uuid tag) {

        BOOST_ASSERT_MSG(!key.empty(), "insert without key");

        if (boost::algorithm::equals(table_name, "config")) {
            //
            // Convert data and store it in "TConfig" table.
            // Since "TConfig" has no cache
            //
            BOOST_ASSERT_MSG(!data.empty(), "insert without data");
            if (!key.empty() && !data.empty()) {

                //  select blocked keys
                auto const k = cyng::io::to_plain(key.at(0));
                if (blocked_config_keys_.count(k) != 0) {
                    //  don't store blocked keys
                    return;
                }

                //  convert to string
                auto const v = cyng::io::to_plain(data.at(0));
                CYNG_LOG_DEBUG(logger_, "[db] merge config data " << key.at(0) << ": \"" << v << "\"");

                auto const ms = config::get_table_config();
                auto stmt = db_.create_statement();

                //
                //  Test if key already in use
                //

                auto const sql_query = cyng::sql::select(db_.get_dialect(), ms).all(ms, false).from().where(cyng::sql::pk())();
                std::pair<int, bool> const r_query = stmt->prepare(sql_query);
                if (r_query.second) {
                    stmt->push(key.at(0), 128); //  key
                    auto res = stmt->get_result();
                    if (res) {
                        //  update
                        auto const val = cyng::value_cast(res->get(2, cyng::TC_STRING, 256), "");
                        if (boost::algorithm::equals(v, val)) {
                            CYNG_LOG_DEBUG(logger_, "[db] config entry \"" << key.at(0) << "\" is already up to date");
                        } else {
                            CYNG_LOG_INFO(logger_, "[db] update config entry " << key.at(0) << " from " << val << " ==> " << v);
                            //  "TConfig" has no "gen" column
                            auto const sql_update = "UPDATE TConfig SET value = ? WHERE path = ?";
                            std::pair<int, bool> const r_update = stmt->prepare(sql_update);
                            if (r_update.second) {
                                stmt->push(cyng::make_object(v), 256); //  value
                                stmt->push(key.at(0), 128);            //  key
                                if (stmt->execute()) {
                                    stmt->clear();
                                } else {
                                    CYNG_LOG_WARNING(logger_, "[db] update error: " << sql_update);
                                }
                            }
                        }

                    } else {
                        //  insert
                        auto const sql_insert = cyng::sql::insert(db_.get_dialect(), ms).bind_values(ms).to_string();
                        stmt->clear();
                        std::pair<int, bool> const r_insert = stmt->prepare(sql_insert);
                        if (r_insert.second) {
                            stmt->push(key.at(0), 128);                                         //  key
                            stmt->push(cyng::make_object(v), 256);                              //  value
                            stmt->push(cyng::make_object(cyng::io::to_typed(data.at(0))), 256); //  default
                            stmt->push(cyng::make_object(data.at(0).tag()), 0);                 //  type
                            stmt->push(key.at(0), 256);                                         //  description
                            if (stmt->execute()) {
                                stmt->clear();
                            } else {
                                CYNG_LOG_WARNING(logger_, "[db] insert error: " << sql_insert);
                                CYNG_LOG_DEBUG(logger_, "[db] default: " << cyng::io::to_typed(data.at(0)));
                                CYNG_LOG_DEBUG(logger_, "[db] type: " << data.at(0).tag());
                                CYNG_LOG_DEBUG(logger_, "[db] description: " << key.at(0));
                            }
                        } else {
                            CYNG_LOG_ERROR(logger_, "[db] insert error: " << sql_insert);
                        }
                    }
                }
            }

        } else {
            auto const pos = sql_map_.find(table_name);
            if (pos != sql_map_.end()) {
                auto const &meta = pos->second;
                if (!config::persistent_insert(meta, db_, key, data, gen)) {
                    CYNG_LOG_ERROR(logger_, "[persistence] insert " << table_name << ": " << key.at(0) << " failed");
                }
            }
        }
    }

    void storage_db::remove(std::string table_name, cyng::key_t key, boost::uuids::uuid tag) {

        auto const pos = sql_map_.find(table_name);
        if (pos != sql_map_.end()) {
            auto const &meta = pos->second;
            if (!config::persistent_remove(meta, db_, key)) {
                CYNG_LOG_ERROR(logger_, "[persistence] remove " << table_name << ": " << key.at(0) << " failed");
            }
        } else {
            CYNG_LOG_WARNING(logger_, "[db] remove - unknown table: " << table_name);
        }
    }

    void storage_db::clear(std::string table_name, boost::uuids::uuid tag) {

        auto const pos = sql_map_.find(table_name);
        if (pos != sql_map_.end()) {
            auto const &ms = pos->second;
            //  clear table
            auto const &meta = pos->second;
            if (!config::persistent_clear(meta, db_)) {
                CYNG_LOG_ERROR(logger_, "[persistence] clear " << table_name << " failed");
            }
        } else {
            CYNG_LOG_WARNING(logger_, "[db] clear - unknown table: " << table_name);
        }
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
                    std::cout << "create: " << m.get_name() << " - " << sql << std::endl;
                }
            }

            //
            //  "TCfgSet" has no tabular in-memory table
            //
            {
                auto const m = config::get_table_cfg_set();
                auto const sql = cyng::sql::create(db.get_dialect(), m).to_string();
                if (!db.execute(sql)) {
                    std::cerr << "**error: " << sql << std::endl;
                } else {
                    std::cout << "create: " << m.get_name() << " - " << sql << std::endl;
                }
            }

            //
            //  "TConfig" is also a special table with no in-memory counter-part
            //
            {
                auto const m = config::get_table_config();
                auto const sql = cyng::sql::create(db.get_dialect(), m).to_string();
                if (!db.execute(sql)) {
                    std::cerr << "**error: " << sql << std::endl;
                } else {
                    std::cout << "create: " << m.get_name() << " - " << sql << std::endl;
                }
            }

            return true;
        } catch (std::exception const &ex) {
            boost::ignore_unused(ex);
        }
        return false;
    }

    bool alter_table(cyng::db::session &db, std::string name) {
        BOOST_ASSERT(db.is_alive());

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
                //  take credit of non-cached tables
                //
                auto const m = config::get_table_cfg_set();
                if (boost::algorithm::equals(m.get_name(), name)) {
                    return recreate_table(db, m);
                }
                std::cerr << "**error: " << name << " not found" << std::endl;
            }

        } catch (std::exception const &ex) {
            boost::ignore_unused(ex);
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

    std::vector<cyng::meta_sql> get_sql_meta_data() {

        return {
            config::get_table_device(),
            config::get_table_meter(),
            config::get_table_meterIEC(),
            config::get_table_meterwMBus(),
            config::get_table_gateway(),
            config::get_table_lora(),
            config::get_table_gui_user(),
            config::get_table_location(),
            config::get_table_cfg_set_meta(),
            config::get_table_statistics(),
            config::get_table_history(),
            //  "TCfgSet" is handled separately since this table has
            //  a different scheme then the in-memory table.
            //  See init_storage()
            //  Same true for "TConfig".
        };
    }

    void generate_access_rights(cyng::db::session &db, std::string const &user) {

        auto ms = config::get_table_gui_user();
        auto const sql = cyng::sql::insert(db.get_dialect(), ms).bind_values(ms).to_string();
        auto stmt = db.create_statement();
        std::pair<int, bool> r = stmt->prepare(sql);
        if (r.second) {

            //	linker with undefined reference to cyng::sha1_hash(std::__cxx11::basic_string<char, std::char_traits<char>,
            // std::allocator<char> > const&)
            auto digest = cyng::sha1_hash("user");

            stmt->push(cyng::make_object(boost::uuids::random_generator()()), 36); //	pk
            stmt->push(cyng::make_object(1u), 32);                                 //	generation
            stmt->push(cyng::make_object(user), 32);                               //	val
            stmt->push(cyng::make_object(cyng::crypto::digest_sha1(digest)), 40);  //	pwd
            stmt->push(cyng::make_object(std::chrono::system_clock::now()), 0);    //	last login
            stmt->push(
                cyng::make_object(
                    "{devices: [\"view\", \"delete\", \"edit\"], meters: [\"view\", \"edit\"], gateways: [\"view\", \"delete\", \"edit\"]}"),
                4096); //	desc
            if (stmt->execute()) {
                stmt->clear();
            }
        }
    }

    void generate_random_devs(cyng::db::session &db, std::uint32_t count) {

        auto ms = config::get_table_device();
        auto const sql = cyng::sql::insert(db.get_dialect(), ms).bind_values(ms).to_string();
        auto stmt = db.create_statement();
        std::pair<int, bool> r = stmt->prepare(sql);
        if (r.second) {
            while (count-- != 0) {

                auto const tag = boost::uuids::random_generator()();
                auto const user = boost::uuids::to_string(tag);

                std::cerr << "#" << count << " user: " << user << std::endl;

                stmt->push(cyng::make_object(tag), 36);                              //	pk
                stmt->push(cyng::make_object(1u), 0);                                //	generation
                stmt->push(cyng::make_object(user), 128);                            //	name
                stmt->push(cyng::make_object(user.substr(0, 8)), 32);                //	pwd
                stmt->push(cyng::make_object(user.substr(25)), 64);                  //	msisdn
                stmt->push(cyng::make_object("#" + std::to_string(count)), 512);     //	descr
                stmt->push(cyng::make_object("model-" + std::to_string(count)), 64); //	id
                stmt->push(cyng::make_object(SMF_VERSION_SUFFIX), 64);               //	vFirmware
                stmt->push(cyng::make_object(false), 0);                             //	enabled
                stmt->push(cyng::make_object(std::chrono::system_clock::now()), 0);  //	creationTime
                if (stmt->execute()) {
                    stmt->clear();
                } else {
                    std::cerr << "**error: " << sql << std::endl;
                    return;
                }
            }
        }
    }

} // namespace smf
