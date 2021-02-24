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
		return cfg_.get_value(cyng::to_path('/', "nms", "address"), boost::asio::ip::address());
	}

	std::uint16_t cfg_nms::get_port() const {
		return cfg_.get_value(cyng::to_path('/', "nms", "port"), static_cast<std::uint16_t>(7261));
	}

	std::string cfg_nms::get_account() const {
		return cfg_.get_value(cyng::to_path('/', "nms", "account"), "");
	}

	std::string cfg_nms::get_pwd() const {
		return cfg_.get_value(cyng::to_path('/', "nms", "pwd"), "");
	}

	bool cfg_nms::is_enabled() const {
		return cfg_.get_value(cyng::to_path('/', "nms", "enabled"), false);
	}

	std::filesystem::path cfg_nms::get_script_path() const {
		return cfg_.get_value(cyng::to_path('/', "nms", "script-path"), std::filesystem::temp_directory_path());
	}

}