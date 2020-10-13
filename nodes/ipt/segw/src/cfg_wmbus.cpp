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
#include <smf/sml/intrinsics/obis_factory.hpp>

#include <cyng/value_cast.hpp>

#include <boost/core/ignore_unused.hpp>
#include <boost/predef.h>

namespace node
{
	//
	//	wireless mbus configuration
	//
	cfg_wmbus::cfg_wmbus(cache& c)
		: cache_(c)
	{}

	bool cfg_wmbus::is_enabled() const
	{
		return cache_.get_cfg(build_cfg_key({ sml::OBIS_IF_wMBUS }, "enabled"), false);
	}

	std::string cfg_wmbus::get_port() const
	{
		return cache_.get_cfg(build_cfg_key({ sml::OBIS_ROOT_HARDWARE_PORT, sml::make_obis(0x91, 0x00, 0x00, 0x00, 0x01, port_idx) }),
#if BOOST_OS_WINDOWS
			std::string("COM8")
#else
			std::string("/dev/ttyAPP1")
#endif
		);
	}

	std::chrono::seconds cfg_wmbus::get_monitor() const
	{
		return cache_.get_cfg(build_cfg_key({ sml::OBIS_IF_wMBUS }, "monitor"), std::chrono::seconds(30));
	}

	boost::asio::serial_port_base::baud_rate cfg_wmbus::get_baud_rate() const
	{
		return boost::asio::serial_port_base::baud_rate(cache_.get_cfg<std::uint32_t>(build_cfg_key({ sml::OBIS_ROOT_HARDWARE_PORT, sml::make_obis(0x91, 0x00, 0x00, 0x00, 0x06, port_idx) }),
#if BOOST_OS_WINDOWS
			57600u
#else
			115200u
#endif
		));
	}

	boost::asio::serial_port_base::parity cfg_wmbus::get_parity() const
	{
		return serial::to_parity(cache_.get_cfg(build_cfg_key({ sml::OBIS_ROOT_HARDWARE_PORT, sml::make_obis(0x91, 0x00, 0x00, 0x00, 0x03, port_idx) }), "none"));
	}

	boost::asio::serial_port_base::flow_control cfg_wmbus::get_flow_control() const
	{
		return serial::to_flow_control(cache_.get_cfg(build_cfg_key({ sml::OBIS_ROOT_HARDWARE_PORT, sml::make_obis(0x91, 0x00, 0x00, 0x00, 0x04, port_idx) }), "none"));
	}

	boost::asio::serial_port_base::stop_bits cfg_wmbus::get_stopbits() const
	{
		return serial::to_stopbits(cache_.get_cfg(build_cfg_key({ sml::OBIS_ROOT_HARDWARE_PORT, sml::make_obis(0x91, 0x00, 0x00, 0x00, 0x05, port_idx) }), "one"));
	}

	boost::asio::serial_port_base::character_size cfg_wmbus::get_databits() const
	{
		return boost::asio::serial_port_base::character_size(cache_.get_cfg(build_cfg_key({ sml::OBIS_ROOT_HARDWARE_PORT, sml::make_obis(0x91, 0x00, 0x00, 0x00, 0x02, port_idx) }), 8u));
	}

	std::string cfg_wmbus::get_hci() const
	{
		return cache_.get_cfg(build_cfg_key({ sml::OBIS_IF_wMBUS }, "HCI"), std::string("none"));
	}

	bool cfg_wmbus::is_CP210x() const
	{
		return boost::algorithm::equals(get_hci(), "CP210x");
	}

	bool cfg_wmbus::generate_profile() const
	{
		return cache_.get_cfg(build_cfg_key({ sml::OBIS_IF_wMBUS }, "generate-profile"), true);
	}

}
