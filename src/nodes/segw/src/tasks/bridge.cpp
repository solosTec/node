/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <tasks/CP210x.h>
#include <tasks/bridge.h>
#include <tasks/broker.h>
#include <tasks/filter.h>
#include <tasks/gpio.h>
#include <tasks/lmn.h>
#include <tasks/nms.h>
#include <tasks/persistence.h>
#include <tasks/rdr.h>

#include <config/cfg_blocklist.h>
#include <config/cfg_broker.h>
#include <config/cfg_gpio.h>
#include <config/cfg_listener.h>
#include <config/cfg_nms.h>
#include <config/cfg_sml.h>
#include <config/cfg_vmeter.h>

#include <smf/obis/defs.h>

#include <cyng/db/storage.h>
#include <cyng/log/record.h>
#include <cyng/parse/buffer.h>
#include <cyng/sys/net.h>
#include <cyng/task/channel.h>

#ifdef _DEBUG_SEGW
#include <iostream>
#endif

#include <boost/uuid/nil_generator.hpp>

namespace smf {

    bridge::bridge(cyng::channel_weak wp, cyng::controller &ctl, cyng::logger logger, cyng::db::session db)
        : sigs_{
            std::bind(&bridge::start, this), // start bridge
            std::bind(&bridge::stop, this, std::placeholders::_1)}
        , channel_(wp)
        , ctl_(ctl)
        , logger_(logger)
        , db_(db)
        , storage_(db_)
        , cache_()
        , cfg_(logger, cache_)
        , fabric_(ctl)
        , router_(ctl, cfg_, logger)
        , sml_(ctl, cfg_, logger)
        , stash_(ctl.get_ctx()) {
        auto sp = channel_.lock();
        if (sp) {
            std::size_t slot{0};
            sp->set_channel_name("start", slot++);
            CYNG_LOG_TRACE(logger_, "task [" << sp->get_name() << "] created");
        }

        CYNG_LOG_INFO(logger_, "segw ready");
    }

    void bridge::stop(cyng::eod) {

        CYNG_LOG_WARNING(logger_, "--- segw gracefull shutdown ---");

        //
        //	RDR - redirector
        //	stop listening
        //
        CYNG_LOG_INFO(logger_, "stop redirectors");
        stop_redirectors();

        //
        //	stop LMN tasks
        stop_lmn_ports();

        //
        //	filter (wireless M-Bus)
        //
        CYNG_LOG_INFO(logger_, "stop: filter");
        stop_filter();

        //
        //	broker clients
        //
        CYNG_LOG_INFO(logger_, "stop: broker clients");
        stop_broker_clients();

        //
        //	IP-T client
        //	connect to IP-T server
        //
        CYNG_LOG_INFO(logger_, "stop: SML router");
        stop_ipt_bus();

        //
        //	virtual meter
        //
        CYNG_LOG_INFO(logger_, "stop: virtual meter");
        // stop_virtual_meter();

        //
        //	NMS server
        //
        CYNG_LOG_INFO(logger_, "stop: NMS server");
        stop_nms_server();

        //
        //	SML server
        //
        CYNG_LOG_INFO(logger_, "stop: SML server");
        sml_.stop();

        //
        //	GPIO
        //
        CYNG_LOG_INFO(logger_, "stop: GPIO");
        stop_gpio();

        //
        //	connect database to data cache
        //
        CYNG_LOG_INFO(logger_, "stop: persistent data");
        stop_cache_persistence();

        //
        //  release all remaining references
        //
        stash_.clear();
    }

    void bridge::start() {
        CYNG_LOG_INFO(logger_, "segw start");

        //
        //	initialize data cache
        //
        CYNG_LOG_INFO(logger_, "initialize: data cache");
        init_data_cache();

        //
        //	load configuration data from data base
        //
        CYNG_LOG_INFO(logger_, "initialize: configuration data");
        load_config_data();

        //
        //	connect database to data cache
        //
        CYNG_LOG_INFO(logger_, "initialize: persistent data");
        init_cache_persistence();

        //
        //	GPIO
        //
        CYNG_LOG_INFO(logger_, "initialize: GPIO");
        init_gpio();

        //
        //	SML server
        //
        CYNG_LOG_INFO(logger_, "initialize: SML server");
        init_sml_server();

        //
        //	NMS server
        //
        CYNG_LOG_INFO(logger_, "initialize: NMS server");
        init_nms_server();

        //
        //	virtual meter
        //
        CYNG_LOG_INFO(logger_, "initialize: virtual meter");
        init_virtual_meter();

        //
        //	IP-T client
        //	connect to IP-T server
        //
        CYNG_LOG_INFO(logger_, "initialize: SML router");
        init_ipt_bus();

        //
        //	broker clients
        //
        CYNG_LOG_INFO(logger_, "initialize: broker clients");
        init_broker_clients();

        //
        //	filter (wireless M-Bus)
        //
        CYNG_LOG_INFO(logger_, "initialize: filter");
        init_filter();

        //
        //	LMN - open serial ports
        //	"let the data come in"
        //
        CYNG_LOG_INFO(logger_, "initialize: LMN ports");
        init_lmn_ports();

        //
        //	RDR - redirector
        //	start listening
        //
        CYNG_LOG_INFO(logger_, "initialize: redirectors");
        init_redirectors();
    }

    void bridge::init_data_cache() {

        auto const data = get_store_meta_data();
        for (auto const &m : data) {

            CYNG_LOG_TRACE(logger_, "create table: " << m.get_name());
            cache_.create_table(m);
        }
    }

    void bridge::load_config_data() {

        //
        //	Preload cache with configuration data.
        //	Preload must be done before we connect to the cache. Otherwise
        //	we would receive our own changes.
        //
        load_configuration();
        // auto tpl = cfg_.get_obj("language-code");
        load_meter(); //  "TMeter"
        // load_data_collectors();
        // load_data_mirror();
        // load_iec_devices();
        // load_privileges();
    }

    void bridge::init_cache_persistence() {

        //
        //	connect database to cache
        //
        auto channel = ctl_.create_named_channel_with_ref<persistence>("persistence", ctl_, logger_, cfg_, storage_);
        BOOST_ASSERT(channel->is_open());
        stash_.lock(channel);
        channel->dispatch("power-return", cyng::make_tuple());

        //
        //  check some values
        //
        cache_.access(
            [&](cyng::table *cfg) {
                auto const nic = get_nic();
                auto const key = cyng::key_generator("nms/nic");
                if (!cfg->exist(key)) {
                    CYNG_LOG_INFO(logger_, "insert nms/nic: " << nic);
                    cfg->merge(
                        key,
                        cyng::data_generator(nic),
                        1u,         //	only needed for insert operations
                        cfg_.tag_); //	tag mybe not available yet
                }
                {
                    auto const r = get_ipv6_linklocal(nic);
                    auto const key_index = cyng::key_generator("nms/nic-index");
                    if (!cfg->exist(key_index)) {
                        CYNG_LOG_INFO(logger_, "insert nms/nic-index: " << r.second);
                        cfg->merge(
                            key,
                            cyng::data_generator(r.second),
                            1u,         //	only needed for insert operations
                            cfg_.tag_); //	tag mybe not available yet
                    }
                    auto const key_linklocal = cyng::key_generator("nms/nic-linklocal");
                    if (!cfg->exist(key_linklocal)) {
                        CYNG_LOG_INFO(logger_, "insert nms/nic-linklocal: " << r.first);
                        cfg->merge(
                            key,
                            cyng::data_generator(r.first),
                            1u,         //	only needed for insert operations
                            cfg_.tag_); //	tag mybe not available yet
                    }
                }
                {
                    auto const ipv4 = get_ipv4_address(nic);
                    auto const key = cyng::key_generator("nms/nic-ipv4");
                    if (!cfg->exist(key)) {
                        CYNG_LOG_INFO(logger_, "insert nms/nic-ipv4: " << ipv4);
                        cfg->merge(
                            key,
                            cyng::data_generator(ipv4),
                            1u,         //	only needed for insert operations
                            cfg_.tag_); //	tag mybe not available yet
                    }
                }
                {
                    auto const delay = std::chrono::seconds(12);
                    auto const key = cyng::key_generator("nms/delay");
                    if (!cfg->exist(key)) {
                        CYNG_LOG_INFO(logger_, "insert nms/delay: " << delay);
                        cfg->merge(
                            key,
                            cyng::data_generator(delay),
                            1u,         //	only needed for insert operations
                            cfg_.tag_); //	tag mybe not available yet
                    }
                }
            },
            cyng::access::write("cfg"));
    }

    void bridge::stop_cache_persistence() {

        auto scp = ctl_.get_registry().lookup("persistence");
        for (auto sp : scp) {
            CYNG_LOG_INFO(logger_, "[persistence] stop #" << sp->get_id());
            stash_.unlock(sp->get_id());
            sp->stop();
        }
    }

    void bridge::load_configuration() {

        cache_.access(
            [&](cyng::table *cfg) {
                cyng::db::storage s(db_);
                s.loop(get_table_cfg(), [&](cyng::record const &rec) -> bool {

#ifdef _DEBUG_SEGW
            // CYNG_LOG_DEBUG(logger_, rec.to_tuple());
#endif
                    auto const path = rec.value("path", "");
                    auto const type = rec.value<std::uint16_t>("type", 15u);
                    auto const val = rec.value("val", "");

                    try {

                        //
                        //	restore original data type from string
                        //
                        auto obj = cyng::restore(val, type);

#ifdef _DEBUG_SEGW
                        CYNG_LOG_DEBUG(logger_, "load - " << path << " = " << obj);
#endif
                        if (boost::algorithm::equals(path, "tag")) {
                            //	set system tag
                            cfg_.tag_ = cyng::value_cast(obj, boost::uuids::nil_uuid());
                        } else if (boost::algorithm::equals(path, cyng::to_str(OBIS_SERVER_ID))) {
                            //	init server ID in cache
                            cfg_.id_ = cyng::hex_to_buffer(val);
                        } else {

                            //
                            //	insert value
                            //
                            cfg->merge(
                                rec.key(),
                                cyng::data_generator(obj),
                                1u,         //	only needed for insert operations
                                cfg_.tag_); //	tag mybe not available yet
                        }
                    } catch (std::exception const &ex) {
                        CYNG_LOG_ERROR(logger_, "cannot load " << path << ": " << ex.what());
                    }

                    return true;
                });
            },
            cyng::access::write("cfg"));
    }
    void bridge::load_meter() {
        cache_.access(
            [&](cyng::table *tbl) {
                cyng::db::storage s(db_);
                s.loop(get_table_meter(), [&](cyng::record const &rec) -> bool {
                    //
                    //  ToDo: fill "meter" table
                    //
                    return true;
                });
            },
            cyng::access::write("meter"));
    }

    void bridge::init_lmn_ports() {

        init_lmn_port(lmn_type::WIRELESS);
        init_lmn_port(lmn_type::WIRED);
    }

    void bridge::init_lmn_port(lmn_type type) {

        cfg_lmn cfg(cfg_, type);
        auto const port = cfg.get_port();

        if (cfg.is_enabled()) {

            CYNG_LOG_INFO(logger_, "init LMN [" << port << "]");
            auto channel = ctl_.create_named_channel_with_ref<lmn>(port, ctl_, logger_, cfg_, type);
            BOOST_ASSERT(channel->is_open());
            stash_.lock(channel);

            cfg_blocklist blocklist(cfg_, type);

            if (boost::algorithm::equals(cfg.get_hci(), "CP210x")) {

                BOOST_ASSERT(type == lmn_type::WIRELESS);

                //
                //	start CP210x parser
                //
                auto hci = ctl_.create_named_channel_with_ref<CP210x>("CP210x", ctl_, logger_);
                BOOST_ASSERT(hci->is_open());
                stash_.lock(hci);

                //
                //	init CP210x
                //
                channel->dispatch("reset-data-sinks");
                // channel->dispatch("add-data-sink", cyng::make_tuple("CP210x"));
                channel->dispatch("add-data-sink", hci->get_id());
                channel->dispatch("open");
                channel->dispatch("write", cyng::make_tuple(cfg.get_hci_init_seq()));

                //
                //	CP210x will forward incoming data to filter
                //
                hci->dispatch("reset-data-sinks", cyng::make_tuple(blocklist.get_task_name()));

            } else {

                //
                //	prepare broker to distribute data received
                //  from this serial port
                //
                CYNG_LOG_TRACE(logger_, "reset-data-sinks -> [" << port << "]");
                channel->dispatch("reset-data-sinks");

                //
                //  get a list of filter tasks
                //
                auto const task_name = blocklist.get_task_name();
                auto const targets = ctl_.get_registry().lookup(task_name);
                for (auto sp : targets) {
                    if (sp) {
                        CYNG_LOG_TRACE(logger_, "add-data-sink -> [" << port << "] " << sp->get_name() << "#" << sp->get_id());
                        channel->dispatch("add-data-sink", sp->get_id());
                    }
                }

                //
                //  start LMN GPIO statistics
                //
                channel->suspend(std::chrono::seconds(1), "update-statistics", cyng::make_tuple());

                //
                //	open serial port
                //
                CYNG_LOG_TRACE(logger_, "open -> [" << port << "]");
                channel->dispatch("open", cyng::make_tuple());
            }

        } else {
            CYNG_LOG_WARNING(logger_, "LMN [" << port << "] is not enabled");
        }
    }

    void bridge::stop_lmn_ports() {
        stop_lmn_port(lmn_type::WIRELESS);
        stop_lmn_port(lmn_type::WIRED);
    }

    void bridge::stop_lmn_port(lmn_type type) {
        cfg_lmn cfg(cfg_, type);
        auto const port = cfg.get_port();

        if (cfg.is_enabled()) {

            auto scp = ctl_.get_registry().lookup(port);
            for (auto sp : scp) {
                CYNG_LOG_INFO(logger_, "stop LMN [" << port << "] #" << sp->get_id());
                stash_.unlock(sp->get_id());
                sp->stop();
            }

            cfg_blocklist blocklist(cfg_, type);

            if (boost::algorithm::equals(cfg.get_hci(), "CP210x")) {

                BOOST_ASSERT(type == lmn_type::WIRELESS);

                //
                //	stop CP210x parser
                //
                auto scp = ctl_.get_registry().lookup("CP210x");
                for (auto sp : scp) {
                    CYNG_LOG_INFO(logger_, "[CP210x] stop #" << sp->get_id());
                    stash_.unlock(sp->get_id());
                    sp->stop();
                }
            }
        } else {
            CYNG_LOG_TRACE(logger_, "LMN [" << port << "] is not enabled");
        }
    }

    void bridge::init_ipt_bus() { router_.start(); }

    void bridge::stop_ipt_bus() { router_.stop(); }

    void bridge::init_broker_clients() {

        init_broker_clients(lmn_type::WIRELESS);
        init_broker_clients(lmn_type::WIRED);
    }

    void bridge::init_broker_clients(lmn_type type) {

        cfg_broker cfg(cfg_, type);
        auto const port = cfg.get_port();

        if (cfg.is_enabled()) {

            if (!cfg.is_lmn_enabled()) {
                CYNG_LOG_WARNING(
                    logger_,
                    "LMN for [" << port
                                << "] is not running. This broker "
                                   "will never receive any data");
            }

            //	All broker for this port have the same name.
            auto const name = cfg.get_task_name();
            auto const login = cfg.has_login();

            auto const size = cfg.update_count(); //  broker/N/count
            CYNG_LOG_INFO(logger_, size << " broker task \"" << name << "\" configured for [" << port << "]");
            auto const vec = cfg.get_all_targets();
            for (auto const &trg : vec) {

                //
                //	start broker with additional information like timeout and
                //  login
                //
                if (trg.is_connect_on_demand()) {

                    auto channel =
                        ctl_.create_named_channel_with_ref<broker_on_demand>(name, ctl_, logger_, cfg_, type, trg.get_index());
                    BOOST_ASSERT(channel->is_open());
                    stash_.lock(channel);
                } else {
                    auto channel = ctl_.create_named_channel_with_ref<broker>(name, ctl_, logger_, cfg_, type, trg.get_index());
                    BOOST_ASSERT(channel->is_open());
                    stash_.lock(channel);
                    channel->dispatch("check-status", trg.get_watchdog());
                }
            }
        } else {
            CYNG_LOG_WARNING(logger_, "broker for [" << port << "] is not enabled");
        }
    }

    void bridge::stop_broker_clients() {

        stop_broker_clients(lmn_type::WIRELESS);
        stop_broker_clients(lmn_type::WIRED);
    }
    void bridge::stop_broker_clients(lmn_type type) {

        cfg_broker cfg(cfg_, type);
        auto const port = cfg.get_port();

        if (cfg.is_enabled()) {

            //	All broker for this port have the same name.
            auto const name = cfg.get_task_name();
            auto scp = ctl_.get_registry().lookup(name);
            for (auto sp : scp) {
                auto const id = sp->get_id();
                CYNG_LOG_INFO(logger_, "[broker " << name << "] stop #" << id);
                stash_.unlock(id);
                sp->stop();
            }

        } else {
            CYNG_LOG_TRACE(logger_, "[broker] " << port << " not running");
        }
    }

    void bridge::init_filter() {
        //
        //	only wireless M-Bus data are filtered
        //
        init_filter(lmn_type::WIRELESS);
    }

    void bridge::init_filter(lmn_type type) {

        cfg_blocklist cfg(cfg_, type);

        //  "filter@" + port name
        auto const task_name = cfg.get_task_name();
        CYNG_LOG_INFO(logger_, "create filter [" << task_name << "]");

        auto channel = ctl_.create_named_channel_with_ref<filter>(task_name, ctl_, logger_, cfg_, type);
        BOOST_ASSERT(channel->is_open());
        stash_.lock(channel);

        //
        //	update statistics every second
        //
        channel->suspend(std::chrono::seconds(1), "update-statistics");

        //
        //	broker tasks are target channels
        //
        cfg_broker broker_cfg(cfg_, type);
        channel->dispatch("reset-data-sinks", cyng::make_tuple(broker_cfg.get_task_name()));
    }

    void bridge::stop_filter() {
        //
        //  only wireless M-Bus supports filter
        //
        stop_filter(lmn_type::WIRELESS);
    }

    void bridge::stop_filter(lmn_type type) {

        cfg_blocklist cfg(cfg_, type);

        auto scp = ctl_.get_registry().lookup(cfg.get_task_name());
        for (auto sp : scp) {
            if (sp) {
                CYNG_LOG_INFO(logger_, "[filter " << cfg.get_task_name() << "] stop #" << sp->get_id());
                stash_.unlock(sp->get_id());
                sp->stop();
            }
        }
    }

    void bridge::init_gpio() {

        cfg_gpio cfg(cfg_);
        auto p = cfg.get_path();
        if (cfg.is_enabled()) {

            //
            //	get all pins and start a task for each pin
            //
            auto const pins = cfg.get_pins();
            for (auto const pin : pins) {
                auto const sp = cfg.get_path(pin);
                auto const name = cfg_gpio::get_name(pin);
                CYNG_LOG_INFO(logger_, "init GPIO [" << name << "]");
                auto channel = ctl_.create_named_channel_with_ref<gpio>(name, logger_, sp);
                BOOST_ASSERT(channel->is_open());
                stash_.lock(channel);
#ifdef _DEBUG
                channel->dispatch("blinking", std::chrono::milliseconds(500));
#else
                if (pin == 53u) {
                    // turn on the Power LED and turn off all others
                    CYNG_LOG_TRACE(logger_, "Init GPIOs: turned on the PWR LED.");
                    channel->dispatch("turn", true);
                } else {
                    channel->dispatch("turn", false);
                }
#endif
            }
        } else {
            CYNG_LOG_WARNING(logger_, "GPIO [" << p << "] is not enabled");
        }
    }

    void bridge::stop_gpio() {
        cfg_gpio cfg(cfg_);
        auto p = cfg.get_path();
        if (cfg.is_enabled()) {

            //
            //	get all pins and start a task for each pin
            //
            auto const pins = cfg.get_pins();
            for (auto const pin : pins) {
                auto const name = cfg_gpio::get_name(pin);
                auto scp = ctl_.get_registry().lookup(name);
                for (auto sp : scp) {
                    if (sp) {
                        CYNG_LOG_INFO(logger_, "[gpio " << name << "] stop #" << sp->get_id());
                        stash_.unlock(sp->get_id());
                        sp->stop();
                    }
                }
            }
        } else {
            CYNG_LOG_TRACE(logger_, "[gpio] not running");
        }
    }

    void bridge::init_virtual_meter() {
        cfg_vmeter cfg(cfg_);
        auto srv = cfg.get_server();
        if (cfg.is_enabled()) {

            CYNG_LOG_INFO(logger_, "start virtual meter [" << srv << "]");
            //
            //	ToDo:
            //

        } else {
            CYNG_LOG_TRACE(logger_, "virtual meter [" << srv << "] is not enabled");
        }
    }

    void bridge::init_sml_server() {
        cfg_sml cfg(cfg_);
        if (cfg.is_enabled()) {
            auto const ep = cfg.get_ep();
            CYNG_LOG_INFO(logger_, "start SML server " << ep);
            sml_.start(ep);

        } else {
            CYNG_LOG_WARNING(logger_, "SML is not enabled");
        }
    }

    void bridge::init_nms_server() {
        cfg_nms cfg(cfg_);
        if (cfg.is_enabled()) {

            auto const nic = cfg.get_nic();

            //
            //  test if nic is avilable
            //
            if (test_nic(nic)) {
                CYNG_LOG_INFO(logger_, "designated nic for link-local communication is \"" << nic << "\"");
            } else {
                CYNG_LOG_ERROR(logger_, "designated nic for link-local communication \"" << nic << "\" is not available");
            }

            //  cyng::channel_weak wp, cyng::controller &ctl, cfg &, cyng::logger
            auto channel = ctl_.create_named_channel_with_ref<nms::server>("nms", ctl_, cfg_, logger_);
            BOOST_ASSERT(channel->is_open());
            stash_.lock(channel);

            //	get endpoint
            auto const ep = cfg.get_nic_linklocal_ep();
            auto const delay = cfg.get_delay();
            CYNG_LOG_INFO(logger_, "start NMS server " << ep << " (delayed by " << delay.count() << " seconds)");
            channel->suspend(delay, "start", ep);

        } else {
            CYNG_LOG_WARNING(logger_, "NMS is not enabled");
        }
    }

    void bridge::stop_nms_server() {
        cfg_nms cfg(cfg_);
        if (cfg.is_enabled()) {
            auto scp = ctl_.get_registry().lookup("nms");
            for (auto sp : scp) {
                if (sp) {
                    CYNG_LOG_INFO(logger_, "[NMS] stop #" << sp->get_id());
                    stash_.unlock(sp->get_id());
                    sp->stop();
                }
            }
        } else {
            CYNG_LOG_TRACE(logger_, "[NMS] not running");
        }
    }

    bool bridge::test_nic(std::string const &nic) {
        auto const nics = cyng::sys::get_nic_names();
        return std::find(std::begin(nics), std::end(nics), nic) != nics.end();
    }

    void bridge::init_redirectors() {

        init_redirector(lmn_type::WIRELESS);
        init_redirector(lmn_type::WIRED);
    }

    void bridge::init_redirector(lmn_type type) {

        //
        // https://docs.microsoft.com/en-us/windows-server/administration/windows-commands/netstat
        //  debug open listeners with (windows):
        //  > netstat -an  2 | find "6006"
        //

        cfg_listener cfg(cfg_, type);
        if (cfg.is_enabled()) {
            CYNG_LOG_INFO(logger_, "create external listener/rdr for port [" << cfg.get_port_name() << "] " << cfg.get_ipv4_ep());
            if (!cfg.is_lmn_enabled()) {
                CYNG_LOG_WARNING(
                    logger_,
                    "LMN for [" << cfg.get_port_name()
                                << "] is not running. This redirector "
                                   "will never transmit any data");
            }
            auto const name = cfg.get_task_name();

            auto channel = ctl_.create_named_channel_with_ref<rdr::server>(
                name, ctl_, cfg_, logger_, type, rdr::server::type::ipv4, cfg.get_ipv4_ep());
            stash_.lock(channel);
            cfg.set_IPv4_task_id(channel->get_id());

            auto const delay = cfg.get_delay();
            CYNG_LOG_INFO(
                logger_,
                "start external listener/rdr [" << name << "] in " << delay.count() << " seconds as task #" << channel->get_id());

            channel->suspend(delay, "start", cyng::make_tuple(delay));

            //
            //  check the IPv6 case only for linux envronments
            //  In future versions the NIC name is part of the configuration.
            //  So it's possible to use other NICs than br0
            //
            init_redirector_ipv6(cfg);

        } else {
            CYNG_LOG_WARNING(logger_, "external listener/rdr for port [" << cfg.get_port_name() << "] is not enabled");
            //
            //  no listener tasks are running
            //
            cfg.remove_IPv4_task_id();
            cfg.remove_IPv6_task_id();
        }
    }

    void bridge::init_redirector_ipv6(cfg_listener const &cfg) {

        //  use same link-local address as NMS and the ip port from the listener
        auto const ep = cfg.get_ipv6_ep();
        cfg_nms nms(cfg_);
        if (nms.get_port() == cfg.get_port()) {
            CYNG_LOG_ERROR(logger_, "IP port number " << cfg.get_port() << " is used for NMS and RDR");
        }

        if (!ep.address().is_unspecified()) {

            auto const name = cfg.get_task_name();
            CYNG_LOG_INFO(logger_, "create link-local listener/rdr [" << name << "] " << ep);

            auto channel = ctl_.create_named_channel_with_ref<rdr::server>(
                name, ctl_, cfg_, logger_, cfg.get_type(), rdr::server::type::ipv6, ep);
            stash_.lock(channel);
            cfg.set_IPv6_task_id(channel->get_id());

            CYNG_LOG_INFO(logger_, "link-local listener for port [" << cfg.get_port_name() << "] " << ep);

            //
            //  start listener
            //
            auto const delay = cfg.get_delay();
            CYNG_LOG_INFO(
                logger_,
                "start link-local listener/rdr [" << name << "] in " << delay.count() << " seconds as task #" << channel->get_id());

            channel->suspend(delay, "start", cyng::make_tuple(delay));

        } else {
            CYNG_LOG_WARNING(
                logger_, "link-local ep " << ep.address() << " for port [" << cfg.get_port_name() << "] is not present");
        }
    }

    void bridge::stop_redirectors() {
        stop_redirector(lmn_type::WIRELESS);
        stop_redirector(lmn_type::WIRED);
    }
    void bridge::stop_redirector(lmn_type type) {

        cfg_listener cfg(cfg_, type);
        if (cfg.is_enabled()) {
            CYNG_LOG_INFO(logger_, "stop listener for port [" << cfg.get_port_name() << "] " << cfg);

            auto const name = cfg.get_task_name();
            auto scp = ctl_.get_registry().lookup(name);
            for (auto sp : scp) {
                CYNG_LOG_INFO(logger_, "[" << name << "] stop #" << sp->get_id());
                stash_.unlock(sp->get_id());
                sp->stop();
            }

        } else {
            CYNG_LOG_TRACE(logger_, "[redirector] not running");
        }
    }

} // namespace smf
