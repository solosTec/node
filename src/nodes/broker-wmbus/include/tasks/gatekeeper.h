/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_BROKER_WMBUS_TASK_GATEKEEPER_H
#define SMF_BROKER_WMBUS_TASK_GATEKEEPER_H

#include <cyng/log/logger.h>
#include <cyng/task/controller.h>
#include <cyng/task/task_fwd.h>

#include <boost/uuid/uuid.hpp>

namespace smf {

    class wmbus_session;
    class gatekeeper {
        template <typename T> friend class cyng::task;

        using signatures_t = std::tuple<std::function<void()>, std::function<void(cyng::eod)>>;

      public:
        gatekeeper(cyng::channel_weak, cyng::logger, std::shared_ptr<wmbus_session>);
        ~gatekeeper();

        void stop(cyng::eod);

      private:
        void timeout();

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;
        cyng::logger logger_;
        std::shared_ptr<wmbus_session> wmbussp_;
    };

} // namespace smf

#endif
