/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <db.h>
#include <session.h>
#include <tasks/ping.h>

#include <smf/cluster/features.h>

#include <cyng/io/ostream.h>
#include <cyng/io/serialize.h>
#include <cyng/log/record.h>
#include <cyng/obj/algorithm/add.hpp>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/container_factory.hpp>
#include <cyng/obj/numeric_cast.hpp>
#include <cyng/parse/buffer.h>
#include <cyng/vm/generator.hpp>
#include <cyng/vm/linearize.hpp>
#include <cyng/vm/mesh.h>
#include <cyng/vm/vm.h>

#include <iostream>

namespace smf {

    session::session(boost::asio::ip::tcp::socket socket, db &cache, cyng::mesh &fabric, cyng::logger logger)
        : socket_(std::move(socket))
        , cache_(cache)
        , ctl_(fabric.get_ctl())
        , logger_(logger)
        , buffer_()
        , buffer_write_()
        , vm_()
        , parser_([this](cyng::object &&obj) {
            //  un-comment this line to debug problems with transferring data over
            //  the cluster bus.
            // CYNG_LOG_DEBUG(logger_, "parser: " << cyng::io::to_typed(obj));
            vm_.load(std::move(obj));
        })
        , slot_(cyng::make_slot(new slot(this)))
        , peer_(boost::uuids::nil_uuid())
        , protocol_layer_("any")
        , dep_key_()
        , ping_() {

        vm_ = init_vm(fabric);
    }

    boost::uuids::uuid session::get_peer() const { return vm_.get_tag(); }

    boost::uuids::uuid session::get_remote_peer() const { return peer_; }

    void session::stop() {

        //
        //  stop depended ping task
        //
        ping_->stop();

        //	https://www.boost.org/doc/libs/1_75_0/doc/html/boost_asio/reference/basic_stream_socket/close/overload2.html
        CYNG_LOG_WARNING(logger_, "stop session");
        boost::system::error_code ec;
        socket_.shutdown(boost::asio::socket_base::shutdown_both, ec);
        socket_.close(ec);

        //
        //	remove all subscriptions
        //
        // BOOST_ASSERT(srvp_ != nullptr);
        auto const count = cache_.get_store().disconnect(slot_);
        CYNG_LOG_TRACE(logger_, "disconnect from " << count << " tables");

        //
        //	stop VM
        //
        vm_.stop();
    }

    void session::start() {
        do_read();
        //
        //	start ping
        //
        ping_ = ctl_.create_channel_with_ref<ping>(
            ctl_, logger_, cache_, std::bind(&session::send_ping_request, this->shared_from_this()));
        BOOST_ASSERT(ping_->is_open());
        ping_->suspend(std::chrono::minutes(1), "update");
    }

    void session::do_read() {
        auto self = shared_from_this();

        socket_.async_read_some(
            boost::asio::buffer(buffer_.data(), buffer_.size()),
            [this, self](boost::system::error_code ec, std::size_t bytes_transferred) {
                if (!ec) {
                    CYNG_LOG_DEBUG(
                        logger_, "session [" << socket_.remote_endpoint() << "] received " << bytes_transferred << " bytes");

                    //
                    //	let parse it
                    //
                    parser_.read(buffer_.begin(), buffer_.begin() + bytes_transferred);

                    //
                    //	continue reading
                    //
                    do_read();
                } else {
                    CYNG_LOG_WARNING(logger_, "[session] read: " << ec.message());
                    //  ping holds a reference of this session and must be terminated.
                    if (ping_ && ping_->is_open()) {
                        ping_->stop();
                    }
                    CYNG_LOG_TRACE(logger_, "[session] use count: " << self.use_count());
                }
            });
    }

    cyng::vm_proxy session::init_vm(cyng::mesh &fabric) {

        return fabric.make_proxy(
            cyng::make_description(
                "cluster.req.login",
                cyng::vm_adaptor<
                    session,
                    void,
                    std::string,
                    std::string,
                    cyng::pid,
                    std::string,
                    boost::uuids::uuid,
                    std::uint32_t,
                    cyng::version>(this, &session::cluster_login)),

            cyng::make_description(
                "cluster.res.ping",
                cyng::vm_adaptor<session, void, std::chrono::system_clock::time_point>(this, &session::cluster_ping)),

            cyng::make_description(
                "cluster.test.msg", cyng::vm_adaptor<session, void, std::string>(this, &session::cluster_test_msg)),

            cyng::make_description(
                "db.req.subscribe",
                cyng::vm_adaptor<session, void, std::string, boost::uuids::uuid>(this, &session::db_req_subscribe)),

            cyng::make_description(
                "db.req.insert",
                cyng::vm_adaptor<session, void, std::string, cyng::key_t, cyng::data_t, std::uint64_t, boost::uuids::uuid>(
                    this, &session::db_req_insert)),

            cyng::make_description(
                "db.req.insert.auto",
                cyng::vm_adaptor<session, void, std::string, cyng::data_t, boost::uuids::uuid>(this, &session::db_req_insert_auto)),

            cyng::make_description(
                "db.req.update",
                cyng::vm_adaptor<session, void, std::string, cyng::key_t, cyng::param_map_t, boost::uuids::uuid>(
                    this, &session::db_req_update)),

            cyng::make_description(
                "db.req.remove",
                cyng::vm_adaptor<session, void, std::string, cyng::key_t, boost::uuids::uuid>(this, &session::db_req_remove)),

            cyng::make_description(
                "db.req.clear", cyng::vm_adaptor<session, void, std::string, boost::uuids::uuid>(this, &session::db_req_clear)),

            cyng::make_description(
                "pty.req.login",
                cyng::vm_adaptor<
                    session,
                    void,
                    boost::uuids::uuid,
                    std::string,
                    std::string,
                    boost::asio::ip::tcp::endpoint,
                    std::string>(this, &session::pty_login)),

            cyng::make_description(
                "pty.req.logout",
                cyng::vm_adaptor<session, void, boost::uuids::uuid, boost::uuids::uuid>(this, &session::pty_logout)),

            cyng::make_description(
                "pty.open.connection",
                cyng::vm_adaptor<session, void, boost::uuids::uuid, boost::uuids::uuid, std::string, cyng::param_map_t>(
                    this, &session::pty_open_connection)),

            cyng::make_description(
                "pty.forward.res.open.connection",
                cyng::vm_adaptor<session, void, boost::uuids::uuid, bool, cyng::param_map_t>(
                    this, &session::pty_forward_res_open_connection)),

            cyng::make_description(
                "pty.return.open.connection",
                cyng::vm_adaptor<session, void, bool, boost::uuids::uuid, boost::uuids::uuid, cyng::param_map_t>(
                    this, &session::pty_return_open_connection)),

            cyng::make_description(
                "pty.transfer.data",
                cyng::vm_adaptor<session, void, boost::uuids::uuid, boost::uuids::uuid, cyng::buffer_t>(
                    this, &session::pty_transfer_data)),
            cyng::make_description(
                "pty.close.connection",
                cyng::vm_adaptor<session, void, boost::uuids::uuid, boost::uuids::uuid, cyng::param_map_t>(
                    this, &session::pty_close_connection)),

            cyng::make_description(
                "pty.register.target",
                cyng::vm_adaptor<
                    session,
                    void,
                    boost::uuids::uuid,
                    boost::uuids::uuid,
                    std::string,
                    std::uint16_t,
                    std::uint8_t,
                    cyng::param_map_t>(this, &session::pty_register_target)),

            cyng::make_description(
                "pty.deregister", cyng::vm_adaptor<session, void, boost::uuids::uuid, std::string>(this, &session::pty_deregister)),

            cyng::make_description(
                "pty.open.channel",
                cyng::vm_adaptor<
                    session,
                    void,
                    boost::uuids::uuid,
                    boost::uuids::uuid,
                    std::string,
                    std::string,
                    std::string,
                    std::string,
                    std::string,
                    std::chrono::seconds,
                    cyng::param_map_t>(this, &session::pty_open_channel)),

            cyng::make_description(
                "pty.close.channel",
                cyng::vm_adaptor<session, void, boost::uuids::uuid, boost::uuids::uuid, std::uint32_t, cyng::param_map_t>(
                    this, &session::pty_close_channel)),

            cyng::make_description(
                "pty.req.push.data",
                cyng::vm_adaptor<
                    session,
                    void,
                    boost::uuids::uuid,
                    boost::uuids::uuid,
                    std::uint32_t,
                    std::uint32_t,
                    cyng::buffer_t,
                    cyng::param_map_t>(this, &session::pty_push_data_req)),

            cyng::make_description("pty.stop", cyng::vm_adaptor<session, void, std::string, cyng::key_t>(this, &session::pty_stop)),

            cyng::make_description(
                "cfg.backup.init",
                cyng::vm_adaptor<session, void, std::string, cyng::key_t, std::chrono::system_clock::time_point>(
                    this, &session::cfg_backup_init)),

            cyng::make_description(
                "cfg.backup.merge",
                cyng::
                    vm_adaptor<session, void, boost::uuids::uuid, cyng::buffer_t, cyng::buffer_t, cyng::obis_path_t, cyng::object>(
                        this, &session::cfg_backup_merge)),

            cyng::make_description(
                "cfg.backup.finish",
                cyng::vm_adaptor<session, void, boost::uuids::uuid, cyng::buffer_t, std::chrono::system_clock::time_point>(
                    this, &session::cfg_backup_finish)),

            cyng::make_description("sys.msg", cyng::vm_adaptor<db, bool, std::string, cyng::severity>(&cache_, &db::push_sys_msg)),

            cyng::make_description(
                "cluster.send.msg",
                cyng::vm_adaptor<session, void, std::deque<cyng::buffer_t>>(
                    this, &session::cluster_send_msg)), // cluster_send_msg() - internal function

            //"cfg.sml.channel"
            cyng::make_description(
                "cfg.sml.channel",
                cyng::vm_adaptor<
                    session,
                    void,
                    bool,
                    cyng::vector_t,
                    std::string,
                    cyng::obis,
                    cyng::param_map_t,
                    boost::uuids::uuid,
                    boost::uuids::uuid>(this, &session::cfg_sml_channel)));
    }

    void session::do_write() {
        BOOST_ASSERT(!buffer_write_.empty());

        //
        //	write actually data to socket
        //
        boost::asio::async_write(
            socket_,
            boost::asio::buffer(buffer_write_.front().data(), buffer_write_.front().size()),
            cyng::expose_dispatcher(vm_).wrap(std::bind(&session::handle_write, this, std::placeholders::_1)));
    }

    void session::handle_write(const boost::system::error_code &ec) {
        if (!ec) {

            BOOST_ASSERT(!buffer_write_.empty());
            buffer_write_.pop_front();
            if (!buffer_write_.empty()) {
                do_write();
            }
        } else {
            CYNG_LOG_ERROR(logger_, "[session] write: " << ec.message());

            // reset();
        }
    }

    void session::cluster_login(
        std::string name,
        std::string pwd,
        cyng::pid n,
        std::string node,
        boost::uuids::uuid tag,
        std::uint32_t features,
        cyng::version v) {
        CYNG_LOG_INFO(
            logger_,
            "session [" << socket_.remote_endpoint() << "] cluster login " << name << ":" << pwd << "@" << node << " #"
                        << n.get_internal_value() << " v" << v << ", tag: " << tag << ", peer: " << vm_.get_tag());

        //
        //	ToDo: check credentials
        //

        //
        //	insert into cluster table
        // check feature set
        //
        BOOST_ASSERT(!tag.is_nil());
        peer_ = tag;
        protocol_layer_ = node; //  class
        cache_.insert_cluster_member(
            tag, vm_.get_tag(), node, v, socket_.remote_endpoint(), n, (features & CONFIG_MANAGER) == CONFIG_MANAGER);

        //
        //	send response
        //
        cluster_send_msg(cyng::serialize_invoke("cluster.res.login", true));

#ifdef _DEBUG
        if (boost::algorithm::equals(node, "ipt")) {
            send_test_msg_to_setup("ipt is online");
        }
#endif
    }

    void session::cluster_ping(std::chrono::system_clock::time_point tp) {
        auto const delta = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - tp);
        CYNG_LOG_TRACE(logger_, "ping: " << delta);

        //
        //  update "cluster" table
        //
        cache_.update_ping_result(peer_, delta);
    }

    void session::db_req_subscribe(std::string table, boost::uuids::uuid tag) {

        CYNG_LOG_INFO(logger_, "session [" << protocol_layer_ << "@" << socket_.remote_endpoint() << "] subscribe " << table);

        cache_.get_store().connect(table, slot_);
    }
    void session::db_req_insert(
        std::string table_name,
        cyng::key_t key,
        cyng::data_t data,
        std::uint64_t generation,
        boost::uuids::uuid tag) {

        std::reverse(key.begin(), key.end());
        std::reverse(data.begin(), data.end());

        BOOST_ASSERT(config::is_known_store_name(table_name));
        auto const b = cache_.get_store().insert(table_name, key, data, generation, tag);

        if (b) {
            try {
                CYNG_LOG_INFO(
                    logger_,
                    "session [" << socket_.remote_endpoint() << "] req.insert " << table_name << " - " << key << " - " << data);
            } catch (std::exception const &ex) {
                CYNG_LOG_WARNING(logger_, "session " << tag << " is offline: " << ex.what());
            }

            if (boost::algorithm::equals(table_name, "meterIEC")) {
                //
                //  insert/update "gwIEC" too
                //
                db_req_insert_gw_iec(key, tag);
            }
        }
    }

    void session::db_req_insert_gw_iec(cyng::key_t key, boost::uuids::uuid tag) {

        cache_.get_store().access(
            [&](cyng::table const *tbl_meter, cyng::table *tbl_gw) {
                //
                //  get record
                //
                auto const rec_meter = tbl_meter->lookup(key);
                if (!rec_meter.empty()) {
                    auto const host = rec_meter.value("host", "");
                    auto const port = rec_meter.value<std::uint16_t>("port", 0);
                    auto const interval = rec_meter.value("interval", std::chrono::seconds(60 * 60));

                    //
                    // build key
                    //
                    auto const s = config::dependend_key::build_name(host, port);
                    auto const gw_key = dep_key_(host, port);

                    //
                    //  lookup IEC gw
                    //
                    auto const rec_gw = tbl_gw->lookup(gw_key);
                    if (rec_gw.empty()) {
                        if (tbl_gw->insert(
                                gw_key,
                                cyng::data_generator(
                                    host,
                                    port,
                                    static_cast<std::uint32_t>(1), //  meterCounter
                                    static_cast<std::uint32_t>(0), // connectCounter
                                    static_cast<std::uint32_t>(0), // failureCounter
                                    static_cast<std::uint16_t>(0), // state
                                    static_cast<std::uint32_t>(0), // current meter index
                                    std::string("[start]"),        //  current meter id/name
                                    //  interval: at least 1 minute
                                    smooth(interval < std::chrono::seconds(60) ? std::chrono::seconds(60) : interval)),
                                1,
                                tag)) {
                            CYNG_LOG_TRACE(logger_, "insert iec gw " << s << " - " << gw_key);
                        } else {
                            CYNG_LOG_WARNING(logger_, "insert iec gw " << s << " - " << gw_key << " failed");
                        }
                    } else {
                        auto const meter_counter = rec_gw.value<std::uint32_t>("meterCounter", 0) + 1;
                        CYNG_LOG_TRACE(logger_, "update iec gw " << s << " to " << meter_counter << " meters");
                        BOOST_ASSERT(meter_counter > 1u);
                        tbl_gw->modify(gw_key, cyng::make_param("meterCounter", meter_counter), tag);

                        //  at least 1 minute
                        auto const interval_gw = rec_gw.value("interval", std::chrono::seconds(60 * 60));
                        if (interval < interval_gw && (interval > std::chrono::seconds(60))) {
                            CYNG_LOG_TRACE(logger_, "update iec gw " << s << " to " << interval.count() << " seconds pull cycle");
                            tbl_gw->modify(gw_key, cyng::make_param("interval", smooth(interval)), tag);
                        }
                    }
                }
            },
            cyng::access::read("meterIEC"),
            cyng::access::write("gwIEC"));
    }

    void session::db_req_insert_auto(std::string table_name, cyng::data_t data, boost::uuids::uuid tag) {

        BOOST_ASSERT(config::is_known_store_name(table_name));

        std::reverse(data.begin(), data.end());

        try {
            CYNG_LOG_INFO(logger_, "session [" << socket_.remote_endpoint() << "] req.insert.auto " << table_name << " - " << data);
        } catch (std::exception const &ex) {
            CYNG_LOG_WARNING(logger_, "session " << tag << " is offline: " << ex.what());
        }

        cache_.get_store().insert_auto(table_name, std::move(data), tag);

        //
        //	check table size
        //
        // init_sys_msg();
        // init_LoRa_uplink();
        // init_iec_uplink();
        // init_wmbus_uplink();
    }

    void session::db_req_update(std::string table_name, cyng::key_t key, cyng::param_map_t data, boost::uuids::uuid source) {

        BOOST_ASSERT(config::is_known_store_name(table_name));

        //
        //	key with multiple columns
        //
        std::reverse(key.begin(), key.end());

        try {
            CYNG_LOG_INFO(
                logger_,
                "session [" << socket_.remote_endpoint() << "] req.update " << table_name << " - " << key << " = " << data);
        } catch (std::exception const &ex) {
            CYNG_LOG_WARNING(logger_, "session " << source << " is offline: " << ex.what());
        }

        if (boost::algorithm::equals(table_name, "gwIEC")) {
            //
            //  Since we are looking for meters with the same host and port we have to use the "old" values
            //  to find them.
            //
            for (auto const &pm : data) {
                if (boost::algorithm::equals(pm.first, "host") || boost::algorithm::equals(pm.first, "port")) {
                    db_req_update_meter_iec_host_or_port(key, pm, source);
                }
            }
        }

        auto const b = cache_.get_store().modify(table_name, key, data, source);

        if (b && boost::algorithm::equals(table_name, "meterIEC")) {
            //
            //  update "gwIEC" too
            //
            db_req_update_gw_iec(key, data, source);
        } else if (b && boost::algorithm::equals(table_name, "gwIEC")) {
            //
            //  update "meterIEC" too
            //
            for (auto const &pm : data) {
                if (boost::algorithm::equals(pm.first, "interval")) {
                    db_req_update_meter_iec(key, data, source);
                }
            }

            db_req_update_meter_iec(key, data, source);

        } else if (b && boost::algorithm::equals(table_name, "config")) {
            //
            //  propagate some special values into other tables
            //
            auto const pos = data.find("value");
            if (!key.empty() && pos != data.end()) {
                auto const name = cyng::value_cast<std::string>(key.at(0), "");
                if (boost::algorithm::equals(name, "def-IEC-interval")) {
                    auto const interval = std::chrono::seconds(cyng::numeric_cast<std::uint32_t>(pos->second, 20) * 60);
                    CYNG_LOG_INFO(
                        logger_,
                        "[session] " << protocol_layer_ << " overwrite IEC readout interval to " << (interval.count() / 60)
                                     << " minutes");
                    update_iec_interval(interval, source);
                }
            }
        }
    }

    void session::update_iec_interval(std::chrono::seconds interval, boost::uuids::uuid source) {
        cache_.get_store().access(
            [&](cyng::table *tbl_gw) {
                cyng::key_list_t to_update;
                tbl_gw->loop([&](cyng::record &&rec, std::size_t) {
                    to_update.emplace(rec.key());
                    return true;
                });

                for (auto const &key : to_update) {
                    tbl_gw->modify(key, cyng::make_param("interval", interval), source);
                }
            },
            cyng::access::write("gwIEC"));

        cache_.get_store().access(
            [&](cyng::table *tbl_meter) {
                cyng::key_list_t to_update;
                tbl_meter->loop([&](cyng::record &&rec, std::size_t) {
                    to_update.emplace(rec.key());
                    return true;
                });

                for (auto const &key : to_update) {
                    tbl_meter->modify(key, cyng::make_param("interval", interval), source);
                }
            },
            cyng::access::write("meterIEC"));
    }

    void session::db_req_update_gw_iec(cyng::key_t key, cyng::param_map_t const &pmap, boost::uuids::uuid source) {

        for (auto const &pm : pmap) {
            if (boost::algorithm::equals(pm.first, "host") || boost::algorithm::equals(pm.first, "port")) {
                //
                //  ToDo: move to other gateway
                //
            }
        }
    }

    void session::db_req_update_meter_iec_host_or_port(cyng::key_t const &key, cyng::param_t const &pm, boost::uuids::uuid source) {
        BOOST_ASSERT(boost::algorithm::equals(pm.first, "host") || boost::algorithm::equals(pm.first, "port"));
        cache_.get_store().access(
            [&](cyng::table const *tbl_gw, cyng::table *tbl_meter) {
                //
                //  get record
                //
                auto const rec_gw = tbl_gw->lookup(key);
                if (!rec_gw.empty()) {
                    auto const host = rec_gw.value("host", "");
                    auto const port = rec_gw.value<std::uint16_t>("port", 0);

                    //
                    //  loop over "meterIEC" and collect all affected IEC meters
                    //
                    cyng::key_list_t to_update;

                    tbl_meter->loop([&](cyng::record &&rec, std::size_t) -> bool {
                        auto const host_tmp = rec.value("host", "");
                        auto const port_tmp = rec.value<std::uint16_t>("port", 0);

                        if (boost::algorithm::equals(host, host_tmp) && (port == port_tmp)) {
                            //
                            //  match
                            //
                            to_update.emplace(rec.key());
                        }
                        return true;
                    });

                    for (auto const &key_meter : to_update) {
                        tbl_meter->modify(key_meter, pm, source);
                    }
                }
            },
            cyng::access::read("gwIEC"),
            cyng::access::write("meterIEC"));
    }

    void session::db_req_update_meter_iec(cyng::key_t key, cyng::param_map_t const &pmap, boost::uuids::uuid source) {
        //
        //  find all affected meters
        //
        cache_.get_store().access(
            [&](cyng::table const *tbl_gw, cyng::table *tbl_meter) {
                //
                //  get record
                //
                auto const rec_gw = tbl_gw->lookup(key);
                if (!rec_gw.empty()) {
                    auto const host = rec_gw.value("host", "");
                    auto const port = rec_gw.value<std::uint16_t>("port", 0);
                    auto const interval = rec_gw.value("interval", std::chrono::seconds(60 * 60));

                    //
                    //  loop over "meterIEC" and collect all affected IEC meters
                    //
                    cyng::key_list_t to_update;

                    tbl_meter->loop([&](cyng::record &&rec, std::size_t) -> bool {
                        auto const host_tmp = rec.value("host", "");
                        auto const port_tmp = rec.value<std::uint16_t>("port", 0);

                        if (boost::algorithm::equals(host, host_tmp) && (port == port_tmp)) {
                            //
                            //  match
                            //
                            to_update.emplace(rec.key());
                        }
                        return true;
                    });
                    //
                    //  update values
                    //
                    CYNG_LOG_TRACE(
                        logger_,
                        "session [" << protocol_layer_ << "] req.update " << to_update.size()
                                    << " affected IEC meters: " << cyng::to_string(pmap));
                    for (auto const &key_meter : to_update) {
                        for (auto const &pm : pmap) {
                            if (boost::algorithm::equals(pm.first, "interval")) {
                                //
                                //  update: all columns have the same name.
                                //  No re-naming required
                                //
                                tbl_meter->modify(key_meter, pm, source);
                            }
                        }
                    }
                }
            },
            cyng::access::read("gwIEC"),
            cyng::access::write("meterIEC"));
    }

    void session::db_req_remove(std::string table_name, cyng::key_t key, boost::uuids::uuid source) {

        BOOST_ASSERT(config::is_known_store_name(table_name));

        std::reverse(key.begin(), key.end());
        if (boost::algorithm::equals(table_name, "meterIEC")) {
            //
            //  remove "gwIEC" record if no gateway has no longer any meters
            //
            db_req_remove_gw_iec(key, source);
        } else if (cache_.get_store().erase(table_name, key, source)) {

            CYNG_LOG_TRACE(logger_, "remove [" << table_name << '/' << key << "] ok");

        } else {
            CYNG_LOG_WARNING(logger_, "remove [" << table_name << '/' << key << "] failed");

#ifdef _DEBUG
            cache_.get_store().access(
                [&](cyng::table const *tbl) {
                    tbl->loop([&](cyng::record &&rec, std::size_t) -> bool {
                        CYNG_LOG_DEBUG(logger_, "search " << key << " - " << rec.to_string());

                        return true;
                    });
                },
                cyng::access::read(table_name));
#endif
        }
    }

    void session::db_req_remove_gw_iec(cyng::key_t key, boost::uuids::uuid source) {
        //  remove "gwIEC" record if no gateway has no longer any meters
        cache_.get_store().access(
            [&](cyng::table *tbl_meter, cyng::table *tbl_gw) {
                //
                //  get record
                //
                auto const rec_meter = tbl_meter->lookup(key);
                if (!rec_meter.empty()) {
                    auto const host = rec_meter.value("host", "");
                    auto const port = rec_meter.value<std::uint16_t>("port", 0);

                    //
                    //  are there more IEC meters with the same host / port configuration
                    //
                    std::size_t counter{0};
                    tbl_meter->loop([&](cyng::record &&rec, std::size_t) -> bool {
                        auto const host_tmp = rec.value("host", "");
                        auto const port_tmp = rec.value<std::uint16_t>("port", 0);

                        if (boost::algorithm::equals(host, host_tmp) && (port == port_tmp)) {
                            counter++;
                        }

                        //
                        //  this record and one record more with the same host and port
                        // configuration
                        //
                        return counter > 1;
                    });

                    //
                    //  remove IEC meter
                    //
                    tbl_meter->erase(key, source);

                    if (counter < 2) {
                        CYNG_LOG_WARNING(logger_, "iec gateway " << host << ':' << port << " has no more meters");

                        //
                        // build key for "gwIEC"
                        //
                        auto const s = config::dependend_key::build_name(host, port);
                        auto const gw_key = dep_key_(host, port);
                        tbl_gw->erase(gw_key, source);
                    }
                }
            },
            cyng::access::write("meterIEC"),
            cyng::access::write("gwIEC"));
    }

    void session::db_req_clear(std::string table_name, boost::uuids::uuid source) { cache_.get_store().clear(table_name, source); }

    void session::cluster_send_msg(std::deque<cyng::buffer_t> msg) {
        cyng::exec(vm_, [=, this]() {
            bool const b = buffer_write_.empty();
            CYNG_LOG_DEBUG(logger_, "send " << msg.size() << " bytes cluster message to peer " << vm_.get_tag());
            cyng::add(buffer_write_, msg);
            if (b) {
                do_write();
            }
        });
    }

    void session::send_ping_request() {
        cluster_send_msg(cyng::serialize_invoke("cluster.req.ping", std::chrono::system_clock::now()));
    }

    void session::cluster_test_msg(std::string msg) {
        CYNG_LOG_TRACE(logger_, "cluster_test_msg: " << msg);
        cluster_send_msg(cyng::serialize_invoke("cluster.test.msg", msg));
    }

#ifdef _DEBUG
    void session::send_test_msg_to_setup(std::string msg) {
        cache_.get_store().access(
            [&, this](cyng::table const *tbl) {
                tbl->loop([&, this](cyng::record &&rec, std::size_t) {
                    if (rec.value("cfg", false)) {
                        // cfg flag is set
                        auto const peer = rec.value("peer", boost::uuids::nil_uuid());
                        BOOST_ASSERT(peer != boost::uuids::nil_uuid());
                        CYNG_LOG_TRACE(logger_, "send test message to " << rec.value("class", "") << " [" << peer << "]: " << msg);
                        //  the following two lines work
                        // vm_.load(cyng::generate_forward("cluster.test.msg", peer, msg));
                        // vm_.run();

                        //  This works too
                        auto prg =
                            cyng::generate_forward("cluster.send.msg", peer, cyng::serialize_invoke("cluster.test.msg", msg));
                        vm_.load(std::move(prg));
                        vm_.run();
                    }
                    return true;
                });
            },
            cyng::access::read("cluster"));
    }
#endif

    //
    //	slot implementation
    //

    session::slot::slot(session *sp)
        : sp_(sp) {}

    bool session::slot::forward(
        cyng::table const *tbl,
        cyng::key_t const &key,
        cyng::data_t const &data,
        std::uint64_t gen,
        boost::uuids::uuid source) {
        //
        //	send to subscriber
        //
        CYNG_LOG_TRACE(sp_->logger_, "forward insert [" << tbl->meta().get_name() << '/' << sp_->protocol_layer_ << "]");

        sp_->cluster_send_msg(cyng::serialize_invoke("db.res.insert", tbl->meta().get_name(), key, data, gen, source));

        return true;
    }

    bool session::slot::forward(
        cyng::table const *tbl,
        cyng::key_t const &key,
        cyng::attr_t const &attr,
        cyng::data_t const &data,
        std::uint64_t gen,
        boost::uuids::uuid source) {
        //
        //	send update to subscriber
        //
        CYNG_LOG_TRACE(
            sp_->logger_,
            "forward update [" << tbl->meta().get_name() << '/' << sp_->protocol_layer_ << "] " << attr.first << " => "
                               << attr.second);

        sp_->cluster_send_msg(cyng::serialize_invoke("db.res.update", tbl->meta().get_name(), key, attr, gen, source));

        return true;
    }

    bool
    session::slot::forward(cyng::table const *tbl, cyng::key_t const &key, cyng::data_t const &data, boost::uuids::uuid source) {
        //
        //	send remove to subscriber
        //
        CYNG_LOG_TRACE(sp_->logger_, "forward remove [" << tbl->meta().get_name() << '/' << sp_->protocol_layer_ << "]");

        sp_->cluster_send_msg(cyng::serialize_invoke("db.res.remove", tbl->meta().get_name(), key, source));

        return true;
    }

    bool session::slot::forward(cyng::table const *tbl, boost::uuids::uuid source) {
        //
        //	send clear to subscriber
        //

        CYNG_LOG_TRACE(sp_->logger_, " forward clear [" << tbl->meta().get_name() << '/' << sp_->protocol_layer_ << "]");

        sp_->cluster_send_msg(cyng::serialize_invoke("db.res.clear", tbl->meta().get_name(), source));

        return true;
    }

    bool session::slot::forward(cyng::table const *tbl, bool trx) {
        CYNG_LOG_TRACE(sp_->logger_, "forward trx [" << tbl->meta().get_name() << "] " << (trx ? "start" : "commit"));

        sp_->cluster_send_msg(cyng::serialize_invoke("db.res.trx", tbl->meta().get_name(), trx));

        return true;
    }

    void session::pty_login(
        boost::uuids::uuid rtag,
        std::string name,
        std::string pwd,
        boost::asio::ip::tcp::endpoint ep,
        std::string data_layer) {
        CYNG_LOG_INFO(logger_, "pty login " << name << ':' << pwd << '@' << ep);

        //
        //	check credentials and get associated device
        //
        auto const [dev, enabled] = cache_.lookup_device(name, pwd);
        if (!dev.is_nil()) {

            //
            //	ToDo: check if there is already a session of this pty
            //

            //
            //	insert into session table
            //
            cache_.insert_pty(
                dev,           //  pk
                vm_.get_tag(), //  peer
                rtag,
                name,
                pwd,
                ep,
                data_layer);

            //
            //	send response
            //
            cluster_send_msg(cyng::serialize_forward("pty.res.login", rtag, true, dev));

            //
            //	update cluster table (pty counter)
            //
            auto const counter = cache_.update_pty_counter(vm_.get_tag(), peer_);
#ifdef _DEBUG_MAIN
            cache_.sys_msg(cyng::severity::LEVEL_TRACE, protocol_layer_, "has", counter, "users");
#else
            boost::ignore_unused(counter);
#endif

        } else {
            CYNG_LOG_WARNING(logger_, "pty login [" << name << "] failed");
            cache_.sys_msg(cyng::severity::LEVEL_TRACE, data_layer, "login [", name, ":", pwd, "] failed");

            //
            //	send response
            //
            cluster_send_msg(cyng::serialize_forward("pty.res.login", rtag, false, boost::uuids::nil_uuid())); //	no
                                                                                                               // device

            //	check auto insert
            auto const auto_login = cache_.get_cfg().get_value("auto-login", false);
            if (auto_login) {
                CYNG_LOG_INFO(logger_, "auto-login is ON - insert [" << name << "] into device table");
                auto const auto_enabled = cache_.get_cfg().get_value("auto-enabled", true);
                cache_.insert_device(rtag, name, pwd, auto_enabled);
            }
        }
    }

    void session::pty_logout(boost::uuids::uuid tag, boost::uuids::uuid dev) {
        CYNG_LOG_INFO(logger_, "pty " << protocol_layer_ << " logout [l-vm:" << vm_.get_tag() << "]");

        //
        //	Remove session table, targets and channels.
        //  connections holds a list of sessions connected to this session.
        //
        auto const [connection_key, success] = cache_.remove_pty(tag, dev);
        if (!success) {
            CYNG_LOG_WARNING(logger_, "pty " << protocol_layer_ << " logout [" << dev << "] failed");
        } else {

            //
            //	remove open connections
            //
            if (!connection_key.empty()) {
                auto const [rtag, peer] = cache_.get_access_params(connection_key);
                // BOOST_ASSERT(peer == vm_.get_tag()); //	ToDo: correct implementation later
                //"pty.req.close.connection"
                cluster_send_msg(cyng::serialize_forward("pty.req.close.connection", rtag));
            }

            //
            //	update cluster table (pty counter)
            //
            auto const counter = cache_.update_pty_counter(vm_.get_tag(), peer_);
#ifdef _DEBUG_MAIN
            cache_.sys_msg(cyng::severity::LEVEL_TRACE, protocol_layer_, "has", counter, "users");
#else
            boost::ignore_unused(counter);
#endif
        }
    }

    void session::pty_open_connection(boost::uuids::uuid tag, boost::uuids::uuid dev, std::string msisdn, cyng::param_map_t token) {

        CYNG_LOG_INFO(logger_, "pty " << protocol_layer_ << " {" << tag << "} connects => " << msisdn);

        //
        //	forward to callee
        //	(1) session already connected,
        //	(2) remote session online, enabled and not connected
        //	(3) remote session vm-tag
        //  (4) remote session tag (session to address with FORWARD op)
        //  (5) caller name
        //  (6) callee name
        //
        auto const [connected, online, vm_key, remote, caller, callee] = cache_.lookup_msisdn(msisdn, dev);

        //
        //	don't call itself
        //
        if (!connected && online) {

            BOOST_ASSERT_MSG(remote != tag, "caller and callee with same tag");

            //
            //	forward connection request
            //
            CYNG_LOG_TRACE(logger_, "pty " << msisdn << " is online: " << callee);

            //	data to identify caller
            token.emplace("caller-tag", cyng::make_object(tag));            //  this is the remote/caller VM tag
            token.emplace("caller-peer", cyng::make_object(vm_.get_tag())); //  this is the local/caller VM tag
            token.emplace("dev", cyng::make_object(dev));                   //  this is the remote/caller session tag
            token.emplace("callee-peer", cyng::make_object(vm_key));        //  remote local VM tag
            token.emplace("caller", cyng::make_object(caller));
            token.emplace("callee", cyng::make_object(callee));
            auto const local = vm_key == vm_.get_tag();
            token.emplace("local", cyng::make_object(local));

#ifdef _DEBUG_MAIN
            CYNG_LOG_DEBUG(
                logger_,
                "pty open connection"
                    << "\ndev\t\t: " << dev << "\ncaller-r-vm\t: " << tag << "\nl-peer\t: " << vm_.get_tag()
                    << "\ncaller\t\t: " << caller << "\nr-peer\t: " << vm_key << "\nlocal\t\t: " << (local ? "Y" : "N"));
#endif

            //
            //	send to next VM in the fabric.
            //  So parties on different sessions/VMs can communicate with each other
            //
            vm_.load(cyng::generate_forward(
                "cluster.send.msg", vm_key, cyng::serialize_forward("pty.req.open.connection", remote, msisdn, local, token)));
            vm_.run();

        } else {

            CYNG_LOG_TRACE(logger_, "pty " << msisdn << " is offline / connected");

            //
            //	session offline or already connected
            //
            cluster_send_msg(cyng::serialize_forward("pty.res.open.connection", tag, false, token)); //	failed
        }
    }

    //  "pty.forward.open.connection"
    void session::pty_forward_res_open_connection(boost::uuids::uuid caller_tag, bool success, cyng::param_map_t token) {
        cluster_send_msg(cyng::serialize_forward("pty.res.open.connection", caller_tag, success, token));
    }

    void session::pty_return_open_connection(
        bool success,
        boost::uuids::uuid dev,    //	callee dev-tag
        boost::uuids::uuid callee, //	callee vm-tag
        cyng::param_map_t token) {

        //
        //  reply to "pty.req.open.connection"
        //
        CYNG_LOG_DEBUG(logger_, "pty establish connection vm:" << vm_.get_tag() << ", token: " << token);
        CYNG_LOG_DEBUG(logger_, "pty establish connection callee dev-tag:" << dev << ", callee vm-tag: " << callee);

        auto const reader = cyng::make_reader(token);
        auto const caller_tag = cyng::value_cast(reader["caller-tag"].get(), boost::uuids::nil_uuid());
        auto const caller_vm = cyng::value_cast(reader["caller-peer"].get(), boost::uuids::nil_uuid());
        auto const caller_dev = cyng::value_cast(reader["dev"].get(), boost::uuids::nil_uuid());
        auto const caller_name = cyng::value_cast(reader["caller"].get(), "");

        auto const callee_vm = cyng::value_cast(reader["callee-peer"].get(), boost::uuids::nil_uuid());
        auto const callee_name = cyng::value_cast(reader["callee"].get(), "");
        auto const local = cyng::value_cast(reader["local"].get(), false);

        CYNG_LOG_INFO(
            logger_,
            "pty establish connection " << caller_name << " <-" << (local ? "L" : "R") << "-> " << callee_name << " "
                                        << (success ? "ok" : "failed"));

#ifdef _DEBUG_MAIN
        CYNG_LOG_DEBUG(
            logger_,
            "pty establish connection "
                << "\ndev\t\t: " << dev << "\ncallee\t\t: " << callee << "\ncaller_tag\t: " << caller_tag
                << "\ncaller_vm\t: " << caller_vm << "\ncaller_dev\t: " << caller_dev << "\ncaller_name\t: " << caller_name
                << "\ncallee_vm\t: " << callee_vm << "\ncallee_name\t: " << callee_name << "\nlocal\t\t: " << (local ? "Y" : "N"));
#endif

        BOOST_ASSERT_MSG(caller_dev != dev, "same dev tag");
        BOOST_ASSERT_MSG(caller_tag != callee, "same session tag");
        BOOST_ASSERT_MSG(caller_vm == vm_.get_tag(), "wrong vm");

        //
        //	update "connection" table
        //
        cache_.establish_connection(
            caller_tag,
            caller_vm,
            caller_dev,
            caller_name,
            callee_name,
            callee,    //	callee tag
            dev,       //	callee dev-tag
            callee_vm, //	callee vm-tag
            local);

        //
        //	forward response to [ip-t] node
        //
        if (local) {
            BOOST_ASSERT(caller_vm == callee_vm);
            cluster_send_msg(cyng::serialize_forward("pty.res.open.connection", caller_tag, success, token)); //	complete
        } else {
            //  caller on different VM
            vm_.load(cyng::generate_forward("pty.forward.res.open.connection", caller_vm, caller_tag, success, token));
            vm_.run();
        }
    }

    void session::pty_transfer_data(boost::uuids::uuid tag, boost::uuids::uuid dev, cyng::buffer_t data) {

        //  * key of remote session
        //  * uuid or remote session
        //  * uuid of remote peer/vm
        auto const [rdev, rtag, rvm] = cache_.get_remote(dev);
        if (!rtag.is_nil()) {

            BOOST_ASSERT(tag != rtag);
            auto const size = data.size();
            CYNG_LOG_TRACE(logger_, "pty transfer " << size << " bytes to " << rtag);

            auto const local = vm_.get_tag() == rvm;
#ifdef _DEBUG_MAIN
            CYNG_LOG_DEBUG(
                logger_,
                "pty transfer data"
                    << "\nr-vm\t: " << tag << "\ndev\t: " << dev << "\nl-vm\t: " << vm_.get_tag() << "\nr-dev\t: " << rdev
                    << "\nrtag\t: " << rtag << "\nr-vm\t: " << rvm << "\nlocal\t: " << (local ? "Y" : "N"));
#endif

            //
            //	select VM
            //
            vm_.load(cyng::generate_forward("cluster.send.msg", rvm, cyng::serialize_forward("pty.transfer.data", rtag, data)));
            vm_.run();

            //
            //  get "connection" key
            //
            cache_.update_connection_throughput(rdev, dev, size);

        } else {
            CYNG_LOG_WARNING(logger_, "pty " << protocol_layer_ << " is not connected {" << dev << "}");
        }
    }

    void session::pty_close_connection(boost::uuids::uuid tag, boost::uuids::uuid dev, cyng::param_map_t token) {
        CYNG_LOG_INFO(logger_, "pty " << protocol_layer_ << " disconnect {" << tag << "}");

        //
        //   remove connection from connection list
        //
        auto const [rdev, rtag, rvm, local] = cache_.clear_connection(dev);

        //
        //  ack closed connection to caller
        //
        cluster_send_msg(cyng::serialize_forward("pty.res.close.connection", tag, true, token)); //	success

        //
        //  forward to remote
        //
        vm_.load(cyng::generate_forward("cluster.send.msg", rvm, cyng::serialize_forward("pty.req.close.connection", rtag)));
        vm_.run();
    }

    void session::pty_register_target(
        boost::uuids::uuid tag,
        boost::uuids::uuid dev,
        std::string name,
        std::uint16_t paket_size,
        std::uint8_t window_size,
        cyng::param_map_t token) {
        BOOST_ASSERT(tag != dev);

        // std::pair<std::uint32_t, bool>
        auto const [channel, success] = cache_.register_target(tag, dev, vm_.get_tag(), name, paket_size, window_size);
        if (success) {
            CYNG_LOG_INFO(logger_, "pty " << protocol_layer_ << " registered target " << name << " {" << tag << "}");
            cache_.sys_msg(cyng::severity::LEVEL_TRACE, protocol_layer_, "target[", name, "#", channel, "] registered");

            cluster_send_msg(cyng::serialize_forward("pty.res.register", tag, true, channel, token)); //	ok

        } else {
            CYNG_LOG_WARNING(logger_, "pty " << protocol_layer_ << " registering target " << name << " {" << tag << "} failed");
            cluster_send_msg(cyng::serialize_forward("pty.res.register", tag, false, channel, token)); //	failed
        }
    }

    void session::pty_deregister(boost::uuids::uuid, std::string name) {
        CYNG_LOG_INFO(logger_, "pty " << protocol_layer_ << " deregister [" << name << "]");
    }

    void session::pty_open_channel(
        boost::uuids::uuid tag,
        boost::uuids::uuid dev,
        std::string name,
        std::string account,
        std::string number,
        std::string sv,
        std::string id,
        std::chrono::seconds timeout,
        cyng::param_map_t token) {
        if (name.empty()) {
            CYNG_LOG_WARNING(logger_, "no target specified");
            cache_.sys_msg(cyng::severity::LEVEL_WARNING, protocol_layer_, "no target specified");

            cluster_send_msg(cyng::serialize_forward(
                "pty.res.open.channel",
                tag,
                false,
                static_cast<std::uint32_t>(0), //	channel
                static_cast<std::uint32_t>(0), //	source
                static_cast<std::uint16_t>(0), //	packet size
                static_cast<std::uint8_t>(1),  //	window size
                static_cast<std::uint8_t>(0),  //	status
                static_cast<std::uint32_t>(0), //	count
                token));                       //	ok

        } else {

            auto const [channel, source, packet_size, count, device] =
                cache_.open_channel(dev, name, account, number, sv, id, timeout);

            if (count == 0) {
                cache_.sys_msg(cyng::severity::LEVEL_WARNING, protocol_layer_, "target [", name, "] not found");
                CYNG_LOG_WARNING(logger_, "pty " << protocol_layer_ << "target [" << name << "] not found");
            } else {
                CYNG_LOG_INFO(
                    logger_,
                    "pty " << device << '/' << protocol_layer_ << " open channel [" << name << "] " << channel << ':' << source);
            }

            cluster_send_msg(cyng::serialize_forward(
                "pty.res.open.channel",
                tag,
                (count != 0),                 //	success
                channel,                      //	channel
                source,                       //	source
                packet_size,                  //	packet size
                static_cast<std::uint8_t>(1), //	window size
                static_cast<std::uint8_t>(0), //	status
                count,                        //	count
                token));                      //	ok
        }
    }

    void
    session::pty_close_channel(boost::uuids::uuid tag, boost::uuids::uuid dev, std::uint32_t channel, cyng::param_map_t token) {
        CYNG_LOG_INFO(logger_, "pty " << protocol_layer_ << " close channel [#" << channel << "] " << token);
        auto const count = cache_.close_channel(channel);

        cluster_send_msg(cyng::serialize_forward(
            "pty.res.close.channel",
            tag,
            (count != 0), //	success
            channel,      //	channel
            count,        //	count
            token));      //	ok
    }

    void session::pty_push_data_req(
        boost::uuids::uuid tag,
        boost::uuids::uuid dev,
        std::uint32_t channel,
        std::uint32_t source,
        cyng::buffer_t data,
        cyng::param_map_t token) {
        CYNG_LOG_INFO(
            logger_,
            "pty " << protocol_layer_ << " push data " << data.size() << " bytes [#" << channel << ":" << source << "] " << token);

        //
        //	distribute data to push target(s)
        //
        auto const vec = cache_.get_matching_channels(channel, data.size());
        for (auto const &pty : vec) {

            //
            //	forward data (multiple times)
            //  substitute channel with target id (pty.channel_)
            //
            CYNG_LOG_DEBUG(
                logger_,
                "pty " << protocol_layer_ << " push data to " << pty.pty_.first << ", " << pty.pty_.second << " - " << channel
                       << " <-> " << pty.channel_);

            cluster_send_msg(cyng::serialize_forward("pty.req.push.data", pty.pty_.first, pty.channel_, source, data));
        }

        cluster_send_msg(cyng::serialize_forward(
            "pty.push.data.res",
            tag,
            !vec.empty(), //	success
            channel,
            source,
            token));
    }

    void session::pty_stop(std::string table_name, cyng::key_t key) {

        BOOST_ASSERT(config::is_known_store_name(table_name));
        BOOST_ASSERT(key.size() == 1);

        if (key.size() == 1) {

            auto const tag = cyng::value_cast(key.at(0), boost::uuids::nil_uuid());

            cache_.get_store().access(
                [&](cyng::table const *tbl_session) {
                    auto const rec = tbl_session->lookup(key);
                    if (!rec.empty()) {
                        auto const rtag = rec.value("rTag", boost::uuids::nil_uuid());
                        auto const peer = rec.value("peer", boost::uuids::nil_uuid());

                        CYNG_LOG_INFO(
                            logger_,
                            "session [" << socket_.remote_endpoint() << "] will stop " << table_name << ": " << rtag << " on peer "
                                        << peer);

                        //
                        //  forward to node/pty
                        //
                        vm_.load(cyng::generate_forward("cluster.send.msg", peer, cyng::serialize_invoke("pty.req.stop", rtag)));
                        vm_.run();

                    } else {
                        CYNG_LOG_ERROR(
                            logger_, "session [" << socket_.remote_endpoint() << "] cannot stop " << table_name << ": " << tag);
                    }
                },
                cyng::access::read(table_name));

            //
            //  execute loaded program
            //
            vm_.run();
        }
    }

    void session::cfg_backup_init(std::string table_name, cyng::key_t key, std::chrono::system_clock::time_point tp) {
        CYNG_LOG_INFO(logger_, "init backup \"" << table_name << "\": " << key);

        BOOST_ASSERT(!key.empty());
        BOOST_ASSERT(config::is_known_store_name(table_name));
        BOOST_ASSERT(boost::algorithm::equals(table_name, "gateway"));

        //
        // * search session table for a matching gateway
        // * find peer and rTag
        // * forward backup request to (ipt) node
        //
        if (boost::algorithm::equals(table_name, "gateway")) {
            cache_.locate_gateway(
                key,
                [&, this](
                    boost::uuids::uuid rtag,
                    boost::uuids::uuid peer,
                    std::string operator_name,
                    std::string operator_pwd,
                    boost::uuids::uuid tag,
                    cyng::buffer_t id,
                    std::string fw,
                    std::string name,
                    std::string msisdn) -> void {
                    CYNG_LOG_INFO(
                        logger_,
                        "session [" << socket_.remote_endpoint() << "] will backup gateway: " << name << '@' << msisdn
                                    << " on peer " << peer);

                    vm_.load(cyng::generate_forward(
                        "cluster.send.msg",
                        peer,
                        cyng::serialize_forward("cfg.req.backup", rtag, operator_name, operator_pwd, tag, id, fw, tp)));
                    //
                    //  execute loaded program
                    //
                    vm_.run();
                });

        } else {
            CYNG_LOG_ERROR(
                logger_, "session [" << socket_.remote_endpoint() << "] backup of \"" << table_name << "\" is not supported");
        }
    }

    void session::cfg_backup_merge(
        boost::uuids::uuid tag,
        cyng::buffer_t gw,
        cyng::buffer_t meter,
        cyng::obis_path_t path,
        cyng::object value) {
        //
        //  distribute to configuration manager
        //
        cache_.get_store().access(
            [&, this](cyng::table const *tbl) {
                tbl->loop([&, this](cyng::record &&rec, std::size_t) {
                    if (rec.value("cfg", false)) {
                        // cfg_mgr.emplace(rec.key());
                        auto const peer = rec.value("peer", boost::uuids::nil_uuid());
                        BOOST_ASSERT(peer != boost::uuids::nil_uuid());
                        CYNG_LOG_TRACE(
                            logger_,
                            "merge backup " << rec.value("class", "") << ": " << tag << ", " << gw << ", " << meter << ", " << path
                                            << ", " << value);

                        vm_.load(cyng::generate_forward(
                            "cluster.send.msg", peer, cyng::serialize_invoke("cfg.backup.merge", tag, gw, meter, path, value)));

                        vm_.run();
                    }
                    return true;
                });
            },
            cyng::access::read("cluster"));
    }

    void session::cfg_backup_finish(boost::uuids::uuid tag, cyng::buffer_t gw, std::chrono::system_clock::time_point tp) {
        CYNG_LOG_INFO(logger_, "session [" << socket_.remote_endpoint() << "] cfg.backup.finish: " << gw);
        //
        //  distribute to configuration manager
        //
        cache_.get_store().access(
            [&, this](cyng::table const *tbl) {
                tbl->loop([&, this](cyng::record &&rec, std::size_t) {
                    if (rec.value("cfg", false)) {
                        // cfg_mgr.emplace(rec.key());
                        auto const peer = rec.value("peer", boost::uuids::nil_uuid());
                        BOOST_ASSERT(peer != boost::uuids::nil_uuid());
                        CYNG_LOG_TRACE(logger_, "backup meta data " << rec.value("class", "") << ": " << tag << ", " << gw);

                        vm_.load(cyng::generate_forward(
                            "cluster.send.msg", peer, cyng::serialize_invoke("cfg.backup.finish", tag, gw, tp)));

                        vm_.run();
                    }
                    return true;
                });
            },
            cyng::access::read("cluster"));
    }

    void session::cfg_sml_channel(
        bool direction,                 //  true == out, false == back
        cyng::vector_t key,             //  gateway
        std::string channel,            //  SML message type
        cyng::obis section,             //  OBIS root
        cyng::param_map_t params,       //  optional parameters (OBIS path) or results
        boost::uuids::uuid source,      //  HTTP session
        boost::uuids::uuid tag_cluster) // source node tag (mostly dash)
    {
        CYNG_LOG_INFO(
            logger_,
            "cfg.sml.channel( " << (direction ? "\"outward\"" : "\"backwards\"") << " " << channel << ", " << section << " )");

        if (direction) {
            // CYNG_LOG_DEBUG(logger_, "cfg_sml_channel_out( source " << source << ", tag_cluster " << tag_cluster);
            //   routing to gateway
            cache_.locate_gateway(
                key,
                [=, this](
                    boost::uuids::uuid rtag,
                    boost::uuids::uuid peer,
                    std::string operator_name,
                    std::string operator_pwd,
                    boost::uuids::uuid tag,
                    cyng::buffer_t id,
                    std::string fw,
                    std::string name,
                    std::string msisdn) -> void {
                    CYNG_LOG_INFO(
                        logger_,
                        "session [" << socket_.remote_endpoint() << "] sml channel: " << channel << ", " << section
                                    << ", source: " << source << ", cluster tag: " << tag_cluster);

                    //
                    //  check data consistency
                    //
                    BOOST_ASSERT_MSG(cyng::value_cast(key.front(), peer) == tag, "inconsistent data");

                    vm_.load(cyng::generate_forward(
                        "cluster.send.msg",
                        peer,
                        cyng::serialize_forward(
                            "cfg.sml.channel",
                            rtag,
                            operator_name,
                            operator_pwd,
                            tag, //  same as key
                            id,
                            channel,
                            section,
                            params,
                            source,
                            vm_.get_tag() //  peer
                            // tag_cluster
                            )));
                    //
                    //   execute loaded program
                    //
                    vm_.run();
                });
        } else {

#ifdef _DEBUG
            cache_.get_store().access(
                [&, this](cyng::table const *tbl) {
                    tbl->loop([&, this](cyng::record &&rec, std::size_t) {
                        CYNG_LOG_DEBUG(
                            logger_, "cfg.data.sml " << source << ", r-peer " << tag_cluster << " - " << rec.to_string());
                        return true;
                    });
                },
                cyng::access::read("cluster"));
#endif

            //  routing to source
            vm_.load(cyng::generate_forward(
                "cluster.send.msg",
                tag_cluster, //  peer
                cyng::serialize_invoke(
                    "cfg.data.sml",
                    source,  // HTTP session
                    key,     // table key (gateway)
                    channel, // SML message type
                    section, // OBIS root
                    params   // results
                    )));

            // execute loaded program
            vm_.run();
        }
    }

    std::chrono::seconds smooth(std::chrono::seconds interval) {
        auto const mod = interval.count() % 300;
        return std::chrono::seconds(interval.count() + (300 - mod));
    }

} // namespace smf
