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

	std::size_t cfg_broker::get_broker_count(cfg_broker::source s) const
	{
		switch (s) {
		case source::WIRELESS_LMN:
			return get_broker_count(build_cfg_key({ sml::OBIS_IF_wMBUS }, "broker-mode"));
		case source::WIRED_LMN:
			return get_broker_count(build_cfg_key({ "rs485", "broker-vector" }));
		default:
			break;
		}
		return 0;
	}

	cfg_broker::broker_list_t cfg_broker::get_broker(cfg_broker::source s) const
	{
		switch (s) {
		case source::WIRELESS_LMN:
			return get_broker(build_cfg_key({ sml::OBIS_IF_wMBUS }, "broker-vector"));
		case source::WIRED_LMN:
			return get_broker(build_cfg_key({ "rs485", "broker-vector" }));
		default:
			break;
		}
		return std::vector<cfg_broker::broker>();
	}

	cfg_broker::broker_list_t cfg_broker::get_broker(std::string path) const
	{
		std::vector<cfg_broker::broker> r;

		auto const str = cache_.get_cfg(path, "");
		if (!str.empty()) {
			//	example:
			//	user:pwd@segw.ch:12002 user:pwd@segw.ch:12004
			auto const vec = cyng::split(str, "\t ");
			for (auto const& node : vec) {
				auto const data = cyng::split(node, "@");
				if (data.size() == 2) {
					auto const cred = cyng::split(data.at(0), ":");
					auto const uri = cyng::split(data.at(1), ":");
					if (cred.size() == 2 && uri.size() == 2) {
						r.emplace_back(cred.at(0), cred.at(1), uri.at(0), static_cast<std::uint16_t>(stoul(uri.at(1))));
					}
				}
			}
		}
		return r;
	}

	std::size_t cfg_broker::get_broker_count(std::string path) const
	{
		auto const str = cache_.get_cfg(path, "");
		return (str.empty()) 
			? 0u
			: cyng::split(str, " ").size()
			;
	}

	bool cfg_broker::is_login_required(cfg_broker::source s) const
	{
		switch (s) {
		case source::WIRELESS_LMN:
			return is_login_required(build_cfg_key({ sml::OBIS_IF_wMBUS }, "broker-login"));
		case source::WIRED_LMN:
			return is_login_required(build_cfg_key({ "rs485", "broker-login" }));
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
			return get_port_name(build_cfg_key({ sml::OBIS_IF_wMBUS }, "port"));
		case source::WIRED_LMN:
			return get_port_name(build_cfg_key({ "rs485", "port" }));
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
