/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include <cfg_gpio.h>
#include <segw.h>
#include <cache.h>


#include <cyng/value_cast.hpp>
#include <boost/core/ignore_unused.hpp>

namespace node
{
	//
	//	GPIO configuration
	//
	cfg_gpio::cfg_gpio(cache& c)
		: cache_(c)
	{}

	std::vector<std::uint32_t> cfg_gpio::get_vector() const
	{
		std::vector<std::uint32_t> vec;
		for (std::size_t idx = 1; ; idx++) {
			auto const pin = cache_.get_cfg<std::uint32_t>(build_cfg_key({ "gpio", "pin", std::to_string(idx) }), 0u);
			if (pin == 0)	break;
			vec.push_back(pin);
		}
		return vec;
	}

	bool cfg_gpio::set_task(std::uint32_t pin, std::size_t tsk)
	{
		return cache_.set_cfg(build_cfg_key({ "gpio", "task", std::to_string(pin) }), cyng::make_object(tsk));
	}

	std::size_t cfg_gpio::get_task(std::uint32_t pin)
	{
		return cache_.get_cfg<std::size_t>(build_cfg_key({ "gpio", "task", std::to_string(pin) }), 0u);
	}

	cyng::filesystem::path cfg_gpio::get_path() const
	{
		return cache_.get_cfg<std::string>(build_cfg_key({ "gpio", "path" }), "/sys/class/gpio");
	}


	cyng::filesystem::path cfg_gpio::get_path(std::uint32_t pin) const
	{
		return get_path() / ("gpio" + std::to_string(pin));
	}

	bool cfg_gpio::is_enabled() const
	{
		return  cache_.get_cfg(build_cfg_key({ "gpio", "enabled" }),
#if defined(__ARMEL__)
			true
#else
			false
#endif
		);
	}

}
