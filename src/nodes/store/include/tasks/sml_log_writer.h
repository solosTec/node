/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_STORE_TASK_SML_LOG_WRITER_H
#define SMF_STORE_TASK_SML_LOG_WRITER_H

#include <cyng/log/log.h>
#include <cyng/log/logger.h>
#include <cyng/log/record.h>
#include <cyng/task/controller.h>
#include <cyng/task/task_fwd.h>

#include <boost/uuid/uuid.hpp>

namespace smf {

    class sml_log_writer {
        template <typename T> friend class cyng::task;

        using signatures_t = std::tuple<
            std::function<void(cyng::buffer_t, cyng::buffer_t)>,
            std::function<void()>,
            std::function<void(std::string, cyng::buffer_t, cyng::object, std::uint32_t, cyng::obis_path_t, cyng::param_map_t)>,
            std::function<void()>,
            std::function<void(cyng::eod)>>;

      public:
        sml_log_writer(
            cyng::channel_weak,
            cyng::controller &ctl,
            cyng::logger logger,
            std::filesystem::path out,
            std::string prefix,
            std::string suffix);
        ~sml_log_writer() = default;

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

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;
        cyng::controller &ctl_;
        cyng::logger logger_, protocol_;
    };

    std::filesystem::path sml_log_filename(std::string prefix, std::string suffix);

} // namespace smf

#endif
