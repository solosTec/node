#include <config/generator.h>

#include <config/cfg_nms.h>
#include <smf.h>
#include <smf/config/protocols.h>
#include <smf/obis/defs.h>

#include <cyng/obj/container_factory.hpp>
#include <cyng/rnd/rnd.hpp>
#include <cyng/sys/clock.h>
#include <cyng/sys/host.h>
#include <cyng/sys/locale.h>
#include <cyng/sys/net.h>

#ifdef _DEBUG_SEGW
#include <iostream>
#endif

namespace smf {
    cyng::vector_t create_default_config(
        std::chrono::system_clock::time_point now,
        std::filesystem::path tmp,
        std::filesystem::path cwd,
        std::string hostname,
        cyng::mac48 srv_mac,
        std::string srv_id,
        boost::uuids::uuid tag) {

        return cyng::make_vector({
            cyng::make_tuple(
                cyng::make_param("generated", now),
                cyng::make_param("version", SMF_VERSION_TAG),
                cyng::make_param("tag", tag),
                cyng::make_param("opcounter", 0), //  operation time counter
#if !defined(OECP_VERSION)
                //  This makes trouble on OECP-1 hardware
                cyng::make_param("country-code", cyng::sys::get_system_locale().at(cyng::sys::info::COUNTRY)),
                cyng::make_param("language-code", cyng::sys::get_system_locale().at(cyng::sys::info::LANGUAGE)),
#endif
                cyng::make_param("generate-profile", false),
                cyng::make_param("utc-offset", cyng::sys::delta_utc(now).count()),

                create_net(srv_mac, srv_id),

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
                create_hardware_spec(srv_id))
        });
    }

    cyng::param_t create_net(cyng::mac48 const &srv_mac, std::string const &srv_id) {

        return cyng::make_param(
            "net",
            cyng::tuple_factory(cyng::make_param("mac", srv_mac), cyng::make_param(cyng::to_string(OBIS_SERVER_ID), srv_id)));
    }

    cyng::param_t create_wireless_virtual_meter_spec() {
        return cyng::make_param(
            "virtual-meter",
            cyng::tuple_factory(
                cyng::make_param("enabled", false),
                cyng::make_param("server", "01-d81c-10000001-3c-02"),                            //	1CD8
                cyng::make_param("protocol", config::get_name(config::protocol::WIRELESS_MBUS)), //	wireless M-Bus
                cyng::make_param("interval", "00:00:07.000000")                                  //	push interval
                ));
    }

    cyng::param_t create_wired_virtual_meter_spec() {
        return cyng::make_param(
            "virtual-meter",
            cyng::tuple_factory(
                cyng::make_param("enabled", false),
                cyng::make_param("server", "51540472"),
                cyng::make_param("protocol", config::get_name(config::protocol::IEC)), //	wired => IEC
                cyng::make_param("interval", "00:02:00.000000")                        //	push interval
                ));
    }

    cyng::tuple_t create_wireless_spec(std::string const &hostname) {

        return cyng::make_tuple(

            cyng::make_param("type", "wireless M-Bus"),

#if defined(BOOST_OS_WINDOWS_AVAILABLE)

            //	iM871A
            cyng::make_param("enabled", false),
            cyng::make_param("port", "COM3"), //	USB serial port
            //	if port number is greater than 9 the following syntax is required: "\\\\.\\COM12"
            cyng::make_param("HCI", "CP210x"), //	iM871A mbus-USB converter
            cyng::make_param("databits", 8),
            cyng::make_param("parity", "none"),       //	none, odd, even
            cyng::make_param("flow-control", "none"), //	none, software, hardware
            cyng::make_param("stopbits", "one"),      //	one, onepointfive, two
            cyng::make_param("speed", 57600),         //	0xE100

#else

            cyng::make_param("enabled", true),
            cyng::make_param("HCI", "none"),
#if (OECP_VERSION == 1)

            //  OECP-1
            //	wireless M-Bus OECP1
            //	stty -F /dev/ttyAPP0 raw
            //	stty -F /dev/ttyAPP0  -echo -echoe -echok
            //	stty -F /dev/ttyAPP0 115200
            //	cat /dev/ttyAPP0 | hexdump

            cyng::make_param("port", "/dev/ttyAPP0"),
            cyng::make_param("databits", 8),
            cyng::make_param("parity", "none"),       //	none, odd, even
            cyng::make_param("flow-control", "none"), //	none, software, hardware
            cyng::make_param("stopbits", "one"),      //	one, onepointfive, two
            cyng::make_param("speed", 115200),
#else
            //  OECP-2
            //  i2cset -y 2 0x8 0xf4 0x1f
            //  hexdump -C /dev/ttymxc0
            cyng::make_param("port", "/dev/ttymxc0"),
            cyng::make_param("databits", 8),
            cyng::make_param("parity", "none"),       //	none, odd, even
            cyng::make_param("flow-control", "none"), //	none, software, hardware
            cyng::make_param("stopbits", "one"),      //	one, onepointfive, two
            cyng::make_param("speed", 115200),
#endif

#endif
            //  wM-Bus-EN13757-4"
            cyng::make_param("protocol", config::get_name(config::protocol::WIRELESS_MBUS)), //	raw, mbus, iec, sml
            cyng::make_param("broker-enabled", true),
            cyng::make_param("broker-login", false),
            // cyng::make_param("broker-timeout", 12), //	seconds
            cyng::make_param("hex-dump", false),
            cyng::make_param(
                "cache",
                cyng::make_tuple(
                    cyng::make_param("enabled", true),                   // active
                    cyng::make_param("push", "segw.ch:2002"),            // TCP/IP server
                    cyng::make_param("period", std::chrono::minutes(60)) // minutes - "00:60:00.000000"
                    )),
            create_wireless_broker(hostname),
            create_wireless_block_list(),
            create_wireless_virtual_meter_spec()

        );
    }

    cyng::param_t create_wireless_block_list() {
        return cyng::make_param(
            "blocklist",
            cyng::param_map_factory
#ifdef _DEBUG
            ("enabled", true) // blocklist enabled
            ("list", cyng::make_vector({"00684279", "12345678"}))
#else
            ("enabled", false)                         // blocklist disabled
            ("list", cyng::make_vector({}))            // no entries
#endif
                ("mode", "drop") //	or accept
            ("period", 30)       //	seconds
                .
                operator cyng::param_map_t());
    }

    cyng::param_t create_wireless_broker(std::string const &hostname) {
        return cyng::make_param(
            "broker",
            cyng::make_vector( //	define multiple broker here
                {
                    cyng::param_map_factory
#if defined(BOOST_OS_WINDOWS_AVAILABLE)
                        ("address", "segw.ch") // host name
                        ("port", 12001)        // port
#else
                        ("address", "192.168.230.208") // host name
                        ("port", 3000)                 // port
#endif
                        ("account", hostname)       //  login name
                        ("pwd", "wM-Bus")           //  password
                        ("connect-on-demand", true) //  broker algorithm (connect on demand, otherwise connect at start)
                        ("write-timeout", std::chrono::seconds(2)) //	seconds - only for on-demand
                        ("watchdog", std::chrono::seconds(12))     //	seconds - only for on-start
                            .
                            operator cyng::param_map_t()
                }));
    }

    cyng::param_t create_rs485_broker(std::string const &hostname) {
        return cyng::make_param(
            "broker",
            cyng::make_vector(                                 // define multiple broker here
                {                                              // define multiple broker here
                 cyng::param_map_factory("address", "segw.ch") // address
                 ("port", 12002)                               // port
                 ("account", hostname)                         // login name
                 ("pwd", "rs485")                              // password
                 ("connect-on-demand", true)                   // broker algorithm (connect on demand, otherwise connect at start)
                 ("write-timeout", 2)                          // seconds - only for on-demand
                 ("watchdog", 12)                              // seconds - only for on-start
                     .
                     operator cyng::param_map_t()}));
    }

    cyng::param_t create_rs485_listener() {
        return cyng::make_param(
            "listener",
            cyng::tuple_factory(
                cyng::make_param("address", "0.0.0.0"),
                // cyng::make_param("link-local", get_nms_address("br0")),
                cyng::make_param("port", 6006),
                cyng::make_param("login", false),                     //	request login
                cyng::make_param("enabled", true),                    //	start rs485 server
                cyng::make_param("delay", std::chrono::seconds(30)),  //	startup delay
                cyng::make_param("timeout", std::chrono::seconds(10)) //	maximum idle time
                ));
    }

    cyng::param_t create_rs485_block_list() {
        return cyng::make_param(
            "blocklist",
            cyng::tuple_factory(cyng::make_param("enabled", false) //	no block list
                                ));
    }

    cyng::tuple_t create_rs485_spec(std::string const &hostname) {
        return cyng::make_tuple(

            cyng::make_param("type", "RS-485"),

#if defined(BOOST_OS_WINDOWS_AVAILABLE)
            //
            //  windows
            //
            cyng::make_param("enabled", false),
            cyng::make_param("port", "COM6"),
            cyng::make_param("parity", "even"), //	none, odd, even

            //	8N1
            cyng::make_param("databits", 8),
            cyng::make_param("flow-control", "none"), //	none, software, hardware
            cyng::make_param("stopbits", "one"),      //	one, onepointfive, two
            cyng::make_param("speed", 2400),          //	initial

#else
                                                       //
                                                       //  any kind of Linux (start...)
                                                       //
#if (OECP_VERSION == 1)
            cyng::make_param("enabled", true),
            cyng::make_param("port", "/dev/ttyAPP1"),
#elif (OECP_VERSION == 2)
            cyng::make_param("enabled", true),
            cyng::make_param("port", "/dev/ttymxc1"),
#else
            cyng::make_param("enabled", false),
            cyng::make_param("port", "/dev/ttyS0"),
#endif

        //	8N1
#if defined(__ARMEL__)
            //
            //  ARM architecture
            //
            cyng::make_param("databits", 7),
            cyng::make_param("stopbits", "two"),      //	one, onepointfive, two
            cyng::make_param("parity", "even"),       //	none, odd, even
#else
            cyng::make_param("databits", 8),
            cyng::make_param("stopbits", "one"), //	one, onepointfive, two
            cyng::make_param("parity", "none"),  //	none, odd, even
#endif
            cyng::make_param("flow-control", "none"), //	none, software, hardware
            cyng::make_param("speed", 9600),          //	initial

        //
        //  any kind of Linux (...end)
        //

#endif
            cyng::make_param("protocol", "raw"), //	raw, mbus, iec, sml
            cyng::make_param("broker-enabled", false),
            cyng::make_param("broker-login", false),
            cyng::make_param("broker-reconnect", 12), //	seconds
            cyng::make_param("hex-dump", false),
            create_rs485_broker(hostname),
            create_rs485_listener(),
            create_rs485_block_list(),
            create_wired_virtual_meter_spec());
    }

    cyng::param_t create_gpio_spec() {
        //	available pins are
        //  OECP1: 46, 47, 50, 53, 64, 68
        //  OECP2: 117, 118, 119, 120 (blue) - see /sbin/leds.sh
        //  if [ ! -e /sys/class/gpio/gpio117 ] ; then
        //      echo 117 > /sys/class/gpio/export
        //  fi
        //  if [ ! -e /sys/class/gpio/gpio118 ] ; then
        //      echo 118 > /sys/class/gpio/export
        //  fi
        //  if [ ! -e /sys/class/gpio/gpio119 ] ; then
        //      echo 119 > /sys/class/gpio/export
        //  fi
        //  if [ ! -e /sys/class/gpio/gpio120 ] ; then
        //      echo 120 > /sys/class/gpio/export
        //  fi

        // echo "out" > /sys/class/gpio/gpio117/direction
        // echo "out" > /sys/class/gpio/gpio118/direction
        // echo "out" > /sys/class/gpio/gpio119/direction
        // echo "out" > /sys/class/gpio/gpio120/direction

        return cyng::make_param(
            "gpio",
            cyng::param_map_factory(
                "enabled",
#if defined(__ARMEL__)
                true
#else
                false
#endif
                ) // enabled/disabled

#if (OECP_VERSION == 1)
            ("path", "/sys/class/gpio")("list", cyng::make_vector({46, 47, 50, 53})) //	, 64, 68 OECP1
#else
            ("path", "/sys/class/gpio")("list", cyng::make_vector({117, 118, 119, 120})) //	OECP2
#endif
            ());
    }

    cyng::param_t create_hardware_spec(std::string const &srv_id) {

        //
        //	generate a random serial number with a length of
        //	8 characters
        //
        auto rnd_sn = cyng::crypto::make_rnd_num();
        std::stringstream sn_ss(rnd_sn(8));
        std::uint32_t sn{0};
        sn_ss >> sn;

        auto const model = detect_model(srv_id);

        return cyng::make_param(
            "hardware",
            cyng::tuple_factory(
        //	manufacturer (81 81 C7 82 03 FF - OBIS_DATA_MANUFACTURER)
#if defined(__CROSS_PLATFORM) && defined(BOOST_OS_LINUX_AVAILABLE)
                cyng::make_param("manufacturer", "PPC"),
#else
                cyng::make_param("manufacturer", "solosTec"),
#endif
                cyng::make_param("model", model),          //	Typenschluessel (81 81 C7 82 09 FF --> 81 81 C7 82 0A 01)
                cyng::make_param("serial", sn),            //	Seriennummer (81 81 C7 82 09 FF --> 81 81 C7 82 0A 02)
                cyng::make_param("class", "8181C78202FF"), //	device class (129-129:199.130.83*255 - OBIS_DEVICE_CLASS) "2D 2D 2D"
                cyng::make_param(
                    "adapter",
                    cyng::tuple_factory(
                        cyng::make_param(cyng::to_string(OBIS_W_MBUS_ADAPTER_MANUFACTURER), "RC1180-MBUS3"), //	manufacturer
                        cyng::make_param(cyng::to_string(OBIS_W_MBUS_ADAPTER_ID), "A815174589030131"),
                        cyng::make_param(cyng::to_string(OBIS_W_MBUS_FIRMWARE), "3.08"), // firmware (81 06 00 02 00 00)
                        cyng::make_param(cyng::to_string(OBIS_W_MBUS_HARDWARE), "2.00")  //	hardware (81 06 00 02 03 FF)
                        ))));
    }

    cyng::param_t create_nms_server_spec(std::filesystem::path const &tmp) {

        auto const nic = get_nic();
        auto const link_local = get_ipv6_linklocal(nic);
        auto const address = cyng::sys::make_link_local_address(link_local.first, link_local.second);
        // auto const ep = boost::asio::ip::tcp::endpoint(address, 7562);

        return cyng::make_param(
            "nms",
            cyng::tuple_factory(
                cyng::make_param("address", address), //  link-local address is default
                cyng::make_param("port", 7562),
                cyng::make_param("account", "operator"),
                cyng::make_param("pwd", "operator"),
                cyng::make_param("nic", nic),
                cyng::make_param("nic-ipv4", get_ipv4_address(nic)),
                cyng::make_param("nic-linklocal", link_local.first),
                cyng::make_param("nic-index", link_local.second),
#if defined(BOOST_OS_LINUX_AVAILABLE)
                cyng::make_param("enabled", true),
#else
                cyng::make_param("enabled", false),
#endif
                cyng::make_param(
                    "script-path",
#if defined(BOOST_OS_LINUX_AVAILABLE)
                    (tmp / "update-script.sh").string()
#else
                    (tmp / "update-script.cmd").string()
#endif
                        ),
#ifdef _DEBUG
                cyng::make_param("debug", true),
#else
                cyng::make_param("debug", false),
#endif
                cyng::make_param("delay", std::chrono::seconds(12)),
                cyng::make_param("mode", "production")));
    }
    cyng::param_t create_sml_server_spec() {

        return cyng::make_param(
            "sml",
            cyng::make_tuple(
                cyng::make_param("address", "0.0.0.0"),
                cyng::make_param("port", 7259),
                cyng::make_param("discover", 5798), //	UDP
                cyng::make_param("account", "operator"),
                cyng::make_param("pwd", "operator"),
                cyng::make_param(
                    "enabled",
#if defined(__CROSS_PLATFORM) && defined(BOOST_OS_LINUX_AVAILABLE)
                    false
#else
                    true
#endif
                    ),
                cyng::make_param("accept-all-ids", false), //	accept only the specified MAC id
                cyng::make_param(
                    "auto-config", //  create a data mirror for each new wireless M-Bus device
#ifdef _DEBUG
                    false
#else
                    true
#endif
                    ),
                cyng::make_param("default-profile", CODE_PROFILE_15_MINUTE) //	profile to use in auto-config mode
                ));
    }

    cyng::param_t create_db_spec(std::filesystem::path const &cwd) {
        return cyng::make_param(
            "DB",
            cyng::make_tuple(
#if defined(OECP_VERSION)
#if (OECP_VERSION == 2)
                //  OECP 2
                cyng::make_param("file-name", "/usr/etc/smf/segw.database"),
#else
                //  OECP 1
                cyng::make_param("file-name", "/usr/local/etc/smf/segw.database"),
#endif
#else
                cyng::make_param("file-name", (cwd / "segw.database").string()),
#endif
                cyng::make_param("busy-timeout", 12),         //	seconds
                cyng::make_param("watchdog", 30),             //	for database connection
                cyng::make_param("connection-type", "SQLite") //	file based
                ));
    }

    cyng::param_t create_ipt_spec(std::string const &hostname) {

        return cyng::make_param("ipt", cyng::make_tuple(create_ipt_config(hostname), create_ipt_params()));
    }

    cyng::param_t create_ipt_config(std::string const &hostname) {

        //
        //  no cryptographic hash (like SHAn) required
        //
        auto const pwd = std::hash<std::string>{}(hostname);

        return cyng::make_param(
            "config", cyng::make_vector({
                //
                //	redundancy I
                //
                cyng::make_tuple(

                    cyng::make_param("host", "localhost"),
                    cyng::make_param("account", hostname),
                    cyng::make_param("service", "26862"),
                    cyng::make_param("pwd", std::to_string(pwd)),
                    cyng::make_param("def-sk", "0102030405060708090001020304050607080900010203040506070809000001"), //	scramble key
                    cyng::make_param("scrambled", true)

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
                        cyng::make_param("pwd", std::to_string(pwd)),
                        cyng::make_param(
                            "def-sk", "0102030405060708090001020304050607080900010203040506070809000001"), //	scramble key
                        cyng::make_param("scrambled", true)

                    )
            }));
    }

    cyng::param_t create_ipt_params() {
        return cyng::make_param(
            "param",
            cyng::tuple_factory(
                cyng::make_param(OBIS_TCP_WAIT_TO_RECONNECT, 1u), //	minutes
                cyng::make_param(OBIS_TCP_CONNECT_RETRIES, 20u),
                cyng::make_param(OBIS_HAS_SSL_CONFIG, 0u), //	has SSL configuration
#if defined(__ARMEL__)
                cyng::make_param("enabled", false)
#else
                cyng::make_param("enabled", true)
#endif
                    ));
    }

    cyng::param_t create_lmn_spec(std::string const &hostname) {
        //	list of all serial ports
        return cyng::make_param("lmn", cyng::make_vector({create_wireless_spec(hostname), create_rs485_spec(hostname)}));
    }

    std::string detect_model(std::string const &srv_id) {
        auto const nics = cyng::sys::get_nic_names();

        if (std::find(std::begin(nics), std::end(nics), "br0") != nics.end()) {
            return "smf-gw:plc:" + srv_id;
        } else if (std::find(std::begin(nics), std::end(nics), "eth0") != nics.end()) {
            return "smf-gw:eth:" + srv_id;
        } else if (std::find(std::begin(nics), std::end(nics), "ens33") != nics.end()) {
            return "smf-gw:virt:" + srv_id; //  linux on VMware
        } else if (std::find(std::begin(nics), std::end(nics), "Ethernet") != nics.end()) {
            return "smf-gw:win:" + srv_id; //  Windows
        }

        return "smf-gw:virtual";
    }

} // namespace smf
