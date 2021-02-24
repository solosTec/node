/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <config/cfg_lmn.h>

namespace smf {

	cfg_lmn::cfg_lmn(cfg& c, lmn_type type)
		: cfg_(c)
		, type_(type)
	{}

	std::uint8_t cfg_lmn::get_index() const {
		return static_cast<std::uint8_t>(type_);
	}
	std::string cfg_lmn::get_path_id() const {
		return std::to_string(get_index());
	}

	std::string cfg_lmn::get_port() const {
		return cfg_.get_value(cyng::to_path('/', "lmn", get_path_id(), "port"), "");
	}

	bool cfg_lmn::is_enabled() const {
		return cfg_.get_value(cyng::to_path('/', "lmn", get_path_id(), "enabled"), true);
	}

	boost::asio::serial_port_base::baud_rate cfg_lmn::get_baud_rate() const {
		return boost::asio::serial_port_base::baud_rate(
			cfg_.get_value(cyng::to_path('/', "lmn", get_path_id(), "speed"), static_cast<std::uint32_t>(2400))
		);
	}

	boost::asio::serial_port_base::parity cfg_lmn::get_parity() const {
		return serial::to_parity(cfg_.get_value(cyng::to_path('/', "lmn", get_path_id(), "parity"), "none"));
	}

	boost::asio::serial_port_base::flow_control cfg_lmn::get_flow_control() const {
		return serial::to_flow_control(cfg_.get_value(cyng::to_path('/', "lmn", get_path_id(), "flow-control"), "none"));
	}

	boost::asio::serial_port_base::stop_bits cfg_lmn::get_stopbits() const {
		return serial::to_stopbits(cfg_.get_value(cyng::to_path('/', "lmn", get_path_id(), "stopbits"), "one"));
	}

	boost::asio::serial_port_base::character_size cfg_lmn::get_databits() const {
		return boost::asio::serial_port_base::character_size(
			cfg_.get_value(cyng::to_path('/', "lmn", get_path_id(), "databits"), static_cast<std::uint8_t>(8))
		);
	}

	bool cfg_lmn::is_broker_enabled() const {
		return cfg_.get_value(cyng::to_path('/', "lmn", get_path_id(), "broker-enabled"), false);
	}

	bool cfg_lmn::has_broker_login() const {
		return cfg_.get_value(cyng::to_path('/', "lmn", get_path_id(), "broker-login"), false);
	}

	std::string cfg_lmn::get_type() const {
		return cfg_.get_value(cyng::to_path('/', "lmn", get_path_id(), "type"), "");
	}

	std::string cfg_lmn::get_protocol() const {
		return cfg_.get_value(cyng::to_path('/', "lmn", get_path_id(), "protocol"), "auto");
	}

	std::string cfg_lmn::get_hci() const {
		return cfg_.get_value(cyng::to_path('/', "lmn", get_path_id(), "HCI"), "none");
	}

	cyng::buffer_t cfg_lmn::get_hci_init_seq() {

		//
		//	send initialization string
		//	LMN port send incoming data to HCI (CP210x) parser
		//	which sends data to wmbus parser.
		//	CP210x hst to bi initialized with:
		//	
		//	Device Mode: Other (0)
		//	Link Mode: T1 (3)
		//	Ctrl Field: 0x00
		//	Man ID: 0x25B3
		//	Device ID: 0x00101851
		//	Version: 0x01
		//	Device Type: 0x00
		//	Radio Channel: 1 (1..8)
		//	Radio Power Level: 13db (7) [0=-8dB,1=-5dB,2=-2dB,3=1dB,4=4dB,5=7dB,6=10dB,7=13dB]
		//	Rx Window (ms): 50 (0x32)
		//	Power Saving Mode: 0
		//	RSSI Attachment: 1
		//	Rx Timestamp: 1
		//	LED Control: 2
		//	RTC: 1
		//	Store NVM: 0
		//	
		//	[A5 81 03 17 00 FF] 00 03 00 [B3 25] [51 18 10 00] 01
		//	00 01 FD 07 32 00 01 01 02 01 00 [83 C9]
		//
		//	Fletcher Checksum (https://www.silabs.com/documents/public/application-notes/AN978-cp210x-usb-to-uart-api-specification.pdf)
		//

		return cyng::make_buffer({ 0xA5, 0x81, 0x03, 0x17, 0x00, 0xFF, 0x00, 0x03, 0x00, 0xB3, 0x25, 0x51, 0x18, 0x10, 0x00, 0x01, 0x00, 0x01, 0xFD, 0x07, 0x32, 0x00, 0x01, 0x01, 0x02, 0x01, 0x00, 0x83, 0xC9 });
	}
}