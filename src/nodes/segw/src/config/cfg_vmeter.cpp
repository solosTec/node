/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <config/cfg_vmeter.h>

 
#ifdef _DEBUG_SEGW
#include <iostream>
#endif


namespace smf {

	cfg_vmeter::cfg_vmeter(cfg& c)
		: cfg_(c)
	{}

	namespace {
		std::string enabled_path() {
			return cyng::to_path(cfg::sep, cfg_vmeter::root, "enabled");
		}
		std::string server_path() {
			return cyng::to_path(cfg::sep, cfg_vmeter::root, "server");
		}
		std::string interval_path() {
			return cyng::to_path(cfg::sep, cfg_vmeter::root, "interval");
		}
		std::string port_index_path() {
			return cyng::to_path(cfg::sep, cfg_vmeter::root, "port-index");
		}
		std::string protocol_path() {
			return cyng::to_path(cfg::sep, cfg_vmeter::root, "protocol");
		}
	}

	bool cfg_vmeter::is_enabled() const {
		return cfg_.get_value(enabled_path(), false);
	}
	std::string cfg_vmeter::get_server() const {
		return cfg_.get_value(server_path(), "");
	}
	std::string cfg_vmeter::get_protocol() const {
		return cfg_.get_value(protocol_path(), "auto");
	}
	std::size_t cfg_vmeter::get_port_index() const {
		return cfg_.get_value(port_index_path(), static_cast<std::size_t>(1));
	}

}