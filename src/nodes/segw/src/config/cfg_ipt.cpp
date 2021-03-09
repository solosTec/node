/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <config/cfg_ipt.h>

#include <smf/obis/defs.h>
#include <smf/ipt/scramble_key_format.h>

 
#ifdef _DEBUG_SEGW
#include <iostream>
#endif


namespace smf {

	cfg_ipt::cfg_ipt(cfg& c)
		: cfg_(c)
	{}

	namespace {
		std::string enabled_path() {
			return cyng::to_path(cfg::sep, cfg_ipt::root, "enabled");
		}

		std::string host_path(std::uint8_t idx) {
			return cyng::to_path(cfg::sep, OBIS_ROOT_IPT_PARAM,
				cyng::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
				cyng::make_obis(0x81, 0x49, 0x17, 0x07, 0x00, idx));
		}
		std::string target_port_path(std::uint8_t idx) {
			return cyng::to_path(cfg::sep, OBIS_ROOT_IPT_PARAM,
				cyng::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
				cyng::make_obis(0x81, 0x49, 0x1A, 0x07, 0x00, idx));
		}
		std::string source_port_path(std::uint8_t idx) {
			return cyng::to_path(cfg::sep, OBIS_ROOT_IPT_PARAM,
				cyng::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
				cyng::make_obis(0x81, 0x49, 0x19, 0x07, 0x00, idx));
		}
		std::string account_path(std::uint8_t idx) {
			return cyng::to_path(cfg::sep, OBIS_ROOT_IPT_PARAM,
				cyng::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
				cyng::make_obis(0x81, 0x49, 0x63, 0x3C, 0x01, idx));
		}

		std::string pwd_path(std::uint8_t idx) {
			return cyng::to_path(cfg::sep, OBIS_ROOT_IPT_PARAM,
				cyng::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
				cyng::make_obis(0x81, 0x49, 0x63, 0x3C, 0x02, idx));
		}
		std::string sk_path(std::uint8_t idx) {
			return cyng::to_path(cfg::sep, OBIS_ROOT_IPT_PARAM,
				cyng::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
				"sk");
		}
		std::string scrambled_path(std::uint8_t idx) {
			return cyng::to_path(cfg::sep, OBIS_ROOT_IPT_PARAM,
				cyng::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
				cyng::make_obis(0x81, 0x49, 0x63, 0x3C, 0x03, idx));
		}

		std::string reconnect_count_path() {
			return cyng::to_path(cfg::sep, OBIS_ROOT_IPT_PARAM,	OBIS_TCP_CONNECT_RETRIES);
		}

		std::string reconnect_timeout_path() {
			return cyng::to_path(cfg::sep, OBIS_ROOT_IPT_PARAM, OBIS_TCP_WAIT_TO_RECONNECT);
		}
		

	}

	bool cfg_ipt::is_enabled() const {
		return cfg_.get_value(enabled_path(), false);
	}

	std::string cfg_ipt::get_host(std::uint8_t idx) const {
		return cfg_.get_value(host_path(idx), "segw.ch");
	}
	std::string cfg_ipt::get_service(std::uint8_t idx) const {
		return cfg_.get_value(target_port_path(idx), "26862");
	}

	std::string cfg_ipt::get_account(std::uint8_t idx) const {
		return cfg_.get_value(account_path(idx), "no-account");
	}
	std::string cfg_ipt::get_pwd(std::uint8_t idx) const {
		return cfg_.get_value(pwd_path(idx), "no-password");
	}
	ipt::scramble_key cfg_ipt::get_sk(std::uint8_t idx) const {
		return ipt::to_sk(cfg_.get_value(sk_path(idx), "0102030405060708090001020304050607080900010203040506070809000001"));
	}
	bool cfg_ipt::is_scrambled(std::uint8_t idx) const {
		return cfg_.get_value(scrambled_path(idx), true);
	}

	ipt::server cfg_ipt::get_server(std::uint8_t idx) const {

		return ipt::server(
			get_host(idx),
			get_service(idx),
			get_account(idx),
			get_pwd(idx),
			get_sk(idx),
			is_scrambled(idx),
			12);	//	monitor
	}

	ipt::toggle::server_vec_t cfg_ipt::get_toggle() const {
		return { get_server(1), get_server(2) };	//	two configurations available
	}

	std::size_t cfg_ipt::get_reconnect_count() const {
		return cfg_.get_value(reconnect_count_path(), static_cast<std::size_t>(12));
	}

	std::chrono::seconds cfg_ipt::get_reconnect_timeout() const {
		return cfg_.get_value(reconnect_timeout_path(), std::chrono::seconds(10));
	}

	const std::string cfg_ipt::root = cyng::to_str(OBIS_ROOT_IPT_PARAM);

}