/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <config/cfg_blocklist.h>
#include <config/cfg_lmn.h>
#include <config/cfg_broker.h>

#include <smf/mbus/radio/header.h>

#include <cyng/obj/container_factory.hpp>

namespace smf {


	cfg_blocklist::cfg_blocklist(cfg& c, lmn_type type)
		: cfg_(c)
		, type_(type)
	{}

	std::string cfg_blocklist::get_path_id() const {
		return std::to_string(get_index());
	}

	bool cfg_blocklist::is_lmn_enabled() const {
		return cfg_lmn(cfg_, type_).is_enabled();
	}

	namespace {
		std::string enabled_path(std::uint8_t type) {
			return cyng::to_path(cfg::sep, cfg_blocklist::root, std::to_string(type), "enabled");
		}
		std::string mode_path(std::uint8_t type) {
			return cyng::to_path(cfg::sep, cfg_blocklist::root, std::to_string(type), "mode");
		}
		std::string list_path(std::uint8_t type) {
			return cyng::to_path(cfg::sep, cfg_blocklist::root, std::to_string(type), "list");
		}
		std::string size_path(std::uint8_t type) {
			return cyng::to_path(cfg::sep, cfg_blocklist::root, std::to_string(type), "size");
		}
		std::string meter_path(std::uint8_t type, std::size_t idx) {
			return cyng::to_path(cfg::sep, cfg_blocklist::root, std::to_string(type), "meter", idx);
		}
		std::string period_path(std::size_t type) {
			return cyng::to_path(cfg::sep, cfg_blocklist::root, std::to_string(type), "period");
		}
	}

	bool cfg_blocklist::is_enabled() const {
		return cfg_.get_value(enabled_path(get_index()), false);
	}

	std::string cfg_blocklist::get_mode() const {
		return cfg_.get_value(mode_path(get_index()), "drop");
	}

	bool cfg_blocklist::is_drop_mode() const {
		return boost::algorithm::equals("drop", get_mode());
	}

	std::vector<std::string> cfg_blocklist::get_list() const {

		std::vector<std::string> list;
		for (std::size_t idx = 0; idx < size(); ++idx) {
			auto const meter = cfg_.get_value(meter_path(get_index(), idx), "");
			if (!meter.empty())	list.push_back(meter);
		}
		return list;
	}

	std::set<std::string> cfg_blocklist::get_set() const {

		std::set<std::string> list;
		for (std::size_t idx = 0; idx < size(); ++idx) {
			auto const meter = cfg_.get_value(meter_path(get_index(), idx), "");
			if (!meter.empty())	list.insert(meter);
		}
		return list;
	}

	bool cfg_blocklist::is_listed(std::string id) const {
		auto const list = get_set();

#ifdef _MSC_VER
		return list.contains(id);
#else
		return list.count(id) > 0;
#endif
	}


	std::chrono::seconds cfg_blocklist::get_max_frequency() const {
		return cfg_.get_value(period_path(get_index()), std::chrono::seconds(10));
	}

	bool cfg_blocklist::is_max_frequency_enabled() const {
		return get_max_frequency().count() > 0;
	}

	std::string cfg_blocklist::get_task_name() const {
		return "filter@" + cfg_broker(cfg_, type_).get_port();
	}

	std::size_t cfg_blocklist::size() const {
		return cfg_.get_value(size_path(get_index()), static_cast<std::size_t>(0));
	}

	bool cfg_blocklist::set_enabled(bool b) const {
		return cfg_.set_value(enabled_path(get_index()), b);
	}

	bool cfg_blocklist::set_mode(std::string mode) const {
		return cfg_.set_value(mode_path(get_index()), mode);
	}

	bool cfg_blocklist::set_list(std::vector<std::string> const& vec) const {

		std::size_t idx{ 0 };
		for (auto const& meter : vec) {
			cfg_.set_value(meter_path(get_index(), idx), meter);
		}

		BOOST_ASSERT(idx == vec.size());
		return cfg_.set_value(size_path(get_index()), idx);
	}

}