/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_BROKER_WMBUS_TASK_PUSH_H
#define SMF_BROKER_WMBUS_TASK_PUSH_H

#include <smf/ipt/bus.h>
#include <smf/sml/generator.h>

#include <cyng/log/logger.h>
#include <cyng/task/controller.h>
#include <cyng/task/task_fwd.h>

#include <boost/uuid/uuid.hpp>

namespace smf {

    class push {
        template <typename T> friend class cyng::task;

        using signatures_t = std::tuple<
            std::function<void(void)>,                                                           //  connect
            std::function<void(cyng::buffer_t srv, cyng::buffer_t)>,                             //  send_sml
            std::function<void(cyng::buffer_t srv, cyng::tuple_t)>,                              //  send_mbus
            std::function<void(void)>,                                                           //  send_dlms
            std::function<void(bool, std::uint32_t, std::uint32_t, std::uint32_t, std::string)>, //  on_channel_open
            std::function<void(cyng::eod)>>;

        enum class protocol_type { SML, IEC, DLMS };

      public:
        push(
            cyng::channel_weak,
            cyng::controller &,
            cyng::logger,
            ipt::toggle::server_vec_t &&,
            ipt::push_channel &&pcc_sml,
            ipt::push_channel &&pcc_iec,
            ipt::push_channel &&pcc_dlms);
        ~push() = default;

      private:
        void stop(cyng::eod);
        void connect();
        void send_sml(cyng::buffer_t srv, cyng::buffer_t payload);
        void send_mbus(cyng::buffer_t srv, cyng::tuple_t);
        void send_dlms();

        //
        //	bus interface
        //
        void ipt_cmd(ipt::header const &, cyng::buffer_t &&);
        void ipt_stream(cyng::buffer_t &&);
        void auth_state(bool, boost::asio::ip::tcp::endpoint, boost::asio::ip::tcp::endpoint);
        void open_push_channels();

        void on_channel_open(bool, std::uint32_t, std::uint32_t, std::uint32_t, std::string);

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;
        cyng::logger logger_;
        ipt::toggle::server_vec_t toggle_;
        ipt::push_channel const pcc_sml_;
        ipt::push_channel const pcc_iec_;
        ipt::push_channel const pcc_dlms_;
        ipt::bus bus_;
        /**
         * maintain a list of open channels
         */
        std::map<protocol_type, std::pair<std::uint32_t, std::uint32_t>> channels_;

        sml::response_generator sml_generator_;
    };

} // namespace smf

#endif
