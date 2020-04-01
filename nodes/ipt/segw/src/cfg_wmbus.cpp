/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "cfg_wmbus.h"
#include "segw.h"
#include "cache.h"
#include <smf/serial/parity.h>
#include <smf/serial/stopbits.h>
#include <smf/serial/flow_control.h>
#include <smf/sml/obis_db.h>

#include <cyng/value_cast.hpp>
#include <boost/core/ignore_unused.hpp>

namespace node
{
	//
	//	wireless mbus configuration
	//
	cfg_wmbus::cfg_wmbus(cache& c)
		: cache_(c)
	{}

	std::string cfg_wmbus::get_port() const
	{

#if BOOST_OS_WINDOWS
		return cache_.get_cfg(build_cfg_key({ sml::OBIS_IF_wMBUS }, "port"), std::string("COM8"));
#else
		return cache_.get_cfg(build_cfg_key({ sml::OBIS_IF_wMBUS }, "port"), std::string("/dev/ttyAPP1"));
#endif
	}

	std::chrono::seconds cfg_wmbus::get_monitor() const
	{
		return cache_.get_cfg(build_cfg_key({ sml::OBIS_IF_wMBUS }, "monitor"), std::chrono::seconds(30));
	}

	boost::asio::serial_port_base::baud_rate cfg_wmbus::get_baud_rate() const
	{
		return boost::asio::serial_port_base::baud_rate(cache_.get_cfg(build_cfg_key({ sml::OBIS_W_MBUS_PROTOCOL }, "speed"), 57600u));
	}

	boost::asio::serial_port_base::parity cfg_wmbus::get_parity() const
	{
		return serial::to_parity(cache_.get_cfg(build_cfg_key({ sml::OBIS_IF_wMBUS }, "parity"), "none"));
	}

	boost::asio::serial_port_base::flow_control cfg_wmbus::get_flow_control() const
	{
		return serial::to_flow_control(cache_.get_cfg(build_cfg_key({ sml::OBIS_IF_wMBUS }, "flow_control"), "none"));
	}

	boost::asio::serial_port_base::stop_bits cfg_wmbus::get_stopbits() const
	{
		return serial::to_stopbits(cache_.get_cfg(build_cfg_key({ sml::OBIS_IF_wMBUS }, "stopbits"), "one"));
	}

	boost::asio::serial_port_base::character_size cfg_wmbus::get_databits() const
	{
		return boost::asio::serial_port_base::character_size(cache_.get_cfg(build_cfg_key({ sml::OBIS_IF_wMBUS }, "databits"), 8u));
	}
}
