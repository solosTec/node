/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <db.h>
#include <session.h>
#include <tasks/ping.h>

#include <cyng/io/ostream.h>
#include <cyng/io/serialize.h>
#include <cyng/log/record.h>
#include <cyng/obj/algorithm/add.hpp>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/container_factory.hpp>
#include <cyng/obj/numeric_cast.hpp>
#include <cyng/vm/generator.hpp>
#include <cyng/vm/linearize.hpp>
#include <cyng/vm/mesh.h>
#include <cyng/vm/vm.h>

#include <iostream>

namespace smf {

    session::session(boost::asio::ip::tcp::socket socket, db &cache, cyng::mesh &fabric, cyng::logger logger)
        : socket_(std::move(socket))
        , cache_(cache)
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
        , dep_key_() {
        vm_ = init_vm(fabric);
        std::size_t slot{0};
        vm_.set_channel_name("cluster.req.login", slot++);           //	get_vm_func_cluster_req_login
        vm_.set_channel_name("db.req.subscribe", slot++);            //	get_vm_func_db_req_subscribe
        vm_.set_channel_name("db.req.insert", slot++);               //	get_vm_func_db_req_insert
        vm_.set_channel_name("db.req.insert.auto", slot++);          //	get_vm_func_db_req_insert_auto
        vm_.set_channel_name("db.req.update", slot++);               //	table modify()
        vm_.set_channel_name("db.req.remove", slot++);               //	table erase()
        vm_.set_channel_name("db.req.clear", slot++);                //	table clear()
        vm_.set_channel_name("pty.req.login", slot++);               //	get_vm_func_pty_login
        vm_.set_channel_name("pty.req.logout", slot++);              //	get_vm_func_pty_logout
        vm_.set_channel_name("pty.open.connection", slot++);         //	get_vm_func_pty_open_connection
        vm_.set_channel_name("pty.forward.open.connection", slot++); //	get_vm_func_pty_forward_open_connection
        vm_.set_channel_name("pty.return.open.connection", slot++);  //	get_vm_func_pty_return_open_connection
        vm_.set_channel_name("pty.transfer.data", slot++);           //	get_vm_func_pty_transfer_data
        vm_.set_channel_name("pty.close.connection", slot++);        //	get_vm_func_pty_close_connection
        vm_.set_channel_name("pty.register.target", slot++);         //	get_vm_func_pty_register_target
        vm_.set_channel_name("pty.deregister", slot++);              //	get_vm_func_pty_deregister
        vm_.set_channel_name("pty.open.channel", slot++);            //	get_vm_func_pty_open_channel
        vm_.set_channel_name("pty.close.channel", slot++);           //	get_vm_func_pty_close_channel
        vm_.set_channel_name("pty.push.data.req", slot++);           //	get_vm_func_pty_push_data_req
        vm_.set_channel_name("sys.msg", slot++);                     //	get_vm_func_sys_msg

        //
        //	start ping
        //
        // auto pt = fabric.get_ctl().create_channel_with_ref<ping>(fabric.get_ctl(), logger_);
        // BOOST_ASSERT(pt->is_open());
    }

    session::~session() {
#ifdef _DEBUG_MAIN
        std::cout << "session(~)" << std::endl;
#endif
    }

    boost::uuids::uuid session::get_peer() const { return vm_.get_tag(); }

    boost::uuids::uuid session::get_remote_peer() const { return peer_; }

    void session::stop() {
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

    void session::start() { do_read(); }

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
                }
            });
    }

    cyng::vm_proxy session::init_vm(cyng::mesh &fabric) {

        return fabric.create_proxy(
            get_vm_func_cluster_req_login(this),
            get_vm_func_db_req_subscribe(this),
            get_vm_func_db_req_insert(this),
            get_vm_func_db_req_insert_auto(this),
            get_vm_func_db_req_update(this),
            get_vm_func_db_req_remove(this),
            get_vm_func_db_req_clear(this),
            get_vm_func_pty_login(this),
            get_vm_func_pty_logout(this),
            get_vm_func_pty_open_connection(this),
            get_vm_func_pty_forward_open_connection(this),
            get_vm_func_pty_return_open_connection(this),
            get_vm_func_pty_transfer_data(this),
            get_vm_func_pty_close_connection(this),
            get_vm_func_pty_register_target(this),
            get_vm_func_pty_deregister(this),
            get_vm_func_pty_open_channel(this),
            get_vm_func_pty_close_channel(this),
            get_vm_func_pty_push_data_req(this),
            get_vm_func_sys_msg(&cache_));
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
        cyng::version v) {
        CYNG_LOG_INFO(
            logger_,
            "session [" << socket_.remote_endpoint() << "] cluster login " << name << ":" << pwd << "@" << node << " #"
                        << n.get_internal_value() << " v" << v);

        //
        //	ToDo: check credentials
        //

        //
        //	insert into cluster table
        //
        BOOST_ASSERT(!tag.is_nil());
        peer_ = tag;
        protocol_layer_ = node;
        cache_.insert_cluster_member(tag, node, v, socket_.remote_endpoint(), n);

        //
        //	send response
        //
        send_cluster_response(cyng::serialize_invoke("cluster.res.login", true));
    }

    void session::db_req_subscribe(std::string table, boost::uuids::uuid tag) {

        CYNG_LOG_INFO(logger_, "session [" << protocol_layer_ << "@" << socket_.remote_endpoint() << "] subscribe " << table);

        cache_.get_store().connect(table, slot_);
    }
    void session::db_req_insert(
        std::string const &table_name,
        cyng::key_t key,
        cyng::data_t data,
        std::uint64_t generation,
        boost::uuids::uuid tag) {

        std::reverse(key.begin(), key.end());
        std::reverse(data.begin(), data.end());

        auto const b = cache_.get_store().insert(table_name, key, data, generation, tag);

        if (b) {
            try {
                CYNG_LOG_INFO(logger_, "session [" << socket_.remote_endpoint() << "] req.insert " << table_name << " - " << data);
            } catch (std::exception const &ex) {
                CYNG_LOG_WARNING(logger_, "session " << tag << " is offline: " << ex.what());
            }

            if (boost::algorithm::equals(table_name, "meterIEC")) {
                //
                //  insert/update "gwIEC" too
                //
                db_req_insert_gw_iec(key, tag);
            } else {
                CYNG_LOG_WARNING(logger_, "[session] req.insert " << table_name << " - " << data << " failed");
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

    void session::db_req_insert_auto(std::string const &table_name, cyng::data_t data, boost::uuids::uuid tag) {

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

    void session::db_req_update(std::string const &table_name, cyng::key_t key, cyng::param_map_t data, boost::uuids::uuid source) {

        try {
            CYNG_LOG_INFO(logger_, "session [" << socket_.remote_endpoint() << "] req.update " << table_name << " - " << data);
        } catch (std::exception const &ex) {
            CYNG_LOG_WARNING(logger_, "session " << source << " is offline: " << ex.what());
        }

        //
        //	key with multiple columns
        //
        std::reverse(key.begin(), key.end());

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

                    tbl_meter->loop([&](cyng::record const &rec, std::size_t) -> bool {
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

                    tbl_meter->loop([&](cyng::record const &rec, std::size_t) -> bool {
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

    void session::db_req_remove(std::string const &table_name, cyng::key_t key, boost::uuids::uuid source) {
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
                    tbl->loop([&](cyng::record const &rec, std::size_t) -> bool {
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
                    tbl_meter->loop([&](cyng::record const &rec, std::size_t) -> bool {
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

    void session::db_req_clear(std::string const &table_name, boost::uuids::uuid source) {
        cache_.get_store().clear(table_name, source);
    }

    void session::send_cluster_response(std::deque<cyng::buffer_t> &&msg) {
        cyng::exec(vm_, [=, this]() {
            bool const b = buffer_write_.empty();
            cyng::add(buffer_write_, msg);
            if (b)
                do_write();
        });
    }

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

        sp_->send_cluster_response(cyng::serialize_invoke("db.res.insert", tbl->meta().get_name(), key, data, gen, source));

        return true;
    }

    bool session::slot::forward(
        cyng::table const *tbl,
        cyng::key_t const &key,
        cyng::attr_t const &attr,
        std::uint64_t gen,
        boost::uuids::uuid source) {
        //
        //	send update to subscriber
        //
        CYNG_LOG_TRACE(
            sp_->logger_,
            "forward update [" << tbl->meta().get_name() << '/' << sp_->protocol_layer_ << "] " << attr.first << " => "
                               << attr.second);

        sp_->send_cluster_response(cyng::serialize_invoke("db.res.update", tbl->meta().get_name(), key, attr, gen, source));

        return true;
    }

    bool session::slot::forward(cyng::table const *tbl, cyng::key_t const &key, boost::uuids::uuid source) {
        //
        //	send remove to subscriber
        //
        CYNG_LOG_TRACE(sp_->logger_, "forward remove [" << tbl->meta().get_name() << '/' << sp_->protocol_layer_ << "]");

        sp_->send_cluster_response(cyng::serialize_invoke("db.res.remove", tbl->meta().get_name(), key, source));

        return true;
    }

    bool session::slot::forward(cyng::table const *tbl, boost::uuids::uuid source) {
        //
        //	send clear to subscriber
        //

        CYNG_LOG_TRACE(sp_->logger_, " forward clear [" << tbl->meta().get_name() << '/' << sp_->protocol_layer_ << "]");

        sp_->send_cluster_response(cyng::serialize_invoke("db.res.clear", tbl->meta().get_name(), source));

        return true;
    }

    bool session::slot::forward(cyng::table const *tbl, bool trx) {
        CYNG_LOG_TRACE(sp_->logger_, "forward trx [" << tbl->meta().get_name() << "] " << (trx ? "start" : "commit"));

        sp_->send_cluster_response(cyng::serialize_invoke("db.res.trx", tbl->meta().get_name(), trx));

        return true;
    }

    void session::pty_login(
        boost::uuids::uuid tag,
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
            cache_.insert_pty(dev, vm_.get_tag(), tag, name, pwd, ep, data_layer);

            //
            //	send response
            //
            send_cluster_response(cyng::serialize_forward("pty.res.login", tag, true, dev));

            //
            //	update cluster table (pty counter)
            //
            auto const counter = cache_.update_pty_counter(vm_.get_tag(), peer_);
#ifdef _DEBUG
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
            send_cluster_response(cyng::serialize_forward("pty.res.login", tag, false, boost::uuids::nil_uuid())); //	no
                                                                                                                   // device

            //	check auto insert
            auto const auto_login = cache_.get_cfg().get_value("auto-login", false);
            if (auto_login) {
                CYNG_LOG_INFO(logger_, "auto-login is ON - insert [" << name << "] into device table");
                auto const auto_enabled = cache_.get_cfg().get_value("auto-enabled", true);
                cache_.insert_device(tag, name, pwd, auto_enabled);
            }
        }
    }

    void session::pty_logout(boost::uuids::uuid tag, boost::uuids::uuid dev) {
        CYNG_LOG_INFO(logger_, "pty " << protocol_layer_ << " logout [vm:" << vm_.get_tag() << "]");

        //
        //	remove session table, targets and channels
        //
        auto const [connections, success] = cache_.remove_pty(tag, dev);
        if (!success) {
            CYNG_LOG_WARNING(logger_, "pty " << protocol_layer_ << " logout [" << dev << "] failed");
        } else {

            //
            //	remove open connections
            //
            for (auto const &key : connections) {
                auto const [rtag, peer] = cache_.get_access_params(key);
                BOOST_ASSERT(peer == vm_.get_tag()); //	correct implementation later
                //"pty.req.close.connection"
                send_cluster_response(cyng::serialize_forward("pty.req.close.connection", rtag));
            }

            //
            //	update cluster table (pty counter)
            //
            auto const counter = cache_.update_pty_counter(vm_.get_tag(), peer_);
#ifdef _DEBUG
            cache_.sys_msg(cyng::severity::LEVEL_TRACE, protocol_layer_, "has", counter, "users");
#else
            boost::ignore_unused(counter);
#endif
        }
    }

    void session::pty_open_connection(boost::uuids::uuid tag, boost::uuids::uuid dev, std::string msisdn, cyng::param_map_t token) {
        CYNG_LOG_INFO(logger_, "pty " << protocol_layer_ << " connect " << msisdn << " {" << tag << "}");

        //
        //	forward to callee
        //	(1) session already connected,
        //	(2) remote session online, enabled and not connected,
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
            token.emplace("caller-tag", cyng::make_object(tag));
            token.emplace("caller-vm", cyng::make_object(vm_.get_tag()));
            token.emplace("callee-vm", cyng::make_object(vm_key));
            token.emplace("dev", cyng::make_object(dev));
            token.emplace("caller", cyng::make_object(caller));
            token.emplace("callee", cyng::make_object(callee));
            token.emplace("local", cyng::make_object(vm_key == vm_.get_tag()));

            //
            //	send to next VM in the fabric
            //
            vm_.load(cyng::generate_forward("pty.forward.open.connection", vm_key, msisdn, remote, vm_key == vm_.get_tag(), token));
            vm_.run();

        } else {

            CYNG_LOG_TRACE(logger_, "pty " << msisdn << " is offline / connected");

            //
            //	session offline or already connected
            //
            send_cluster_response(cyng::serialize_forward("pty.res.open.connection", tag, false, token)); //	failed
        }
    }

    void session::pty_forward_open_connection(std::string msisdn, boost::uuids::uuid tag, bool local, cyng::param_map_t token) {
        CYNG_LOG_TRACE(logger_, "pty forward open connection " << msisdn << " - " << (local ? "local " : "distributed "));

        //
        //	send to cluster node
        //
        send_cluster_response(cyng::serialize_forward("pty.req.open.connection", tag, msisdn, local, token));
    }

    void session::pty_return_open_connection(
        bool success,
        boost::uuids::uuid dev //	callee dev-tag
        ,
        boost::uuids::uuid callee //	callee vm-tag
        ,
        cyng::param_map_t token) {
        CYNG_LOG_INFO(logger_, "pty establish connection " << (success ? "ok" : "failed"));
        CYNG_LOG_DEBUG(logger_, "pty establish connection vm:" << vm_.get_tag() << ", token: " << token);
        CYNG_LOG_DEBUG(logger_, "pty establish connection callee dev-tag:" << dev << ", callee vm-tag: " << callee);

        auto const reader = cyng::make_reader(token);
        auto const caller_tag = cyng::value_cast(reader["caller-tag"].get(), boost::uuids::nil_uuid());
        auto const caller_vm = cyng::value_cast(reader["caller-vm"].get(), boost::uuids::nil_uuid());
        auto const caller_dev = cyng::value_cast(reader["dev"].get(), boost::uuids::nil_uuid());
        auto const caller_name = cyng::value_cast(reader["caller"].get(), "");

        auto const callee_vm = cyng::value_cast(reader["callee-vm"].get(), boost::uuids::nil_uuid());
        auto const callee_name = cyng::value_cast(reader["callee"].get(), "");
        auto const local = cyng::value_cast(reader["local"].get(), false);

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
            callee //	callee tag
            ,
            dev //	callee dev-tag
            ,
            callee_vm //	callee vm-tag
            ,
            local);

        //
        //	forward response to [ip-t] node
        //
        send_cluster_response(cyng::serialize_forward("pty.res.open.connection", caller_tag, success, token)); //	complete
    }

    void session::pty_transfer_data(boost::uuids::uuid tag, boost::uuids::uuid dev, cyng::buffer_t data) {
        auto const [rtag, rpeer] = cache_.get_remote(dev);
        if (!rtag.is_nil()) {

            BOOST_ASSERT(tag != rtag);
            CYNG_LOG_TRACE(logger_, "pty transfer " << data.size() << " bytes to " << rtag);

            //
            //	ToDo: enabled other VMs too
            //
            send_cluster_response(cyng::serialize_forward("pty.transfer.data", rtag, data));

        } else {
            CYNG_LOG_WARNING(logger_, "pty " << protocol_layer_ << " is not connected {" << dev << "}");
        }
    }

    void session::pty_close_connection(boost::uuids::uuid tag, boost::uuids::uuid dev, cyng::param_map_t token) {
        CYNG_LOG_INFO(logger_, "pty " << protocol_layer_ << " disconnect {" << tag << "}");
        send_cluster_response(cyng::serialize_forward("pty.res.close.connection", tag, false, token)); //	failed
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

            send_cluster_response(cyng::serialize_forward("pty.res.register", tag, true, channel, token)); //	ok

        } else {
            CYNG_LOG_WARNING(logger_, "pty " << protocol_layer_ << " registering target " << name << " {" << tag << "} failed");
            send_cluster_response(cyng::serialize_forward("pty.res.register", tag, false, channel, token)); //	failed
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

            send_cluster_response(cyng::serialize_forward(
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

            auto const [channel, source, packet_size, count] = cache_.open_channel(dev, name, account, number, sv, id, timeout);

            if (count == 0) {
                cache_.sys_msg(cyng::severity::LEVEL_WARNING, protocol_layer_, "target [", name, "] not found");
                CYNG_LOG_WARNING(logger_, "pty " << protocol_layer_ << "target [" << name << "] not found");
            } else {
                CYNG_LOG_INFO(logger_, "pty " << protocol_layer_ << " open channel [" << name << "] " << channel << ':' << source);
            }

            send_cluster_response(cyng::serialize_forward(
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

        send_cluster_response(cyng::serialize_forward(
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

            send_cluster_response(cyng::serialize_forward("pty.push.data.req", pty.pty_.first, pty.channel_, source, data));
        }

        send_cluster_response(cyng::serialize_forward(
            "pty.push.data.res",
            tag,
            !vec.empty(), //	success
            channel,
            source,
            token));
    }

    std::function<void(std::string, std::string, cyng::pid, std::string, boost::uuids::uuid, cyng::version)>
    session::get_vm_func_cluster_req_login(session *ptr) {
        return std::bind(
            &session::cluster_login,
            ptr,
            std::placeholders::_1,
            std::placeholders::_2,
            std::placeholders::_3,
            std::placeholders::_4,
            std::placeholders::_5,
            std::placeholders::_6);
    }

    std::function<void(std::string, boost::uuids::uuid tag)> session::get_vm_func_db_req_subscribe(session *ptr) {
        return std::bind(&session::db_req_subscribe, ptr, std::placeholders::_1, std::placeholders::_2);
    }

    std::function<void(std::string, cyng::key_t, cyng::data_t, std::uint64_t, boost::uuids::uuid)>
    session::get_vm_func_db_req_insert(session *ptr) {
        return std::bind(
            &session::db_req_insert,
            ptr,
            std::placeholders::_1,
            std::placeholders::_2,
            std::placeholders::_3,
            std::placeholders::_4,
            std::placeholders::_5);
    }

    //	"db.req.insert.auto"
    std::function<void(std::string, cyng::data_t, boost::uuids::uuid)> session::get_vm_func_db_req_insert_auto(session *ptr) {
        return std::bind(&session::db_req_insert_auto, ptr, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    }

    //	"db.req.update" aka merge()
    std::function<void(std::string, cyng::key_t, cyng::param_map_t, boost::uuids::uuid)>
    session::get_vm_func_db_req_update(session *ptr) {
        return std::bind(
            &session::db_req_update,
            ptr,
            std::placeholders::_1,
            std::placeholders::_2,
            std::placeholders::_3,
            std::placeholders::_4);
    }

    //	"db.req.remove"
    std::function<void(std::string, cyng::key_t, boost::uuids::uuid)> session::get_vm_func_db_req_remove(session *ptr) {
        return std::bind(&session::db_req_remove, ptr, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    }

    //	"db.req.clear"
    std::function<void(std::string, boost::uuids::uuid)> session::get_vm_func_db_req_clear(session *ptr) {
        return std::bind(&session::db_req_clear, ptr, std::placeholders::_1, std::placeholders::_2);
    }

    std::function<void(boost::uuids::uuid, std::string, std::string, boost::asio::ip::tcp::endpoint, std::string)>
    session::get_vm_func_pty_login(session *ptr) {
        return std::bind(
            &session::pty_login,
            ptr,
            std::placeholders::_1,
            std::placeholders::_2,
            std::placeholders::_3,
            std::placeholders::_4,
            std::placeholders::_5);
    }

    std::function<void(boost::uuids::uuid, boost::uuids::uuid)> session::get_vm_func_pty_logout(session *ptr) {
        return std::bind(&session::pty_logout, ptr, std::placeholders::_1, std::placeholders::_2);
    }

    std::function<void(boost::uuids::uuid, boost::uuids::uuid, std::string, cyng::param_map_t)>
    session::get_vm_func_pty_open_connection(session *ptr) {
        return std::bind(
            &session::pty_open_connection,
            ptr,
            std::placeholders::_1,
            std::placeholders::_2,
            std::placeholders::_3,
            std::placeholders::_4);
    }

    std::function<void(std::string, boost::uuids::uuid, bool, cyng::param_map_t)>
    session::get_vm_func_pty_forward_open_connection(session *ptr) {
        return std::bind(
            &session::pty_forward_open_connection,
            ptr,
            std::placeholders::_1,
            std::placeholders::_2,
            std::placeholders::_3,
            std::placeholders::_4);
    }

    std::function<void(bool, boost::uuids::uuid, boost::uuids::uuid, cyng::param_map_t)>
    session::get_vm_func_pty_return_open_connection(session *ptr) {
        return std::bind(
            &session::pty_return_open_connection,
            ptr,
            std::placeholders::_1,
            std::placeholders::_2,
            std::placeholders::_3,
            std::placeholders::_4);
    }

    std::function<void(boost::uuids::uuid, boost::uuids::uuid, cyng::buffer_t)>
    session::get_vm_func_pty_transfer_data(session *ptr) {
        return std::bind(&session::pty_transfer_data, ptr, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    }

    std::function<void(boost::uuids::uuid, boost::uuids::uuid, cyng::param_map_t)>
    session::get_vm_func_pty_close_connection(session *ptr) {
        return std::bind(&session::pty_close_connection, ptr, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    }

    std::function<void(boost::uuids::uuid, boost::uuids::uuid, std::string, std::uint16_t, std::uint8_t, cyng::param_map_t)>
    session::get_vm_func_pty_register_target(session *ptr) {
        return std::bind(
            &session::pty_register_target,
            ptr,
            std::placeholders::_1,
            std::placeholders::_2,
            std::placeholders::_3,
            std::placeholders::_4,
            std::placeholders::_5,
            std::placeholders::_6);
    }

    std::function<void(boost::uuids::uuid, std::string)> session::get_vm_func_pty_deregister(session *ptr) {
        return std::bind(&session::pty_deregister, ptr, std::placeholders::_1, std::placeholders::_2);
    }

    std::function<void(
        boost::uuids::uuid,
        boost::uuids::uuid,
        std::string,
        std::string,
        std::string,
        std::string,
        std::string,
        std::chrono::seconds,
        cyng::param_map_t)>
    session::get_vm_func_pty_open_channel(session *ptr) {
        return std::bind(
            &session::pty_open_channel,
            ptr,
            std::placeholders::_1,
            std::placeholders::_2,
            std::placeholders::_3,
            std::placeholders::_4,
            std::placeholders::_5,
            std::placeholders::_6,
            std::placeholders::_7,
            std::placeholders::_8,
            std::placeholders::_9);
    }

    std::function<void(boost::uuids::uuid, boost::uuids::uuid, std::uint32_t, cyng::param_map_t)>
    session::get_vm_func_pty_close_channel(session *ptr) {
        return std::bind(
            &session::pty_close_channel,
            ptr,
            std::placeholders::_1,
            std::placeholders::_2,
            std::placeholders::_3,
            std::placeholders::_4);
    }

    std::function<void(boost::uuids::uuid, boost::uuids::uuid, std::uint32_t, std::uint32_t, cyng::buffer_t, cyng::param_map_t)>
    session::get_vm_func_pty_push_data_req(session *ptr) {
        return std::bind(
            &session::pty_push_data_req,
            ptr,
            std::placeholders::_1,
            std::placeholders::_2,
            std::placeholders::_3,
            std::placeholders::_4,
            std::placeholders::_5,
            std::placeholders::_6);
    }

    std::function<bool(std::string msg, cyng::severity)> session::get_vm_func_sys_msg(db *ptr) {
        return std::bind(&db::push_sys_msg, ptr, std::placeholders::_1, std::placeholders::_2);
    }

    std::chrono::seconds smooth(std::chrono::seconds interval) {
        auto const mod = interval.count() % 300;
        return std::chrono::seconds(interval.count() + (300 - mod));
    }

} // namespace smf
