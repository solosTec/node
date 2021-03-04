/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <controller.h>
#include <storage_functions.h>
#include <tasks/bridge.h>

#include <smf/obis/defs.h>
#include <smf/ipt/config.h>

#include <cyng/obj/intrinsics/container.h>
#include <cyng/obj/container_factory.hpp>
#include <cyng/obj/util.hpp>
#include <cyng/obj/object.h>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/sys/locale.h>
#include <cyng/sys/host.h>
#include <cyng/sys/mac.h>
#include <cyng/task/controller.h>
#include <cyng/io/io_buffer.h>
#include <cyng/log/record.h>

#include <locale>
#include <iostream>

#include <boost/predef.h>

namespace smf {

	controller::controller(config::startup const& config) 
		: controller_base(config)
	{}

	bool controller::run_options(boost::program_options::variables_map& vars) {

		//
		//	
		//
		if (vars["init"].as< bool >())	{
			//	initialize database
			init_storage(read_config_section(config_.json_path_, config_.config_index_));
			return true;
		}
		if (vars["transfer"].as< bool >())	{
			//	transfer JSON configuration into database
			transfer_config(read_config_section(config_.json_path_, config_.config_index_));
			return true;
		}
		if (vars["clear"].as< bool >())	{
			//	clear configuration from database
			clear_config(read_config_section(config_.json_path_, config_.config_index_));
			return true;
		}
		if (vars["list"].as< bool >()) {
			//	show database configuration
			list_config(read_config_section(config_.json_path_, config_.config_index_));
			return true;
		}
		if (vars.count("set-value") != 0) {
			auto vec = vars["set-value"].as<std::vector<std::string>>();
			if (vec.size() == 3) {
				//	set configuration value
				set_config_value(read_config_section(config_.json_path_, config_.config_index_), vec.at(0), vec.at(1), vec.at(2));
			}
			else {
				std::cerr
					<< "set-value requires 3 parameters: \"path\" \"value\" \"type\""
					<< std::endl; 
			}
			return true;
		}

		//
		//	call base classe
		//
		return controller_base::run_options(vars);
	}

	cyng::vector_t controller::create_default_config(std::chrono::system_clock::time_point&& now
		, std::filesystem::path&& tmp
		, std::filesystem::path&& cwd) {

		//
		//	get hostname
		//
		auto const hostname = cyng::sys::get_hostname();

		return cyng::make_vector({
			cyng::make_tuple(
				cyng::make_param("generated", now),
				cyng::make_param("log-dir", tmp.string()),
				cyng::make_param("tag", get_random_tag()),
				cyng::make_param("country-code", "CH"),
				cyng::make_param("language-code", cyng::sys::get_system_locale()),
				cyng::make_param("generate-profile", false),

				create_server_id(),

				//	SQLite3
				create_db_spec(cwd),

				//	IP-T client
				create_ipt_spec(hostname),

				//	SML server
				create_sml_server_spec(),

				//	NMS server
				create_nms_server_spec(tmp),

				//	array of all available serial ports
				//	with broker
				create_lmn_spec(hostname),

				//	GPIO
				create_gpio_spec(),

				//	hardware
				create_hardware_spec(),

				//	virtual meter
				create_virtual_meter_spec()
			)
		});
	}

	cyng::param_t controller::create_server_id() const {

		//	get adapter data
		//
		auto macs = cyng::sys::get_mac48_adresses();

		//
		//	get local link addresses
		//
		std::vector<boost::asio::ip::address> local_links;
		std::transform(std::begin(macs), std::end(macs), std::back_inserter(local_links), [](cyng::mac48 const& mac)->boost::asio::ip::address {
			return mac.to_link_local();
		});

		if (macs.empty()) {
			//	random private MAC
			macs.push_back(cyng::generate_random_mac48());
		}

		//	smf_CROSSCOMPILING = true
		//	__NATIVE_PLATFORM
#if defined(__CROSS_PLATFORM)
		auto srv_mac = macs.back();	//	eth2
#else
		auto srv_mac = macs.front();
#endif

		//	build a server ID
		auto tmp = cyng::to_buffer(srv_mac);
		tmp.insert(tmp.begin(), 0x05);

		return cyng::make_param("net", cyng::tuple_factory(
			cyng::make_param("mac", srv_mac),
			cyng::make_param("nics", macs),
			cyng::make_param(cyng::to_str(OBIS_SERVER_ID), cyng::io::to_hex(tmp)),
			cyng::make_param("local-links", local_links)
		));
	}

	cyng::param_t controller::create_virtual_meter_spec() const {
		return cyng::make_param("virtual-meter", cyng::tuple_factory(
			cyng::make_param("enabled", false),
			cyng::make_param("server", "01-d81c-10000001-3c-02"),	//	1CD8
			cyng::make_param("port-index", 1),	//	0 = wireless, 1 = wired
			cyng::make_param("protocol", "iec"),	//	wired => IEC
			cyng::make_param("interval", 26000)	//	seconds push interval
		));
	}

	cyng::tuple_t controller::create_wireless_spec(std::string const& hostname) const {

		//	wireless M-Bus
		//	stty -F /dev/ttyAPP0 raw
		//	stty -F /dev/ttyAPP0  -echo -echoe -echok
		//	stty -F /dev/ttyAPP0 115200 
		//	cat /dev/ttyAPP0 | hexdump 

		return cyng::make_tuple(

			cyng::make_param("type", "wireless M-Bus"),

#if defined(BOOST_OS_WINDOWS_AVAILABLE)

			//	iM871A
			cyng::make_param("enabled", false),
			cyng::make_param("port", "COM3"),	//	USB serial port
			//	if port number is greater than 9 the following syntax is required: "\\\\.\\COM12"
			cyng::make_param("HCI", "CP210x"),	//	iM871A mbus-USB converter
			cyng::make_param("databits", 8),
			cyng::make_param("parity", "none"),	//	none, odd, even
			cyng::make_param("flow-control", "none"),	//	none, software, hardware
			cyng::make_param("stopbits", "one"),	//	one, onepointfive, two
			cyng::make_param("speed", 57600),	//	0xE100

#else

			cyng::make_param("enabled", true),
			cyng::make_param("port", "/dev/ttyAPP0"),
			cyng::make_param("HCI", "none"),
			cyng::make_param("databits", 8),
			cyng::make_param("parity", "none"),	//	none, odd, even
			cyng::make_param("flow-control", "none"),	//	none, software, hardware
			cyng::make_param("stopbits", "one"),	//	one, onepointfive, two
			cyng::make_param("speed", 115200),

#endif
			cyng::make_param("protocol", "wM-Bus:EN13757-4"),		//	raw, mbus, iec, sml
			cyng::make_param("broker-enabled", false),
			cyng::make_param("broker-login", false),
			cyng::make_param("broker-timeout", 12),	//	seconds
			create_wireless_broker(hostname),
			create_wireless_block_list()

		);
	}

	cyng::param_t controller::create_wireless_block_list() const {
		return cyng::make_param("blocklist", cyng::param_map_factory
#ifdef _DEBUG
			("enabled", true)
			("list", cyng::make_vector({ "00684279", "12345678" }))
#else
			("enabled", false)
			("list", cyng::make_vector({}))
#endif
			("mode", "drop")	//	or accept
			("period", 30)	//	seconds
			.operator cyng::param_map_t()
		);
	}

	cyng::param_t controller::create_wireless_broker(std::string const& hostname) const {
		return cyng::make_param("broker", cyng::make_vector({
			//	define multiple broker here
			cyng::param_map_factory("address", "segw.ch")
				("port", 12001)
				("account", hostname)
				("pwd", "wM-Bus")
			.operator cyng::param_map_t()
		}));
	}

	cyng::param_t controller::create_rs485_broker(std::string const& hostname) const {
		return cyng::make_param("broker", cyng::make_vector({
			//	define multiple broker here
			cyng::param_map_factory("address", "segw.ch")
				("port", 12002)
				("account", hostname)
				("pwd", "rs485")
			.operator cyng::param_map_t()
			}));
	}

	cyng::param_t controller::create_rs485_listener() const {
		return cyng::make_param("listener", cyng::tuple_factory(
			cyng::make_param("address", "0.0.0.0"),
			cyng::make_param("port", 6006),
			cyng::make_param("login", false),		//	request login
			cyng::make_param("enabled", false)	//	start rs485 server
		));
	}

	cyng::tuple_t controller::create_rs485_spec(std::string const& hostname) const {
		return cyng::make_tuple(

			cyng::make_param("type", "RS-485"),

#if defined(BOOST_OS_WINDOWS_AVAILABLE)
			cyng::make_param("enabled", false),
			cyng::make_param("port", "COM6"),
			cyng::make_param("parity", "even"),	//	none, odd, even

			//	8N1
			cyng::make_param("databits", 8),
			cyng::make_param("flow-control", "none"),	//	none, software, hardware
			cyng::make_param("stopbits", "one"),	//	one, onepointfive, two
			cyng::make_param("speed", 2400),		//	initial

#else
			cyng::make_param("enabled", true),
			cyng::make_param("port", "/dev/ttyAPP1"),
			cyng::make_param("parity", "none"),	//	none, odd, even

			//	8N1
			cyng::make_param("databits", 8),
			cyng::make_param("flow-control", "none"),	//	none, software, hardware
			cyng::make_param("stopbits", "one"),	//	one, onepointfive, two
			cyng::make_param("speed", 9600),		//	initial

#endif
			cyng::make_param("protocol", "raw"),		//	raw, mbus, iec, sml
			cyng::make_param("broker-enabled", false),
			cyng::make_param("broker-login", false),
			cyng::make_param("broker-reconnect", 12),	//	seconds
			create_rs485_broker(hostname),
			create_rs485_listener()
		);
	}

	cyng::param_t controller::create_gpio_spec() const {
		return cyng::make_param("gpio", cyng::param_map_factory
		("enabled",
#if defined(__ARMEL__)
			true
#else
			false
#endif
		)
			("path", "/sys/class/gpio")
			("list", cyng::make_vector({ 46, 47, 50, 53 }))
			()
		);
	}

	cyng::param_t controller::create_hardware_spec() const {
		return cyng::make_param("hardware", cyng::tuple_factory(
			//	manufacturer (81 81 C7 82 03 FF - OBIS_DATA_MANUFACTURER)
#if defined(__CROSS_PLATFORM) && defined(BOOST_OS_LINUX_AVAILABLE)
			cyng::make_param("manufacturer", "PPC"),
#else
			cyng::make_param("manufacturer", "solosTec"),
#endif
			cyng::make_param("model", "virtual.gateway"),	//	Typenschl�ssel (81 81 C7 82 09 FF --> 81 81 C7 82 0A 01)
			//cyng::make_param("serial", sn),	//	Seriennummer (81 81 C7 82 09 FF --> 81 81 C7 82 0A 02)
			cyng::make_param("class", "129-129:199.130.83*255")	//	device class (81 81 C7 82 02 FF - OBIS_DEVICE_CLASS) "2D 2D 2D"
			//cyng::make_param("adapter", cyng::tuple_factory(
			//	cyng::make_param(sml::OBIS_W_MBUS_ADAPTER_MANUFACTURER.to_str(), "RC1180-MBUS3"),	//	manufacturer (81 06 00 00 01 00)
			//	cyng::make_param(sml::OBIS_W_MBUS_ADAPTER_ID.to_str(), "A8 15 17 45 89 03 01 31"),	//	adapter ID (81 06 00 00 03 00)
			//	cyng::make_param(sml::OBIS_W_MBUS_FIRMWARE.to_str(), "3.08"),	//	firmware (81 06 00 02 00 00)
			//	cyng::make_param(sml::OBIS_W_MBUS_HARDWARE.to_str(), "2.00")	//	hardware (81 06 00 02 03 FF)
			//))
		));
	}

	cyng::param_t controller::create_nms_server_spec(std::filesystem::path const& tmp) const {
		return cyng::make_param("nms", cyng::tuple_factory(
			cyng::make_param("address", "0.0.0.0"),
			cyng::make_param("port", 7261),
			cyng::make_param("account", "operator"),
			cyng::make_param("pwd", "operator"),
#if defined(__CROSS_PLATFORM) && defined(BOOST_OS_LINUX_AVAILABLE)
			cyng::make_param("enabled", true),
#else
			cyng::make_param("enabled", false),
#endif
			cyng::make_param("script-path",
#if defined(BOOST_OS_LINUX_AVAILABLE)
				(tmp / "update-script.sh").string()
#else
				(tmp / "update-script.cmd").string()
#endif
			),
#ifdef _DEBUG
			cyng::make_param("debug", true)
#else
			cyng::make_param("debug", false)
#endif

		));

	}
	cyng::param_t controller::create_sml_server_spec() const {

		return cyng::make_param("sml", cyng::make_tuple(
			cyng::make_param("address", "0.0.0.0"),
			cyng::make_param("port", 7259),
			cyng::make_param("discover", 5798),	//	UDP
			cyng::make_param("account", "operator"),
			cyng::make_param("pwd", "operator"),
#if defined(__CROSS_PLATFORM) && defined(BOOST_OS_LINUX_AVAILABLE)
			cyng::make_param("enabled", false),
#else
			cyng::make_param("enabled", true),
#endif
			cyng::make_param("accept-all-ids", false)	//	accept only the specified MAC id
		));
	}

	cyng::param_t controller::create_db_spec(std::filesystem::path const& cwd) const {
		return cyng::make_param("DB", cyng::make_tuple(
#if defined(__CROSS_PLATFORM) && defined(BOOST_OS_LINUX_AVAILABLE)
			cyng::make_param("file-name", "/usr/local/etc/smf/segw.database"),
#else
			cyng::make_param("file-name", (cwd / "segw.database").string()),
#endif
			cyng::make_param("busy-timeout", 12),			//	seconds
			cyng::make_param("watchdog", 30),				//	for database connection
			cyng::make_param("connection-type", "SQLite")	//	file based
		));
	}

	cyng::param_t controller::create_ipt_spec(std::string const& hostname)  const {

		return cyng::make_param("ipt", cyng::make_tuple(
			create_ipt_config(hostname),
			create_ipt_params()
		));
	}

	cyng::param_t controller::create_ipt_config(std::string const& hostname)  const {

		return cyng::make_param("config", cyng::make_vector({

		//
		//	redundancy I
		//
		cyng::make_tuple(

#if defined(__CROSS_PLATFORM) && defined(BOOST_OS_LINUX_AVAILABLE)
			cyng::make_param("host", "segw.ch"),
			cyng::make_param("account", hostname),
			cyng::make_param("service", "26862"),
#else
			cyng::make_param("host", "127.0.0.1"),
			cyng::make_param("account", "segw"),
			cyng::make_param("service", "26862"),
#endif

			cyng::make_param("pwd", "NODE_PWD"),
			cyng::make_param("def-sk", "0102030405060708090001020304050607080900010203040506070809000001"),	//	scramble key
			cyng::make_param("scrambled", true)
			//cyng::make_param("monitor", rnd_monitor())),	//	seconds

		),

		//
		//	redundancy II
		//
		cyng::make_tuple(

#if defined(__CROSS_PLATFORM) && defined(BOOST_OS_LINUX_AVAILABLE)
			cyng::make_param("host", "192.168.1.100"),
			cyng::make_param("account", hostname),
			cyng::make_param("service", "26862"),
#else
			cyng::make_param("host", "127.0.0.1"),
			cyng::make_param("account", "segw"),
			cyng::make_param("service", "26863"),
#endif
			cyng::make_param("pwd", "NODE_PWD"),
			cyng::make_param("def-sk", "0102030405060708090001020304050607080900010203040506070809000001"),	//	scramble key
			cyng::make_param("scrambled", true)
			//cyng::make_param("monitor", rnd_monitor())),	//	seconds

		)
		}
		));
	}

	cyng::param_t controller::create_ipt_params() const {
		return cyng::make_param("param", cyng::tuple_factory(
			cyng::make_param(OBIS_TCP_WAIT_TO_RECONNECT, 1u),	//	minutes
			cyng::make_param(OBIS_TCP_CONNECT_RETRIES, 20u),
			cyng::make_param(OBIS_HAS_SSL_CONFIG, 0u),	//	has SSL configuration
			cyng::make_param("enabled", true)
		));
	}

	cyng::param_t controller::create_lmn_spec(std::string const& hostname) const {
		//	list of all serial ports
		return cyng::make_param("lmn", cyng::make_vector({
			create_wireless_spec(hostname),
			create_rs485_spec(hostname)
			}));
	}

	void controller::init_storage(cyng::object&& cfg) {

		auto const reader = cyng::make_reader(std::move(cfg));
		auto s = cyng::db::create_db_session(reader.get("DB"));
		if (s.is_alive())	smf::init_storage(s);
	}

	void controller::transfer_config(cyng::object&& cfg) {
		auto const reader = cyng::make_reader(cfg);
		auto s = cyng::db::create_db_session(reader.get("DB"));
		if (s.is_alive())	smf::transfer_config(s, std::move(cfg));
	}

	void controller::clear_config(cyng::object&& cfg) {
		auto const reader = cyng::make_reader(std::move(cfg));
		auto s = cyng::db::create_db_session(reader.get("DB"));
		if (s.is_alive())	smf::clear_config(s);
	}

	void controller::list_config(cyng::object&& cfg) {
		auto const reader = cyng::make_reader(std::move(cfg));
		auto s = cyng::db::create_db_session(reader.get("DB"));
		if (s.is_alive())	smf::list_config(s);
	}

	void controller::set_config_value(cyng::object&& cfg
		, std::string const& path
		, std::string const& value
		, std::string const& type) {

		auto const reader = cyng::make_reader(std::move(cfg));
		auto s = cyng::db::create_db_session(reader.get("DB"));
		if (s.is_alive()) {
			if (smf::set_config_value(s, path, value, type)) {
				std::cout << path << " := " << value << " (" << type << ")" << std::endl;
			}
		}
	}

	void controller::run(cyng::controller& ctl, cyng::logger logger, cyng::object const& cfg) {

		auto const reader = cyng::make_reader(cfg);
		auto s = cyng::db::create_db_session(reader.get("DB"));

		auto const ipt_cfg = cyng::container_cast<cyng::vector_t>(reader["ipt"]["config"].get());
		auto tgl = ipt::read_config(ipt_cfg);
		BOOST_ASSERT(!tgl.empty());
		CYNG_LOG_INFO(logger, tgl.size() << " ip-t server configured");

		auto channel = ctl.create_named_channel_with_ref<bridge>("bridge", ctl, logger, s, std::move(tgl));
		BOOST_ASSERT(channel->is_open());
		channel->dispatch("start", cyng::make_tuple());
	}


}