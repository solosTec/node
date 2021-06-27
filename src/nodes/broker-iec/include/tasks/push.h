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
            std::function<void(void)>,                                                           //  connect
            std::function<void()>,                                                               //  open
            std::function<void()>,                                                               //  close
            std::function<void(cyng::buffer_t payload)>,                                         //  forward
            std::function<void(bool, std::uint32_t, std::uint32_t, std::uint32_t, std::string)>, //  on_channel_open
            std::function<void(bool, std::uint32_t)>,                                            //  on_channel_close
            std::function<void(cyng::eod)>                                                       //  stop
            >;

      public:
        push(
            cyng::channel_weak,
            cyng::controller &,
            cyng::logger,
            ipt::toggle::server_vec_t &&,
            ipt::push_channel const &pcc,
            std::string const &client);
        ~push();

      private:
        void stop(cyng::eod);
        void open();
        void connect();

        //
        //	bus interface
        //
        void ipt_cmd(ipt::header const &, cyng::buffer_t &&);
        void ipt_stream(cyng::buffer_t &&);
        void auth_state(bool);

        void send_iec(cyng::buffer_t payload);
        void on_channel_open(bool success, std::uint32_t, std::uint32_t, std::uint32_t, std::string);
        void on_channel_close(bool success, std::uint32_t);

        /**
         * cache data, open channel and send to target
         */
        void forward(cyng::buffer_t payload);
        void close();

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;
        cyng::registry &registry_;
        cyng::logger logger_;
        ipt::push_channel const pcc_;
        std::string const client_task_;
        ipt::bus bus_;
        ipt::channel_id id_;
        /**
         * Buffer for outgoing data.
         */
        std::deque<cyng::buffer_t> buffer_write_;
    };

} // namespace smf

#endif
