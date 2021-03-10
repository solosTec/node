/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <config/cfg_listener.h>
#include <config/cfg_lmn.h>

#include <cyng/obj/container_factory.hpp>

namespace smf {


	cfg_listener::cfg_listener(cfg& c, lmn_type type)
		: cfg_(c)
		, type_(type)
	{}

	std::string cfg_listener::get_path_id() const {
		return std::to_string(get_index());
	}

	bool cfg_listener::is_lmn_enabled() const {
		return cfg_lmn(cfg_, type_).is_enabled();
	}

	std::string cfg_listener::get_port_name() const {
		return cfg_lmn(cfg_, type_).get_port();
	}

	namespace {
		std::string address_path(std::uint8_t type) {
			return cyng::to_path(cfg::sep, cfg_listener::root, std::to_string(type), "address");
		}
		std::string port_path(std::uint8_t type) {
			return cyng::to_path(cfg::sep, cfg_listener::root, std::to_string(type), "port");
		}
		std::string enabled_path(std::uint8_t type) {
			return cyng::to_path(cfg::sep, cfg_listener::root, std::to_string(type), "enabled");
		}
		std::string login_path(std::uint8_t type) {
			return cyng::to_path(cfg::sep, cfg_listener::root, std::to_string(type), "login");
		}
	}

	boost::asio::ip::address cfg_listener::get_address() const {
		return cfg_.get_value(address_path(get_index()), boost::asio::ip::address());
	}

	std::uint16_t cfg_listener::get_port() const {
		return cfg_.get_value(port_path(get_index()), static_cast<std::uint16_t>(12000));
	}

	boost::asio::ip::tcp::endpoint cfg_listener::get_ep() const {
		return { get_address(), get_port() };
	}

	bool cfg_listener::is_enabled() const {
		return cfg_.get_value(enabled_path(get_index()), false);
	}

	bool cfg_listener::has_login() const {
		return cfg_.get_value(login_path(get_index()), false);
	}

	bool cfg_listener::set_address(std::string address) const {
		return cfg_.set_value(address_path(get_index()), address);
	}

	bool cfg_listener::set_port(std::uint16_t port) const {
		return cfg_.set_value(port_path(get_index()), port);
	}

	bool cfg_listener::set_login(bool b) const {
		return cfg_.set_value(login_path(get_index()), b);
	}

	bool cfg_listener::set_enabled(bool b) const {
		return cfg_.set_value(enabled_path(get_index()), b);
	}

	std::ostream& operator<<(std::ostream& os, cfg_listener const& cfg) {
		os 
			<< cfg.get_port_name()
			<< '@'
			<< cfg.get_address()
			<< ':'
			<< cfg.get_port()
			;
		return os;
	}

}