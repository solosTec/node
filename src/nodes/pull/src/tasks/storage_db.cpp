
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

// #include <smfsec/hash/sha1.h>

#include <iostream>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace smf {

    storage_db::storage_db(
        cyng::channel_weak wp,
        cyng::controller &ctl,
        bus &cluster_bus,
        cyng::logger logger,
        cyng::param_map_t &&cfg)
        : sigs_{std::bind(&storage_db::open, this), // open
        std::bind(&storage_db::subscribe, this), // subscribe
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
        , sql_map_() {

        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"open", "subscribe", "update", "insert", "remove", "clear"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] started");
        }
    }

    void storage_db::open() {
        if (!db_.is_alive()) {
            CYNG_LOG_ERROR(logger_, "cannot open database");
        } else {
            CYNG_LOG_INFO(logger_, "database is open");
            //
            //	database is open - init sql map
            //
            init_sql_map();
        }
    }

    void storage_db::init_sql_map() {
        auto const tbl_vec = get_sql_meta_data();
        for (auto const &ms : tbl_vec) {
            auto const m = cyng::to_mem(ms);

            //
            //	initialize sql map with the name of the memory table
            //
            CYNG_LOG_INFO(logger_, "init SQL table " << m.get_name());
            sql_map_.emplace(m.get_name(), ms);
        }
        CYNG_LOG_INFO(logger_, "[persistence] " << sql_map_.size() << " tables initialized");
    }
    void storage_db::stop(cyng::eod) {
        db_.close();
        CYNG_LOG_WARNING(logger_, "task [" << channel_.lock()->get_name() << "] stopped");
    }

    void storage_db::update(std::string table_name, cyng::key_t key, cyng::attr_t attr, std::uint64_t gen, boost::uuids::uuid tag) {

        BOOST_ASSERT_MSG(!key.empty(), "update without key");
        BOOST_ASSERT_MSG(!boost::algorithm::equals(table_name, "config"), "config table not supported");

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

    void storage_db::insert(std::string table, cyng::key_t key, cyng::data_t data, std::uint64_t gen, boost::uuids::uuid tag) {

        BOOST_ASSERT_MSG(!key.empty(), "insert without key");
        BOOST_ASSERT_MSG(!boost::algorithm::equals(table, "config"), "config table not supported");

        auto const pos = sql_map_.find(table);
        if (pos != sql_map_.end()) {
            auto const &meta = pos->second;
            if (!config::persistent_insert(meta, db_, key, data, gen)) {
                CYNG_LOG_ERROR(logger_, "[persistence] insert " << table << ": " << key.at(0) << " failed");
            } else {
                CYNG_LOG_DEBUG(logger_, "[cluster] db insert into table \"" << table << "\"");
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

    void storage_db::subscribe() {
        CYNG_LOG_INFO(logger_, "[persistence] subscribe " << sql_map_.size() << " tables");
        for (auto const &tbl : sql_map_) {
            CYNG_LOG_INFO(logger_, "[persistence] subscribe table " << tbl.first);
            cluster_bus_.req_subscribe(tbl.first);
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
            //  "TConfig" is also a special table with no in-memory counter-part
            //

            return true;
        } catch (std::exception const &ex) {
            boost::ignore_unused(ex);
        }
        return false;
    }

    std::vector<cyng::meta_sql> get_sql_meta_data() {

        return {
            config::get_table_device(), // TDevice
            config::get_table_meter(),  // TMeter
            // config::get_table_meterIEC(),
            // config::get_table_meterwMBus(),
            // config::get_table_gateway(),
            // config::get_table_lora(),
            // config::get_table_gui_user(),
            // config::get_table_location(),
            // config::get_table_cfg_set_meta(),
            // config::get_table_statistics(),
            // config::get_table_history(),
            //   "TCfgSet" is handled separately since this table has
            //   a different scheme then the in-memory table.
            //   See init_storage()
            //   Same true for "TConfig".
        };
    }

} // namespace smf
