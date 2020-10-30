/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include <cfg_rs485.h>
#include <segw.h>
#include <cache.h>
#include <smf/serial/parity.h>
#include <smf/serial/stopbits.h>
#include <smf/serial/flow_control.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/intrinsics/obis_factory.hpp>

#include <cyng/value_cast.hpp>
#include <cyng/numeric_cast.hpp>

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
		return cache_.get_cfg(build_cfg_key({ sml::OBIS_ROOT_SERIAL, sml::make_obis(sml::OBIS_ROOT_SERIAL, port_idx), sml::OBIS_SERIAL_NAME }),
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

	std::chrono::seconds cfg_rs485::get_delay(std::chrono::seconds cap) const
	{
		auto const monitor = get_monitor();
		return (monitor > cap)
			? cap
			: monitor
			;
	}

	boost::asio::serial_port_base::baud_rate cfg_rs485::get_baud_rate() const
	{
		return boost::asio::serial_port_base::baud_rate(cache_.get_cfg<std::uint32_t>(build_cfg_key({ sml::OBIS_ROOT_SERIAL, sml::make_obis(sml::OBIS_ROOT_SERIAL, port_idx), sml::make_obis(sml::OBIS_SERIAL_SPEED, port_idx) }), 2400u));
	}

	bool cfg_rs485::set_baud_rate(cyng::object obj)
	{
		auto const val = cyng::numeric_cast<std::uint32_t>(obj, 8u);
		return cache_.set_cfg<std::uint32_t>(build_cfg_key({ sml::OBIS_ROOT_SERIAL, sml::make_obis(sml::OBIS_ROOT_SERIAL, port_idx), sml::make_obis(sml::OBIS_SERIAL_SPEED, port_idx) }), val);
	}

	boost::asio::serial_port_base::parity cfg_rs485::get_parity() const
	{
		return serial::to_parity(cache_.get_cfg(build_cfg_key({ sml::OBIS_ROOT_SERIAL, sml::make_obis(sml::OBIS_ROOT_SERIAL, port_idx), sml::make_obis(sml::OBIS_SERIAL_PARITY, port_idx) }), "none"));
	}

	bool cfg_rs485::set_parity(cyng::object obj) const
	{
		auto const parity = cyng::value_cast<std::string>(obj, "none");
		return cache_.set_cfg(build_cfg_key({ sml::OBIS_ROOT_SERIAL, sml::make_obis(sml::OBIS_ROOT_SERIAL, port_idx), sml::make_obis(sml::OBIS_SERIAL_PARITY, port_idx) }), parity);
	}

	boost::asio::serial_port_base::flow_control cfg_rs485::get_flow_control() const
	{
		return serial::to_flow_control(cache_.get_cfg(build_cfg_key({ sml::OBIS_ROOT_SERIAL, sml::make_obis(sml::OBIS_ROOT_SERIAL, port_idx), sml::make_obis(sml::OBIS_SERIAL_FLOW_CONTROL, port_idx) }), "none"));
	}

	bool cfg_rs485::set_flow_control(cyng::object obj) const
	{
		auto const val = cyng::value_cast<std::string>(obj, "none");
		return cache_.set_cfg(build_cfg_key({ sml::OBIS_ROOT_SERIAL, sml::make_obis(sml::OBIS_ROOT_SERIAL, port_idx), sml::make_obis(sml::OBIS_SERIAL_FLOW_CONTROL, port_idx) }), val);
	}

	boost::asio::serial_port_base::stop_bits cfg_rs485::get_stopbits() const
	{
		return serial::to_stopbits(cache_.get_cfg(build_cfg_key({ sml::OBIS_ROOT_SERIAL, sml::make_obis(sml::OBIS_ROOT_SERIAL, port_idx), sml::make_obis(sml::OBIS_SERIAL_STOPBITS, port_idx) }), "one"));
	}

	bool cfg_rs485::set_stopbits(cyng::object obj) const
	{
		auto const val = cyng::value_cast<std::string>(obj, "one");
		return cache_.set_cfg(build_cfg_key({ sml::OBIS_ROOT_SERIAL, sml::make_obis(sml::OBIS_ROOT_SERIAL, port_idx), sml::make_obis(sml::OBIS_SERIAL_STOPBITS, port_idx) }), val);
	}

	boost::asio::serial_port_base::character_size cfg_rs485::get_databits() const
	{
		return boost::asio::serial_port_base::character_size(cache_.get_cfg(build_cfg_key({ sml::OBIS_ROOT_SERIAL, sml::make_obis(sml::OBIS_ROOT_SERIAL, port_idx), sml::make_obis(sml::OBIS_SERIAL_DATABITS, port_idx) }), 8u));
	}

	bool cfg_rs485::set_databits(cyng::object obj) const
	{
		auto const val = cyng::numeric_cast<std::uint32_t>(obj, 8u);
		return cache_.set_cfg<std::uint32_t>(build_cfg_key({ sml::OBIS_ROOT_SERIAL, sml::make_obis(sml::OBIS_ROOT_SERIAL, port_idx), sml::make_obis(sml::OBIS_SERIAL_DATABITS, port_idx) }), val);
	}

	std::size_t cfg_rs485::get_lmn_task() const
	{
		return cache_.get_cfg<std::uint64_t>(build_cfg_key({ sml::OBIS_ROOT_SERIAL, sml::make_obis(sml::OBIS_ROOT_SERIAL, port_idx), sml::OBIS_SERIAL_TASK }), 0);
	}

	bool cfg_rs485::set_lmn_task(std::size_t tsk) const
	{
		return cache_.set_cfg<std::uint64_t>(build_cfg_key({ sml::OBIS_ROOT_SERIAL, sml::make_obis(sml::OBIS_ROOT_SERIAL, port_idx), sml::OBIS_SERIAL_TASK }), tsk);
	}

	cfg_rs485::protocol cfg_rs485::get_protocol() const
	{
		auto const p = get_protocol_by_name();
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

	std::string cfg_rs485::get_protocol_by_name() const
	{
		return cache_.get_cfg(build_cfg_key({ "rs485", "protocol" }), "raw");
	}

	bool cfg_rs485::set_protocol(cyng::object obj) const
	{
		auto const val = cyng::value_cast<std::string>(obj, "raw");
		return cache_.set_cfg(build_cfg_key({ "rs485", "protocol" }), val);
	}

	bool cfg_rs485::is_enabled() const
	{
		return cache_.get_cfg(build_cfg_key({"rs485", "enabled"}), false);
	}

	bool cfg_rs485::set_enabled(cyng::object obj) const
	{
		auto const val = cyng::value_cast(obj, true);
		return cache_.set_cfg(build_cfg_key({ "rs485", "enabled" }), val);
	}

}
