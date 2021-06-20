/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_BROKER_WMBUS_TASK_PUSH_H
#define SMF_BROKER_WMBUS_TASK_PUSH_H

#include <smf/ipt/bus.h>

#include <cyng/log/logger.h>
#include <cyng/task/controller.h>
#include <cyng/task/task_fwd.h>

#include <boost/uuid/uuid.hpp>

namespace smf {

    class push {
        template <typename T> friend class cyng::task;

        using signatures_t = std::tuple<
            std::function<void(void)>,                                                     //  connect
            std::function<void(cyng::buffer_t payload)>,                                   //  send_iec
            std::function<void(std::uint32_t, std::uint32_t, std::uint32_t, std::string)>, //  on_channel_open
            std::function<void(cyng::eod)>                                                 //  stop
            >;

      public:
        push(cyng::channel_weak, cyng::controller &, cyng::logger, ipt::toggle::server_vec_t &&, ipt::push_channel const &pcc);
        ~push();

        void stop(cyng::eod);

      private:
        void connect();

        //
        //	bus interface
        //
        void ipt_cmd(ipt::header const &, cyng::buffer_t &&);
        void ipt_stream(cyng::buffer_t &&);
        void auth_state(bool);

        void send_iec(cyng::buffer_t payload);
        void on_channel_open(std::uint32_t, std::uint32_t, std::uint32_t, std::string);

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;
        cyng::logger logger_;
        ipt::toggle::server_vec_t toggle_;
        ipt::push_channel const pcc_;
        ipt::bus bus_;
        std::pair<std::uint32_t, std::uint32_t> id_;
    };

} // namespace smf

#endif
