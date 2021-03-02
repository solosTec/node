/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_CONFIG_NMS_H
#define SMF_SEGW_CONFIG_NMS_H

#include <cfg.h>
#include <filesystem>

 namespace smf {

	 class cfg_nms
	 {
	 public:
		 cfg_nms(cfg&);
		 cfg_nms(cfg_nms&) = default;

		 boost::asio::ip::tcp::endpoint get_ep() const;
		 boost::asio::ip::address get_address() const;
		 std::uint16_t get_port() const;
		 std::string get_account() const;
		 std::string get_pwd() const;
		 bool is_enabled() const;
		 bool is_debug() const;

		 std::filesystem::path get_script_path() const;

		 bool set_address(boost::asio::ip::address) const;
		 bool set_port(std::uint16_t port) const;
		 bool set_account(std::string) const;
		 bool set_pwd(std::string) const;
		 bool set_enabled(bool) const;
		 bool set_debug(bool) const;

		 constexpr static char root[] = "nms";
	 private:
		 cfg& cfg_;
	 };

}

#endif