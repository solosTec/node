/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_STORE_TASK_IEC_DB_WRITER_H
#define SMF_STORE_TASK_IEC_DB_WRITER_H

#include <cyng/db/session.h>
#include <cyng/log/logger.h>
#include <cyng/task/controller.h>
#include <cyng/task/task_fwd.h>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/random_generator.hpp>

namespace smf {

    class iec_db_writer {
        template <typename T> friend class cyng::task;

        using signatures_t = std::tuple<
            std::function<void(std::string)>,
            std::function<void(cyng::obis code, std::string value, std::string unit)>,
            std::function<void()>,
            std::function<void(cyng::eod)>>;

      public:
        iec_db_writer(cyng::channel_weak, cyng::controller &ctl, cyng::logger logger, cyng::db::session);
        iec_db_writer() = default;

        static bool init_storage(cyng::db::session);

      private:
        void stop(cyng::eod);
        void open(std::string);
        void store(cyng::obis code, std::string value, std::string unit);
        void commit();

        static std::vector<cyng::meta_store> get_store_meta_data();
        static std::vector<cyng::meta_sql> get_sql_meta_data();

        static cyng::meta_store get_store_readout();
        static cyng::meta_sql get_table_readout();
        static cyng::meta_store get_store_readout_data();
        static cyng::meta_sql get_table_readout_data();

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;
        cyng::controller &ctl_;
        cyng::logger logger_;
        cyng::db::session db_;
        boost::uuids::random_generator_mt19937 uidgen_;
        std::string id_;
    };

} // namespace smf

#endif
