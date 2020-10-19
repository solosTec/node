/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "cfg_mbus.h"
#include "segw.h"
#include "cache.h"
#include <smf/sml/obis_db.h>

#include <cyng/value_cast.hpp>
#include <cyng/numeric_cast.hpp>

#include <boost/core/ignore_unused.hpp>

namespace node
{
	//
	//	wired m-bus configuration
	//
	cfg_mbus::cfg_mbus(cache& c)
		: cache_(c)
	{}

	bool cfg_mbus::is_enabled() const
	{
		return cache_.get_cfg(build_cfg_key({ "rs485", "enabled" }), false);
	}

	std::chrono::seconds cfg_mbus::get_readout_interval() const
	{
		return cache_.get_cfg(build_cfg_key({ sml::OBIS_CLASS_MBUS, sml::OBIS_CLASS_MBUS_RO_INTERVAL }), std::chrono::seconds(3600));
	}

	std::chrono::seconds cfg_mbus::get_search_interval() const
	{
		return cache_.get_cfg(build_cfg_key({ sml::OBIS_CLASS_MBUS, sml::OBIS_CLASS_MBUS_SEARCH_INTERVAL }), std::chrono::seconds(0));
	}

	bool cfg_mbus::get_search_device() const
	{
		return cache_.get_cfg(build_cfg_key({ sml::OBIS_CLASS_MBUS, sml::OBIS_CLASS_MBUS_SEARCH_DEVICE }), true);
	}

	bool cfg_mbus::is_auto_activation() const
	{
		return cache_.get_cfg(build_cfg_key({ sml::OBIS_CLASS_MBUS, sml::OBIS_CLASS_MBUS_AUTO_ACTICATE }), false);
	}

	bool cfg_mbus::generate_profile() const
	{
		return cache_.get_cfg(build_cfg_key({ sml::OBIS_CLASS_MBUS }, "generate-profile"), true);
	}

}
