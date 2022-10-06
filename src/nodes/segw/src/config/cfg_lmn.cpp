/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <config/cfg_broker.h>
#include <config/cfg_lmn.h>

namespace smf {

    std::string cfg_lmn::port_path(std::uint8_t idx) { return cyng::to_path(cfg::sep, cfg_lmn::root, std::to_string(idx), "port"); }
    std::string cfg_lmn::enabled_path(std::uint8_t idx) {
        return cyng::to_path(cfg::sep, cfg_lmn::root, std::to_string(idx), "enabled");
    }
    std::string cfg_lmn::speed_path(std::uint8_t idx) {
        return cyng::to_path(cfg::sep, cfg_lmn::root, std::to_string(idx), "speed");
    }
    std::string cfg_lmn::parity_path(std::uint8_t idx) {
        return cyng::to_path(cfg::sep, cfg_lmn::root, std::to_string(idx), "parity");
    }
    std::string cfg_lmn::flow_control_path(std::uint8_t idx) {
        return cyng::to_path(cfg::sep, cfg_lmn::root, std::to_string(idx), "flow.control");
    }
    std::string cfg_lmn::stopbits_path(std::uint8_t idx) {
        return cyng::to_path(cfg::sep, cfg_lmn::root, std::to_string(idx), "stopbits");
    }
    std::string cfg_lmn::databits_path(std::uint8_t idx) {
        return cyng::to_path(cfg::sep, cfg_lmn::root, std::to_string(idx), "databits");
    }
    std::string cfg_lmn::broker_enabled_path(std::uint8_t idx) {
        return cyng::to_path(cfg::sep, cfg_lmn::root, std::to_string(idx), "broker.enabled");
    }
    std::string cfg_lmn::broker_login_path(std::uint8_t idx) {
        return cyng::to_path(cfg::sep, cfg_lmn::root, std::to_string(idx), "broker.login");
    }
    // std::string cfg_lmn::broker_timeout_path(std::uint8_t idx) {
    //	return cyng::to_path(cfg::sep, cfg_lmn::root, std::to_string(idx), "broker-timeout");
    //}

    std::string cfg_lmn::type_path(std::uint8_t idx) { return cyng::to_path(cfg::sep, cfg_lmn::root, std::to_string(idx), "type"); }
    std::string cfg_lmn::protocol_path(std::uint8_t idx) {
        return cyng::to_path(cfg::sep, cfg_lmn::root, std::to_string(idx), "protocol");
    }
    std::string cfg_lmn::HCI_path(std::uint8_t idx) { return cyng::to_path(cfg::sep, cfg_lmn::root, std::to_string(idx), "HCI"); }
    std::string cfg_lmn::hex_dump_path(std::uint8_t idx) {
        return cyng::to_path(cfg::sep, cfg_lmn::root, std::to_string(idx), "hex.dump");
    }

    lmn_type lookup_by_name(cfg &c, std::string const &name) {

        for (std::uint8_t idx = 0; idx < 4; idx++) {
            auto const port = c.get_value(cfg_lmn::port_path(idx), "");
            if (boost::algorithm::equals(port, name))
                return static_cast<lmn_type>(idx);
        }
        return lmn_type::OTHER;
    }

    cfg_lmn::cfg_lmn(cfg &c, lmn_type type)
        : cfg_(c)
        , type_(type) {}

    cfg_lmn::cfg_lmn(cfg &c, std::string name)
        : cfg_(c)
        , type_(lookup_by_name(c, name)) {}

    std::string cfg_lmn::get_task_name() const { return cfg_broker(cfg_, type_).get_task_name(); }

    std::string cfg_lmn::get_path_id() const { return std::to_string(get_index()); }

    std::string cfg_lmn::get_port() const { return cfg_.get_value(port_path(get_index()), ""); }

    bool cfg_lmn::is_enabled() const { return cfg_.get_value(enabled_path(get_index()), true); }

    bool cfg_lmn::is_hex_dump() const { return cfg_.get_value(hex_dump_path(get_index()), false); }

    bool cfg_lmn::set_enabled(bool b) const { return cfg_.set_value(enabled_path(get_index()), b); }

    bool cfg_lmn::set_baud_rate(std::uint32_t val) const { return cfg_.set_value(speed_path(get_index()), val); }

    bool cfg_lmn::set_parity(std::string val) const { return cfg_.set_value(parity_path(get_index()), val); }

    bool cfg_lmn::set_flow_control(std::string val) const { return cfg_.set_value(flow_control_path(get_index()), val); }

    bool cfg_lmn::set_stopbits(std::string val) const { return cfg_.set_value(stopbits_path(get_index()), val); }

    bool cfg_lmn::set_databits(std::uint8_t val) const { return cfg_.set_value(databits_path(get_index()), val); }

    bool cfg_lmn::set_protocol(std::string val) const { return cfg_.set_value(protocol_path(get_index()), val); }

    boost::asio::serial_port_base::baud_rate cfg_lmn::get_baud_rate() const {
        return boost::asio::serial_port_base::baud_rate(cfg_.get_value(speed_path(get_index()), static_cast<std::uint32_t>(2400)));
    }

    boost::asio::serial_port_base::parity cfg_lmn::get_parity() const {
        return serial::to_parity(cfg_.get_value(parity_path(get_index()), "none"));
    }

    boost::asio::serial_port_base::flow_control cfg_lmn::get_flow_control() const {
        return serial::to_flow_control(cfg_.get_value(flow_control_path(get_index()), "none"));
    }

    boost::asio::serial_port_base::stop_bits cfg_lmn::get_stopbits() const {
        return serial::to_stopbits(cfg_.get_value(stopbits_path(get_index()), "one"));
    }

    boost::asio::serial_port_base::character_size cfg_lmn::get_databits() const {
        return boost::asio::serial_port_base::character_size(
            cfg_.get_value(databits_path(get_index()), static_cast<std::uint8_t>(8)));
    }

    bool cfg_lmn::is_broker_enabled() const { return cfg_.get_value(broker_enabled_path(get_index()), false); }

    bool cfg_lmn::has_broker_login() const { return cfg_.get_value(broker_login_path(get_index()), false); }

    // std::chrono::seconds cfg_lmn::get_broker_timeout() const {
    //	return cfg_.get_value(broker_timeout_path(get_index()), std::chrono::seconds(12));
    //}

    std::string cfg_lmn::get_type() const { return cfg_.get_value(type_path(get_index()), ""); }

    std::string cfg_lmn::get_protocol() const { return cfg_.get_value(protocol_path(get_index()), "auto"); }

    std::string cfg_lmn::get_hci() const { return cfg_.get_value(HCI_path(get_index()), "none"); }

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
        //	Fletcher Checksum
        //(https://www.silabs.com/documents/public/application-notes/AN978-cp210x-usb-to-uart-api-specification.pdf)
        //

        return cyng::make_buffer({0xA5, 0x81, 0x03, 0x17, 0x00, 0xFF, 0x00, 0x03, 0x00, 0xB3, 0x25, 0x51, 0x18, 0x10, 0x00,
                                  0x01, 0x00, 0x01, 0xFD, 0x07, 0x32, 0x00, 0x01, 0x01, 0x02, 0x01, 0x00, 0x83, 0xC9});
    }
} // namespace smf
