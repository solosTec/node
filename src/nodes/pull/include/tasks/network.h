/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_PULL_TASK_NETWORK_H
#define SMF_PULL_TASK_NETWORK_H

#include <smf/ipt/bus.h>

#include <cyng/log/logger.h>
#include <cyng/task/controller.h>
#include <cyng/task/stash.h>
#include <cyng/task/task_fwd.h>

#include <boost/uuid/uuid.hpp>

namespace smf {

    class network {
        template <typename T> friend class cyng::task;

        using signatures_t = std::tuple<
            std::function<void()>, // connect
            // std::function<void(cyng::param_map_t)>, // start
            std::function<void(cyng::eod)> // stop
            >;

      public:
        network(
            cyng::channel_weak,
            cyng::controller &,
            boost::uuids::uuid tag,
            cyng::logger logger,
            std::string const &node_name,
            std::string const &model,
            ipt::toggle::server_vec_t &&,
            std::set<std::string> const &targets);

        ~network() = default;

        void stop(cyng::eod);

      private:
        void connect();

        //
        //	bus interface
        //
        void ipt_cmd(ipt::header const &, cyng::buffer_t &&);
        void ipt_stream(cyng::buffer_t &&);
        void auth_state(bool, boost::asio::ip::tcp::endpoint, boost::asio::ip::tcp::endpoint);

        void register_targets();

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;
        cyng::controller &ctl_;
        boost::uuids::uuid const tag_;
        cyng::logger logger_;
        ipt::toggle::server_vec_t toggle_;
        std::set<std::string> const targets_;
        ipt::bus bus_;

        cyng::stash stash_;
    };

} // namespace smf

#endif
