/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_IPT_TASK_PROXY_H
#define SMF_IPT_TASK_PROXY_H

#include <smf/cluster/bus.h>
//#include <smf/sml/reader.h>
#include <smf/sml/generator.h>
#include <smf/sml/unpack.h>

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

        using signatures_t = std::tuple<std::function<void(cyng::mac48)>, std::function<void(cyng::eod)>>;

        enum class state {
            INITIAL,
            AUTHORIZED,
        } state_;

      public:
        proxy(
            cyng::channel_weak wp,
            cyng::logger,
            std::shared_ptr<ipt_session>,
            bus &cluster_bus,
            std::string name,
            std::string pwd,
            cyng::buffer_t id);
        ~proxy();

      private:
        void stop(cyng::eod);
        void cfg_backup(cyng::mac48);

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;
        cyng::logger logger_;
        std::shared_ptr<ipt_session> iptsp_;
        bus &cluster_bus_;
        cyng::buffer_t const id_;
        sml::unpack parser_;
        sml::request_generator req_gen_;
        //  request_generator(std::string const& name, std::string const& pwd);
    };

} // namespace smf

#endif
