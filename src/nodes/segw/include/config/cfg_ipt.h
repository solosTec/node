/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_CONFIG_IPT_H
#define SMF_SEGW_CONFIG_IPT_H

#include <cfg.h>
#include <smf/ipt/scramble_key.h>
#include <smf/ipt/config.h>

 namespace smf {

	 class cfg_ipt
	 {
	 public:
		 cfg_ipt(cfg&);
		 cfg_ipt(cfg_ipt&) = default;

		 bool is_enabled() const;

		 std::string get_host(std::uint8_t idx) const;
		 std::string get_service(std::uint8_t idx) const;
		 std::string get_account(std::uint8_t idx) const;
		 std::string get_pwd(std::uint8_t idx) const;
		 ipt::scramble_key get_sk(std::uint8_t idx) const;
		 bool is_scrambled(std::uint8_t idx) const;

		 /**
		  * get all server data with one query
		  */
		 ipt::server get_server(std::uint8_t idx) const;

		 /**
		  * read config vector
		  */
		 ipt::toggle::server_vec_t get_toggle() const;


		 std::size_t get_reconnect_count() const;
		 std::chrono::seconds get_reconnect_timeout() const;


		 static const std::string root;
	 private:
		 cfg& cfg_;
	 };

}

#endif