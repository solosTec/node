/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SETUP_TASK_STORAGE_JSON_H
#define SMF_SETUP_TASK_STORAGE_JSON_H

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
    class storage_json {
        template <typename T> friend class cyng::task;

        using signatures_t = std::tuple<
            std::function<void(void)>,
            std::function<
                void(std::string table_name, cyng::key_t key, cyng::attr_t attr, std::uint64_t gen, boost::uuids::uuid tag)>,
            std::function<void(std::string, cyng::key_t key, cyng::data_t data, std::uint64_t gen, boost::uuids::uuid tag)>,
            std::function<void(std::string, cyng::key_t key, boost::uuids::uuid tag)>,
            std::function<void(std::string, boost::uuids::uuid tag)>,
            std::function<void(cyng::eod)>>;

      public:
        storage_json(
            cyng::channel_weak,
            cyng::controller &,
            bus &,
            cyng::store &cache,
            cyng::logger logger,
            cyng::param_map_t &&cfg,
            std::set<std::string> &&);
        ~storage_json();

        void stop(cyng::eod);

      private:
        void open();
        void update(std::string table_name, cyng::key_t key, cyng::attr_t attr, std::uint64_t gen, boost::uuids::uuid tag);

        void insert(std::string, cyng::key_t key, cyng::data_t data, std::uint64_t gen, boost::uuids::uuid tag);

        void remove(std::string, cyng::key_t key, boost::uuids::uuid tag);

        void clear(std::string, boost::uuids::uuid tag);

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;
        cyng::controller &ctl_;
        bus &cluster_bus_;
        cyng::logger logger_;
        cyng::store &store_;
    };

} // namespace smf

#endif
