/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <db.h>
#include <smf.h>

#include <cyng/io/ostream.h>
#include <cyng/log/record.h>
#include <cyng/obj/container_factory.hpp>
#include <cyng/obj/util.hpp>
#include <cyng/parse/buffer.h>
#include <cyng/parse/string.h>
#include <cyng/sys/process.h>

#include <boost/uuid/nil_generator.hpp>

namespace smf {

    db::db(cyng::store &cache, cyng::logger logger, boost::uuids::uuid tag)
        : cache_(cache)
        , logger_(logger)
        , cfg_(cache, tag)
        , store_map_()
        , uuid_gen_()
        , source_(1)         //	ip-t source id
        , channel_target_(1) //	ip-t channel id
        , channel_pty_(1)    //	ip-t channel id
    {}

    void db::init(cyng::param_map_t const &session_cfg, std::string const &country_code, std::string const &lang_code) {

        //
        //	initialize cache
        //
        auto const vec = get_store_meta_data();
        for (auto const m : vec) {
            CYNG_LOG_INFO(logger_, "init table [" << m.get_name() << "]");
            store_map_.emplace(m.get_name(), m);
            cache_.create_table(m);
        }

        BOOST_ASSERT(vec.size() == cache_.size());

        //
        //	set start values
        //
        set_start_values(session_cfg, country_code, lang_code);

        //
        //	create auto tables
        //
        init_sys_msg();
        init_LoRa_uplink();
        init_iec_uplink();
        init_wmbus_uplink();

#ifdef _DEBUG_MAIN
        // init_demo_data();
#endif
    }

    void db::set_start_values(cyng::param_map_t const &session_cfg, std::string const &country_code, std::string const &lang_code) {

        //  this is table "config"
        cfg_.set_value("startup", std::chrono::system_clock::now());

        cfg_.set_value("smf-version", SMF_VERSION_NAME);
        cfg_.set_value("boost-version", SMF_BOOST_VERSION);
        cfg_.set_value("ssl-version", SMF_OPEN_SSL_VERSION);
        cfg_.set_value("compiler-version", SMF_COMPILER_VERSION);

        cfg_.set_value("country.code", country_code);
        cfg_.set_value("language.code", lang_code);

        //
        //	load session configuration from config file
        //
        for (auto const &param : session_cfg) {
            CYNG_LOG_TRACE(logger_, "session configuration " << param.first << ": " << param.second);
            cfg_.set_value(param.first, param.second);
        }

        //
        //	main node as cluster member
        //
        insert_cluster_member(
            cfg_.get_tag(),
            boost::uuids::nil_uuid(), //  no VM
            "main",
            cyng::version(SMF_VERSION_MAJOR, SMF_VERSION_MINOR),
            boost::asio::ip::tcp::endpoint(),
            cyng::sys::get_process_id(),
            false);
    }

    bool db::insert_cluster_member(
        boost::uuids::uuid tag,
        boost::uuids::uuid peer,
        std::string class_name,
        cyng::version v,
        boost::asio::ip::tcp::endpoint ep,
        cyng::pid pid,
        bool cfg_mgr) {

        return cache_.insert(
            "cluster",
            cyng::key_generator(tag),
            cyng::data_generator(
                peer,
                class_name,
                std::chrono::system_clock::now(),
                v,
                static_cast<std::uint64_t>(0),
                std::chrono::microseconds(0),
                ep,
                pid,
                cfg_mgr),
            1u, //	only needed for insert operations
            cfg_.get_tag());
    }

    bool db::remove_cluster_member(boost::uuids::uuid tag) {
        return cache_.erase("cluster", cyng::key_generator(tag), cfg_.get_tag());
    }

    bool db::insert_pty(
        boost::uuids::uuid tag,  //	device tag
        boost::uuids::uuid peer, //	local vm-tag
        boost::uuids::uuid rtag, //	remote vm-tag (required for forward ops)
        std::string const &name,
        std::string const &pwd,
        boost::asio::ip::tcp::endpoint ep,
        std::string const &data_layer) {

        CYNG_LOG_TRACE(
            logger_, "[db] insert session " << name << ", tag: " << tag << ", peer: " << peer << ", remote tag: " << rtag);

        //
        //  update session table
        //
        return cache_.insert(
            "session",
            cyng::key_generator(tag),
            cyng::data_generator(
                peer,
                name,
                ep,
                rtag,
                source_++,                        //	source
                std::chrono::system_clock::now(), //	login time
                boost::uuids::uuid(),             //	rTag
                "ipt",                            //	player
                data_layer,
                static_cast<std::uint64_t>(0),
                static_cast<std::uint64_t>(0),
                static_cast<std::uint64_t>(0)),
            1u, //	only needed for insert operations
            cfg_.get_tag());
    }

    std::tuple<cyng::key_t, bool, std::string> db::remove_pty(boost::uuids::uuid tag, boost::uuids::uuid dev) {

        bool r = false;
        cyng::key_t key_remote;
        std::string name;

        cache_.access(
            [&](cyng::table *tbl_pty, cyng::table *tbl_target, cyng::table *tbl_channel, cyng::table *tbl_conn) {
                //
                //  get session
                //	clean up open connections
                //
                auto const key = cyng::key_generator(dev);
                auto const rec_pty = tbl_pty->lookup(key);
                // auto const source = rec_pty.value("source", 0); //  ip-t source id (unique)
                if (rec_pty.empty()) {
                    //
                    //	most likely reason: login failed
                    //
                    CYNG_LOG_WARNING(logger_, "[db] remove pty " << dev << " not found");
                } else {

                    //
                    //  remote session key, if session has an open connection
                    //
                    auto const ctag = rec_pty.value("cTag", boost::uuids::nil_uuid());
                    name = rec_pty.value("name", "-"); //  device name
                    if (!ctag.is_nil()) {

                        //
                        //  build the key for table "connection"
                        //  and remove the record
                        //
                        auto const key_connection = cyng::key_generator(cyng::merge(ctag, dev));

                        auto const b = tbl_conn->erase(key_connection, cfg_.get_tag());
                        BOOST_ASSERT_MSG(b, "connection does not exists");

                        //
                        //  get remote session
                        //
                        key_remote = cyng::key_generator(ctag);

                        //
                        //  reset "cTag" attribute of remote sessions
                        //
                        tbl_pty->modify(key_remote, cyng::make_param("cTag", boost::uuids::nil_uuid()), cfg_.get_tag());
                    }
                }

                //
                //	remove from session table
                //
                // #ifdef _DEBUG
                // tbl_pty->loop([&](cyng::record &&rec, std::size_t) -> bool {
                //     // if (key == rec.key())
                //     CYNG_LOG_TRACE(
                //         logger_,
                //         "[db] " << (key == rec.key() ? "removed" : "running") << " session " << dev << ", tag: " << tag
                //                 << ", rec : " << rec.to_string());
                //     return true;
                // });
                // #endif
                r = tbl_pty->erase(key, cfg_.get_tag());
                if (!r) {
                    CYNG_LOG_WARNING(logger_, "[db] remove session " << dev << ", tag: " << tag << " failed");
                }

                //
                //	remove from target table
                //
                auto const target_count = tbl_target->erase_if(
                    [&](cyng::record &&rec) -> bool {
                        auto const device = rec.value("device", boost::uuids::nil_uuid());
                        return device == dev;
                    },
                    cfg_.get_tag());

                if (target_count != 0)
                    CYNG_LOG_INFO(logger_, "[db] remove session " << dev << ", with " << target_count << " targets");

                //
                //	remove from channel table
                //
                auto const cannel_count = tbl_channel->erase_if(
                    [&](cyng::record &&rec) -> bool {
                        auto const owner = rec.value("owner", boost::uuids::nil_uuid());
                        if (owner == dev) {

                            //
                            //  decrease listener count in table "target"
                            //
                            auto const target = rec.value<std::uint32_t>("target", 0);
                            auto const key_target = cyng::key_generator(target);
                            auto const rec_target = tbl_target->lookup(key_target);
                            if (!rec_target.empty()) {
                                tbl_target->compute<std::uint64_t>(
                                    key_target,
                                    "channels",
                                    [](std::uint64_t channel) -> std::uint64_t { return (channel == 0) ? 0 : channel - 1; },
                                    cfg_.get_tag());
                            } else {
                                //  target should exist since a session should not open a channel
                                //  to it's own target
                                CYNG_LOG_WARNING(logger_, "[db] target #" << target << " not found");
                            }
                            return true;
                        }
                        return false;
                    },
                    cfg_.get_tag());

                if (cannel_count != 0) {
                    CYNG_LOG_INFO(logger_, "[db] remove session " << dev << ", with " << cannel_count << " channels");
                }
            },
            cyng::access::write("session"),
            cyng::access::write("target"),
            cyng::access::write("channel"),
            cyng::access::write("connection"));

        return {key_remote, r, name};
    }

    pty db::get_access_params(cyng::key_t key) {
        //	 rTag and peer of the specified session
        boost::uuids::uuid rtag = boost::uuids::nil_uuid(), peer = boost::uuids::nil_uuid();

        cache_.access(
            [&](cyng::table const *tbl_pty) {
                auto const rec = tbl_pty->lookup(key);
                if (!rec.empty()) {
                    rtag = rec.value("rTag", rtag);
                    peer = rec.value("peer", peer);
                }
            },
            cyng::access::read("session"));

        return {rtag, peer};
    }

    std::size_t db::remove_pty_by_peer(boost::uuids::uuid peer, boost::uuids::uuid remote_peer) {
        cyng::key_list_t keys;

        cache_.access(
            [&](cyng::table *tbl_pty, cyng::table *tbl_target, cyng::table *tbl_cls) {
                tbl_pty->loop([&](cyng::record &&rec, std::size_t) -> bool {
                    // CYNG_LOG_DEBUG(logger_, "[db] remove peer [" << peer << "] " << rec.to_string());

                    auto const tag = rec.value("peer", peer);
                    if (tag == peer)
                        keys.insert(rec.key());
                    return true;
                });

                //
                //	remove ptys
                //
                CYNG_LOG_INFO(logger_, "[db] remove [" << keys.size() << "] ptys");
                for (auto const &key : keys) {
                    tbl_pty->erase(key, cfg_.get_tag());
                }

                //
                //	remove targets
                //
                keys.clear();
                tbl_target->loop([&](cyng::record &&rec, std::size_t) -> bool {
                    auto const tag = rec.value("peer", peer);
                    auto const name = rec.value("name", "");
                    if (tag == peer) {
                        CYNG_LOG_TRACE(logger_, "[db] remove target [" << name << "]");
                        keys.insert(rec.key());
                    }
                    return true;
                });

                //
                //	remove targets
                //
                for (auto const &key : keys) {
                    tbl_target->erase(key, cfg_.get_tag());
                }

                //
                //	update cluster table
                //
                tbl_cls->erase(cyng::key_generator(remote_peer), cfg_.get_tag());
            },
            cyng::access::write("session"),
            cyng::access::write("target"),
            cyng::access::write("cluster"));

        return keys.size();
    }

    std::size_t db::update_pty_counter(boost::uuids::uuid peer, boost::uuids::uuid remote_peer) {
        std::uint64_t count{0};
        cache_.access(
            [&](cyng::table const *tbl_pty, cyng::table *tbl_cls) {
                tbl_pty->loop([&](cyng::record &&rec, std::size_t) -> bool {
                    auto const tag = rec.value("peer", peer);
                    if (tag == peer)
                        count++;
                    return true;
                });

                CYNG_LOG_TRACE(logger_, "[db] session [" << peer << "] has " << count << " ptys");

                //
                //	update cluster table
                //
                tbl_cls->modify(cyng::key_generator(remote_peer), cyng::make_param("clients", count), cfg_.get_tag());
            },
            cyng::access::read("session"),
            cyng::access::write("cluster"));
        return count;
    }

    void db::update_pty_statistics(pty_event evt, boost::uuids::uuid dev, boost::uuids::uuid peer) {

        auto const key = cyng::key_generator(dev);
        auto const now = cyng::make_utc_date();

        cache_.access(
            [&](cyng::table *tbl_stat, cyng::table *tbl_hist) {
                // CYNG_LOG_TRACE(logger_, "[db] session [" << peer << "] has " << count << " ptys");
                switch (evt) {
                case EVT_LOGIN: {
                    auto const rec = tbl_stat->lookup(key);
                    if (rec.empty()) {
                        //
                        //  first entry
                        //
                        tbl_stat->insert(key, cyng::data_generator(now, now, 1u), 1u, cfg_.get_tag());
                    } else {
                        //
                        //  update "lastLogin" and "loginCounter"
                        //
                        auto const counter = rec.value("loginCounter", static_cast<std::uint32_t>(0u));
                        tbl_stat->modify(
                            key, cyng::param_map_factory("lastLogin", now)("loginCounter", counter + 1), cfg_.get_tag());
                    }
                    //
                    //  Only use this when it is known how the table will be deleted again.
                    //
                    // tbl_hist->insert(cyng::key_generator(dev, peer, now), cyng::data_generator("login"), 1u, cfg_.get_tag());
                } break;
                case EVT_LOGOUT: {
                    auto const rec = tbl_stat->lookup(key);
                    if (!rec.empty()) {
                        tbl_stat->modify(key, cyng::make_param("lastLogin", now), cfg_.get_tag());
                    } else {
                        //  error
                    }
                    //
                    //  Only use this when it is known how the table will be deleted again.
                    //
                    // tbl_hist->insert(cyng::key_generator(dev, peer, now), cyng::data_generator("logout"), 1u, cfg_.get_tag());
                } break;
                default: break;
                }
            },
            cyng::access::write("statistics"),
            cyng::access::write("history"));
    }

    std::pair<boost::uuids::uuid, bool> db::lookup_device(std::string const &name, std::string const &pwd) {
        boost::uuids::uuid tag = boost::uuids::nil_uuid();
        bool enabled = false;

        cache_.access(
            [&](cyng::table const *tbl) {
                tbl->loop([&](cyng::record &&rec, std::size_t) -> bool {
                    auto const n = rec.value("name", "");
                    auto const p = rec.value("pwd", "");
                    if (boost::algorithm::equals(name, n) && boost::algorithm::equals(pwd, p)) {

                        CYNG_LOG_INFO(logger_, "login [" << name << "] => [" << p << "]");

                        tag = rec.value("tag", tag);
                        enabled = rec.value("enabled", false);
                        return false;
                    }
                    return true;
                });
            },
            cyng::access::read("device"));

        return std::make_pair(tag, enabled);
    }

    bool db::insert_device(boost::uuids::uuid tag, std::string const &name, std::string const &pwd, bool enabled) {
        return cache_.insert(
            "device",
            cyng::key_generator(tag),
            cyng::data_generator(name, pwd, name, "auto", "id", "1.0", enabled, std::chrono::system_clock::now()),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
    }

    std::tuple<bool, bool, boost::uuids::uuid, boost::uuids::uuid, std::string, std::string>
    db::lookup_msisdn(std::string msisdn, boost::uuids::uuid dev) {
        //	(1) session already connected,
        //	(2) remote session online, enabled and not connected,
        //	(3) remote session vm-tag
        //  (4) remote session tag
        //  (5) caller name
        //  (6) callee name

        bool connected = true;
        bool online = false;
        std::string caller_name, callee_name;
        boost::uuids::uuid vm_tag = boost::uuids::nil_uuid();
        boost::uuids::uuid remote = boost::uuids::nil_uuid();
        cache_.access(
            [&](cyng::table const *tbl_dev, cyng::table const *tbl_session) {
                //
                //	test if session is already connected
                //	"cTag" is the session tag of the remote session
                //
                auto const caller = tbl_session->lookup(cyng::key_generator(dev));
                connected = !caller.value("cTag", boost::uuids::nil_uuid()).is_nil();
                caller_name = caller.value("name", "");

                //
                //	don't need more information if already connected
                //
                if (!connected) {

                    //
                    //	search device
                    //
                    cyng::key_t key;
                    bool enabled = false;
                    tbl_dev->loop([&](cyng::record &&rec, std::size_t) -> bool {
                        auto const n = rec.value("msisdn", "");
                        if (boost::algorithm::equals(msisdn, n)) {
                            key = rec.key();
                            enabled = rec.value("enabled", false);
                            callee_name = rec.value("name", "");
                            return false;
                        }
                        return true;
                    });

                    //
                    //	test if session is online
                    //
                    if (!key.empty() && enabled) {
                        auto const rec = tbl_session->lookup(key);
                        if (!rec.empty()) {
                            vm_tag = rec.value("peer", vm_tag);
                            remote = rec.value("rTag", remote);
                            online = rec.value("cTag", boost::uuids::nil_uuid()).is_nil();
                        }
                    }
                }
            },
            cyng::access::read("device"),
            cyng::access::read("session"));

        return {connected, online, vm_tag, remote, caller_name, callee_name};
    }

    void db::establish_connection(
        boost::uuids::uuid caller_tag,
        boost::uuids::uuid caller_vm,
        boost::uuids::uuid caller_dev, //  pk
        std::string caller_name,
        std::string callee_name,
        boost::uuids::uuid tag,    //	callee tag
        boost::uuids::uuid dev,    //	callee dev-tag - pk
        boost::uuids::uuid callee, //	callee vm-tag
        bool local) {
        cache_.access(
            [&](cyng::table *tbl_session, cyng::table *tbl_connection) {
                auto const key_caller = cyng::key_generator(caller_dev);
                auto const key_callee = cyng::key_generator(dev);

                auto const b1 = tbl_session->modify(key_caller, cyng::make_param("cTag", dev), cfg_.get_tag());
                auto const b2 = tbl_session->modify(key_callee, cyng::make_param("cTag", caller_dev), cfg_.get_tag());

            // BOOST_ASSERT_MSG(b1, "caller not found");
            // BOOST_ASSERT_MSG(b2, "callee not found");
#ifdef _DEBUG
                if (!b1) {
                    CYNG_LOG_WARNING(logger_, "[db] session (1) " << caller_dev << " not modified");
                    tbl_session->loop([&](cyng::record &&rec, std::size_t) -> bool {
                        CYNG_LOG_TRACE(
                            logger_,
                            "[db] search session " << caller_dev << ", tag: " << caller_tag << ", rec: " << rec.to_string());
                        return true;
                    });
                }
                if (!b2) {
                    CYNG_LOG_WARNING(logger_, "[db] session (2) " << dev << " not modified");
                    tbl_session->loop([&](cyng::record &&rec, std::size_t) -> bool {
                        CYNG_LOG_TRACE(logger_, "[db] search session " << dev << ", tag: " << tag << ", rec: " << rec.to_string());
                        return true;
                    });
                }
#else
                boost::ignore_unused(b1);
                boost::ignore_unused(b2);
#endif
                auto const rec_caller = tbl_session->lookup(key_caller);
                auto const rec_callee = tbl_session->lookup(key_callee);

                auto const layer_caller = rec_caller.value("pLayer", "");
                auto const layer_callee = rec_callee.value("pLayer", "");

                //
                //	both "dev-tags" are merged to one UUID that is independend from the ordering of the parameters.
                //
                tbl_connection->insert(
                    cyng::key_generator(cyng::merge(caller_dev, dev)),
                    cyng::data_generator(
                        caller_name,
                        callee_name,
                        std::chrono::system_clock::now(),
                        local,
                        layer_caller,
                        layer_callee,
                        static_cast<std::uint64_t>(0)),
                    1,
                    cfg_.get_tag());
            },
            cyng::access::write("session"),
            cyng::access::write("connection"));
    }

    std::tuple<boost::uuids::uuid, boost::uuids::uuid, boost::uuids::uuid, bool> db::clear_connection(boost::uuids::uuid dev) {
        boost::uuids::uuid rdev = boost::uuids::nil_uuid(), rpeer = boost::uuids::nil_uuid(), rtag = boost::uuids::nil_uuid();
        bool local = false;
        cache_.access(
            [&](cyng::table *tbl_session, cyng::table *tbl_connection) {
                auto const rec_dev = tbl_session->lookup(cyng::key_generator(dev));
                if (!rec_dev.empty()) {
                    rdev = rec_dev.value("cTag", dev); //  remote
                    auto const peer = rec_dev.value("peer", dev);
#ifdef _DEBUG_MAIN
                    auto const name = rec_dev.value("name", "");
                    auto const ep = rec_dev.value("ep", boost::asio::ip::tcp::endpoint());
                    auto const source = rec_dev.value("source", std::uint32_t(0));
                    auto const pLayer = rec_dev.value("pLayer", "");
                    auto const dLayer = rec_dev.value("dLayer", "");

                    CYNG_LOG_DEBUG(
                        logger_,
                        "dev record"
                            << "\ndev (key)\t: " << dev << "\npeer\t: " << peer << "\nname\t: " << name << "\nep\t: " << ep
                            << "\nr-vm\t: " << rtag << "\nsource\t: " << source << "\ncTag\t: " << rdev << "\npLayer\t: " << pLayer
                            << "\ndLayer\t: " << dLayer);
#endif
                    tbl_session->modify(rec_dev.key(), cyng::make_param("cTag", boost::uuids::nil_uuid()), cfg_.get_tag());

                    auto const rec_rdev = tbl_session->lookup(cyng::key_generator(rdev));
                    if (!rec_rdev.empty()) {
                        rtag = rec_rdev.value("rTag", dev);
                        rpeer = rec_rdev.value("peer", dev);
                        local = peer == rpeer;
#ifdef _DEBUG_MAIN
                        // auto const r_peer = rec_rdev.value("peer", dev);
                        auto const r_name = rec_rdev.value("name", "");
                        auto const r_ep = rec_rdev.value("ep", boost::asio::ip::tcp::endpoint());
                        auto const r_rTag = rec_rdev.value("rTag", dev);
                        auto const r_cTag = rec_rdev.value("cTag", dev);
                        auto const r_source = rec_rdev.value("source", std::uint32_t(0));
                        auto const r_pLayer = rec_rdev.value("pLayer", "");
                        auto const r_dLayer = rec_rdev.value("dLayer", "");

                        CYNG_LOG_DEBUG(
                            logger_,
                            "rdev record"
                                << "\ndev (key)\t: " << rdev << "\npeer\t: " << rpeer << "\nname\t: " << r_name
                                << "\nep\t: " << r_ep << "\nr-vm\t: " << r_rTag << "\nsource\t: " << r_source
                                << "\ncTag\t: " << r_cTag << "\npLayer\t: " << r_pLayer << "\ndLayer\t: " << r_dLayer);

                        //  definition of a connection
                        BOOST_ASSERT(dev == r_cTag);

#endif
                        tbl_session->modify(rec_rdev.key(), cyng::make_param("cTag", boost::uuids::nil_uuid()), cfg_.get_tag());

                        //
                        //  remove connection record
                        //
                        if (!tbl_connection->erase(cyng::key_generator(cyng::merge(dev, rdev)), cfg_.get_tag())) {
                            CYNG_LOG_ERROR(logger_, "connection clearing failed");
                        }
                    }
                }
            },
            cyng::access::write("session"),
            cyng::access::write("connection"));

        return {rdev, rtag, rpeer, local};
    }

    std::tuple<boost::uuids::uuid, boost::uuids::uuid, boost::uuids::uuid> db::get_remote(boost::uuids::uuid dev) {
        //
        //  This function loops over all sessions to find the matching remote session.
        //  This doesn't scale well for a high number of sessions with much open
        //  connections.
        //  ToDo: Store the key "tag" of the remote session instead the "rtag" or implement some kind
        //  of index in the in-memory table class.
        //
        boost::uuids::uuid rdev = boost::uuids::nil_uuid(), rpeer = boost::uuids::nil_uuid(), rtag = boost::uuids::nil_uuid();
        cache_.access(
            [&](cyng::table const *tbl_session) {
                auto const rec = tbl_session->lookup(cyng::key_generator(dev));
                if (!rec.empty()) {
                    rdev = rec.value("cTag", rdev); //  same as dev

                    auto const key_connected = cyng::key_generator(rdev);
                    auto const rec_connected = tbl_session->lookup(key_connected);
                    if (!rec_connected.empty()) {
                        rtag = rec_connected.value("rTag", rtag);
                        rpeer = rec_connected.value("peer", rpeer);
                    }
                }
            },
            cyng::access::read("session"));

        return {rdev, rtag, rpeer};
    }

    void db::init_sys_msg() {
        auto const max_msg = cfg_.get_value<std::size_t>("max.messages", 1000);
        auto const tag = get_cfg().get_tag();

        auto const ms = config::get_store_sys_msg();
        auto const start_key = cyng::key_generator(static_cast<std::uint64_t>(0));
        cache_.create_auto_table(ms, start_key, [this, max_msg, tag](cyng::table *tbl, cyng::key_t const &key) {
            BOOST_ASSERT(key.size() == 1);

            auto const n = cyng::value_cast<std::uint64_t>(key.at(0), 0);
            if (tbl->size() > max_msg) {

                //
                //	reduce table size
                //
                auto last = n - max_msg;
                while (tbl->erase(cyng::key_generator(last), tag)) {
                    --last;
                }
            }
            return cyng::key_generator(n + 1);
        });
    }

    void db::init_LoRa_uplink() {
        auto const max_msg = cfg_.get_value<std::int64_t>("max.LoRa.records", 500);
        auto const tag = get_cfg().get_tag();

        auto const ms = config::get_store_uplink_lora();
        auto const start_key = cyng::key_generator(static_cast<std::uint64_t>(0));
        cache_.create_auto_table(ms, start_key, [this, max_msg, tag](cyng::table *tbl, cyng::key_t const &key) {
            BOOST_ASSERT(key.size() == 1);

            auto const n = cyng::value_cast<std::uint64_t>(key.at(0), 0);
            if (tbl->size() > static_cast<std::size_t>(max_msg)) {

                //
                //	reduce table size
                //
                auto last = n - max_msg;
                while (tbl->erase(cyng::key_generator(last), tag)) {
                    --last;
                }
            }
            return cyng::key_generator(n + 1);
        });
    }

    void db::init_iec_uplink() {
        auto const max_msg = cfg_.get_value<std::int64_t>("max.IEC.records", 600);
        auto const tag = get_cfg().get_tag();

        auto const ms = config::get_store_uplink_iec();
        auto const start_key = cyng::key_generator(static_cast<std::uint64_t>(0));
        cache_.create_auto_table(ms, start_key, [this, max_msg, tag](cyng::table *tbl, cyng::key_t const &key) {
            BOOST_ASSERT(key.size() == 1);

            auto const n = cyng::value_cast<std::uint64_t>(key.at(0), 0);
            if (tbl->size() > static_cast<std::size_t>(max_msg)) {

                //
                //	reduce table size
                //
                auto last = n - max_msg;
                while (tbl->erase(cyng::key_generator(last), tag)) {
                    --last;
                }
            }
            return cyng::key_generator(n + 1);
        });
    }

    void db::init_wmbus_uplink() {
        auto const max_msg = cfg_.get_value<std::int64_t>("max.wMBus.records", 500);
        auto const tag = get_cfg().get_tag();

        auto const ms = config::get_store_uplink_wmbus();
        auto const start_key = cyng::key_generator(static_cast<std::uint64_t>(0));
        cache_.create_auto_table(ms, start_key, [this, max_msg, tag](cyng::table *tbl, cyng::key_t const &key) {
            BOOST_ASSERT(key.size() == 1);

            auto const n = cyng::value_cast<std::uint64_t>(key.at(0), 0);
            if (tbl->size() > static_cast<std::size_t>(max_msg)) {

                //
                //	reduce table size
                //
                auto last = n - max_msg;
                while (tbl->erase(cyng::key_generator(last), tag)) {
                    --last;
                }
            }
            return cyng::key_generator(n + 1);
        });
    }

    void db::init_demo_data() {
        //
        //	insert test device
        //
        auto const tag_01 = cyng::to_uuid("4808b333-3545-4d9c-a57a-ed19d7c0548b");
        auto b = cache_.insert(
            "device",
            cyng::key_generator(tag_01),
            cyng::data_generator("IEC", "pwd", "msisdn", "descr", "id", "vFirmware", true, std::chrono::system_clock::now()),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");

        auto const tag_02 = cyng::to_uuid("7afde964-b59c-4a04-b0a3-72dd63f4b6c0");
        b = cache_.insert(
            "device",
            cyng::key_generator(tag_02),
            cyng::data_generator("wM-Bus", "pwd", "msisdn", "descr", "id", "vFirmware", true, std::chrono::system_clock::now()),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");

        b = cache_.insert(
            "meterIEC",
            cyng::key_generator(tag_01),
            cyng::data_generator(
                "192.168.0.200",
                static_cast<std::uint16_t>(2000u),
                std::chrono::seconds(840),
                std::chrono::system_clock::time_point(std::chrono::hours(0))),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");

        b = cache_.insert(
            "meterwMBus",
            cyng::key_generator(tag_02),
            cyng::data_generator(
                boost::asio::ip::make_address("192.168.0.200"),
                static_cast<std::uint16_t>(6000u),
                cyng::make_aes_key<cyng::crypto::aes128_size>(cyng::hex_to_buffer("6E3272357538782F413F4428472B4B62")),
                std::chrono::system_clock::time_point(std::chrono::hours(0))),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");

        //
        // onee
        // MA0000000000000000000000003496219,RS485,10.132.28.150,6000,Elster,Elster AS 1440,IEC 62056,Lot Yakut,C1 House
        // 101,Yes,,,
        //
        auto const tag_03 = cyng::to_uuid("be3931f9-6266-44db-b2b6-bd3b94b7a563");
        b = cache_.insert(
            "device",
            cyng::key_generator(tag_03),
            cyng::data_generator(
                "MA0000000000000000000000003496219",
                "secret",
                "3496219",
                "Lot Yakut,C1 House 101",
                "",
                "",
                true,
                std::chrono::system_clock::now()),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");
        b = cache_.insert(
            "meterIEC",
            cyng::key_generator(tag_03),
            cyng::data_generator(
                "10.132.28.150",
                static_cast<std::uint16_t>(6000u),
                std::chrono::seconds(840),
                std::chrono::system_clock::time_point(std::chrono::hours(0))),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");
        //	meter
        b = cache_.insert(
            "meter",
            cyng::key_generator(tag_03),
            cyng::data_generator(
                "01-e61e-13090016-3c-07",         //	ident nummer (i.e. 1EMH0006441734, 01-e61e-13090016-3c-07)
                "16000913",                       //	[string] meter number (i.e. 16000913) 4 bytes
                "MA87687686876",                  // cyng::column("code", cyng::TC_STRING),		//	[string] metering code - changed at
                                                  // 2019-01-31
                "MAN",                            // cyng::column("maker", cyng::TC_STRING),		//	[string] manufacturer
                std::chrono::system_clock::now(), // cyng::column("tom", cyng::TC_TIME_POINT),	//	[ts] time of
                                                  // manufacture
                "11600000",           // cyng::column("vFirmware", cyng::TC_STRING),	//	[string] firmwareversion (i.e. 11600000)
                "parametrierversion", // cyng::column("vParam", cyng::TC_STRING),	//	[string] parametrierversion (i.e.
                                      // 16A098828.pse)
                "06441734",           // cyng::column("factoryNr", cyng::TC_STRING),	//	[string] fabrik nummer (i.e. 06441734)
                "NXT4-S20EW-6N00-4000-5020-E50/Q", // cyng::column("item", cyng::TC_STRING),		//	[string]
                                                   // ArtikeltypBezeichnung = "NXT4-S20EW-6N00-4000-5020-E50/Q"
                "Q3/Q1", // cyng::column("mClass", cyng::TC_STRING),	//	[string] Metrological Class: A, B, C, Q3/Q1, ...
                boost::uuids::nil_uuid(), // cyng::column("gw", cyng::TC_UUID),			//	optional gateway pk
                "IEC" // cyng::column("protocol", cyng::TC_STRING)	//	[string] data protocol (IEC, M-Bus, COSEM, ...)
                ),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");

        auto const tag_04 = cyng::to_uuid("41c97d7a-6e37-4c49-99cf-3c87a2a11dbc");
        b = cache_.insert(
            "device",
            cyng::key_generator(tag_04),
            cyng::data_generator(
                "MA0000000000000000000000003496225",
                "secret",
                "3496225",
                "C1 House 94",
                "",
                "",
                true,
                std::chrono::system_clock::now()),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");
        b = cache_.insert(
            "meterIEC",
            cyng::key_generator(tag_04),
            cyng::data_generator(
                "10.132.28.151",
                static_cast<std::uint16_t>(6000u),
                std::chrono::seconds(840),
                std::chrono::system_clock::time_point(std::chrono::hours(0))),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");

        auto const tag_05 = cyng::to_uuid("5b436e3f-adac-41d9-84fe-57b0c6207f7d");
        b = cache_.insert(
            "device",
            cyng::key_generator(tag_05),
            cyng::data_generator(
                "MA0000000000000000000000035074614",
                "secret",
                "3496230",
                "Yasmina,House A2 Meter Board",
                "",
                "",
                true,
                std::chrono::system_clock::now()),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");
        b = cache_.insert(
            "meter",
            cyng::key_generator(tag_05),
            cyng::data_generator(
                "35074614",                          //	ident nummer (i.e. 1EMH0006441734, 01-e61e-13090016-3c-07)
                "35074614",                          //	[string] meter number (i.e. 16000913) 4 bytes
                "MA0000000000000000000000035074614", // cyng::column("code", cyng::TC_STRING),		//	[string]
                                                     // metering code
                                                     // - changed at 2019-01-31
                "ELS",                               // cyng::column("maker", cyng::TC_STRING),		//	[string] manufacturer
                std::chrono::system_clock::now(),    // cyng::column("tom", cyng::TC_TIME_POINT),	//	[ts] time of
                                                     // manufacture
                "",        // cyng::column("vFirmware", cyng::TC_STRING),	//	[string] firmwareversion (i.e. 11600000)
                "",        // cyng::column("vParam", cyng::TC_STRING),	//	[string] parametrierversion (i.e. 16A098828.pse)
                "",        // cyng::column("factoryNr", cyng::TC_STRING),	//	[string] fabrik nummer (i.e. 06441734)
                "AS 1440", // cyng::column("item", cyng::TC_STRING),		//	[string] ArtikeltypBezeichnung =
                           // "NXT4-S20EW-6N00-4000-5020-E50/Q"
                "",        // cyng::column("mClass", cyng::TC_STRING),	//	[string] Metrological Class: A, B, C, Q3/Q1, ...
                boost::uuids::nil_uuid(), // cyng::column("gw", cyng::TC_UUID),			//	optional gateway pk
                "IEC" // cyng::column("protocol", cyng::TC_STRING)	//	[string] data protocol (IEC, M-Bus, COSEM, ...)
                ),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");
        b = cache_.insert(
            "meterIEC",
            cyng::key_generator(tag_05),
            cyng::data_generator(
                "10.132.24.150",
                static_cast<std::uint16_t>(6000u),
                std::chrono::seconds(840),
                std::chrono::system_clock::time_point(std::chrono::hours(0))),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");

        auto const tag_06 = cyng::to_uuid("405473f4-edd6-4afa-8cea-c224d5918f29");
        b = cache_.insert(
            "device",
            cyng::key_generator(tag_06),
            cyng::data_generator(
                "MA0000000000000000000000004354752",
                "secret",
                "4354752",
                "Maison 44",
                "",
                "",
                true,
                std::chrono::system_clock::now()),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");
        b = cache_.insert(
            "meterwMBus",
            cyng::key_generator(tag_06),
            cyng::data_generator(
                boost::asio::ip::make_address("10.132.32.142"),
                static_cast<std::uint16_t>(2000u),
                cyng::make_aes_key<cyng::crypto::aes128_size>(cyng::hex_to_buffer("2BFFCB61D7E8DC439239555D3DFE1B1D")),
                std::chrono::system_clock::time_point(std::chrono::hours(0))),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");

        //
        //	gateway LSMTest2
        //
        auto const tag_07 = cyng::to_uuid("28c4b783-f35d-49f1-9027-a75dbae9f5e2");
        b = cache_.insert(
            "device",
            cyng::key_generator(tag_07),
            cyng::data_generator("LSMTest2", "LSMTest2", "LSMTest2", "labor", "", "", true, std::chrono::system_clock::now()),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");
        b = cache_.insert(
            "gateway",
            cyng::key_generator(tag_07),
            cyng::data_generator(
                "0500153B01EC46",                 //	server ID
                "EMH",                            //	manufacturer
                std::chrono::system_clock::now(), //	tom
                "06441734",
                cyng::mac48(),      // cyng::column("ifService", cyng::TC_MAC48),		//	(5) MAC of service interface
                cyng::mac48(),      // cyng::column("ifData", cyng::TC_STRING),		//	(6) MAC of WAN interface
                "pw",               // cyng::column("pwdDef", cyng::TC_STRING),		//	(7) Default PW
                "root",             // cyng::column("pwdRoot", cyng::TC_STRING),		//	(8) root PW
                "A815408943050131", // cyng::column("mbus", cyng::TC_STRING),			//	(9) W-Mbus ID (i.e.
                                    // A815408943050131)
                "operator",         // cyng::column("userName", cyng::TC_STRING),		//	(10)
                "operator"          // cyng::column("userPwd", cyng::TC_STRING)		//	(11)
                ),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");

        //
        //	test gateways/devices
        //
        auto const tag_08 = cyng::to_uuid("27527834-aedc-4508-995c-1db0636a92c4");
        //	01-e61e-29436587-bf-03
        //	87654329
        b = cache_.insert(
            "device",
            cyng::key_generator(tag_08),
            cyng::data_generator(
                "87654329", "secret", "87654329", "01-e61e-29436587-bf-03", "", "", true, std::chrono::system_clock::now()),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");
        b = cache_.insert(
            "meterwMBus",
            cyng::key_generator(tag_08),
            cyng::data_generator(
                boost::asio::ip::make_address("0.0.0.0"),
                static_cast<std::uint16_t>(12000u),
                cyng::make_aes_key<cyng::crypto::aes128_size>(cyng::hex_to_buffer("51728910E66D83F851728910E66D83F8")),
                std::chrono::system_clock::time_point(std::chrono::hours(0))),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");
        b = cache_.insert(
            "meter",
            cyng::key_generator(tag_08),
            cyng::data_generator(
                "01-e61e-29436587-bf-03",         //	ident nummer (i.e. 1EMH0006441734, 01-e61e-13090016-3c-07)
                "87654329",                       //	[string] meter number (i.e. 16000913) 4 bytes
                "CH87654329",                     // cyng::column("code", cyng::TC_STRING),		//	[string] metering code - changed at
                                                  // 2019-01-31
                "MAN",                            // cyng::column("maker", cyng::TC_STRING),		//	[string] manufacturer
                std::chrono::system_clock::now(), // cyng::column("tom", cyng::TC_TIME_POINT),	//	[ts] time of
                                                  // manufacture
                "", // cyng::column("vFirmware", cyng::TC_STRING),	//	[string] firmwareversion (i.e. 11600000)
                "", // cyng::column("vParam", cyng::TC_STRING),	//	[string] parametrierversion (i.e. 16A098828.pse)
                "", // cyng::column("factoryNr", cyng::TC_STRING),	//	[string] fabrik nummer (i.e. 06441734)
                "", // cyng::column("item", cyng::TC_STRING),		//	[string] ArtikeltypBezeichnung =
                    // "NXT4-S20EW-6N00-4000-5020-E50/Q"
                "", // cyng::column("mClass", cyng::TC_STRING),	//	[string] Metrological Class: A, B, C, Q3/Q1, ...
                boost::uuids::nil_uuid(), // cyng::column("gw", cyng::TC_UUID),			//	optional gateway pk
                "M-Bus"                   // cyng::column("protocol", cyng::TC_STRING)	//	[string] data protocol (IEC, M-Bus, COSEM,
                                          // ...)
                ),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");

        auto const tag_09 = cyng::to_uuid("7fb2b466-d2f5-4057-ba66-566082dbb288");
        //	01-e61e-13090016-3c-07
        //	16000913
        b = cache_.insert(
            "device",
            cyng::key_generator(tag_09),
            cyng::data_generator(
                "16000913", "secret", "16000913", "01-e61e-13090016-3c-07", "", "", true, std::chrono::system_clock::now()),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");
        b = cache_.insert(
            "meterwMBus",
            cyng::key_generator(tag_09),
            cyng::data_generator(
                boost::asio::ip::make_address("0.0.0.0"),
                static_cast<std::uint16_t>(12000u),
                cyng::make_aes_key<cyng::crypto::aes128_size>(cyng::hex_to_buffer("51728910E66D83F851728910E66D83F8")),
                std::chrono::system_clock::time_point(std::chrono::hours(0))),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");
        b = cache_.insert(
            "meter",
            cyng::key_generator(tag_09),
            cyng::data_generator(
                "01-e61e-13090016-3c-07",         //	ident nummer (i.e. 1EMH0006441734, 01-e61e-13090016-3c-07)
                "16000913",                       //	[string] meter number (i.e. 16000913) 4 bytes
                "CH16000913",                     // cyng::column("code", cyng::TC_STRING),		//	[string] metering code - changed at
                                                  // 2019-01-31
                "MAN",                            // cyng::column("maker", cyng::TC_STRING),		//	[string] manufacturer
                std::chrono::system_clock::now(), // cyng::column("tom", cyng::TC_TIME_POINT),	//	[ts] time of
                                                  // manufacture
                "", // cyng::column("vFirmware", cyng::TC_STRING),	//	[string] firmwareversion (i.e. 11600000)
                "", // cyng::column("vParam", cyng::TC_STRING),	//	[string] parametrierversion (i.e. 16A098828.pse)
                "", // cyng::column("factoryNr", cyng::TC_STRING),	//	[string] fabrik nummer (i.e. 06441734)
                "", // cyng::column("item", cyng::TC_STRING),		//	[string] ArtikeltypBezeichnung =
                    // "NXT4-S20EW-6N00-4000-5020-E50/Q"
                "", // cyng::column("mClass", cyng::TC_STRING),	//	[string] Metrological Class: A, B, C, Q3/Q1, ...
                boost::uuids::nil_uuid(), // cyng::column("gw", cyng::TC_UUID),			//	optional gateway pk
                "M-Bus"                   // cyng::column("protocol", cyng::TC_STRING)	//	[string] data protocol (IEC, M-Bus, COSEM,
                                          // ...)
                ),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");

        auto const tag_10 = cyng::to_uuid("89f7be0e-01c5-4d3a-99cf-f009e25cac76");
        //	01-a815-74314504-01-02
        //	04453174
        b = cache_.insert(
            "device",
            cyng::key_generator(tag_10),
            cyng::data_generator(
                "04453174", "secret", "04453174", "01-a815-74314504-01-02", "", "", true, std::chrono::system_clock::now()),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");
        b = cache_.insert(
            "meterwMBus",
            cyng::key_generator(tag_10),
            cyng::data_generator(
                boost::asio::ip::make_address("0.0.0.0"),
                static_cast<std::uint16_t>(12000u),
                cyng::make_aes_key<cyng::crypto::aes128_size>(cyng::hex_to_buffer("23A84B07EBCBAF948895DF0E9133520D")),
                std::chrono::system_clock::time_point(std::chrono::hours(0))),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");
        b = cache_.insert(
            "meter",
            cyng::key_generator(tag_10),
            cyng::data_generator(
                "01-a815-74314504-01-02",         //	ident nummer (i.e. 1EMH0006441734, 01-e61e-13090016-3c-07)
                "04453174",                       //	[string] meter number (i.e. 16000913) 4 bytes
                "CH04453174",                     // cyng::column("code", cyng::TC_STRING),		//	[string] metering code - changed at
                                                  // 2019-01-31
                "MAN",                            // cyng::column("maker", cyng::TC_STRING),		//	[string] manufacturer
                std::chrono::system_clock::now(), // cyng::column("tom", cyng::TC_TIME_POINT),	//	[ts] time of
                                                  // manufacture
                "", // cyng::column("vFirmware", cyng::TC_STRING),	//	[string] firmwareversion (i.e. 11600000)
                "", // cyng::column("vParam", cyng::TC_STRING),	//	[string] parametrierversion (i.e. 16A098828.pse)
                "", // cyng::column("factoryNr", cyng::TC_STRING),	//	[string] fabrik nummer (i.e. 06441734)
                "", // cyng::column("item", cyng::TC_STRING),		//	[string] ArtikeltypBezeichnung =
                    // "NXT4-S20EW-6N00-4000-5020-E50/Q"
                "", // cyng::column("mClass", cyng::TC_STRING),	//	[string] Metrological Class: A, B, C, Q3/Q1, ...
                boost::uuids::nil_uuid(), // cyng::column("gw", cyng::TC_UUID),			//	optional gateway pk
                "M-Bus"                   // cyng::column("protocol", cyng::TC_STRING)	//	[string] data protocol (IEC, M-Bus, COSEM,
                                          // ...)
                ),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");

        auto const tag_11 = cyng::to_uuid("0b935305-7ac4-46e6-a835-7068159a73a8");
        //	01-e61e-79426800-02-0e
        //	00684279
        b = cache_.insert(
            "device",
            cyng::key_generator(tag_11),
            cyng::data_generator(
                "00684279", "secret", "00684279", "01-e61e-79426800-02-0e", "", "", true, std::chrono::system_clock::now()),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");
        b = cache_.insert(
            "meterwMBus",
            cyng::key_generator(tag_11),
            cyng::data_generator(
                boost::asio::ip::make_address("0.0.0.0"),
                static_cast<std::uint16_t>(12000u),
                cyng::make_aes_key<cyng::crypto::aes128_size>(cyng::hex_to_buffer("6140B8C066EDDE3773EDF7F8007A45AB")),
                std::chrono::system_clock::time_point(std::chrono::hours(0))),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");
        b = cache_.insert(
            "meter",
            cyng::key_generator(tag_11),
            cyng::data_generator(
                "01-e61e-79426800-02-0e",         //	ident nummer (i.e. 1EMH0006441734, 01-e61e-13090016-3c-07)
                "00684279",                       //	[string] meter number (i.e. 16000913) 4 bytes
                "CH00684279",                     // cyng::column("code", cyng::TC_STRING),		//	[string] metering code - changed at
                                                  // 2019-01-31
                "MAN",                            // cyng::column("maker", cyng::TC_STRING),		//	[string] manufacturer
                std::chrono::system_clock::now(), // cyng::column("tom", cyng::TC_TIME_POINT),	//	[ts] time of
                                                  // manufacture
                "", // cyng::column("vFirmware", cyng::TC_STRING),	//	[string] firmwareversion (i.e. 11600000)
                "", // cyng::column("vParam", cyng::TC_STRING),	//	[string] parametrierversion (i.e. 16A098828.pse)
                "", // cyng::column("factoryNr", cyng::TC_STRING),	//	[string] fabrik nummer (i.e. 06441734)
                "", // cyng::column("item", cyng::TC_STRING),		//	[string] ArtikeltypBezeichnung =
                    // "NXT4-S20EW-6N00-4000-5020-E50/Q"
                "", // cyng::column("mClass", cyng::TC_STRING),	//	[string] Metrological Class: A, B, C, Q3/Q1, ...
                boost::uuids::nil_uuid(), // cyng::column("gw", cyng::TC_UUID),			//	optional gateway pk
                "M-Bus"                   // cyng::column("protocol", cyng::TC_STRING)	//	[string] data protocol (IEC, M-Bus, COSEM,
                                          // ...)
                ),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");

        auto const tag_12 = cyng::to_uuid("b801e64f-fabf-4023-ab2e-4bcb8f64f464");
        //	01-e61e-57140621-36-03
        //	21061457
        b = cache_.insert(
            "device",
            cyng::key_generator(tag_12),
            cyng::data_generator(
                "21061457", "secret", "21061457", "01-e61e-57140621-36-03", "", "", true, std::chrono::system_clock::now()),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");
        b = cache_.insert(
            "meterwMBus",
            cyng::key_generator(tag_12),
            cyng::data_generator(
                boost::asio::ip::make_address("0.0.0.0"),
                static_cast<std::uint16_t>(12000u),
                cyng::make_aes_key<cyng::crypto::aes128_size>(cyng::hex_to_buffer("6140B8C066EDDE3773EDF7F8007A45AB")),
                std::chrono::system_clock::time_point(std::chrono::hours(0))),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");
        b = cache_.insert(
            "meter",
            cyng::key_generator(tag_12),
            cyng::data_generator(
                "01-e61e-57140621-36-03",         //	ident nummer (i.e. 1EMH0006441734, 01-e61e-13090016-3c-07)
                "21061457",                       //	[string] meter number (i.e. 16000913) 4 bytes
                "CH21061457",                     // cyng::column("code", cyng::TC_STRING),		//	[string] metering code - changed at
                                                  // 2019-01-31
                "MAN",                            // cyng::column("maker", cyng::TC_STRING),		//	[string] manufacturer
                std::chrono::system_clock::now(), // cyng::column("tom", cyng::TC_TIME_POINT),	//	[ts] time of
                                                  // manufacture
                "", // cyng::column("vFirmware", cyng::TC_STRING),	//	[string] firmwareversion (i.e. 11600000)
                "", // cyng::column("vParam", cyng::TC_STRING),	//	[string] parametrierversion (i.e. 16A098828.pse)
                "", // cyng::column("factoryNr", cyng::TC_STRING),	//	[string] fabrik nummer (i.e. 06441734)
                "", // cyng::column("item", cyng::TC_STRING),		//	[string] ArtikeltypBezeichnung =
                    // "NXT4-S20EW-6N00-4000-5020-E50/Q"
                "", // cyng::column("mClass", cyng::TC_STRING),	//	[string] Metrological Class: A, B, C, Q3/Q1, ...
                boost::uuids::nil_uuid(), // cyng::column("gw", cyng::TC_UUID),			//	optional gateway pk
                "M-Bus"                   // cyng::column("protocol", cyng::TC_STRING)	//	[string] data protocol (IEC, M-Bus, COSEM,
                                          // ...)
                ),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");

        //	00636408
        auto const tag_13 = cyng::to_uuid("e6e62eae-2a80-4185-8e72-5505d4dfe74b");
        //	01-e61e-08646300-36-03
        //	00636408
        b = cache_.insert(
            "device",
            cyng::key_generator(tag_13),
            cyng::data_generator(
                "00636408", "secret", "00636408", "01-e61e-08646300-36-03", "", "", true, std::chrono::system_clock::now()),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");
        b = cache_.insert(
            "meterwMBus",
            cyng::key_generator(tag_13),
            cyng::data_generator(
                boost::asio::ip::make_address("0.0.0.0"),
                static_cast<std::uint16_t>(12000u),
                cyng::make_aes_key<cyng::crypto::aes128_size>(cyng::hex_to_buffer("00000000000000000000000000000000")),
                std::chrono::system_clock::time_point(std::chrono::hours(0))),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");
        b = cache_.insert(
            "meter",
            cyng::key_generator(tag_13),
            cyng::data_generator(
                "01-e61e-08646300-36-03",         //	ident nummer (i.e. 1EMH0006441734, 01-e61e-13090016-3c-07)
                "00636408",                       //	[string] meter number (i.e. 16000913) 4 bytes
                "CH00636408",                     // cyng::column("code", cyng::TC_STRING),		//	[string] metering code - changed at
                                                  // 2019-01-31
                "MAN",                            // cyng::column("maker", cyng::TC_STRING),		//	[string] manufacturer
                std::chrono::system_clock::now(), // cyng::column("tom", cyng::TC_TIME_POINT),	//	[ts] time of manufacture
                "", // cyng::column("vFirmware", cyng::TC_STRING),	//	[string] firmwareversion (i.e. 11600000)
                "", // cyng::column("vParam", cyng::TC_STRING),	//	[string] parametrierversion (i.e. 16A098828.pse)
                "", // cyng::column("factoryNr", cyng::TC_STRING),	//	[string] fabrik nummer (i.e. 06441734)
                "", // cyng::column("item", cyng::TC_STRING),		//	[string] ArtikeltypBezeichnung =
                    // "NXT4-S20EW-6N00-4000-5020-E50/Q"
                "", // cyng::column("mClass", cyng::TC_STRING),	//	[string] Metrological Class: A, B, C, Q3/Q1, ...
                boost::uuids::nil_uuid(), // cyng::column("gw", cyng::TC_UUID),			//	optional gateway pk
                "M-Bus"                   // cyng::column("protocol", cyng::TC_STRING)	//	[string] data protocol (IEC, M-Bus, COSEM,
                                          // ...)
                ),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");

        //	00200796

        //
        //	gateway LSMTest3
        //
        auto const tag_14 = cyng::to_uuid("8e971cf0-36bd-4b36-92f0-a070a0194fae");
        b = cache_.insert(
            "device",
            cyng::key_generator(tag_14),
            cyng::data_generator("LSMTest3", "LSMTest3", "LSMTest3", "labor", "", "", true, std::chrono::system_clock::now()),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");
        b = cache_.insert(
            "gateway",
            cyng::key_generator(tag_14),
            cyng::data_generator(
                "0500153B021774",                 //	server ID
                "EMH",                            //	manufacturer
                std::chrono::system_clock::now(), //	tom
                "B021774",
                cyng::mac48(),      // cyng::column("ifService", cyng::TC_MAC48),		//	(5) MAC of service interface
                cyng::mac48(),      // cyng::column("ifData", cyng::TC_STRING),		//	(6) MAC of WAN interface
                "pw",               // cyng::column("pwdDef", cyng::TC_STRING),		//	(7) Default PW
                "root",             // cyng::column("pwdRoot", cyng::TC_STRING),		//	(8) root PW
                "A815408943050131", // cyng::column("mbus", cyng::TC_STRING),			//	(9) W-Mbus ID (i.e.
                                    // A815408943050131)
                "operator",         // cyng::column("userName", cyng::TC_STRING),		//	(10)
                "operator"          // cyng::column("userPwd", cyng::TC_STRING)		//	(11)
                ),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");

        //
        //	as 1440 (labor)
        //
        auto const tag_15 = cyng::to_uuid("1e6a5c94-c493-4ff6-a790-9b35c431c0e2");
        b = cache_.insert(
            "device",
            cyng::key_generator(tag_15),
            cyng::data_generator(
                "CH0000000000000000000000003218421",
                "secret",
                "03218421",
                "test device",
                "",
                "",
                true,
                std::chrono::system_clock::now()),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        b = cache_.insert(
            "meter",
            cyng::key_generator(tag_15),
            cyng::data_generator(
                "01-e61e-08646300-36-03",            //	ident nummer (i.e. 1EMH0006441734, 01-e61e-13090016-3c-07)
                "03218421",                          //	[string] meter number (i.e. 16000913) 4 bytes
                "CH0000000000000000000000003218421", // cyng::column("code", cyng::TC_STRING),		//	[string]
                                                     // metering code
                                                     // - changed at 2019-01-31
                "ELS",                               // cyng::column("maker", cyng::TC_STRING),		//	[string] manufacturer
                std::chrono::system_clock::now(),    // cyng::column("tom", cyng::TC_TIME_POINT),	//	[ts] time of
                                                     // manufacture
                "", // cyng::column("vFirmware", cyng::TC_STRING),	//	[string] firmwareversion (i.e. 11600000)
                "", // cyng::column("vParam", cyng::TC_STRING),	//	[string] parametrierversion (i.e. 16A098828.pse)
                "1019 1000 0321 8421", // cyng::column("factoryNr", cyng::TC_STRING),	//	[string] fabrik nummer (i.e.
                                       // 06441734)
                "AS1440",              // cyng::column("item", cyng::TC_STRING),		//	[string] ArtikeltypBezeichnung =
                                       // "NXT4-S20EW-6N00-4000-5020-E50/Q"
                "", // cyng::column("mClass", cyng::TC_STRING),	//	[string] Metrological Class: A, B, C, Q3/Q1, ...
                boost::uuids::nil_uuid(), // cyng::column("gw", cyng::TC_UUID),			//	optional gateway pk
                "IEC" // cyng::column("protocol", cyng::TC_STRING)	//	[string] data protocol (IEC, M-Bus, COSEM, ...)
                ),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");
        b = cache_.insert(
            "meterIEC",
            cyng::key_generator(tag_15),
            cyng::data_generator(
                "192.168.0.200",
                static_cast<std::uint16_t>(6006u),
                std::chrono::seconds(840),
                std::chrono::system_clock::time_point(std::chrono::hours(0))),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");

        //
        //	MA0000000000000000000000035074616,RS485,10.132.24.150,6000,Elster,Elster AS 1440,IEC 62056,Yasmina,House A2 Meter
        // Board - Bairiki,Yes,,,
        //
        auto const tag_16 = cyng::to_uuid("bd169a51-dbc4-4c9a-8c03-18101fface39");
        b = cache_.insert(
            "device",
            cyng::key_generator(tag_16),
            cyng::data_generator(
                "MA0000000000000000000000035074616",
                "secret",
                "3496230",
                "Yasmina,House A2 Meter Board",
                "",
                "",
                true,
                std::chrono::system_clock::now()),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");
        b = cache_.insert(
            "meter",
            cyng::key_generator(tag_16),
            cyng::data_generator(
                "35074616",                          //	ident nummer (i.e. 1EMH0006441734, 01-e61e-13090016-3c-07)
                "35074616",                          //	[string] meter number (i.e. 16000913) 4 bytes
                "MA0000000000000000000000035074616", // cyng::column("code", cyng::TC_STRING),		//	[string]
                                                     // metering code
                                                     // - changed at 2019-01-31
                "ELS",                               // cyng::column("maker", cyng::TC_STRING),		//	[string] manufacturer
                std::chrono::system_clock::now(),    // cyng::column("tom", cyng::TC_TIME_POINT),	//	[ts] time of
                                                     // manufacture
                "",        // cyng::column("vFirmware", cyng::TC_STRING),	//	[string] firmwareversion (i.e. 11600000)
                "",        // cyng::column("vParam", cyng::TC_STRING),	//	[string] parametrierversion (i.e. 16A098828.pse)
                "",        // cyng::column("factoryNr", cyng::TC_STRING),	//	[string] fabrik nummer (i.e. 06441734)
                "AS 1440", // cyng::column("item", cyng::TC_STRING),		//	[string] ArtikeltypBezeichnung =
                           // "NXT4-S20EW-6N00-4000-5020-E50/Q"
                "",        // cyng::column("mClass", cyng::TC_STRING),	//	[string] Metrological Class: A, B, C, Q3/Q1, ...
                boost::uuids::nil_uuid(), // cyng::column("gw", cyng::TC_UUID),			//	optional gateway pk
                "IEC" // cyng::column("protocol", cyng::TC_STRING)	//	[string] data protocol (IEC, M-Bus, COSEM, ...)
                ),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");
        b = cache_.insert(
            "meterIEC",
            cyng::key_generator(tag_16),
            cyng::data_generator(
                "10.132.24.150",
                static_cast<std::uint16_t>(6000u),
                std::chrono::seconds(840),
                std::chrono::system_clock::time_point(std::chrono::hours(0))),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");

        //
        //	gateway LSMTest1
        //
        auto const tag_17 = cyng::to_uuid("34868fbb-3e4e-4df6-bd5f-50b2b6f68708");
        b = cache_.insert(
            "device",
            cyng::key_generator(tag_17),
            cyng::data_generator(
                "LSMTest1", "LSMTest1", "LSMTest1", "Testdevice (1) labor", "", "", true, std::chrono::system_clock::now()),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");
        b = cache_.insert(
            "gateway",
            cyng::key_generator(tag_17),
            cyng::data_generator(
                "0500153B022980",                 //	server ID
                "EMH",                            //	manufacturer
                std::chrono::system_clock::now(), //	tom
                "B022980",
                cyng::mac48(),      // cyng::column("ifService", cyng::TC_MAC48),		//	(5) MAC of service interface
                cyng::mac48(),      // cyng::column("ifData", cyng::TC_STRING),		//	(6) MAC of WAN interface
                "pw",               // cyng::column("pwdDef", cyng::TC_STRING),		//	(7) Default PW
                "root",             // cyng::column("pwdRoot", cyng::TC_STRING),		//	(8) root PW
                "A815408943050131", // cyng::column("mbus", cyng::TC_STRING),			//	(9) W-Mbus ID (i.e.
                                    // A815408943050131)
                "operator",         // cyng::column("userName", cyng::TC_STRING),		//	(10)
                "operator"          // cyng::column("userPwd", cyng::TC_STRING)		//	(11)
                ),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");

        //
        //	gateway LSMTest5
        //
        auto const tag_18 = cyng::to_uuid("55fbb39d-cd1d-4c24-b407-aba86a23a789");
        b = cache_.insert(
            "device",
            cyng::key_generator(tag_18),
            cyng::data_generator(
                "LSMTest5", "LSMTest5", "LSMTest5", "Testdevice (5) labor", "", "", true, std::chrono::system_clock::now()),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");
        b = cache_.insert(
            "gateway",
            cyng::key_generator(tag_18),
            cyng::data_generator(
                "0500153B02297e",                 //	server ID
                "EMH",                            //	manufacturer
                std::chrono::system_clock::now(), //	tom
                "B02297e",
                cyng::mac48(),      // cyng::column("ifService", cyng::TC_MAC48),		//	(5) MAC of service interface
                cyng::mac48(),      // cyng::column("ifData", cyng::TC_STRING),		//	(6) MAC of WAN interface
                "pw",               // cyng::column("pwdDef", cyng::TC_STRING),		//	(7) Default PW
                "root",             // cyng::column("pwdRoot", cyng::TC_STRING),		//	(8) root PW
                "A815408943050131", // cyng::column("mbus", cyng::TC_STRING),			//	(9) W-Mbus ID (i.e.
                                    // A815408943050131)
                "operator",         // cyng::column("userName", cyng::TC_STRING),		//	(10)
                "operator"          // cyng::column("userPwd", cyng::TC_STRING)		//	(11)
                ),
            1u //	only needed for insert operations
            ,
            cfg_.get_tag());
        BOOST_ASSERT_MSG(b, "insert failed");
    }

    bool db::push_sys_msg(std::string msg, cyng::severity level) {
        return cache_.insert_auto("sysMsg", cyng::data_generator(std::chrono::system_clock::now(), level, msg), cfg_.get_tag());
    }

    void db::log_dispatch_error(std::string task, std::string slot) {
        //  internal dispatch error
        push_sys_msg("internal dispatch error: " + task + "/" + slot, cyng::severity::LEVEL_FATAL);
    }

    std::pair<std::uint32_t, bool> db::register_target(
        boost::uuids::uuid tag,
        boost::uuids::uuid dev,
        boost::uuids::uuid peer, //	local vm
        std::string name,
        std::uint16_t paket_size,
        std::uint8_t window_size) {
        //
        //	ToDo: test for duplicate target names of same session
        //

        bool r = false;
        auto const channel = channel_target_++;

        //
        //	insert into target table
        //
        cache_.access(
            [&](cyng::table *tbl_trg, cyng::table const *tbl_dev) {
                auto const rec = tbl_dev->lookup(cyng::key_generator(dev));
                if (!rec.empty()) {

                    auto const owner = rec.value<std::string>("name", "");
                    CYNG_LOG_INFO(logger_, "[db] register target - device [" << owner << "]");
                    r = tbl_trg->insert(
                        cyng::key_generator(channel),
                        cyng::data_generator(
                            tag,
                            peer,
                            name,
                            dev,
                            owner,
                            paket_size,
                            window_size,
                            std::chrono::system_clock::now(),
                            static_cast<std::uint64_t>(0),  //	px
                            static_cast<std::uint64_t>(0),  //  msg counter
                            static_cast<std::uint64_t>(0)), //  open channels
                        1,
                        cfg_.get_tag());

                } else {
                    CYNG_LOG_WARNING(logger_, "[db] register target - device " << dev << " not found");
                }
            },
            cyng::access::write("target"),
            cyng::access::read("device"));

        return {channel, r};
    }

    std::tuple<std::uint32_t, std::uint32_t, std::uint16_t, std::uint32_t, std::string> db::open_channel(
        boost::uuids::uuid dev, //  key in table "device"
        std::string name,       // target name
        std::string account,    // account (unused yet)
        std::string number,     // msisdn (unused yet)
        std::string sv,         // software version (unused yet)
        std::string id,         // device id / model (unused yet)
        std::chrono::seconds timeout) {
        //
        //  initialize with an invalid value
        //
        std::uint32_t source = 0;

        //
        //  Each attempt to open a channel increases the channel counter
        //
        std::uint32_t const channel = channel_pty_++;

        //
        //  start with the maximum and reduces if there are targets with a lower limit
        //
        std::uint16_t packet_size = std::numeric_limits<std::uint16_t>::max();
        cyng::key_list_t targets;

        //
        //  device name as debug/log info
        //
        std::string device;

        //
        //	get source channel from session
        //
        cache_.access(
            [&, this](cyng::table const *tbl_session, cyng::table *tbl_target, cyng::table *tbl_channel) {
                auto const key = cyng::key_generator(dev);
                auto const rec_session = tbl_session->lookup(key);
                if (!rec_session.empty()) {

                    source = rec_session.value<std::uint32_t>("source", 0);
                    CYNG_LOG_DEBUG(logger_, "channel to " << name << " has source id: " << source);
                    BOOST_ASSERT(source != 0);

                    device = rec_session.value("name", "");

                    //
                    //	ToDo: check if device is enabled
                    //

                    //
                    //	find matching target
                    //
                    std::tie(targets, packet_size) = get_matching_targets(tbl_target, name, account, number, sv, id, dev);

                    CYNG_LOG_INFO(
                        logger_, targets.size() << " target(s) of name " << name << " found - packet size " << packet_size);

                    //
                    //	ToDo: check device name, number, version and id
                    //

                    for (auto const &key_target : targets) {

                        //
                        //	read target data
                        //
                        auto const rec_target = tbl_target->lookup(key_target);
                        auto const target_id = rec_target.value<std::uint32_t>("tag", 0);
                        BOOST_ASSERT(target_id != 0);

                        //
                        //	this is the tag of the target session
                        //
                        auto const target_tag = rec_target.value("session", boost::uuids::nil_uuid());
                        BOOST_ASSERT(!target_tag.is_nil());
                        auto const target_peer = rec_target.value("peer", boost::uuids::nil_uuid());
                        BOOST_ASSERT(!target_peer.is_nil());

                        CYNG_LOG_DEBUG(logger_, "target tag: " << target_tag << ", target peer: " << target_peer);

                        tbl_channel->insert(
                            cyng::key_generator(channel, target_id),
                            cyng::data_generator(target_tag, target_peer, packet_size, timeout, dev),
                            1, // generation
                            cfg_.get_tag());

                        //
                        //  increase channel counter in table "target"
                        //
                        tbl_target->compute<std::uint64_t>(
                            key_target,
                            "channels",
                            [](std::uint64_t channel) -> std::uint64_t { return channel + 1; },
                            cfg_.get_tag());
                    }

                } else {
                    CYNG_LOG_ERROR(logger_, "session " << dev << " not found");
                }
            },
            cyng::access::read("session"),
            cyng::access::write("target"),
            cyng::access::write("channel"));

        //	[channel, source, packet_size, count]
        return {channel, source, packet_size, static_cast<std::uint32_t>(targets.size()), device};
    }

    std::pair<cyng::key_list_t, std::uint16_t> db::get_matching_targets(
        cyng::table const *tbl, //  "target"
        std::string name,
        std::string account,
        std::string number,
        std::string sv,
        std::string id,
        boost::uuids::uuid dev) {
        cyng::key_list_t keys;
        std::uint16_t packet_size = std::numeric_limits<std::uint16_t>::max();

        tbl->loop([&](cyng::record &&rec, std::size_t) -> bool {
            auto const target = rec.value("name", "");
            BOOST_ASSERT(!target.empty());
            auto const device = rec.value("device", boost::uuids::nil_uuid());
            auto const account = rec.value("account", "");

            //
            //	dont't open channels to your own targets
            //
            if (dev == device) {
                CYNG_LOG_WARNING(logger_, "session " << dev << " opens channel to it's own target " << name);
            }

            if (boost::algorithm::starts_with(target, name)) {

                //
                //	matching target
                //
                keys.insert(rec.key());
                CYNG_LOG_DEBUG(logger_, "found matching target " << target << " from device " << account);

                auto const ps = rec.value("pSize", packet_size);

                //
                //	a minimum size of 0x100 bytes is required
                //
                if (ps < packet_size && ps > 0x100) {
                    packet_size = ps;
                }
            }

            return true;
        });

        return {keys, packet_size};
    }

    std::vector<push_target> db::get_matching_channels(std::uint32_t channel, std::size_t size) {
        std::vector<push_target> vec;
        BOOST_ASSERT(channel != 0);

        cache_.access(
            [&, this](cyng::table const *tbl_channel, cyng::table *tbl_target) {
                tbl_channel->loop([&](cyng::record &&rec, std::size_t) -> bool {
                    auto const c = rec.value<std::uint32_t>("channel", 0);
                    BOOST_ASSERT(c != 0);
                    if (channel == c) {

                        //
                        //	target channel
                        //
                        auto const target = rec.value<std::uint32_t>("target", 0);
                        BOOST_ASSERT(target != 0);
                        CYNG_LOG_TRACE(logger_, "[db] matching target: " << channel << ':' << target << " found");

                        auto const rec_target = tbl_target->lookup(cyng::key_generator(target));
                        if (!rec_target.empty()) {
                            // auto const account = rec.value("account", "");

                            CYNG_LOG_DEBUG(
                                logger_, "[db] matching target: " << channel << ':' << target << ": " << rec_target.to_string());

                            //
                            //	add to result vector
                            //
                            vec.push_back(
                                {std::make_pair(
                                     rec_target.value("session", boost::uuids::nil_uuid()),
                                     rec_target.value("peer", boost::uuids::nil_uuid())),
                                 target});

                            //
                            //	update message counter and px
                            //
                            auto const counter = rec_target.value<std::uint64_t>("counter", 0);
                            auto const px = rec_target.value<std::uint64_t>("px", 0);
                            tbl_target->modify(
                                rec_target.key(), cyng::param_map_factory("px", px + size)("counter", counter + 1), cfg_.get_tag());

                        } else {
                            CYNG_LOG_WARNING(logger_, "[db] channel: " << channel << ':' << target << " has no target entry");
                        }
                    }
                    return true;
                });
            },
            cyng::access::read("channel"),
            cyng::access::write("target"));

        return vec;
    }

    std::size_t db::close_channel(std::uint32_t channel) {
        cyng::key_list_t keys;
        cache_.access(
            [&, this](cyng::table *tbl_channel, cyng::table *tbl_target) {
                tbl_channel->loop([&, this](cyng::record &&rec, std::size_t) -> bool {
                    auto const c = rec.value<std::uint32_t>("channel", 0);
                    if (c == channel) {
                        keys.insert(rec.key());

                        //
                        //  decrease listener count in table "target"
                        //
                        auto const target = rec.value<std::uint32_t>("target", 0);
                        auto const key_target = cyng::key_generator(target);
                        auto const rec_target = tbl_target->lookup(key_target);
                        if (!rec_target.empty()) {
                            tbl_target->compute<std::uint64_t>(
                                key_target,
                                "channels",
                                [](std::uint64_t channel) -> std::uint64_t { return (channel == 0) ? 0 : channel - 1; },
                                cfg_.get_tag());
                        } else {
                            CYNG_LOG_WARNING(logger_, "[db] target: #" << target << " not found");
                        }
                    }
                    return true;
                });

                for (auto const &key : keys) {

                    //
                    //  remove channel
                    //
                    tbl_channel->erase(key, cfg_.get_tag());
                }
            },
            cyng::access::write("channel"),
            cyng::access::write("target"));
        return keys.size();
    }

    void db::update_connection_throughput(boost::uuids::uuid rtag, boost::uuids::uuid dev, std::uint64_t size) {
        auto const key_conn = cyng::key_generator(cyng::merge(rtag, dev));
        cache_.access(
            [&, this](cyng::table *tbl_conn) {
                auto const rec = tbl_conn->lookup(key_conn);
                if (!rec.empty()) {
                    auto const throughput = rec.value<std::uint64_t>("throughput", 0u) + size;
                    tbl_conn->modify(key_conn, cyng::make_param("throughput", throughput), cfg_.get_tag());
                }
            },
            cyng::access::write("connection"));
    }

    void db::update_ping_result(boost::uuids::uuid peer, std::chrono::microseconds delta) {
        auto const key_peer = cyng::key_generator(peer);
        cache_.access(
            [&, this](cyng::table *tbl_cluster) { tbl_cluster->modify(key_peer, cyng::make_param("ping", delta), cfg_.get_tag()); },
            cyng::access::write("cluster"));
    }

    void db::locate_gateway(
        cyng::key_t key,
        std::function<void(
            boost::uuids::uuid,
            boost::uuids::uuid,
            std::string,
            std::string,
            boost::uuids::uuid,
            cyng::buffer_t,
            std::string,
            std::string,
            std::string)> cb) {

        cache_.access(
            [=, this](cyng::table const *tbl_session, cyng::table const *tbl_gw, cyng::table const *tbl_dev) {
                auto const rec = tbl_session->lookup(key);
                if (!rec.empty()) {
                    auto const tag = rec.value("tag", boost::uuids::nil_uuid()); //  same as key
                    auto const rtag = rec.value("rTag", boost::uuids::nil_uuid());
                    auto const peer = rec.value("peer", boost::uuids::nil_uuid());
                    auto const name = rec.value("name", "");

                    //
                    //  ToDo: check connection state
                    //  and set connection state (to block incoming calls)
                    //

                    auto const rec_gw = tbl_gw->lookup(key);
                    if (!rec_gw.empty()) {
                        //  convert server ID to cyng::buffer_t
                        auto const id = cyng::hex_to_buffer(rec_gw.value("serverId", ""));
                        auto const operator_name = rec_gw.value("userName", "");
                        auto const operator_pwd = rec_gw.value("userPwd", "");
                        auto const rec_dev = tbl_dev->lookup(key);

                        if (!rec_dev.empty()) {
                            auto const account = rec_dev.value("name", name);
                            auto const pwd = rec_dev.value("pwd", "");
                            auto const msisdn = rec_dev.value("msisdn", "");
                            BOOST_ASSERT(boost::algorithm::equals(account, name));
                            auto const fw = rec_dev.value("vFirmware", "");

                            //
                            //  note that some tables are locked
                            //
                            cb(rtag, peer, operator_name, operator_pwd, tag, id, fw, name, msisdn);
                        }
                    }
                } else {
                    CYNG_LOG_ERROR(logger_, "cannot find session/gateway: " << key);
                    std::size_t counter{0};
                    tbl_session->loop([&](cyng::record &&rec, std::size_t) -> bool {
                        ++counter;
                        CYNG_LOG_DEBUG(logger_, "#" << counter << " - " << rec.to_string());
                        return true;
                    });
                }
            },
            cyng::access::read("session"),
            cyng::access::read("gateway"),
            cyng::access::read("device"));
    }

    push_target::push_target()
        : pty_(boost::uuids::nil_uuid(), boost::uuids::nil_uuid())
        , channel_(0) {}

    push_target::push_target(pty p, std::uint32_t channel)
        : pty_(p)
        , channel_(channel) {}

    std::vector<cyng::meta_store> get_store_meta_data() {
        return {
            config::get_config(),             // "config"
            config::get_store_device(),       // "device"
            config::get_store_meter(),        // "meter"
            config::get_store_meterIEC(),     // "meterIEC"
            config::get_store_meterwMBus(),   // "meterwMBus"
            config::get_store_gateway(),      // "gateway"
            config::get_store_lora(),         // "loRaDevice"
            config::get_store_gui_user(),     // "guiUser"
            config::get_store_target(),       // "target"
            config::get_store_cluster(),      // "cluster"
            config::get_store_location(),     // "location"
            config::get_store_cfg_set_meta(), // "cfgSetMeta"
            config::get_store_statistics(),
            config::get_store_history(),
            //  volatile
            config::get_store_session(),    // "session"
            config::get_store_connection(), // "connection"
            config::get_store_channel(),    //	only in main node
            config::get_store_gwIEC(),
            config::get_store_gwwMBus(),
        };
    }

} // namespace smf
