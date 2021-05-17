/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_STORE_TASK_NETWORK_H
#define SMF_STORE_TASK_NETWORK_H

#include <smf/ipt/bus.h>

#include <cyng/log/logger.h>
#include <cyng/task/controller.h>
#include <cyng/task/stash.h>
#include <cyng/task/task_fwd.h>

#include <boost/uuid/uuid.hpp>

namespace smf {

    class network {
        template <typename T> friend class cyng::task;

        using signatures_t = std::tuple<std::function<void()>, std::function<void(cyng::eod)>>;

      public:
        network(
            std::weak_ptr<cyng::channel>,
            cyng::controller &,
            boost::uuids::uuid tag,
            cyng::logger logger,
            std::string const &node_name,
            std::string const &model,
            ipt::toggle::server_vec_t &&,
            std::vector<std::string> const &config_types,
            std::vector<std::string> const &sml_targets,
            std::vector<std::string> const &iec_targets,
            std::vector<std::string> const &writer);
        ~network();

        void stop(cyng::eod);

      private:
        void connect();

        //
        //	bus interface
        //
        void ipt_cmd(ipt::header const &, cyng::buffer_t &&);
        void ipt_stream(cyng::buffer_t &&);
        void auth_state(bool);

        void register_targets();

      private:
        signatures_t sigs_;
        std::weak_ptr<cyng::channel> channel_;
        cyng::controller &ctl_;
        boost::uuids::uuid const tag_;
        cyng::logger logger_;
        ipt::toggle::server_vec_t toggle_;

        std::vector<std::string> const sml_targets_;
        std::vector<std::string> const iec_targets_;
        std::vector<std::string> const writer_;

        ipt::bus bus_;

        cyng::stash stash_;
    };

} // namespace smf

#endif
