/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <config/cfg_listener.h>
#include <config/cfg_lmn.h>
#include <config/cfg_nms.h>
#include <distributor.h>

#include <cyng/log/record.h>
#include <cyng/task/controller.h>

namespace smf {

    distributor::distributor(cyng::logger logger, cyng::controller &ctl, cfg &config)
        : logger_(logger)
        , ctl_(ctl)
        , cfg_(config) {}

    void distributor::update(std::string key, cyng::object const &val) {

        update_lmn(key, lmn_type::WIRED, val);
        update_lmn(key, lmn_type::WIRELESS, val);

        //
        //  If the listener port was changed the acceptor of the listener server
        //  must be bound to the new endpoint.
        //
        if (boost::algorithm::equals(key, "listener/1/port") || boost::algorithm::equals(key, "listener/1/address")) {

            cfg_listener cfg(cfg_, lmn_type::WIRED);

            //
            //  update IPv4 listener
            //
            auto const task_id_ipv4 = cfg.get_IPv4_task_id();
            if (task_id_ipv4 != 0) {
                auto const ep = cfg.get_ipv4_ep();
                CYNG_LOG_INFO(logger_, "[distributor] set global listener tcp/ip endpoint to " << ep);
                ctl_.get_registry().dispatch(
                    task_id_ipv4,
                    "restart",
                    //  handle dispatch errors
                    std::bind(cyng::log_dispatch_error, logger_, std::placeholders::_1, std::placeholders::_2),
                    cyng::make_tuple());
            }

            //
            //  update IPv6 listener (link-local)
            //
            auto const task_id_ipv6 = cfg.get_IPv6_task_id();
            if (task_id_ipv6 != 0) {

                auto const ep = cfg.get_link_local_ep();
                CYNG_LOG_INFO(logger_, "[distributor] use link-local tcp/ip endpoint " << ep);
                ctl_.get_registry().dispatch(
                    task_id_ipv6,
                    "restart",
                    //  handle dispatch errors
                    std::bind(cyng::log_dispatch_error, logger_, std::placeholders::_1, std::placeholders::_2),
                    cyng::make_tuple());
            }
        }
    }

    void distributor::update_lmn(std::string const &key, lmn_type type, cyng::object const &val) {

        cfg_lmn cfg(cfg_, type);

        if (boost::algorithm::equals(key, cfg_lmn::speed_path(get_index(type)))) {
            ctl_.get_registry().dispatch(
                cfg.get_port(),
                "set-baud-rate",
                //  handle dispatch errors
                std::bind(cyng::log_dispatch_error, logger_, std::placeholders::_1, std::placeholders::_2),
                cyng::tuple_t{val});
        } else if (boost::algorithm::equals(key, cfg_lmn::parity_path(get_index(type)))) {
            ctl_.get_registry().dispatch(
                cfg.get_port(),
                "set-parity",
                //  handle dispatch errors
                std::bind(cyng::log_dispatch_error, logger_, std::placeholders::_1, std::placeholders::_2),
                cyng::tuple_t{val});
        } else if (boost::algorithm::equals(key, cfg_lmn::flow_control_path(get_index(type)))) {
            ctl_.get_registry().dispatch(
                cfg.get_port(),
                "set-flow-control",
                //  handle dispatch errors
                std::bind(cyng::log_dispatch_error, logger_, std::placeholders::_1, std::placeholders::_2),
                cyng::tuple_t{val});
        } else if (boost::algorithm::equals(key, cfg_lmn::stopbits_path(get_index(type)))) {
            ctl_.get_registry().dispatch(
                cfg.get_port(),
                "set-stopbits",
                //  handle dispatch errors
                std::bind(cyng::log_dispatch_error, logger_, std::placeholders::_1, std::placeholders::_2),
                cyng::tuple_t{val});
        } else if (boost::algorithm::equals(key, cfg_lmn::databits_path(get_index(type)))) {
            ctl_.get_registry().dispatch(
                cfg.get_port(),
                "set-databits",
                //  handle dispatch errors
                std::bind(cyng::log_dispatch_error, logger_, std::placeholders::_1, std::placeholders::_2),
                cyng::tuple_t{val});
        }
    }

} // namespace smf
