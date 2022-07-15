/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_STORE_TASK_SML_DB_WRITER_H
#define SMF_STORE_TASK_SML_DB_WRITER_H

#include <smf/mbus/server_id.h>

#include <cyng/db/session.h>
#include <cyng/log/logger.h>
#include <cyng/task/controller.h>
#include <cyng/task/task_fwd.h>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>

namespace smf {

    class sml_db_writer {
        template <typename T> friend class cyng::task;

        using signatures_t = std::tuple<
            std::function<void(cyng::buffer_t, cyng::buffer_t)>,
            std::function<void()>,
            std::function<void(std::string, cyng::buffer_t, cyng::object, std::uint32_t, cyng::obis_path_t, cyng::param_map_t)>,
            std::function<void()>,
            std::function<void(cyng::eod)>>;

      public:
        sml_db_writer(cyng::channel_weak, cyng::controller &ctl, cyng::logger logger, cyng::db::session);
        ~sml_db_writer() = default;

        static bool init_storage(cyng::db::session);

      private:
        void stop(cyng::eod);
        void open_response(cyng::buffer_t, cyng::buffer_t);
        void close_response();
        void get_profile_list_response(
            std::string,
            cyng::buffer_t,
            cyng::object,
            std::uint32_t,
            cyng::obis_path_t,
            cyng::param_map_t);
        void get_proc_parameter_response();

        void store(
            std::string trx,
            srv_id_t &&,
            cyng::obis profile,
            cyng::object act_time,
            std::uint32_t status,
            cyng::param_map_t const &values);
        void store(boost::uuids::uuid, cyng::param_map_t const &);

        static std::vector<cyng::meta_store> get_store_meta_data();
        static std::vector<cyng::meta_sql> get_sql_meta_data();

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;
        cyng::controller &ctl_;
        cyng::logger logger_;
        cyng::db::session db_;
        boost::uuids::random_generator_mt19937 uidgen_;
    };

} // namespace smf

#endif
