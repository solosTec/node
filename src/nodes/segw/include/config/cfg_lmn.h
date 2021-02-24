/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_CONFIG_LMN_H
#define SMF_SEGW_CONFIG_LMN_H

#include <cfg.h>

#include <smf/serial/parity.h>
#include <smf/serial/stopbits.h>
#include <smf/serial/flow_control.h>

 namespace smf {
	
	 enum class lmn_type : std::uint8_t {
		 WIRELESS = 0,	//	IF_wMBUS
		 WIRED = 1,		//	rs485
		 ETHERNET,
		 OTHER = 0,
	 };

	 class cfg_lmn
	 {
	 public:
		 cfg_lmn(cfg&, lmn_type);
		 cfg_lmn(cfg_lmn&) = default;

		 /**
		  * numerical value of the specified LMN enum type
		  */
		 std::uint8_t get_index() const;
		 std::string get_path_id() const;

		 /**
		  * examples are "/dev/ttyAPP0" on linux systems
		  * of "COM3" on windows.
		  */
		 std::string get_port() const;

		 /**
		  * Control to start the LMN port task or not.
		  *
		  * @return "rs485:enabled"
		  */
		 bool is_enabled() const;
		 //bool set_enabled(cyng::object obj) const;

		 boost::asio::serial_port_base::baud_rate get_baud_rate() const;
		 boost::asio::serial_port_base::parity get_parity() const;
		 boost::asio::serial_port_base::flow_control get_flow_control() const;
		 boost::asio::serial_port_base::stop_bits get_stopbits() const;
		 boost::asio::serial_port_base::character_size get_databits() const;

		 bool is_broker_enabled() const;
		 bool has_broker_login() const;

		 /**
		  * configured type name
		  */
		 std::string get_type() const;

	 private:
		 cfg& cfg_;
		 lmn_type const type_;
	 };
}

#endif