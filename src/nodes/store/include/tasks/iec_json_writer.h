/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_STORE_TASK_IEC_JSON_WRITER_H
#define SMF_STORE_TASK_IEC_JSON_WRITER_H

#include <cyng/log/logger.h>
#include <cyng/task/controller.h>
#include <cyng/task/task_fwd.h>

#include <boost/uuid/uuid.hpp>

namespace smf {

    class iec_json_writer {
        template <typename T> friend class cyng::task;

        using signatures_t = std::tuple<
            std::function<void(std::string)>,
            std::function<void(cyng::obis code, std::string value, std::string unit)>,
            std::function<void()>,
            std::function<void(cyng::eod)> //  stop/(
            >;

      public:
        iec_json_writer(
            cyng::channel_weak,
            cyng::controller &ctl,
            cyng::logger logger,
            std::filesystem::path out,
            std::string prefix,
            std::string suffix);
        ~iec_json_writer() = default;

      private:
        void stop(cyng::eod);
        void open(std::string);
        void store(cyng::obis code, std::string value, std::string unit);
        void commit();

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;
        cyng::controller &ctl_;
        cyng::logger logger_;
        std::filesystem::path const root_dir_;
        std::string const prefix_;
        std::string const suffix_;
    };

    std::filesystem::path
    iec_json_filename(std::string prefix, std::string suffix, std::string server_id, std::chrono::system_clock::time_point now);

} // namespace smf

#endif
