﻿/*
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
			//	segw.ch:12002 segw.ch:12004
			auto const vec = cyng::split(str, "\t ");
			for (auto const& node : vec) {
				auto const data = cyng::split(node, ":");
				if (data.size() == 2) {
					r.emplace_back(data.at(0), static_cast<std::uint16_t>(stoul(data.at(1))));
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

	cfg_broker::broker::broker(std::string address, std::uint16_t port)
		: address_(address)
		, port_(port)
	{}

	std::string const& cfg_broker::broker::get_address() const
	{
		return address_;
	}

	std::uint16_t cfg_broker::broker::get_port() const
	{
		return port_;
	}

}
