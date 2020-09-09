/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "cfg_ipt.h"
#include "segw.h"
#include "cache.h"
#include <smf/sml/obis_db.h>
#include <smf/sml/intrinsics/obis_factory.hpp>
#include <smf/ipt/scramble_key_format.h>

#include <cyng/value_cast.hpp>
#include <boost/core/ignore_unused.hpp>

namespace node
{
	//
	//	IP-T configuration
	//
	cfg_ipt::cfg_ipt(cache& c)
		: cache_(c)
	{}

	ipt::scramble_key cfg_ipt::get_ipt_sk()
	{
		return get_ipt_sk(get_ipt_master_index() + 1);
	}

	std::chrono::minutes cfg_ipt::get_ipt_tcp_wait_to_reconnect()
	{
		return cache_.get_cfg(build_cfg_key({ sml::OBIS_ROOT_IPT_PARAM
			, sml::OBIS_TCP_WAIT_TO_RECONNECT }), std::chrono::minutes(1u));
	}

	std::uint32_t cfg_ipt::get_ipt_tcp_connect_retries()
	{
		return cache_.get_cfg<std::uint32_t>(build_cfg_key({ sml::OBIS_ROOT_IPT_PARAM
			, sml::OBIS_TCP_CONNECT_RETRIES }), 3u);
	}

	bool cfg_ipt::has_ipt_ssl()
	{
		return cache_.get_cfg(build_cfg_key({ sml::OBIS_ROOT_IPT_PARAM
			, sml::OBIS_HAS_SSL_CONFIG }), false);
	}

	std::uint8_t cfg_ipt::get_ipt_master_index()
	{
		return cache_.get_cfg<std::uint8_t>(build_cfg_key({ sml::OBIS_ROOT_IPT_PARAM }, "master"), 0u);
	}

	ipt::master_record cfg_ipt::switch_ipt_redundancy()
	{
		//
		//	Simulate the logic from the class ipt::redundancy
		//
		auto const idx = (get_ipt_master_index() == 0)
			? 1
			: 0;

		//
		//	write back
		//
		cache_.set_cfg(build_cfg_key({ sml::OBIS_ROOT_IPT_PARAM }, "master"), idx);

		//
		//	return new master config record
		//
		return get_ipt_cfg(idx + 1);
	}

	ipt::master_record cfg_ipt::get_ipt_cfg(std::uint8_t idx)
	{
		//81490D0700FF:81490D070001:814917070001|1|127.0.0.1|127.0.0.1|15
		//81490D0700FF:81490D070001:81491A070001|1|26862|26862|15
		//81490D0700FF:81490D070001:8149633C0101|1|gateway|gateway|15
		//81490D0700FF:81490D070001:8149633C0201|1|gateway|gateway|15
		//81490D0700FF:81490D070001:scrambled|1|true|true|1
		//81490D0700FF:81490D070001:sk|1|0102030405060708090001020304050607080900010203040506070809000001|0102030405060708090001020304050607080900010203040506070809000001|15


		return ipt::master_record(get_ipt_host(idx)
			, std::to_string(get_ipt_port_target(idx))
			, get_ipt_user(idx)
			, get_ipt_pwd(idx)
			, get_ipt_sk(idx)
			, is_ipt_scrambled(idx)
			, get_ipt_tcp_wait_to_reconnect().count());
	}

	ipt::redundancy cfg_ipt::get_ipt_redundancy()
	{
		ipt::master_config_t const vec{ get_ipt_cfg(1) , get_ipt_cfg(2) };
		return ipt::redundancy(vec, get_ipt_master_index());
	}

	ipt::master_record cfg_ipt::get_ipt_master()
	{
		return get_ipt_cfg(get_ipt_master_index() + 1);
	}

	std::string cfg_ipt::get_ipt_host(std::uint8_t idx)
	{
		return cache_.get_cfg<std::string>(build_cfg_key({ sml::OBIS_ROOT_IPT_PARAM
			, sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx)
			, sml::make_obis(0x81, 0x49, 0x17, 0x07, 0x00, idx) }), "");
	}

	std::uint16_t cfg_ipt::get_ipt_port_target(std::uint8_t idx)
	{
		try {
			auto const service = cache_.get_cfg<std::string>(build_cfg_key({ sml::OBIS_ROOT_IPT_PARAM
				, sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx)
				, sml::make_obis(0x81, 0x49, 0x1A, 0x07, 0x00, idx) }), "26862");
			return static_cast<std::uint16_t>(std::stoul(service));
		}
		catch (std::exception const&) {
		}
		return 26862u;
	}

	std::uint16_t cfg_ipt::get_ipt_port_source(std::uint8_t idx)
	{
		return cache_.get_cfg<std::uint16_t>(build_cfg_key({ sml::OBIS_ROOT_IPT_PARAM
			, sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx)
			, sml::make_obis(0x81, 0x49, 0x19, 0x07, 0x00, idx) }), 0u);
	}

	std::string cfg_ipt::get_ipt_user(std::uint8_t idx)
	{
		return cache_.get_cfg<std::string>(build_cfg_key({ sml::OBIS_ROOT_IPT_PARAM
			, sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx)
			, sml::make_obis(0x81, 0x49, 0x63, 0x3C, 0x01, idx) }), "");
	}

	std::string cfg_ipt::get_ipt_pwd(std::uint8_t idx)
	{
		return cache_.get_cfg<std::string>(build_cfg_key({ sml::OBIS_ROOT_IPT_PARAM
			, sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx)
			, sml::make_obis(0x81, 0x49, 0x63, 0x3C, 0x02, idx) }), "");
	}

	bool cfg_ipt::is_ipt_scrambled(std::uint8_t idx)
	{
		return cache_.get_cfg(build_cfg_key({ sml::OBIS_ROOT_IPT_PARAM
			, sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx) }
		, "scrambled"), false);
	}

	ipt::scramble_key cfg_ipt::get_ipt_sk(std::uint8_t idx)
	{
		auto const s = cache_.get_cfg<std::string>(build_cfg_key({ sml::OBIS_ROOT_IPT_PARAM
			, sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx) }
		, "sk"), "0102030405060708090001020304050607080900010203040506070809000001");

		return ipt::from_string(s);
	}

}
