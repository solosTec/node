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

        using signatures_t = std::tuple<std::function<void(cyng::eod)>>;

      public:
        iec_influx_writer(cyng::channel_weak, cyng::controller &ctl, cyng::logger logger);
        ~iec_influx_writer();

      private:
        void stop(cyng::eod);

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;
        cyng::controller &ctl_;
        cyng::logger logger_;
    };

} // namespace smf

#endif
