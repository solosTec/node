/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_MODEM_TASK_GATEKEEPER_H
#define SMF_MODEM_TASK_GATEKEEPER_H

#include <smf/cluster/bus.h>

#include <cyng/log/logger.h>
#include <cyng/task/controller.h>
#include <cyng/task/task_fwd.h>

#include <boost/uuid/uuid.hpp>

namespace smf {

    class modem_session;
    class gatekeeper {
        template <typename T> friend class cyng::task;

        using signatures_t = std::tuple<std::function<void()>, std::function<void(cyng::eod)>>;

      public:
        gatekeeper(cyng::channel_weak wp, cyng::logger, std::shared_ptr<modem_session>, bus &cluster_bus);
        ~gatekeeper();

        void stop(cyng::eod);

      private:
        void timeout();

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;
        cyng::logger logger_;
        std::shared_ptr<modem_session> modemsp_;
        bus &cluster_bus_;
    };

} // namespace smf

#endif
