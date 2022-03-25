/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/storage_db.h>

#include <smf.h>
#include <smf/cluster/bus.h>
#include <smf/config/schemes.h>

#include <cyng/db/details/statement_interface.h>
#include <cyng/db/storage.h>
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
        cyng::param_map_t &&cfg)
        : sigs_{std::bind(&storage_db::open, this), std::bind(&storage_db::load_stores, this), std::bind(&storage_db::update, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5), std::bind(&storage_db::insert, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5), std::bind(&storage_db::remove, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), std::bind(&storage_db::clear, this, std::placeholders::_1, std::placeholders::_2), std::bind(&storage_db::stop, this, std::placeholders::_1)}
        , channel_(wp)
        , ctl_(ctl)
        , cluster_bus_(cluster_bus)
        , logger_(logger)
        , db_(cyng::db::create_db_session(cfg))
        , store_(cache)
        , store_map_()
        , sql_map_() {

        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"open", "load", "update", "insert", "remove", "clear"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] started");
        }
    }

    storage_db::~storage_db() {
#ifdef _DEBUG_SETUP
        std::cout << "storage_db(~)" << std::endl;
#endif
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
        //	load data from database
        //
        auto const tbl_vec = get_sql_meta_data();
        for (auto const &ms : tbl_vec) {
            load_store(ms);
        }

        //
        //  "main" node ready to start post processing
        //
        CYNG_LOG_INFO(logger_, "[storage] " << tbl_vec.size() << " tables read into the cache");
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
                    tbl->insert(rec.key(), rec.data(), rec.get_generation(), cluster_bus_.get_tag());

                    return true;
                });
            },
            cyng::access::write(m.get_name()));

        CYNG_LOG_INFO(logger_, "table [" << m.get_name() << "] holds " << store_.size(m.get_name()) << " entries");

        //
        //	start sync task
        //
        cluster_bus_.req_subscribe(m.get_name());
    }

    void storage_db::stop(cyng::eod) {
        db_.close();
        CYNG_LOG_WARNING(logger_, "task [" << channel_.lock()->get_name() << "] stopped");
    }

    void storage_db::update(std::string table_name, cyng::key_t key, cyng::attr_t attr, std::uint64_t gen, boost::uuids::uuid tag) {

        auto const pos = sql_map_.find(table_name);
        if (pos != sql_map_.end()) {

            auto const &meta = pos->second;
            auto const sql =
                cyng::sql::update(db_.get_dialect(), meta).set_placeholder(meta, attr.first).where(meta, cyng::sql::pk())();

            auto stmt = db_.create_statement();
            std::pair<int, bool> const r = stmt->prepare(sql);
            if (r.second) {
                BOOST_ASSERT(r.first == 2 + key.size()); //	attribute to "gen"

                auto const width = meta.get_body_column(attr.first).width_;
                stmt->push(attr.second, width);
                stmt->push(cyng::make_object(gen), 0); //	name

                //
                //	key
                //
                for (auto &kp : key) {
                    stmt->push(kp, 36); //	pk
                }

                if (stmt->execute()) {
                    stmt->clear();
                }

            } else {
                CYNG_LOG_WARNING(logger_, "[db] update error: " << sql);
            }
        } else {
            CYNG_LOG_WARNING(logger_, "[db] update - unknown table: " << table_name);
        }
    }

    void storage_db::insert(std::string table_name, cyng::key_t key, cyng::data_t data, std::uint64_t gen, boost::uuids::uuid tag) {

        auto const pos = sql_map_.find(table_name);
        if (pos != sql_map_.end()) {
            auto const &meta = pos->second;
            auto const sql = cyng::sql::insert(db_.get_dialect(), meta).bind_values(meta).to_str();

            auto stmt = db_.create_statement();
            std::pair<int, bool> const r = stmt->prepare(sql);
            if (r.second) {
                BOOST_ASSERT(r.first == data.size() + key.size() + 1);

                //
                //	pk
                //
                std::size_t col_index{0};
                for (auto &kp : key) {
                    auto const width = meta.get_column(col_index).width_;
                    stmt->push(kp, width); //	pk
                    ++col_index;
                }

                //
                //	gen
                //
                stmt->push(cyng::make_object(gen), 0);
                ++col_index;

                //
                //	body
                //
                for (auto &val : data) {
                    auto const width = meta.get_column(col_index).width_;
                    stmt->push(val, width);
                    ++col_index;
                }

                if (stmt->execute()) {
                    stmt->clear();
                }

            } else {
                CYNG_LOG_WARNING(logger_, "[db] insert error: " << sql);
            }
        } else {
            CYNG_LOG_WARNING(logger_, "[db] insert - unknown table: " << table_name);
        }
    }

    void storage_db::remove(std::string table_name, cyng::key_t key, boost::uuids::uuid tag) {

        auto const pos = sql_map_.find(table_name);
        if (pos != sql_map_.end()) {
            auto const &meta = pos->second;
            auto const sql = cyng::sql::remove(db_.get_dialect(), meta).self().where(meta, cyng::sql::pk())();

            auto stmt = db_.create_statement();
            std::pair<int, bool> const r = stmt->prepare(sql);
            if (r.second) {

                BOOST_ASSERT(r.first == key.size());

                //
                //	pk
                //
                std::size_t col_index{0};
                for (auto &kp : key) {
                    auto const width = meta.get_column(col_index).width_;
                    stmt->push(kp, width); //	pk
                    ++col_index;
                }

                if (stmt->execute()) {
                    stmt->clear();
                }

            } else {
                CYNG_LOG_WARNING(logger_, "[db] delete error: " << sql);
            }
        } else {
            CYNG_LOG_WARNING(logger_, "[db] remove - unknown table: " << table_name);
        }
    }

    void storage_db::clear(std::string table_name, boost::uuids::uuid tag) {

        auto const pos = sql_map_.find(table_name);
        if (pos != sql_map_.end()) {
            auto const &ms = pos->second;

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
                auto const sql = cyng::sql::create(db.get_dialect(), m).to_str();
                if (!db.execute(sql)) {
                    std::cerr << "**error: " << sql << std::endl;
                } else {
                    std::cout << "create: " << m.get_name() << " - " << sql << std::endl;
                }
            }

            //
            //  TCfgSet has no tabular in-memory table
            //
            auto const m = config::get_table_cfg_set();
            auto const sql = cyng::sql::create(db.get_dialect(), m).to_str();
            if (!db.execute(sql)) {
                std::cerr << "**error: " << sql << std::endl;
            } else {
                std::cout << "create: " << m.get_name() << " - " << sql << std::endl;
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
        auto const sql_drop = cyng::sql::drop(db.get_dialect(), m).to_str();
        if (!db.execute(sql_drop)) {
            std::cerr << "**error: " << sql_drop << std::endl;
        } else {
            std::cout << m.get_name() << " - " << sql_drop << std::endl;

            //
            //  create table
            //
            auto const sql_create = cyng::sql::create(db.get_dialect(), m).to_str();
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
            config::get_table_cfg_set_meta()};
    }

    void generate_access_rights(cyng::db::session &db, std::string const &user) {

        auto ms = config::get_table_gui_user();
        auto const sql = cyng::sql::insert(db.get_dialect(), ms).bind_values(ms).to_str();
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
        auto const sql = cyng::sql::insert(db.get_dialect(), ms).bind_values(ms).to_str();
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
