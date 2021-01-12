/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include <cfg_nms.h>
#include <cache.h>
#include <smf/sml/obis_db.h>

#include <cyng/compatibility/io_service.h>

#include <boost/core/ignore_unused.hpp>
#include <boost/predef.h>

namespace node
{
	//
	//	NMS configuration
	//
	cfg_nms::cfg_nms(cache& c)
		: cache_(c)
	{}

	boost::asio::ip::tcp::endpoint cfg_nms::get_ep() const
	{
		return { get_address(), get_port() };
	}

	boost::asio::ip::address cfg_nms::get_address() const
	{
		return cache_.get_cfg(build_cfg_key({ sml::OBIS_ROOT_NMS, sml::OBIS_NMS_ADDRESS }), boost::asio::ip::address());
	}
	std::uint16_t cfg_nms::get_port() const
	{
		return cache_.get_cfg(build_cfg_key({ sml::OBIS_ROOT_NMS, sml::OBIS_NMS_PORT }), static_cast<std::uint16_t>(7261));
	}
	std::string cfg_nms::get_user() const
	{
		return cache_.get_cfg(build_cfg_key({ sml::OBIS_ROOT_NMS, sml::OBIS_NMS_USER }), "");
	}
	std::string cfg_nms::get_pwd() const
	{
		return cache_.get_cfg(build_cfg_key({ sml::OBIS_ROOT_NMS, sml::OBIS_NMS_PWD }), "");
	}
	bool cfg_nms::is_enabled() const
	{
		return cache_.get_cfg(build_cfg_key({ sml::OBIS_ROOT_NMS, sml::OBIS_NMS_ENABLED }), false);
	}

	bool cfg_nms::set_address(boost::asio::ip::address address) const
	{
		return cache_.set_cfg(build_cfg_key({
				sml::OBIS_ROOT_NMS, sml::OBIS_NMS_ADDRESS }), address);
	}
	bool cfg_nms::set_port(std::uint16_t port) const
	{
		return cache_.set_cfg(build_cfg_key({
				sml::OBIS_ROOT_NMS, sml::OBIS_NMS_PORT }), port);
	}
	bool cfg_nms::set_user(std::string user) const
	{
		return cache_.set_cfg(build_cfg_key({
				sml::OBIS_ROOT_NMS, sml::OBIS_NMS_USER }), user);
	}
	bool cfg_nms::set_pwd(std::string pwd) const
	{
		return cache_.set_cfg(build_cfg_key({
				sml::OBIS_ROOT_NMS, sml::OBIS_NMS_PWD }), pwd);
	}
	bool cfg_nms::set_enabled(bool enabled) const
	{
		return cache_.set_cfg(build_cfg_key({
				sml::OBIS_ROOT_NMS, sml::OBIS_NMS_ENABLED }), enabled);
	}

}
