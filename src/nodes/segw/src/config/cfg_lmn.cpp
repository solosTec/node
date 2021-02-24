/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <config/cfg_lmn.h>

namespace smf {

	cfg_lmn::cfg_lmn(cfg& c, lmn_type type)
		: cfg_(c)
		, type_(type)
	{}

	std::uint8_t cfg_lmn::get_index() const {
		return static_cast<std::uint8_t>(type_);
	}
	std::string cfg_lmn::get_path_id() const {
		return std::to_string(get_index());
	}

	std::string cfg_lmn::get_port() const {
		return cfg_.get_value(cyng::to_path('/', "lmn", get_path_id(), "port"), "");
	}

	bool cfg_lmn::is_enabled() const {
		return cfg_.get_value(cyng::to_path('/', "lmn", get_path_id(), "enabled"), true);
	}

	boost::asio::serial_port_base::baud_rate cfg_lmn::get_baud_rate() const {
		return boost::asio::serial_port_base::baud_rate(
			cfg_.get_value(cyng::to_path('/', "lmn", get_path_id(), "speed"), static_cast<std::uint32_t>(2400))
		);
	}

	boost::asio::serial_port_base::parity cfg_lmn::get_parity() const {
		return serial::to_parity(cfg_.get_value(cyng::to_path('/', "lmn", get_path_id(), "parity"), "none"));
	}

	boost::asio::serial_port_base::flow_control cfg_lmn::get_flow_control() const {
		return serial::to_flow_control(cfg_.get_value(cyng::to_path('/', "lmn", get_path_id(), "flow-control"), "none"));
	}

	boost::asio::serial_port_base::stop_bits cfg_lmn::get_stopbits() const {
		return serial::to_stopbits(cfg_.get_value(cyng::to_path('/', "lmn", get_path_id(), "stopbits"), "one"));
	}

	boost::asio::serial_port_base::character_size cfg_lmn::get_databits() const {
		return boost::asio::serial_port_base::character_size(
			cfg_.get_value(cyng::to_path('/', "lmn", get_path_id(), "databits"), static_cast<std::uint8_t>(8))
		);
	}

	bool cfg_lmn::is_broker_enabled() const {
		return cfg_.get_value(cyng::to_path('/', "lmn", get_path_id(), "broker-enabled"), false);
	}

	bool cfg_lmn::has_broker_login() const {
		return cfg_.get_value(cyng::to_path('/', "lmn", get_path_id(), "broker-login"), false);
	}

	std::string cfg_lmn::get_type() const {
		return cfg_.get_value(cyng::to_path('/', "lmn", get_path_id(), "type"), "");
	}

}