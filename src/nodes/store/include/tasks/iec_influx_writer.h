/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_STORE_TASK_IEC_INFLUX_WRITER_H
#define SMF_STORE_TASK_IEC_INFLUX_WRITER_H

#include <cyng/log/logger.h>
#include <cyng/task/controller.h>
#include <cyng/task/task_fwd.h>

#include <boost/uuid/uuid.hpp>

namespace smf {

    class iec_influx_writer {
        template <typename T> friend class cyng::task;

        using signatures_t = std::tuple<
            std::function<void(std::string)>,
            std::function<void(cyng::obis code, std::string value, std::string unit)>,
            std::function<void()>,
            std::function<void(cyng::eod)>>;

      public:
        iec_influx_writer(
            cyng::channel_weak,
            cyng::controller &ctl,
            cyng::logger logger,
            std::string const &host,
            std::string const &service,
            std::string const &protocol,
            std::string const &cert,
            std::string const &db);
        ~iec_influx_writer();

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
        std::string const host_;
        std::string const service_;
        std::string const protocol_; //  http/hhtps/ws
        std::string const cert_;
        std::string const db_; //  adressed database
        std::string id_;       //!< meter id
    };

} // namespace smf

#endif
