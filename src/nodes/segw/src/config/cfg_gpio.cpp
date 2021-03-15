/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <config/cfg_gpio.h>

#include <cyng/obj/numeric_cast.hpp>

#ifdef _DEBUG_SEGW
#include <iostream>
#endif

namespace smf {

	namespace {
		std::string enabled_path() {
			return cyng::to_path(cfg::sep, cfg_gpio::root, "enabled");
		}
		std::string pin_path(std::size_t idx) {
			return cyng::to_path(cfg::sep, cfg_gpio::root, "pin", std::to_string(idx));
		}
	}

	cfg_gpio::cfg_gpio(cfg& c)
		: cfg_(c)
	{}

	bool cfg_gpio::is_enabled() const {
		return cfg_.get_value(enabled_path(), true);
	}

	/**
	 * @return a list of all GPIO IDs
	 */
	std::vector<std::uint32_t> cfg_gpio::get_pins() const {
		std::vector<std::uint32_t> vec;
		for (std::size_t idx = 1; ; idx++) {
			auto const pin = cyng::numeric_cast<std::uint32_t>(cfg_.get_obj(pin_path(idx)), 0u);
			if (pin == 0u)	break;
			vec.push_back(pin);
		}
		return vec;
	}

	std::filesystem::path cfg_gpio::get_path() const {
		return cfg_.get_value(cyng::to_path('/', "gpio", "path"), "/sys/class/gpio");
	}

	std::filesystem::path cfg_gpio::get_path(std::uint32_t pin) const {
		return get_path() / ("gpio" + std::to_string(pin));
	}

	std::string cfg_gpio::get_name(std::uint32_t pin) {
		return "gpio" + std::to_string(pin);
	}

	std::uint32_t cfg_gpio::get_pin(lmn_type type) {
		switch (type) {
		case lmn_type::WIRELESS:	return 50u;
		case lmn_type::WIRED:		return 47u;
		case lmn_type::ETHERNET:	return 46u;	//	the blue one
		default:
			break;
		}
		return 53u;	//	power 
	}

	std::string cfg_gpio::get_name(lmn_type type) {
		return get_name(get_pin(type));
	}

}