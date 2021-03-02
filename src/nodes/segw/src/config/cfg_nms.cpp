/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <config/cfg_nms.h>
 
#ifdef _DEBUG_SEGW
#include <iostream>
#endif


namespace smf {

	namespace {
		std::string address_path() {
			return cyng::to_path('/', cfg_nms::root, "address");
		}
		std::string port_path() {
			return cyng::to_path('/', cfg_nms::root, "port");
		}
		std::string account_path() {
			return cyng::to_path('/', cfg_nms::root, "account");
		}
		std::string pwd_path() {
			return cyng::to_path('/', cfg_nms::root, "pwd");
		}
		std::string enabled_path() {
			return cyng::to_path('/', cfg_nms::root, "enabled");
		}
		std::string debug_path() {
			return cyng::to_path('/', cfg_nms::root, "debug");
		}
		std::string script_path() {
			return cyng::to_path('/', cfg_nms::root, "script-path");
		}
	}

	cfg_nms::cfg_nms(cfg& c)
		: cfg_(c)
	{}

	//	nms/account|operator|operator|14|NMS login name
	//	nms/address|0.0.0.0|0.0.0.0|14|NMS bind address
	//	nms/enabled|false|false|1|NMS enabled
	//	nms/port|1c5d|1c5d|7|NMS listener port (7261)
	//	nms/pwd|operator|operator|14|NMS login password
	//	nms/script-path|"C:\\Users\\WWW\\AppData\\Local\\Temp\\update-script.cmd"|"C:\\Users\\Sylko\\AppData\\Local\\Temp\\update-script.cmd"|15|path to update script

	boost::asio::ip::tcp::endpoint cfg_nms::get_ep() const {
		return { get_address(), get_port() };
	}

	boost::asio::ip::address cfg_nms::get_address() const {
		return cfg_.get_value(address_path(), boost::asio::ip::address());
	}

	bool cfg_nms::set_address(boost::asio::ip::address address) const {
		return cfg_.set_value(address_path(), address);
	}

	std::uint16_t cfg_nms::get_port() const {
		return cfg_.get_value(port_path(), static_cast<std::uint16_t>(7261));
	}

	bool cfg_nms::set_port(std::uint16_t port) const {
		return cfg_.set_value(port_path(), port);
	}

	std::string cfg_nms::get_account() const {
		return cfg_.get_value(account_path(), "");
	}

	bool cfg_nms::set_account(std::string account) const {
		return cfg_.set_value(account_path(), account);
	}

	std::string cfg_nms::get_pwd() const {
		return cfg_.get_value(pwd_path(), "");
	}

	bool cfg_nms::set_pwd(std::string pwd) const {
		return cfg_.set_value(pwd_path(), pwd);
	}

	bool cfg_nms::is_enabled() const {
		return cfg_.get_value(enabled_path(), false);
	}

	bool cfg_nms::set_enabled(bool b) const {
		return cfg_.set_value(enabled_path(), b);
	}

	bool cfg_nms::is_debug() const {
		return cfg_.get_value(debug_path(), false);
	}

	bool cfg_nms::set_debug(bool b) const {
		return cfg_.set_value(debug_path(), b);
	}

	std::filesystem::path cfg_nms::get_script_path() const {
		return cfg_.get_value(script_path(), std::filesystem::temp_directory_path());
	}

}