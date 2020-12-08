/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include <cfg_redirector.h>
#include <segw.h>
#include <cache.h>
#include <cfg_rs485.h>
#include <cfg_wmbus.h>

#include <smf/sml/obis_db.h>
#include <smf/sml/intrinsics/obis_factory.hpp>

#include <cyng/value_cast.hpp>
#include <cyng/util/split.h>

#include <boost/core/ignore_unused.hpp>
#include <boost/predef.h>

namespace node
{
	//
	//	broker configuration
	//
	cfg_redirector::cfg_redirector(cache& c)
		: cache_(c)
	{}

	segw::server cfg_redirector::get_server(source s) const
	{
		switch (s) {
		case source::WIRELESS_LMN:
		case source::WIRED_LMN:
			return get_server(static_cast<std::uint8_t>(s));
		default:
			break;
		}
		return segw::server("", "", "", 0);
	}

	cyng::object cfg_redirector::get_server_obj(source s) const
	{
		auto const srv = get_server(s);
		return (srv.empty())
			? cyng::make_object()
			: cyng::make_object(to_param_map(get_server(s)))
			;
	}


	void cfg_redirector::set_server(source s, segw::server const& data)
	{
		set_address(s, data.get_address());
		set_port(s, data.get_port());
		set_account(s, data.get_account());
		set_pwd(s, data.get_pwd());
	}

	void cfg_redirector::set_server(std::string const& port_name, segw::server const& data)
	{
		set_server(get_port_source(port_name), data);
	}

	bool cfg_redirector::set_address(source s, std::string const& address)
	{
		auto const port_idx = static_cast<std::uint8_t>(s);
		return cache_.set_cfg(build_cfg_key({
				sml::OBIS_ROOT_REDIRECTOR,
				sml::make_obis(sml::OBIS_ROOT_REDIRECTOR, port_idx),
				sml::make_obis(sml::OBIS_REDIRECTOR_ADDRESS, port_idx) }), address);
	}

	bool cfg_redirector::set_address(std::string const& port_name, std::string const& address)
	{
		return set_address(get_port_source(port_name), address);
	}

	bool cfg_redirector::set_port(source s, std::uint16_t port)
	{
		auto const port_idx = static_cast<std::uint8_t>(s);
		return cache_.set_cfg(build_cfg_key({
				sml::OBIS_ROOT_REDIRECTOR,
				sml::make_obis(sml::OBIS_ROOT_REDIRECTOR, port_idx),
				sml::make_obis(sml::OBIS_REDIRECTOR_SERVICE, port_idx) }), port);
	}

	bool cfg_redirector::set_account(source s, std::string const& user)
	{
		auto const port_idx = static_cast<std::uint8_t>(s);
		return cache_.set_cfg(build_cfg_key({
				sml::OBIS_ROOT_REDIRECTOR,
				sml::make_obis(sml::OBIS_ROOT_REDIRECTOR, port_idx),
				sml::make_obis(sml::OBIS_REDIRECTOR_USER, port_idx) }), user);
	}

	bool cfg_redirector::set_account(std::string const& port_name, std::string const& user)
	{
		return set_account(get_port_source(port_name), user);
	}

	bool cfg_redirector::set_pwd(source s, std::string const& pwd)
	{
		auto const port_idx = static_cast<std::uint8_t>(s);
		return cache_.set_cfg(build_cfg_key({
				sml::OBIS_ROOT_REDIRECTOR,
				sml::make_obis(sml::OBIS_ROOT_REDIRECTOR, port_idx),
				sml::make_obis(sml::OBIS_REDIRECTOR_PWD, port_idx) }), pwd);
	}

	bool cfg_redirector::set_pwd(std::string const& port_name, std::string const& pwd)
	{
		return set_pwd(get_port_source(port_name), pwd);
	}

	bool cfg_redirector::set_port(std::string const& port_name, std::uint16_t port)
	{
		return set_port(get_port_source(port_name), port);
	}


	segw::server cfg_redirector::get_server(std::uint8_t port_idx) const
	{
		auto const address = cache_.get_cfg(build_cfg_key({
			sml::OBIS_ROOT_REDIRECTOR,
			sml::make_obis(sml::OBIS_ROOT_REDIRECTOR, port_idx),
			sml::make_obis(sml::OBIS_REDIRECTOR_ADDRESS, port_idx) }), "");

		auto const port = cache_.get_cfg(build_cfg_key({
			sml::OBIS_ROOT_REDIRECTOR,
			sml::make_obis(sml::OBIS_ROOT_REDIRECTOR, port_idx),
			sml::make_obis(sml::OBIS_REDIRECTOR_SERVICE, port_idx) }), static_cast<std::uint16_t>(6001u));

		auto const account = cache_.get_cfg(build_cfg_key({
			sml::OBIS_ROOT_REDIRECTOR,
			sml::make_obis(sml::OBIS_ROOT_REDIRECTOR, port_idx),
			sml::make_obis(sml::OBIS_REDIRECTOR_USER, port_idx) }), "");

		auto const pwd = cache_.get_cfg(build_cfg_key({
			sml::OBIS_ROOT_REDIRECTOR,
			sml::make_obis(sml::OBIS_ROOT_REDIRECTOR, port_idx),
			sml::make_obis(sml::OBIS_REDIRECTOR_PWD, port_idx) }), "");

		return segw::server(account, pwd, address, port);
	}

	bool cfg_redirector::is_login_required(source s) const
	{
		switch (s) {
		case source::WIRELESS_LMN:
		case source::WIRED_LMN:
			return is_login_required(build_cfg_key({ 
				sml::OBIS_ROOT_REDIRECTOR, 
				sml::make_obis(sml::OBIS_ROOT_REDIRECTOR, static_cast<std::uint8_t>(s)),
				sml::OBIS_REDIRECTOR_LOGIN }));
		default:
			break;
		}
		return false;
	}

	bool cfg_redirector::is_login_required(std::string path) const
	{
		return cache_.get_cfg(path, true);
	}

	bool cfg_redirector::set_login_required(source s, bool b) const
	{
		auto const idx = static_cast<std::uint8_t>(s);
		return cache_.set_cfg(build_cfg_key({
				sml::OBIS_ROOT_REDIRECTOR,
				sml::make_obis(sml::OBIS_ROOT_REDIRECTOR, static_cast<std::uint8_t>(idx)),
				sml::OBIS_REDIRECTOR_LOGIN }), b);
	}

	bool cfg_redirector::set_login_required(std::string const& port_name, bool b) const
	{
		return set_login_required(get_port_source(port_name), b);
	}

	std::string cfg_redirector::get_port_name(source s) const
	{
		switch (s) {
		case source::WIRELESS_LMN:
		case source::WIRED_LMN:
			return get_port_name(build_cfg_key({ sml::OBIS_ROOT_SERIAL, sml::make_obis(sml::OBIS_ROOT_SERIAL, static_cast<std::uint8_t>(s)), sml::OBIS_SERIAL_NAME }));
		default:
			break;
		}
		return "";
	}

	std::uint8_t cfg_redirector::get_port_id(std::string const& port_name) const
	{
		return static_cast<std::uint8_t>(get_port_source(port_name));
	}

	source cfg_redirector::get_port_source(std::string const& port_name) const
	{
		cfg_rs485 const rs485(cache_);
		cfg_wmbus const wmbus(cache_);

		if (boost::algorithm::equals(port_name, rs485.get_port())) {
			return source::WIRED_LMN;
		}
		else if (boost::algorithm::equals(port_name, wmbus.get_port())) {
			return source::WIRELESS_LMN;
		}
		return source::OTHER;
	}

	std::string cfg_redirector::get_port_name(std::string path) const
	{
		return cache_.get_cfg(path, "");
	}

	bool cfg_redirector::is_enabled(source s) const
	{
		auto const idx = static_cast<std::uint8_t>(s);
		return cache_.get_cfg(build_cfg_key({
				sml::OBIS_ROOT_REDIRECTOR,
				sml::make_obis(sml::OBIS_ROOT_REDIRECTOR, static_cast<std::uint8_t>(idx)) }, "enabled"), true);
	}

	bool cfg_redirector::is_enabled(std::string const& port_name) const
	{
		return is_enabled(get_port_source(port_name));
	}

	bool cfg_redirector::set_enabled(source s, cyng::object obj) const
	{
		//
		//	
		//
		auto const idx = static_cast<std::uint8_t>(s);
		auto const val = cyng::value_cast(obj, true);	//	guarantee a boolean value
		return cache_.set_cfg(build_cfg_key({
				sml::OBIS_ROOT_REDIRECTOR,
				sml::make_obis(sml::OBIS_ROOT_REDIRECTOR, static_cast<std::uint8_t>(idx)) }, "enabled"), val);
	}

	bool cfg_redirector::set_enabled(std::string const& port_name, cyng::object obj) const
	{
		return set_enabled(get_port_source(port_name), obj);
	}

}
