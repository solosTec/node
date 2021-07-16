/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_ROUTER_H
#define SMF_SEGW_ROUTER_H

#include <cfg.h>
#include <sml/response_engine.h>

#include <smf/ipt/bus.h>
#include <smf/sml/generator.h>
#include <smf/sml/msg.h>
#include <smf/sml/unpack.h>

#include <cyng/task/task_fwd.h>
#include <cyng/vm/vm_fwd.h>

namespace smf {

    /**
     * routing of IP-T/SML messages
     */
    class router {
      public:
        router(cyng::controller &, cfg &config, cyng::logger);
        void start();
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

      private:
        cyng::controller &ctl_;
        cfg &cfg_;
        cyng::logger logger_;
        std::unique_ptr<ipt::bus> bus_;
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
