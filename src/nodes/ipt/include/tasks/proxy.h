/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_IPT_TASK_PROXY_H
#define SMF_IPT_TASK_PROXY_H

#include <smf/cluster/bus.h>

#include <cyng/log/logger.h>
#include <cyng/task/controller.h>
#include <cyng/task/task_fwd.h>

#include <boost/uuid/uuid.hpp>

namespace smf {

    class ipt_session;
    /**
     * SML proxy
     */
    class proxy {
        template <typename T> friend class cyng::task;

        using signatures_t = std::tuple<std::function<void(std::string, std::string, std::string)>, std::function<void(cyng::eod)>>;

        enum class state {
            INITIAL,
            AUTHORIZED,
        } state_;

      public:
        proxy(cyng::channel_weak wp, cyng::logger, std::shared_ptr<ipt_session>, bus &cluster_bus);
        ~proxy();

      private:
        void stop(cyng::eod);
        void cfg_backup(std::string name, std::string pwd, std::string id);

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;
        cyng::logger logger_;
        std::shared_ptr<ipt_session> iptsp_;
        bus &cluster_bus_;
    };

} // namespace smf

#endif
