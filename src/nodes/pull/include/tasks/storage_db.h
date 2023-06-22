/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_PULL_TASK_STORAGE_DB_H
#define SMF_PULL_TASK_STORAGE_DB_H

#include <smf/config/schemes.h>

#include <cyng/db/session.h>
#include <cyng/log/logger.h>
#include <cyng/obj/intrinsics/eod.h>
#include <cyng/store/db.h>
#include <cyng/task/task_fwd.h>

#include <functional>
#include <memory>
#include <string>
#include <tuple>

#include <boost/uuid/uuid.hpp>

namespace smf {

    class bus;
    class storage_db {
        template <typename T> friend class cyng::task;

        using signatures_t = std::tuple<
            std::function<void(void)>, // open
            std::function<void(void)>, // subscribe
            std::function<
                void(std::string table_name, cyng::key_t key, cyng::attr_t attr, std::uint64_t gen, boost::uuids::uuid tag)>,
            std::function<void(std::string, cyng::key_t key, cyng::data_t data, std::uint64_t gen, boost::uuids::uuid tag)>,
            std::function<void(std::string, cyng::key_t key, boost::uuids::uuid tag)>,
            std::function<void(std::string, boost::uuids::uuid tag)>,
            std::function<void(cyng::eod)>>;

      public:
        storage_db(cyng::channel_weak, cyng::controller &, bus &, cyng::logger logger, cyng::param_map_t &&cfg);
        ~storage_db() = default;

        void stop(cyng::eod);

      private:
        void open();
        void init_sql_map();

        void update(std::string table_name, cyng::key_t key, cyng::attr_t attr, std::uint64_t gen, boost::uuids::uuid tag);

        void insert(std::string, cyng::key_t key, cyng::data_t data, std::uint64_t gen, boost::uuids::uuid tag);

        void remove(std::string, cyng::key_t key, boost::uuids::uuid tag);

        void clear(std::string, boost::uuids::uuid tag);

        void subscribe();

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;
        cyng::controller &ctl_;
        bus &cluster_bus_;
        cyng::logger logger_;
        cyng::db::session db_;

        config::sql_map sql_map_;
    };

    bool init_storage(cyng::db::session &db);
    std::vector<cyng::meta_sql> get_sql_meta_data();
} // namespace smf

#endif
