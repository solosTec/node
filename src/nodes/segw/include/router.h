/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_ROUTER_H
#define SMF_SEGW_ROUTER_H

#include <cfg.h>
#include <config/cfg_hardware.h>
#include <config/cfg_ipt.h>

#include <sml/response_engine.h>

#include <smf/ipt/bus.h>
#include <smf/sml/generator.h>
#include <smf/sml/msg.h>
#include <smf/sml/unpack.h>

#include <cyng/task/task_fwd.h>
#include <cyng/vm/vm_fwd.h>

namespace smf {

    class storage;

    /**
     * routing of IP-T/SML messages
     */
    class router {
      public:
        router(cyng::logger, cyng::controller &, cfg &config, storage &);
        bool start();
        void stop();

      private:
        void ipt_cmd(ipt::header const &, cyng::buffer_t &&);
        void ipt_stream(cyng::buffer_t &&);
        void auth_state(bool, boost::asio::ip::tcp::endpoint, boost::asio::ip::tcp::endpoint);
        void register_targets();
        void reply();

        void generate_open_response(
            std::string trx,
            std::string,
            cyng::buffer_t,
            cyng::buffer_t,
            std::string,
            std::string,
            std::string,
            std::uint8_t);
        void generate_close_response(std::string trx, cyng::object gsign);

        void init_ipt_push();
        void stop_ipt_push();
        void init_ipt_store();
        void stop_ipt_store();

      private:
        cyng::logger logger_;
        cyng::controller &ctl_;
        cfg &cfg_;
        cfg_ipt ipt_cfg_;
        cfg_hardware hw_cfg_;
        ipt::bus bus_;
        sml::unpack parser_;
        sml::messages_t messages_; //	sml response
        sml::response_generator res_gen_;

        //
        //  response engines
        //
        response_engine engine_;
    };

} // namespace smf

#endif
