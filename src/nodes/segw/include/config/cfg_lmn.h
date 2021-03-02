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

	 /**
	  * Takes the name of a serial port and lookup for the LMN type
	  */
	 lmn_type lookup_by_name(cfg& c, std::string const& name);

	 class cfg_lmn
	 {
	 public:
		 cfg_lmn(cfg&, lmn_type);
		 cfg_lmn(cfg&, std::string);
		 cfg_lmn(cfg_lmn&) = default;

		 /**
		  * numerical value of the specified LMN enum type
		  */
		 constexpr std::uint8_t get_index() const {
			 return static_cast<std::uint8_t>(type_);
		 }

		 /**
		  * LMN type as numerical string
		  */
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

		 boost::asio::serial_port_base::baud_rate get_baud_rate() const;
		 boost::asio::serial_port_base::parity get_parity() const;
		 boost::asio::serial_port_base::flow_control get_flow_control() const;
		 boost::asio::serial_port_base::stop_bits get_stopbits() const;
		 boost::asio::serial_port_base::character_size get_databits() const;

		 //
		 //	broker
		 //
		 bool is_broker_enabled() const;
		 bool has_broker_login() const;
		 std::chrono::seconds get_broker_timeout() const;

		 /**
		  * configured type name
		  * @return a textual description of this port type
		  */
		 std::string get_type() const;
		 constexpr lmn_type get_lmn_type() const {
			 return type_;
		 }

		 /**
		  * expected protocol 
		  */
		 std::string get_protocol() const;

		 /**
		  * @see cfg_broker.cpp
		  */
		 std::string get_task_name() const;


		 /**
		  * Currently only CP210x supported.
		  * 
		  * @return name of an optional HCI adapter
		  */
		 std::string get_hci() const;
		 static cyng::buffer_t get_hci_init_seq();


		 bool set_enabled(bool) const;
		 bool set_baud_rate(std::uint32_t) const;
		 bool set_parity(std::string) const;
		 bool set_flow_control(std::string) const;
		 bool set_stopbits(std::string) const;
		 bool set_databits(std::uint8_t) const;

		 bool set_protocol(std::string) const;

		 constexpr static char root[] = "lmn";

	 private:
		 cfg& cfg_;
		 lmn_type const type_;
	 };
}

#endif