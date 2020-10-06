/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "cfg_rs485.h"
#include "segw.h"
#include "cache.h"
#include <smf/serial/parity.h>
#include <smf/serial/stopbits.h>
#include <smf/serial/flow_control.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/intrinsics/obis_factory.hpp>

#include <cyng/value_cast.hpp>
#include <boost/core/ignore_unused.hpp>

namespace node
{
	//
	//	RS 485 configuration
	//
	cfg_rs485::cfg_rs485(cache& c)
		: cache_(c)
	{}

	std::string cfg_rs485::get_port() const
	{
		return cache_.get_cfg(build_cfg_key({ sml::OBIS_ROOT_HARDWARE_PORT, sml::make_obis(0x91, 0x00, 0x00, 0x00, 0x01, port_idx) }), 
#if BOOST_OS_WINDOWS
			std::string("COM1")
#else
			std::string("/dev/ttyAPP0")
#endif
		);
	}

	std::chrono::seconds cfg_rs485::get_monitor() const
	{
		return cache_.get_cfg(build_cfg_key({ "rs485", "monitor" }), std::chrono::seconds(30));
	}

	boost::asio::serial_port_base::baud_rate cfg_rs485::get_baud_rate() const
	{
		return boost::asio::serial_port_base::baud_rate(cache_.get_cfg(build_cfg_key({ sml::OBIS_ROOT_HARDWARE_PORT, sml::make_obis(0x91, 0x00, 0x00, 0x00, 0x06, port_idx) }), 2400u));
	}

	boost::asio::serial_port_base::parity cfg_rs485::get_parity() const
	{
		return serial::to_parity(cache_.get_cfg(build_cfg_key({ sml::OBIS_ROOT_HARDWARE_PORT, sml::make_obis(0x91, 0x00, 0x00, 0x00, 0x03, port_idx) }), "none"));
	}

	boost::asio::serial_port_base::flow_control cfg_rs485::get_flow_control() const
	{
		return serial::to_flow_control(cache_.get_cfg(build_cfg_key({ sml::OBIS_ROOT_HARDWARE_PORT, sml::make_obis(0x91, 0x00, 0x00, 0x00, 0x04, port_idx) }), "none"));
	}

	boost::asio::serial_port_base::stop_bits cfg_rs485::get_stopbits() const
	{
		return serial::to_stopbits(cache_.get_cfg(build_cfg_key({ sml::OBIS_ROOT_HARDWARE_PORT, sml::make_obis(0x91, 0x00, 0x00, 0x00, 0x05, port_idx) }), "one"));
	}

	boost::asio::serial_port_base::character_size cfg_rs485::get_databits() const
	{
		return boost::asio::serial_port_base::character_size(cache_.get_cfg(build_cfg_key({ sml::OBIS_ROOT_HARDWARE_PORT, sml::make_obis(0x91, 0x00, 0x00, 0x00, 0x02, port_idx) }), 8u));
	}

	cfg_rs485::protocol cfg_rs485::get_protocol() const
	{
		auto const p = cache_.get_cfg(build_cfg_key({ "rs485", "protocol" }), "raw");
		if (boost::algorithm::equals(p, "mbus")) {
			return protocol::MBUS;
		}
		else if (boost::algorithm::equals(p, "iec")) {
			return protocol::IEC;
		}
		else if (boost::algorithm::equals(p, "sml")) {
			return protocol::SML;
		}
		return protocol::RAW;
	}

}
