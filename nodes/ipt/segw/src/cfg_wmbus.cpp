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
#include <cyng/numeric_cast.hpp>

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

	bool cfg_wmbus::set_enabled(cyng::object obj) const
	{
		auto const val = cyng::value_cast(obj, true);
		return cache_.set_cfg(build_cfg_key({ sml::OBIS_IF_wMBUS }, "enabled"), val);
	}

	std::string cfg_wmbus::get_port() const
	{
		return cache_.get_cfg(build_cfg_key({ 
			sml::OBIS_ROOT_SERIAL, 
			sml::make_obis(sml::OBIS_ROOT_SERIAL, port_idx), 
			sml::OBIS_SERIAL_NAME }),
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

	std::chrono::seconds cfg_wmbus::get_delay(std::chrono::seconds cap) const
	{
		auto const monitor = get_monitor();
		return (monitor > cap)
			? cap
			: monitor
			;
	}

	boost::asio::serial_port_base::baud_rate cfg_wmbus::get_baud_rate() const
	{
		return boost::asio::serial_port_base::baud_rate(cache_.get_cfg<std::uint32_t>(build_cfg_key({ 
			sml::OBIS_ROOT_SERIAL, 
			sml::make_obis(sml::OBIS_ROOT_SERIAL, port_idx),
			sml::make_obis(sml::OBIS_SERIAL_SPEED, port_idx) }),
#if BOOST_OS_WINDOWS
			57600u
#else
			115200u
#endif
		));
	}

	bool cfg_wmbus::set_baud_rate(cyng::object obj)
	{
		auto const val = cyng::numeric_cast<std::uint32_t>(obj, 8u);
		return cache_.set_cfg<std::uint32_t>(build_cfg_key({ 
			sml::OBIS_ROOT_SERIAL, 
			sml::make_obis(sml::OBIS_ROOT_SERIAL, port_idx),
			sml::make_obis(sml::OBIS_SERIAL_SPEED, port_idx) }), val);
	}

	boost::asio::serial_port_base::parity cfg_wmbus::get_parity() const
	{
		return serial::to_parity(cache_.get_cfg(build_cfg_key({ 
			sml::OBIS_ROOT_SERIAL, 
			sml::make_obis(sml::OBIS_ROOT_SERIAL, port_idx),
			sml::make_obis(sml::OBIS_SERIAL_PARITY, port_idx) }), "none"));
	}

	bool cfg_wmbus::set_parity(cyng::object obj) const
	{
		auto const parity = cyng::value_cast<std::string>(obj, "none");
		return cache_.set_cfg(build_cfg_key({ 
			sml::OBIS_ROOT_SERIAL, 
			sml::make_obis(sml::OBIS_ROOT_SERIAL, port_idx),
			sml::make_obis(sml::OBIS_SERIAL_PARITY, port_idx) }), parity);
	}

	boost::asio::serial_port_base::flow_control cfg_wmbus::get_flow_control() const
	{
		return serial::to_flow_control(cache_.get_cfg(build_cfg_key({ 
			sml::OBIS_ROOT_SERIAL, 
			sml::make_obis(sml::OBIS_ROOT_SERIAL, port_idx),
			sml::make_obis(sml::OBIS_SERIAL_FLOW_CONTROL, port_idx) }), "none"));
	}

	bool cfg_wmbus::set_flow_control(cyng::object obj) const
	{
		auto const val = cyng::value_cast<std::string>(obj, "none");
		return cache_.set_cfg(build_cfg_key({ 
			sml::OBIS_ROOT_SERIAL, 
			sml::make_obis(sml::OBIS_ROOT_SERIAL, port_idx),
			sml::make_obis(sml::OBIS_SERIAL_FLOW_CONTROL, port_idx) }), val);
	}

	boost::asio::serial_port_base::stop_bits cfg_wmbus::get_stopbits() const
	{
		return serial::to_stopbits(cache_.get_cfg(build_cfg_key({ 
			sml::OBIS_ROOT_SERIAL, 
			sml::make_obis(sml::OBIS_ROOT_SERIAL, port_idx),
			sml::make_obis(sml::OBIS_SERIAL_STOPBITS, port_idx) }), "one"));
	}

	bool cfg_wmbus::set_stopbits(cyng::object obj) const
	{
		auto const val = cyng::value_cast<std::string>(obj, "one");
		return cache_.set_cfg(build_cfg_key({ 
			sml::OBIS_ROOT_SERIAL, 
			sml::make_obis(sml::OBIS_ROOT_SERIAL, port_idx),
			sml::make_obis(sml::OBIS_SERIAL_STOPBITS, port_idx) }), val);
	}

	boost::asio::serial_port_base::character_size cfg_wmbus::get_databits() const
	{
		return boost::asio::serial_port_base::character_size(cache_.get_cfg(build_cfg_key({ 
			sml::OBIS_ROOT_SERIAL, 
			sml::make_obis(sml::OBIS_ROOT_SERIAL, port_idx),
			sml::make_obis(sml::OBIS_SERIAL_DATABITS, port_idx) }), 8u));
	}
	bool cfg_wmbus::set_databits(cyng::object obj) const
	{
		auto const val = cyng::numeric_cast<std::uint32_t>(obj, 8u);
		return cache_.set_cfg<std::uint32_t>(build_cfg_key({ 
			sml::OBIS_ROOT_SERIAL, 
			sml::make_obis(sml::OBIS_ROOT_SERIAL, port_idx),
			sml::make_obis(sml::OBIS_SERIAL_DATABITS, port_idx) }), val);
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
