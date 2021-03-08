/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <distributor.h>
#include <config/cfg_lmn.h>

#include <cyng/task/controller.h>

namespace smf {

	distributor::distributor(cyng::controller& ctl, cfg& config)
		: ctl_(ctl)
		, cfg_(config)
	{}

	void distributor::update(std::string key, cyng::object const& val) {

		update_lmn(key, lmn_type::WIRED, val);
		update_lmn(key, lmn_type::WIRELESS, val);

	}

	void distributor::update_lmn(std::string const& key, lmn_type type, cyng::object const& val) {

		cfg_lmn cfg(cfg_, type);

		if (boost::algorithm::equals(key, cfg_lmn::speed_path(get_index(type)))) {
			ctl_.get_registry().dispatch(cfg.get_port(), "set-baud-rate", cyng::tuple_t{ val });
		}
		else if (boost::algorithm::equals(key, cfg_lmn::parity_path(get_index(type)))) {
			ctl_.get_registry().dispatch(cfg.get_port(), "set-parity", cyng::tuple_t{ val });
		}
		else if (boost::algorithm::equals(key, cfg_lmn::flow_control_path(get_index(type)))) {
			ctl_.get_registry().dispatch(cfg.get_port(), "set-flow-control", cyng::tuple_t{ val });
		}
		else if (boost::algorithm::equals(key, cfg_lmn::stopbits_path(get_index(type)))) {
			ctl_.get_registry().dispatch(cfg.get_port(), "set-stopbits", cyng::tuple_t{ val });
		}
		else if (boost::algorithm::equals(key, cfg_lmn::databits_path(get_index(type)))) {
			ctl_.get_registry().dispatch(cfg.get_port(), "set-databits", cyng::tuple_t{ val });
		}

	}

}