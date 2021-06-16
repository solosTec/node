/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_ROUTER_H
#define SMF_SEGW_ROUTER_H

#include <cfg.h>

#include <smf/ipt/bus.h>
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
        void auth_state(bool);
        void register_targets();

        void generate_open_response(std::string trx, cyng::tuple_t const &msg);
        void generate_close_response(std::string trx, cyng::tuple_t const &msg);

      private:
        cyng::controller &ctl_;
        cfg &cfg_;
        cyng::logger logger_;
        std::unique_ptr<ipt::bus> bus_;
        sml::unpack parser_;
        sml::messages_t messages_; //	sml response
    };
} // namespace smf

#endif
