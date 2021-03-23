/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_CONFIG_LISTENER_H
#define SMF_SEGW_CONFIG_LISTENER_H

#include <config/cfg_lmn.h>
#include <vector>
#include <ostream>

 namespace smf {
	
	 class cfg_listener
	 {
	 public:
		 cfg_listener(cfg&, lmn_type);
		 cfg_listener(cfg_listener&) = default;

		 /**
		  * numerical value of the specified LMN enum type
		  */
		 constexpr std::uint8_t get_index() const {
			 return static_cast<std::uint8_t>(type_);
		 }

		 std::string get_path_id() const;

		 bool is_enabled() const;
		 bool has_login() const;

		 /**
		  * @return name of physical/serial port
		  */
		 std::string get_port_name() const;

		 /**
		  * A broker without an active LMN is virtually useless.
		  * This method helps to detect such a misconfiguration.
		  */
		 bool is_lmn_enabled() const;

		 boost::asio::ip::address get_address() const;
		 std::uint16_t get_port() const;
		 std::chrono::seconds get_timeout() const;

		 boost::asio::ip::tcp::endpoint get_ep() const;

		 bool set_address(std::string) const;
		 bool set_port(std::uint16_t) const;

		 bool set_login(bool) const;
		 bool set_enabled(bool) const;
		 bool set_timeout(std::chrono::seconds) const;

		 constexpr static char root[] = "listener";

	 private:
		 cfg& cfg_;
		 lmn_type const type_;
	 };

	 /**
	  * debug helper
	  */
	 std::ostream& operator<<(std::ostream& os, cfg_listener const&);

}

#endif