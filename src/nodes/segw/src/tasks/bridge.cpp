/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <tasks/bridge.h>

#include <tasks/CP210x.h>
#include <tasks/broker.h>
#include <tasks/counter.h>
#include <tasks/emt.h>
#include <tasks/en13757.h>
#include <tasks/filter.h>
#include <tasks/gpio.h>
#include <tasks/lmn.h>
#include <tasks/nms.h>
#include <tasks/persistence.h>
#include <tasks/rdr.h>

#include <config/cfg_blocklist.h>
#include <config/cfg_broker.h>
#include <config/cfg_cache.h>
#include <config/cfg_gpio.h>
#include <config/cfg_http_post.h>
#include <config/cfg_listener.h>
#include <config/cfg_nms.h>
#include <config/cfg_sml.h>
#include <config/cfg_vmeter.h>
#include <smf/obis/db.h>
#include <smf/obis/defs.h>

#include <cyng/db/storage.h>
#include <cyng/log/record.h>
#include <cyng/obj/numeric_cast.hpp>
#include <cyng/parse/buffer.h>
#include <cyng/parse/string.h>
#include <cyng/sys/net.h>
#include <cyng/task/channel.h>

#ifdef _DEBUG_SEGW
#include <iostream>
#endif

#include <boost/uuid/nil_generator.hpp>

namespace smf {

    bridge::bridge(cyng::channel_weak wp, cyng::controller &ctl, cyng::logger logger, cyng::db::session db, std::vector<cyng::meta_store> const& md)
        : sigs_{
            std::bind(&bridge::start, this), // start bridge
            std::bind(&bridge::stop, this, std::placeholders::_1)}
        , channel_(wp)
        , ctl_(ctl)
        , logger_(logger)
        , db_(db)
        , storage_(db_)
        , cache_(std::begin(md), std::end(md))  // initialize data cache
        , cfg_(logger, cache_, load_configuration(logger, db_, cache_)) //  get tag and server id
        , fabric_(ctl)
        , router_(logger, ctl, cfg_, storage_)
        , sml_(logger, ctl, cfg_)
        , stash_(ctl.get_ctx()) {

        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"start"});
            CYNG_LOG_TRACE(logger_, "task [" << sp->get_name() << "] created");
        }

        CYNG_LOG_INFO(logger_, "segw ready");
    }

    void bridge::start() {
        CYNG_LOG_INFO(logger_, "segw start");

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
        //	start the operating hours counter
        //
        CYNG_LOG_INFO(logger_, "initialize: operating hours counter ");
        init_op_counter();

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
        CYNG_LOG_INFO(logger_, "initialize: filter (wireless M-Bus)");
        init_filter();

        //
        //	LMN - open serial ports or virtual meter
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
        //	stop the operating hours counter
        //
        CYNG_LOG_INFO(logger_, "stop: operating hours counter ");
        stop_op_counter();

        //
        //	connect database to data cache
        //
        CYNG_LOG_INFO(logger_, "stop: persistent data");
        stop_cache_persistence();

        //
        //  release all remaining references
        //
        stash_.stop();
        stash_.clear();

        //
        //  relaxed task termination
        //
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    void bridge::load_config_data() {

        //
        //	Preload cache with configuration data.
        //	Preload must be done before we connect to the cache. Otherwise
        //	we would receive our own changes.
        //
        load_meter(); //  "TMeterMBus", "TMeterIEC"
        //  ToDo: load_iec_devices();
        load_table(get_table_data_collector());
        load_table(get_table_data_mirror());
        load_table(get_table_push_ops());
        load_table(get_table_push_register());
        // ToDo: load_privileges();

        /**
         * load readout data from database
         */
        load_table(get_table_mbus_cache());
    }

    void bridge::init_cache_persistence() {

        //
        //	connect database to cache
        //
        auto channel = ctl_.create_named_channel_with_ref<persistence>("persistence", ctl_, logger_, cfg_, storage_);
        BOOST_ASSERT(channel->is_open());
        stash_.lock(channel);
        channel->dispatch("oplog.power.return");

        //
        // Check some values.
        // The main motivation for this code section is to make sure
        // that some configuration entries are available. Missing
        // entries are possible after an update or when configuration files
        // are invalid.
        //
        cache_.access(
            [&](cyng::table *cfg) {
                //
                //  nms/nic
                //
                auto const nic = get_nic();
                auto const key = cyng::key_generator("nms/nic");
                if (!cfg->exists(key)) {
                    CYNG_LOG_INFO(logger_, "insert nms/nic: " << nic);
                    cfg->insert(
                        key,
                        cyng::data_generator(nic),
                        1u,              //	only needed for insert operations
                        cfg_.get_tag()); //	tag mybe not available yet
                }
                {
                    //
                    //  nms/nic.index
                    //
                    auto const r = get_ipv6_linklocal(nic);
                    auto const key_index = cyng::key_generator("nms/nic.index");
                    if (!cfg->exists(key_index)) {
                        CYNG_LOG_INFO(logger_, "insert nms/nic.index: " << r.second);
                        cfg->insert(
                            key_index,
                            cyng::data_generator(r.second),
                            1u,              //	only needed for insert operations
                            cfg_.get_tag()); //	tag mybe not available yet
                    }
                    //
                    //  nms/nic-linklocal
                    //
                    auto const key_linklocal = cyng::key_generator("nms/nic.linklocal");
                    if (!cfg->exists(key_linklocal)) {
                        CYNG_LOG_INFO(logger_, "insert nms/nic.linklocal: " << r.first);
                        cfg->insert(
                            key_linklocal,
                            cyng::data_generator(r.first),
                            1u,              //	only needed for insert operations
                            cfg_.get_tag()); //	tag mybe not available yet
                    }
                }
                {
                    //
                    //  nms/nic.ipv4
                    //
                    auto const ipv4 = get_ipv4_address(nic);
                    auto const key = cyng::key_generator("nms/nic.ipv4");
                    if (!cfg->exists(key)) {
                        CYNG_LOG_INFO(logger_, "insert nms/nic.ipv4: " << ipv4);
                        cfg->insert(
                            key,
                            cyng::data_generator(ipv4),
                            1u,              //	only needed for insert operations
                            cfg_.get_tag()); //	tag mybe not available yet
                    }
                }
                {
                    //
                    //  nms/delay
                    //
                    auto const delay = std::chrono::seconds(12);
                    auto const key = cyng::key_generator("nms/delay");
                    if (!cfg->exists(key)) {
                        CYNG_LOG_INFO(logger_, "insert nms/delay: " << delay << " seconds");
                        cfg->insert(
                            key,
                            cyng::data_generator(delay),
                            1u,              //	only needed for insert operations
                            cfg_.get_tag()); //	tag mybe not available yet
                    }
                }
                {
                    //
                    //  nms/mode
                    //
                    auto const key = cyng::key_generator("nms/mode");
                    if (!cfg->exists(key)) {
                        std::string const mode("production");
                        CYNG_LOG_INFO(logger_, "insert nms/mode: \"" << mode << "\"");
                        cfg->insert(
                            key,
                            cyng::data_generator(mode),
                            1u,              //	only needed for insert operations
                            cfg_.get_tag()); //	tag mybe not available yet
                    }
                }
                {
                    //
                    //  opcounter
                    // Fixme: Insert operation triggers a warning, since the entry is already
                    // an element in the database.
                    //
                    auto const key = cyng::key_generator("opcounter");
                    if (!cfg->exists(key)) {
                        CYNG_LOG_INFO(logger_, "initialize opcounter");
                        cfg->insert(
                            key,
                            cyng::data_generator(static_cast<std::uint32_t>(1u)),
                            1u,              //	only needed for insert operations
                            cfg_.get_tag()); //	tag mybe not available yet
                    }
                }
            },
            cyng::access::write("cfg"));
    }

    void bridge::stop_cache_persistence() {

        CYNG_LOG_INFO(logger_, "stop task persistence");
        stash_.stop("persistence");
    }

    void bridge::init_op_counter() {
        CYNG_LOG_INFO(logger_, "init operation counter");
        auto channel = ctl_.create_named_channel_with_ref<counter>("counter", logger_, cfg_);
        BOOST_ASSERT(channel->is_open());
        channel->dispatch("inc");
        stash_.lock(channel);
    }

    void bridge::stop_op_counter() {
        CYNG_LOG_INFO(logger_, "stop task operation counter");
        // ctl_.get_registry().dispatch("counter", "save");
        stash_.stop("counter");
    }

    std::tuple<boost::uuids::uuid, cyng::buffer_t, std::uint32_t>
    load_configuration(cyng::logger logger, cyng::db::session db, cyng::store &cache) {

        //
        //  a list of keys that are outdated
        //
        std::set<std::string> keys_to_remove;

        boost::uuids::uuid tag = boost::uuids::nil_uuid();
        cyng::buffer_t id = cyng::make_buffer({0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
        std::uint32_t opcounter{0};

        cache.access(
            [&](cyng::table *cfg) {
                cyng::db::storage s(db);
                s.loop(get_table_cfg(), [&](cyng::record const &rec) -> bool {

#ifdef _DEBUG_SEGW
                    CYNG_LOG_DEBUG(logger, rec.to_tuple());
#endif
                    auto const path = sanitize_key(rec.value("path", ""));
                    auto const key = sanitize_key(path);
                    auto const type = rec.value<std::uint16_t>("type", 15u);
                    auto const val = rec.value("value", "");

                    if (!boost::algorithm::equals(path, key)) {
                        keys_to_remove.emplace(path);
                    }

                    try {

                        //
                        //	restore original data type from string
                        //
                        auto obj = cyng::restore(val, type);

#ifdef _DEBUG_SEGW
                    // CYNG_LOG_DEBUG(logger, "load - " << key << " = " << obj);
#endif
                        if (boost::algorithm::equals(key, "tag") && (obj.tag() == cyng::TC_STRING)) {
                            //	set system tag
                            auto const stag = cyng::value_cast(obj, "");
                            BOOST_ASSERT_MSG(stag.size() == 36, "invalid tag string");
                            tag = cyng::to_uuid(stag, boost::uuids::nil_uuid());
                            CYNG_LOG_INFO(logger, "source tag: " << tag);
                            BOOST_ASSERT_MSG(!tag.is_nil(), "invalid tag value");
                        } else if (boost::algorithm::equals(key, cyng::to_string(OBIS_SERVER_ID))) {
                            //	init server ID in cache
                            id = cyng::hex_to_buffer(val);
                            CYNG_LOG_INFO(logger, "server id: " << cyng::to_string(id));
                        } else if (boost::algorithm::equals(key, "nms/nic.index")) {
                            //  enforce u32
                            auto const nicidx = cyng::numeric_cast<std::uint32_t>(obj, 0u);
                            auto const index = validate_nic_index(nicidx);
                            if (nicidx != index) {
                                CYNG_LOG_ERROR(logger, "[" << nicidx << "] is an invalid nic index - use [" << index << "]");
                                cfg->insert(
                                    cyng::key_generator(key),
                                    cyng::data_generator(index),
                                    1,
                                    tag); //	tag maybe not initialized yet

                                //
                                //  list possible values
                                //
                                auto const cfg_v6 = cyng::sys::get_ipv6_configuration();
                                for (auto const &cfg : cfg_v6) {
                                    CYNG_LOG_TRACE(logger, "[" << cfg.index_ << "]: " << cfg.device_);
                                }
                            }
                        } else if (boost::algorithm::equals(key, "opcounter")) {

                            opcounter = cyng::numeric_cast<std::uint32_t>(obj, 0u);
                            CYNG_LOG_INFO(logger, "initial operation timer: " << opcounter << " seconds");
                            cfg->merge(
                                cyng::key_generator(key),
                                cyng::data_generator(opcounter),
                                1u,   //	only needed for insert operations
                                tag); //	tag maybe not initialized yet

                        } else {

                            //
                            //	insert value
                            //
                            if (!cfg->merge(
                                    cyng::key_generator(key),
                                    // rec.key(),
                                    cyng::data_generator(obj),
                                    1u,     //	only needed for insert operations
                                    tag)) { //	tag maybe not initialized yet

                                CYNG_LOG_ERROR(logger, "cannot merge config key " << key << ": " << val << '#' << type);
                            }
                        }
                    } catch (std::exception const &ex) {
                        CYNG_LOG_ERROR(logger, "cannot load " << key << ": " << ex.what());
                    }

                    return true;
                });
            },
            cyng::access::write("cfg"));

        //
        //  remove outdated keys
        //
        for (auto const &key : keys_to_remove) {
            del_config_value(db, key);
        }

        return std::make_tuple(tag, id, opcounter);
    }

    void bridge::load_meter() {

        cache_.access(
            [&](cyng::table *tbl) {
                cyng::db::storage s(db_);
                s.loop(get_table_meter_mbus(), [&](cyng::record const &rec) -> bool {
                    //
                    //  set visible = false
                    //
                    cyng::buffer_t id;
                    id = rec.value("serverID", id);
                    if (!tbl->insert(
                            rec.key(),
                            cyng::data_generator(
                                rec.at("lastSeen"),
                                rec.at("class"),
                                false, //  visible = false
                                rec.at("status"),
                                rec.at("mask"),
                                rec.at("pubKey"),
                                rec.at("aes"),
                                rec.at("secondary")),
                            rec.get_generation(),
                            cfg_.get_tag())) {
                        CYNG_LOG_ERROR(
                            logger_, "load table " << srv_id_to_str(id) << " into table " << tbl->meta().get_name() << " failed");
                    } else {
                        CYNG_LOG_INFO(logger_, "load table " << srv_id_to_str(id) << " into table " << tbl->meta().get_name());
                    }

                    return true;
                });
            },
            cyng::access::write("meterMBus"));
    }

    void bridge::load_table(cyng::meta_sql const &ms) {
        //
        //	get in-memory meta data
        //
        auto const m = cyng::to_mem(ms);
        CYNG_LOG_INFO(logger_, "[bridge] load table [" << ms.get_name() << "/" << m.get_name() << "]");

        cache_.access(
            [&](cyng::table *tbl) {
                cyng::db::storage s(db_);
                s.loop(ms, [&](cyng::record const &rec) -> bool {
                    CYNG_LOG_TRACE(logger_, "[storage] load " << rec.to_string());
                    if (!tbl->insert(rec.key(), rec.data(), rec.get_generation(), cfg_.get_tag())) {
                        CYNG_LOG_WARNING(
                            logger_,
                            "[bridge] load table [" << ms.get_name() << "/" << m.get_name() << "]: " << rec.to_string()
                                                    << " failed");
                    }

                    return true;
                });
            },
            cyng::access::write(m.get_name()));
    }

    void bridge::init_lmn_ports() {
        if (init_lmn_port(lmn_type::WIRELESS)) {
            cfg_.update_status_word(sml::status_bit::WIRELESS_MBUS_IF_AVAILABLE, true);
        }
        init_lmn_port(lmn_type::WIRED);
    }

    bool bridge::init_lmn_port(lmn_type type) {
        cfg_lmn cfg(cfg_, type);
        auto const port = cfg.get_port();

        if (cfg.is_enabled()) {

            CYNG_LOG_INFO(logger_, "init LMN [" << port << "]");

            cfg_vmeter vmeter(cfg_, type);
            if (vmeter.is_enabled()) {
                auto srv = vmeter.get_server();
                CYNG_LOG_INFO(logger_, "start virtual meter [" << srv << "]");
                //
                //  ToDo: substitute LMN task with the virtual meter task
                //
            }
            auto channel = ctl_.create_named_channel_with_ref<lmn>(port, ctl_, logger_, cfg_, type);
            BOOST_ASSERT(channel->is_open());
            stash_.lock(channel);

            CYNG_LOG_TRACE(logger_, "reset-data-sinks -> [" << port << "]");
            channel->dispatch("reset-data-sinks");

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
                //	init LMN
                //
                channel->dispatch("add-data-sink", hci->get_id()); //  this is the only dataa sink
                channel->dispatch("open");
                channel->dispatch("write", cyng::make_tuple(cfg.get_hci_init_seq()));

                //
                //	CP210x will forward incoming data to filter
                //
                hci->dispatch("add-data-sink", blocklist.get_task_name());

            } else {

                //
                //	prepare broker to distribute data received
                //  from this serial port
                //

                //
                //  get a list of filter tasks
                //
                auto const task_name = blocklist.get_task_name();
                ctl_.get_registry().lookup(task_name, [this, channel, port](std::vector<cyng::channel_ptr> targets) {
                    for (auto sp : targets) {
                        if (sp) {
                            CYNG_LOG_TRACE(logger_, "add-data-sink -> [" << port << "] " << sp->get_name() << "#" << sp->get_id());
                            channel->dispatch("add-data-sink", sp->get_id());
                        }
                    }
                });

                //
                //  start LMN GPIO statistics
                //
                channel->suspend(std::chrono::seconds(1), "update-statistics");

                //
                //	open serial port
                //
                CYNG_LOG_TRACE(logger_, "open -> [" << port << "]");
                channel->dispatch("open", cyng::make_tuple());
            }

            return true;
        }
        CYNG_LOG_WARNING(logger_, "LMN [" << port << "] is not enabled");
        return false;
    }

    void bridge::stop_lmn_ports() {
        stop_lmn_port(lmn_type::WIRELESS);
        cfg_.update_status_word(sml::status_bit::WIRELESS_MBUS_IF_AVAILABLE, false);

        stop_lmn_port(lmn_type::WIRED);
    }

    void bridge::stop_lmn_port(lmn_type type) {
        cfg_lmn cfg(cfg_, type);
        auto const port = cfg.get_port();

        if (cfg.is_enabled()) {

            stash_.stop(port); //  unlock and stop

            cfg_blocklist blocklist(cfg_, type);

            if (boost::algorithm::equals(cfg.get_hci(), "CP210x")) {

                BOOST_ASSERT(type == lmn_type::WIRELESS);

                //
                //	stop CP210x parser
                //
                stash_.stop("CP210x"); //  unlock and stop
            }
        } else {
            CYNG_LOG_TRACE(logger_, "LMN [" << port << "] is not enabled");
        }
    }

    void bridge::init_ipt_bus() {
        if (router_.start()) {
            //
            //	IP-T push
            //
            CYNG_LOG_INFO(logger_, "initialize: IP-T");
        }
    }

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
                    channel->dispatch("start"); //  init state_holder_
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
            stash_.stop(name); //  unlock and stop

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

        if (type == lmn_type::WIRELESS) {

            //
            //  start filter for wireless M-Bus data (EN-13757)
            //
            //  "filter@" + port name
            auto const task_name = cfg.get_task_name();
            CYNG_LOG_INFO(logger_, "create filter task [" << task_name << "]");

            auto channel_filter = ctl_.create_named_channel_with_ref<filter>(task_name, ctl_, logger_, cfg_, type);
            BOOST_ASSERT(channel_filter->is_open());
            stash_.lock(channel_filter);

            //
            //	update statistics every second
            //
            channel_filter->suspend(std::chrono::seconds(1), "update-statistics");

            //
            //  start wireless M-Bus processor (protocol EN-13757)
            //
            auto channel_en13757 = ctl_.create_named_channel_with_ref<en13757>("EN-13757", ctl_, logger_, db_, cfg_);
            stash_.lock(channel_en13757);
            auto const en13757_task_name = channel_en13757->get_name();
            BOOST_ASSERT_MSG(en13757_task_name == "EN-13757", "invalid task name");

            //
            //  start wireless M-Bus reader to generate HTTP POST messages
            //
            cfg_http_post http_post_cfg(cfg_, type);
            auto const emt_task_name = http_post_cfg.get_emt_task_name();
            CYNG_LOG_INFO(logger_, "create emt task [" << emt_task_name << "]");
            auto channel_emt = ctl_.create_named_channel_with_ref<emt>(emt_task_name, ctl_, logger_, db_, cfg_);
            stash_.lock(channel_emt);
            if (!http_post_cfg.is_enabled()) {
                CYNG_LOG_WARNING(logger_, "task [" << emt_task_name << "] is diabled");
            }

            //
            //  clear listeners
            //
            channel_filter->dispatch("reset-data-sinks");

            //
            //	broker tasks are target channels
            //  examples:
            //  broker@COM3
            //  broker@/dev/ttyAPP0
            //
            cfg_broker broker_cfg(cfg_, type);
            auto const broker_task_name = broker_cfg.get_task_name();
            channel_filter->dispatch("add-data-sink", broker_task_name);
            channel_filter->dispatch("add-data-sink", en13757_task_name); //  load profile
            channel_filter->dispatch("add-data-sink", emt_task_name);     //  HTTP POST

            //  send data periodically
            cfg_cache cache_cfg(cfg_, type);
            CYNG_LOG_TRACE(logger_, "[" << en13757_task_name << "] push cycle " << cache_cfg.get_interval());
            channel_en13757->suspend(cache_cfg.get_interval(), "push");

            CYNG_LOG_TRACE(logger_, "[" << emt_task_name << "] push cycle " << http_post_cfg.get_interval());
            channel_emt->suspend(http_post_cfg.get_interval(), "push");

        } else {
            CYNG_LOG_ERROR(logger_, "[filter] LMN type " << get_name(type) << " doesn't support filters");
        }
    }

    void bridge::stop_filter() {
        //
        //  only wireless M-Bus supports filter
        //
        stop_filter(lmn_type::WIRELESS);
        stash_.stop("EN-13757"); //  unlock and stop
    }

    void bridge::stop_filter(lmn_type type) {
        cfg_blocklist cfg(cfg_, type);

        auto const name = cfg.get_task_name();
        CYNG_LOG_INFO(logger_, "stop filter task [" << name << "]");
        stash_.stop(name); //  unlock and stop
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
                CYNG_LOG_INFO(logger_, "stop gpio task " << name);
                stash_.stop(name); //  unlock and stop
            }
        } else {
            CYNG_LOG_TRACE(logger_, "[gpio] not running");
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
            auto const mode = cfg.get_mode_name();
            CYNG_LOG_INFO(logger_, "[NMS] is in \"" << mode << "\" mode");

            //  cyng::channel_weak wp, cyng::controller &ctl, cfg &, cyng::logger
            auto channel = ctl_.create_named_channel_with_ref<nms::server>("nms", ctl_, cfg_, logger_);
            BOOST_ASSERT(channel->is_open());
            stash_.lock(channel);

            //	get endpoint: "nms/address" and "nms/port"
            auto const ep = cfg.get_ep();
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
            stash_.stop("nms"); //  unlock and stop
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

            auto channel =
                ctl_.create_named_channel_with_ref<rdr::server>(name, ctl_, cfg_, logger_, type, rdr::server::type::ipv4);
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
        auto const ep = cfg.get_link_local_ep();
        cfg_nms nms(cfg_);
        if (nms.get_port() == cfg.get_port()) {
            CYNG_LOG_ERROR(logger_, "IP port number " << cfg.get_port() << " is used for NMS and RDR");
        }

        if (!ep.address().is_unspecified()) {

            auto const name = cfg.get_task_name();
            CYNG_LOG_INFO(logger_, "create link-local listener/rdr [" << name << "] " << ep);

            auto channel =
                ctl_.create_named_channel_with_ref<rdr::server>(name, ctl_, cfg_, logger_, cfg.get_type(), rdr::server::type::ipv6);
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
            stash_.stop(name); //  unlock and stop

        } else {
            CYNG_LOG_TRACE(logger_, "[redirector] not running");
        }
    }

    std::uint32_t validate_nic_index(std::uint32_t index) {
        auto const cfg_v6 = cyng::sys::get_ipv6_configuration();
        auto const pos = std::find_if(
            std::begin(cfg_v6), std::end(cfg_v6), [index](cyng::sys::ipv_cfg const &cfg) { return cfg.index_ == index; });

        if (pos == cfg_v6.end()) {
            auto const nic = get_nic();
            auto const r = get_ipv6_linklocal(nic);

            //
            //  return an existing index
            //
            return r.second;
        }
        return index;
    }

} // namespace smf
