/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef IEC_STORE_TASK_SML_CSV_WRITER_H
#define IEC_STORE_TASK_SML_CSV_WRITER_H

#include <cyng/log/logger.h>
#include <cyng/task/controller.h>
#include <cyng/task/task_fwd.h>

#include <fstream>

#include <boost/uuid/uuid.hpp>

namespace smf {

    class iec_csv_writer {
        template <typename T> friend class cyng::task;

        using signatures_t = std::tuple<
            std::function<void(std::string)>,
            std::function<void(cyng::obis code, std::string value, std::string unit)>,
            std::function<void()>,
            std::function<void(cyng::eod)>>;

      public:
        iec_csv_writer(
            cyng::channel_weak,
            cyng::controller &ctl,
            cyng::logger logger,
            std::filesystem::path out,
            std::string prefix,
            std::string suffix,
            bool header);
        ~iec_csv_writer();

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
        std::filesystem::path const out_;
        std::ofstream ostream_;
        std::string id_; //	current meter id/name
    };

} // namespace smf

#endif
