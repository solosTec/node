/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <controller.h>

#include <config/cfg_nms.h>
#include <config/generator.h>
#include <smf.h>
#include <smf/obis/defs.h>
// #include <storage_functions.h>
#include <config/transfer.h>
#include <tasks/bridge.h>
#include <tasks/gpio.h>

#include <cyng/io/io_buffer.h>
#include <cyng/log/record.h>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/container_factory.hpp>
#include <cyng/obj/intrinsics/container.h>
#include <cyng/obj/object.h>
#include <cyng/obj/util.hpp>
#include <cyng/sys/clock.h>
#include <cyng/sys/host.h>
#include <cyng/sys/mac.h>
#include <cyng/sys/net.h>
#include <cyng/task/controller.h>

#include <iostream>
#include <locale>

#include <boost/algorithm/string.hpp>
#include <boost/asio/serial_port.hpp>
#include <boost/predef.h>
#include <boost/uuid/nil_generator.hpp>

namespace smf {

    controller::controller(config::startup const &config)
        : controller_base(config) {}

    bool controller::run_options(boost::program_options::variables_map &vars) {

        //
        //
        //
        if (vars["init"].as<bool>()) {
            //	initialize database
            std::cout << "config file [" << config_.json_path_ << "]#" << config_.config_index_ << std::endl;
            init_storage(read_config_section(config_.json_path_, config_.config_index_));
            return true;
        }
        if (!vars["transfer"].defaulted()) {
            auto const transfer = vars["transfer"].as<std::string>();
            BOOST_ASSERT(!transfer.empty());
            if (boost::algorithm::equals(transfer, "in")) {
                std::cout << "read config file [" << config_.json_path_ << "]#" << config_.config_index_ << " into database"
                          << std::endl;
                read_config(read_config_section(config_.json_path_, config_.config_index_));
            } else if (boost::algorithm::equals(transfer, "out")) {
                std::cout << "write config file [" << config_.json_path_ << "]#" << config_.config_index_ << std::endl;
                write_config(read_config_section(config_.json_path_, config_.config_index_), config_.json_path_);
            } else {
                std::cerr << "**error: Valid transfer options are \"in\" or \"out\"." << std::endl;
            }
            return true;
        }
        if (vars["clear"].as<bool>()) {
            //	clear configuration from database
            std::cout << "config file [" << config_.json_path_ << "]#" << config_.config_index_ << std::endl;
            clear_config(read_config_section(config_.json_path_, config_.config_index_));
            return true;
        }
        if (vars["list"].as<bool>()) {
            //	show database configuration
            std::cout << "config file [" << config_.json_path_ << "]#" << config_.config_index_ << std::endl;
            list_config(read_config_section(config_.json_path_, config_.config_index_));
            return true;
        }
        if (vars.count("set-value") != 0) {
            auto vec = vars["set-value"].as<std::vector<std::string>>();
            if (vec.size() == 3) {
                //	set configuration value
                set_config_value(read_config_section(config_.json_path_, config_.config_index_), vec.at(0), vec.at(1), vec.at(2));
            } else {
                std::cerr << "set-value requires 3 parameters: \"path\" \"value\" \"type\"" << std::endl;
            }
            return true;
        }
        if (vars.count("add-value") != 0) {
            auto vec = vars["add-value"].as<std::vector<std::string>>();
            if (vec.size() == 3) {
                //	add configuration value
                add_config_value(read_config_section(config_.json_path_, config_.config_index_), vec.at(0), vec.at(1), vec.at(2));
            } else {
                std::cerr << "add-value requires 3 parameters: \"path\" \"value\" \"type\"" << std::endl;
            }
            return true;
        }
        if (vars.count("del-value") != 0) {
            std::string const key = vars["del-value"].as<std::string>();
            if (!key.empty()) {
                //	remove configuration value
                del_config_value(read_config_section(config_.json_path_, config_.config_index_), key);
                return true;
            }
            return false;
        }
        if (vars.count("switch-gpio") != 0) {
            auto vec = vars["switch-gpio"].as<std::vector<std::string>>();
            if (vec.size() == 2) {
                //	switch GPIO/LED
                switch_gpio(read_config_section(config_.json_path_, config_.config_index_), vec.at(0), vec.at(1));
            } else {
                std::cerr << "switch-gpio requires 2 parameters: \"number\" [on|off]" << std::endl;
            }
            return true;
        }
        if (vars["nms-default"].as<bool>()) {
            //	print NMS defaults
            print_nms_defaults(std::cout);
            return true;
        }
        if (vars.count("nms-mode") != 0) {
            std::string const mode = vars["nms-mode"].as<std::string>();
            //	set nms/address
            set_nms_mode(read_config_section(config_.json_path_, config_.config_index_), mode.empty() ? "production" : mode);
            return true;
        }
        auto const table = vars["alter"].as<std::string>();
        if (!table.empty()) {
            //	drop and re-create table
            alter_table(read_config_section(config_.json_path_, config_.config_index_), table);
            return true;
        }
        if (!vars["tty"].defaulted()) {
            auto const tty = vars["tty"].as<std::string>();
            BOOST_ASSERT(!tty.empty());
            //	show options of serial port
            print_tty_options(std::cout, tty);
            return true;
        }

        //
        //	call base classe
        //
        return controller_base::run_options(vars);
    }

    cyng::vector_t controller::create_default_config(
        std::chrono::system_clock::time_point &&now,
        std::filesystem::path &&tmp,
        std::filesystem::path &&cwd) {

        auto const [srv_mac, srv_id] = get_server_id();

        return smf::create_default_config(now, tmp, cwd, cyng::sys::get_hostname(), srv_mac, srv_id, get_random_tag());
    }

    std::tuple<cyng::mac48, std::string> controller::get_server_id() const {

        //
        //	get adapter data
        //
        auto macs = cyng::sys::get_mac48_adresses();

        //
        // remove macs with null values
        // erase_if() requires C++20
        // @see https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1115r3.pdf
        //
#if __cplusplus >= 202002
        std::erase_if(macs, [](cyng::mac48 const &mac) -> bool { return cyng::is_nil(mac); });
#else
        macs.erase(std::remove_if(macs.begin(), macs.end(), [](cyng::mac48 const &mac) -> bool { return cyng::is_nil(mac); }));
#endif

        //
        //	get local link addresses
        //
        std::vector<boost::asio::ip::address> local_links;
        std::transform(
            std::begin(macs),
            std::end(macs),
            std::back_inserter(local_links),
            [](cyng::mac48 const &mac) -> boost::asio::ip::address { return mac.to_link_local(); });

        if (macs.empty()) {
            //	random private MAC
            macs.push_back(cyng::generate_random_mac48());
        }

#if defined(__CROSS_PLATFORM)
        auto srv_mac = macs.back(); //	eth2
#else
        auto srv_mac = macs.front();
#endif
        //	build a server ID
        auto tmp = cyng::to_buffer(srv_mac);
        tmp.insert(tmp.begin(), 0x05);

        return {srv_mac, cyng::io::to_hex(tmp)};
    }

    void controller::init_storage(cyng::object &&cfg) {

        auto const reader = cyng::make_reader(std::move(cfg));
        auto s = cyng::db::create_db_session(reader.get("DB"));
        if (s.is_alive()) {
            std::cout << "***info   : database file is " << reader["DB"].get<std::string>("file.name", "") << std::endl;
            smf::init_storage(s);
        } else {
            std::cerr << "**error: no configuration found" << std::endl;
        }
    }

    void controller::read_config(cyng::object &&cfg) {
        auto const reader = cyng::make_reader(cfg);
        auto s = cyng::db::create_db_session(reader.get("DB"));
        if (s.is_alive()) {
            std::cout << "***info   : database file is " << reader["DB"].get<std::string>("file.name", "") << std::endl;
            smf::read_json_config(s, std::move(cfg));
        } else {
            std::cerr << "**error: no configuration found" << std::endl;
        }
    }

    void controller::write_config(cyng::object &&cfg, std::string file_name) {
        auto const reader = cyng::make_reader(cfg);
        auto s = cyng::db::create_db_session(reader.get("DB"));
        if (s.is_alive()) {
            std::cout << "***info   : database file is " << reader["DB"].get<std::string>("file.name", "") << std::endl;
            auto const db = cyng::container_cast<cyng::param_map_t>(reader.get("DB"));
            smf::write_json_config(s, std::move(cfg), file_name, db);
        } else {
            std::cerr << "**error: no configuration found" << std::endl;
        }
    }

    void controller::clear_config(cyng::object &&cfg) {
        auto const reader = cyng::make_reader(std::move(cfg));
        auto s = cyng::db::create_db_session(reader.get("DB"));
        if (s.is_alive()) {
            std::cout << "***info   : database file is " << reader["DB"].get<std::string>("file.name", "") << std::endl;
            smf::clear_config(s);
        } else {
            std::cerr << "**error: no configuration found" << std::endl;
        }
    }

    void controller::list_config(cyng::object &&cfg) {
        auto const reader = cyng::make_reader(std::move(cfg));

        std::cout << "open database file [" << cyng::value_cast(reader["DB"]["file.name"].get(), "") << "]" << std::endl;

        auto s = cyng::db::create_db_session(reader.get("DB"));
        if (s.is_alive()) {
            smf::list_config(s);
        } else {
            std::cerr << "**error: no configuration found" << std::endl;
        }
    }

    void
    controller::set_config_value(cyng::object &&cfg, std::string const &path, std::string const &value, std::string const &type) {

        auto const reader = cyng::make_reader(std::move(cfg));

        std::cout << "open database file [" << cyng::value_cast(reader["DB"]["file.name"].get(), "") << "]" << std::endl;

        auto s = cyng::db::create_db_session(reader.get("DB"));
        if (s.is_alive()) {
            if (smf::set_config_value(s, path, value, type)) {
                if (boost::algorithm::equals(type, "u8")) {
                    auto const u8 = static_cast<std::uint8_t>(std::stoul(value));
                    std::cout << path << " := " << value << " / 0x" << std::hex << +u8 << " (" << type << ")" << std::endl;
                } else if (boost::algorithm::equals(type, "u16")) {
                    auto const u16 = static_cast<std::uint16_t>(std::stoul(value));
                    std::cout << path << " := " << value << " / 0x" << std::hex << u16 << " (" << type << ")" << std::endl;
                } else if (boost::algorithm::equals(type, "u32")) {
                    auto const u32 = static_cast<std::uint32_t>(std::stoul(value));
                    std::cout << path << " := " << value << " / 0x" << std::hex << u32 << " (" << type << ")" << std::endl;
                } else if (boost::algorithm::equals(type, "u64")) {
                    auto const u64 = static_cast<std::uint64_t>(std::stoull(value));
                    std::cout << path << " := " << value << " / 0x" << std::hex << u64 << " (" << type << ")" << std::endl;
                } else {
                    std::cout << path << " := " << value << " (" << type << ")" << std::endl;
                }
            }
        }
    }

    void
    controller::add_config_value(cyng::object &&cfg, std::string const &path, std::string const &value, std::string const &type) {

        auto const reader = cyng::make_reader(std::move(cfg));

        std::cout << "open database file [" << cyng::value_cast(reader["DB"]["file.name"].get(), "") << "]" << std::endl;

        auto s = cyng::db::create_db_session(reader.get("DB"));
        if (s.is_alive()) {
            if (smf::add_config_value(s, path, value, type)) {
                std::cout << path << " := " << value << " (" << type << ")" << std::endl;
            }
        } else {
            std::cerr << "**error: no configuration found" << std::endl;
        }
    }

    void controller::del_config_value(cyng::object &&cfg, std::string const &path) {
        auto const reader = cyng::make_reader(std::move(cfg));

        std::cout << "open database file [" << cyng::value_cast(reader["DB"]["file.name"].get(), "") << "]" << std::endl;

        auto s = cyng::db::create_db_session(reader.get("DB"));
        if (s.is_alive()) {
            if (smf::del_config_value(s, path)) {
                std::cout << path << " removed" << std::endl;
            }
        } else {
            std::cerr << "**error: no configuration found" << std::endl;
        }
    }

    void controller::alter_table(cyng::object &&cfg, std::string table) {
        auto const reader = cyng::make_reader(std::move(cfg));
        auto s = cyng::db::create_db_session(reader.get("DB"));
        if (s.is_alive()) {
            std::cout << "file-name: " << reader["DB"].get<std::string>("file.name", "") << std::endl;
            smf::alter_table(s, table); //
        } else {
            std::cerr << "**error: no configuration found" << std::endl;
        }
    }

    void controller::set_nms_mode(cyng::object &&cfg, std::string mode) {
        auto const reader = cyng::make_reader(std::move(cfg));

        std::cout << "open database file [" << cyng::value_cast(reader["DB"]["file.name"].get(), "") << "]" << std::endl;
        auto s = cyng::db::create_db_session(reader.get("DB"));
        if (s.is_alive()) {
            if (boost::algorithm::equals(mode, "production") || boost::algorithm::equals(mode, "test") ||
                boost::algorithm::equals(mode, "local")) {
                if (smf::set_nms_mode(s, mode)) {
                    std::cout << "NMS mode is " << mode << std::endl;
                } else {
                    std::cerr << "set NMS mode failed" << std::endl;
                }
            } else {
                std::cerr << "unknown NMS mode - use [production|test|local]" << mode << std::endl;
            }
        }
    }

    void controller::switch_gpio(cyng::object &&cfg, std::string const &str, std::string const &state) {

        auto const reader = cyng::make_reader(std::move(cfg));

        //
        //	convert special values
        //  OECP1: 46,  47, 5 0,  53, 64, 68
        //  OECP2: 117, 118, 119, 120 (blue), 121 (A2)
        //
        auto const number = [](std::string const &str) -> std::string {
#if (OECP_VERSION == 1)
            if (boost::algorithm::equals(str, "mbus") || boost::algorithm::equals(str, "wmbus"))
                return "50";
            else if (boost::algorithm::equals(str, "rs485") || boost::algorithm::equals(str, "RS-485"))
                return "53";
            else if (boost::algorithm::equals(str, "ether"))
                return "46";
            else if (boost::algorithm::equals(str, "blue"))
                //  the blue one
                return "64";
#else
            if (boost::algorithm::equals(str, "mbus") || boost::algorithm::equals(str, "wmbus"))
                return "117";
            else if (boost::algorithm::equals(str, "rs485") || boost::algorithm::equals(str, "RS-485"))
                return "118";
            else if (boost::algorithm::equals(str, "ether"))
                return "119";
            else if (boost::algorithm::equals(str, "a4") || boost::algorithm::equals(str, "blue"))
                return "120";
            else if (boost::algorithm::equals(str, "a2"))
                return "121";
            else if (boost::algorithm::equals(str, "a3"))
                return "123";
#endif
            else
                return str;
        };

        auto const path =
            std::filesystem::path(cyng::value_cast(reader["gpio"]["path"].get(), "/")) / ("gpio" + number(str)) / "value";
        std::cout << "system path: [" << path.generic_string() << "]" << std::endl;
        if (!smf::switch_gpio(path, boost::algorithm::equals(state, "on"))) {
            std::cerr << "cannot open GPIO [" << path.generic_string() << "]" << std::endl;
        }
    }

    void controller::run(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        cyng::object const &cfg,
        std::string const &node_name) {

        auto const reader = cyng::make_reader(cfg);
        auto s = cyng::db::create_db_session(reader.get("DB"));

        auto const md = get_store_meta_data();
        bridge_ = ctl.create_named_channel_with_ref<bridge>("bridge", ctl, logger, s, md);
        BOOST_ASSERT(bridge_->is_open());
        bridge_->dispatch("start");
    }

    void controller::shutdown(cyng::registry &reg, cyng::stash &channels, cyng::logger logger) {

        //
        //	main task
        //
        config::stop_tasks(logger, reg, "bridge");
        // logger.stop();

        std::this_thread::sleep_for(std::chrono::seconds(2));

        //
        //	stop all running tasks
        //
        // reg.shutdown();
    }

    void print_nms_defaults(std::ostream &os) {
        auto const nic = get_nic();
        auto const r = get_ipv6_linklocal(nic);
        auto const port = get_default_nms_port();
        std::cout << "default configuration: " << std::endl;
        std::cout << "nic                  : [" << nic << "]" << std::endl;
        std::cout << "first IPv4 address   : " << get_ipv4_address(nic) << std::endl;
        std::cout << "link-local address   : " << r.first << std::endl;
        std::cout << "adapter index        : " << r.second << std::endl;
        std::cout << "port number          : " << port << std::endl;
        std::cout << "link-local endpoint  : "
                  << boost::asio::ip::tcp::endpoint(cyng::sys::make_link_local_address(r.first, r.second), port) << std::endl;
    }

    void print_tty_options(std::ostream &os, std::string tty) {
        boost::system::error_code ec;
        boost::asio::io_service io_service;
        boost::asio::serial_port port(io_service);
        port.open(tty, ec);
        if (!ec) {
            boost::asio::serial_port_base::baud_rate baud_rate;
            port.get_option(baud_rate);
            os << "baud-rate: " << baud_rate.value() << std::endl;

            boost::asio::serial_port_base::parity parity;
            port.get_option(parity);
            os << "parity: " << serial::to_string(parity) << std::endl;

            boost::asio::serial_port_base::character_size databits;
            port.get_option(databits);
            os << "databits: " << +databits.value() << std::endl;

            boost::asio::serial_port_base::stop_bits stop_bits;
            port.get_option(stop_bits);
            os << "stopbits: " << serial::to_string(stop_bits) << std::endl;

            boost::asio::serial_port_base::flow_control flow_control;
            port.get_option(flow_control);
            os << "flow-control: " << serial::to_string(flow_control) << std::endl;

        } else {
            os << "[" << tty << "] cannot open: " << ec.message() << std::endl;
        }
    }

} // namespace smf
