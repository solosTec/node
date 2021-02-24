/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <config/cfg_sml.h>

 
#ifdef _DEBUG_SEGW
#include <iostream>
#endif


namespace smf {

	cfg_sml::cfg_sml(cfg& c)
		: cfg_(c)
	{}

	//	sml/accept-all-ids|false|false|1|check server IDs
	//	sml/account|operator|operator|14|SML login name
	//	sml/address|0.0.0.0|0.0.0.0|14|SML bind address
	//	sml/discover|5798|5798|14|SML discover port
	//	sml/enabled|true|true|1|SML enabled
	//	sml/pwd|operator|operator|14|SML login password
	//	sml/service|7259|7259|14|SML service port

	boost::asio::ip::tcp::endpoint cfg_sml::get_ep() const {
		return { get_address(), get_port() };
	}

	boost::asio::ip::address cfg_sml::get_address() const {
		return cfg_.get_value(cyng::to_path('/', "sml", "address"), boost::asio::ip::address());
	}

	std::uint16_t cfg_sml::get_port() const {
		return cfg_.get_value(cyng::to_path('/', "sml", "port"), static_cast<std::uint16_t>(7259));
	}

	std::uint16_t cfg_sml::get_discovery_port() const {
		return cfg_.get_value(cyng::to_path('/', "sml", "discover"), static_cast<std::uint16_t>(5798));
	}

	std::string cfg_sml::get_account() const {
		return cfg_.get_value(cyng::to_path('/', "sml", "account"), "");
	}

	std::string cfg_sml::get_pwd() const {
		return cfg_.get_value(cyng::to_path('/', "sml", "pwd"), "");
	}

	bool cfg_sml::is_enabled() const {
		return cfg_.get_value(cyng::to_path('/', "sml", "enabled"), false);
	}

}