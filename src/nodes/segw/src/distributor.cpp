/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <config/cfg_listener.h>
#include <config/cfg_lmn.h>
#include <distributor.h>

#include <cyng/task/controller.h>

namespace smf {

    distributor::distributor(cyng::controller &ctl, cfg &config)
        : ctl_(ctl)
        , cfg_(config) {}

    void distributor::update(std::string key, cyng::object const &val) {

        update_lmn(key, lmn_type::WIRED, val);
        update_lmn(key, lmn_type::WIRELESS, val);
        if (boost::algorithm::equals(key, "listener/1/port") || boost::algorithm::equals(key, "listener/1/address")) {
            cfg_listener cfg(cfg_, lmn_type::WIRED);
            auto const task_name = cfg.get_task_name();
            ctl_.get_registry().dispatch(task_name, "rebind", cfg.get_ipv4_ep());
        }
    }

    void distributor::update_lmn(std::string const &key, lmn_type type, cyng::object const &val) {

        cfg_lmn cfg(cfg_, type);

        if (boost::algorithm::equals(key, cfg_lmn::speed_path(get_index(type)))) {
            ctl_.get_registry().dispatch(cfg.get_port(), "set-baud-rate", cyng::tuple_t{val});
        } else if (boost::algorithm::equals(key, cfg_lmn::parity_path(get_index(type)))) {
            ctl_.get_registry().dispatch(cfg.get_port(), "set-parity", cyng::tuple_t{val});
        } else if (boost::algorithm::equals(key, cfg_lmn::flow_control_path(get_index(type)))) {
            ctl_.get_registry().dispatch(cfg.get_port(), "set-flow-control", cyng::tuple_t{val});
        } else if (boost::algorithm::equals(key, cfg_lmn::stopbits_path(get_index(type)))) {
            ctl_.get_registry().dispatch(cfg.get_port(), "set-stopbits", cyng::tuple_t{val});
        } else if (boost::algorithm::equals(key, cfg_lmn::databits_path(get_index(type)))) {
            ctl_.get_registry().dispatch(cfg.get_port(), "set-databits", cyng::tuple_t{val});
        }
    }

} // namespace smf
