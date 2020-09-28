/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "cfg_broker.h"
#include "segw.h"
#include "cache.h"
#include <smf/sml/obis_db.h>
#include <smf/sml/intrinsics/obis_factory.hpp>

#include <cyng/value_cast.hpp>
#include <cyng/util/split.h>

#include <boost/core/ignore_unused.hpp>
#include <boost/predef.h>

namespace node
{
	//
	//	broker configuration
	//
	cfg_broker::cfg_broker(cache& c)
		: cache_(c)
	{}

	cfg_broker::broker_list_t cfg_broker::get_broker(cfg_broker::source s) const
	{
		switch (s) {
		case source::WIRELESS_LMN:
		case source::WIRED_LMN:
			return get_broker(static_cast<std::uint8_t>(s));
		default:
			break;
		}
		return std::vector<cfg_broker::broker>();
	}

	cfg_broker::broker_list_t cfg_broker::get_broker(std::uint8_t port_idx) const
	{
		std::vector<cfg_broker::broker> r;

		//	
		//	test for up to 16 broker
		//	
		for (std::uint8_t idx = 1; idx < 16; ++idx) {

			auto const address = cache_.get_cfg(build_cfg_key({ 
				sml::OBIS_ROOT_BROKER, 
				sml::make_obis(sml::OBIS_ROOT_BROKER, port_idx),
				sml::make_obis(sml::OBIS_BROKER_SERVER, idx) }), "?");

			if (!boost::algorithm::equals(address, "?")) {

				auto const port = cache_.get_cfg(build_cfg_key({
					sml::OBIS_ROOT_BROKER,
					sml::make_obis(sml::OBIS_ROOT_BROKER, port_idx),
					sml::make_obis(sml::OBIS_BROKER_SERVICE, idx) }), static_cast<std::uint16_t>(12001u));
				auto const account = cache_.get_cfg(build_cfg_key({
					sml::OBIS_ROOT_BROKER,
					sml::make_obis(sml::OBIS_ROOT_BROKER, port_idx),
					sml::make_obis(sml::OBIS_BROKER_USER, idx) }), "");
				auto const pwd = cache_.get_cfg(build_cfg_key({
					sml::OBIS_ROOT_BROKER,
					sml::make_obis(sml::OBIS_ROOT_BROKER, port_idx),
					sml::make_obis(sml::OBIS_BROKER_PWD, idx) }), "");

				r.emplace_back(account, pwd, address, port);
			}
		}

		return r;
	}

	bool cfg_broker::is_login_required(cfg_broker::source s) const
	{
		switch (s) {
		case source::WIRELESS_LMN:
		case source::WIRED_LMN:
			return is_login_required(build_cfg_key({ 
				sml::OBIS_ROOT_BROKER, 
				sml::make_obis(sml::OBIS_ROOT_BROKER, static_cast<std::uint8_t>(s)),
				sml::OBIS_BROKER_LOGIN }));
		default:
			break;
		}
		return false;
	}

	bool cfg_broker::is_login_required(std::string path) const
	{
		return cache_.get_cfg(path, true);
	}

	std::string cfg_broker::get_port_name(cfg_broker::source s) const
	{
		switch (s) {
		case source::WIRELESS_LMN:
		case source::WIRED_LMN:
			return get_port_name(build_cfg_key({ sml::OBIS_ROOT_HARDWARE_PORT, sml::make_obis(sml::OBIS_HARDWARE_PORT_NAME, static_cast<std::uint8_t>(s)) }));
		default:
			break;
		}
		return "";
	}

	std::string cfg_broker::get_port_name(std::string path) const
	{
		return cache_.get_cfg(path, "");
	}


	cfg_broker::broker::broker(std::string account, std::string pwd, std::string address, std::uint16_t port)
		: account_(account)
		, pwd_(pwd)
		, address_(address)
		, port_(port)
	{}

	std::string const& cfg_broker::broker::get_account() const
	{
		return account_;
	}

	std::string const& cfg_broker::broker::get_pwd() const
	{
		return pwd_;
	}

	std::string const& cfg_broker::broker::get_address() const
	{
		return address_;
	}

	std::uint16_t cfg_broker::broker::get_port() const
	{
		return port_;
	}

}
