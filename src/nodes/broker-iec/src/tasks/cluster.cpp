/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <smf/ipt/config.h>
#include <tasks/client.h>
#include <tasks/cluster.h>
#include <tasks/push.h>

#include <cyng/log/record.h>
#include <cyng/obj/container_factory.hpp>
#include <cyng/obj/util.hpp>
#include <cyng/task/channel.h>

#include <iostream>

namespace smf {

    cluster::cluster(cyng::channel_weak wp
		, cyng::controller& ctl
		, boost::uuids::uuid tag
		, std::string const& node_name
		, cyng::logger logger
		, toggle::server_vec_t&& cfg_cluster
		, bool login
        , std::size_t reconnect_timeout
        , ipt::toggle::server_vec_t && cfg_ipt
        , ipt::push_channel &&pcc)
	: sigs_{ 
		std::bind(&cluster::connect, this),
		std::bind(&cluster::stop, this, std::placeholders::_1),
	}
		, channel_(wp)
		, ctl_(ctl)
		, tag_(tag)
		, logger_(logger)
        , reconnect_timeout_(reconnect_timeout)
        , cfg_ipt_(std::move(cfg_ipt))
        , pcc_(std::move(pcc))
		, fabric_(ctl)
		, bus_(ctl, logger, std::move(cfg_cluster), node_name, tag, NO_FEATURES, this)
		, store_()
		, db_(std::make_shared<db>(store_, logger_, tag_, channel_))
        , delay_(0)
        , dep_key_()
        , stash_(ctl.get_ctx())
	{
        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"connect"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] started");
        }
        CYNG_LOG_TRACE(logger, "reconnect timeout is " << reconnect_timeout << " seconds");
    }

    void cluster::stop(cyng::eod) {
        CYNG_LOG_WARNING(logger_, "[cluster] stop task(" << tag_ << ")");
        stash_.stop();
        bus_.stop();
    }

    void cluster::connect() {
        //
        //	join cluster
        //
        bus_.start();
    }

    std::size_t cluster::check_gateway(cyng::record const &rec) {

        auto const host = rec.value("host", "");
        auto const port = rec.value<std::uint16_t>("port", 0);
        auto const interval = rec.value("interval", std::chrono::seconds(60 * 60));

        auto const meter_counter = rec.value<std::uint32_t>("meterCounter", 0);
        auto const connect_counter = rec.value<std::uint32_t>("connectCounter", 0);
        auto const failure_counter = rec.value<std::uint32_t>("failureCounter", 0);

        //
        //	construct task name
        //
        auto const task_name = make_task_name(host, port);

        CYNG_LOG_TRACE(
            logger_, "[cluster] start client " << task_name << " in " << delay_ << " with " << meter_counter << " meter(s)");

        //
        //	start client
        //
        auto channel = ctl_.create_named_channel_with_ref<client>(
                               task_name,
                               ctl_,
                               bus_,
                               logger_,
                               rec.key(),
                               connect_counter,
                               failure_counter,
                               host,
                               std::to_string(port),
                               std::chrono::seconds(reconnect_timeout_))
                           .first;
        BOOST_ASSERT(channel->is_open());

        //
        // find all meters and calculate the smallest time interval.
        //
        std::uint32_t counter{0};
        store_.access(
            [&](cyng::table const *tbl_iec, cyng::table const *tbl_meter, cyng::table const *tbl_device) {
                tbl_iec->loop([&](cyng::record &&rec_iec, std::size_t) -> bool {
                    auto const host_iec = rec_iec.value("host", "");
                    auto const port_iec = rec_iec.value<std::uint16_t>("port", 0);

                    if (boost::algorithm::equals(host_iec, host) && port_iec == port) {

                        //
                        //  update meter counter
                        //
                        ++counter;

                        //
                        //	lookup meter
                        //
                        auto const meter = tbl_meter->lookup(rec_iec.key());
                        if (!meter.empty()) {

                            //
                            //	get meter name
                            //
                            auto const name = meter.value("meter", "no-meter");

                            auto const rec_device = tbl_device->lookup(rec_iec.key());
                            if (!rec_device.empty()) {

                                auto const account = rec_device.value("name", "");
                                auto const pwd = rec_device.value("pwd", "");
                                auto const enabled = rec_device.value("enabled", false);
                                if (!enabled) {
                                    CYNG_LOG_WARNING(logger_, "[db] device " << account << " is not enabled");
                                    bus_.sys_msg(
                                        cyng::severity::LEVEL_WARNING, "[iec] device ", rec_device.to_string(), " is not enabled");
                                } else {

                                    CYNG_LOG_INFO(
                                        logger_,
                                        "[db] " << task_name << " add meter " << name << " - " << counter << "/" << meter_counter);
                                    //  handle dispatch errors
                                    channel->dispatch(
                                        "add.meter",
                                        std::bind(&bus::log_dispatch_error, &bus_, std::placeholders::_1, std::placeholders::_2),
                                        name,
                                        rec_iec.key());

                                    //
                                    //  create push task
                                    //
                                    auto const tid = create_push_task(name, account, pwd, task_name);
                                }
                            } else {
                                CYNG_LOG_WARNING(logger_, "[db] device for meter " << name << " not found");
                                bus_.sys_msg(cyng::severity::LEVEL_WARNING, "[iec] device for meter ", name, " not found");
                            }
                        } else {
                            CYNG_LOG_WARNING(logger_, "[db] address " << host << ':' << port << " without meter configuration ");
                            bus_.sys_msg(cyng::severity::LEVEL_WARNING, "[iec] ", host, ":", port, "without meter configuration");
                        }
                    }

                    //
                    //  small optimization
                    //  return false if all configured meters complete
                    //
                    return counter != meter_counter;
                });
            },
            cyng::access::read("meterIEC"),
            cyng::access::read("meter"),
            cyng::access::read("device"));

        //
        //  start all clients with a random delay between 10 and 300 seconds
        //
        //  handle dispatch errors
        channel->dispatch(
            "init", std::bind(&bus::log_dispatch_error, &bus_, std::placeholders::_1, std::placeholders::_2), interval);
        //  handle dispatch errors
        channel->suspend(
            std::chrono::seconds(delay_) + std::chrono::seconds(12), //  +12 sec offset
            "start",
            std::bind(&bus::log_dispatch_error, &bus_, std::placeholders::_1, std::placeholders::_2));

        //
        //  client stays alive since using a timer with a reference to the task
        //

        // BOOST_ASSERT(counter == meter_counter); //  check data consistency
        if (counter != meter_counter) {
            CYNG_LOG_WARNING(
                logger_,
                "[cluster] client " << task_name << " has an inconsistent configuration: " << counter << "/" << meter_counter);
            bus_.sys_msg(
                cyng::severity::LEVEL_WARNING,
                "[iec] ",
                task_name,
                "has an inconsistent configuration: ",
                counter,
                "/",
                meter_counter);
        }

        //
        //  reset IEC gateway status
        //
        bus_.req_db_update(
            "gwIEC",
            rec.key(),
            cyng::param_map_factory("state", static_cast<std::uint16_t>(0)) //  offline
            ("meter", "[start]")                                            //  current meter id/name
        );

        //
        //  update delay
        //
        update_delay(counter);

        return counter;
    }

    void cluster::check_iec_meter(cyng::record const &rec) {

        store_.access(
            [this, &rec](cyng::table const *tbl_meter, cyng::table const *tbl_device, cyng::table const *tbl_gw) {
                auto const host = rec.value("host", "");
                auto const port = rec.value<std::uint16_t>("port", 0);
                //
                //	construct task name
                //
                auto const task_name = make_task_name(host, port);

                //
                //  precondition is that gateway is already configured
                //
                auto const key_gw = dep_key_(host, port);
                if (tbl_gw->exists(key_gw)) {

                    //
                    //  look meter record
                    //
                    auto const rec_meter = tbl_meter->lookup(rec.key());
                    if (!rec_meter.empty()) {

                        auto const name = rec_meter.value("meter", "");

                        auto const rec_device = tbl_device->lookup(rec.key());
                        if (!rec_device.empty()) {

                            auto const account = rec_device.value("name", "");
                            auto const pwd = rec_device.value("pwd", "");
                            auto const enabled = rec_device.value("enabled", false);
                            if (!enabled) {
                                CYNG_LOG_WARNING(logger_, "[db] device " << account << " is not enabled");
                                bus_.sys_msg(
                                    cyng::severity::LEVEL_WARNING, "[iec] device ", rec_device.to_string(), " is not enabled");
                            } else {

                                //
                                //  add meter
                                //
                                CYNG_LOG_INFO(logger_, "[db] " << task_name << " add meter " << name);
                                //  handle dispatch errors
                                ctl_.get_registry().dispatch(
                                    task_name,
                                    "add.meter",
                                    std::bind(&bus::log_dispatch_error, &bus_, std::placeholders::_1, std::placeholders::_2),
                                    name,
                                    rec.key());

                                //
                                //  create push task
                                //
                                auto const tid = create_push_task(name, account, pwd, task_name);
                            }

                        } else {
                            CYNG_LOG_WARNING(logger_, "[db] device for meter " << name << " not found");
                            bus_.sys_msg(cyng::severity::LEVEL_WARNING, "[iec] device for meter ", name, " not found");
                        }

                    } else {
                        CYNG_LOG_WARNING(logger_, "[db] IEC meter " << rec.to_string() << " for " << task_name << " not found");
                        bus_.sys_msg(
                            cyng::severity::LEVEL_WARNING, "[iec] meter ", rec.to_string(), " for ", task_name, " not found");
                    }
                } else {
                    CYNG_LOG_WARNING(logger_, "[db] gw " << task_name << " not ready yet");
                }
            },
            cyng::access::read("meter"),
            cyng::access::read("device"),
            cyng::access::read("gwIEC"));
    }

    std::size_t cluster::create_push_task(
        std::string const &name,
        std::string const &account,
        std::string const &pwd,
        std::string const &client) {

        //
        //  push task is identified by the meter name/id
        //
        auto channel =
            ctl_.create_named_channel_with_ref<push>(name, ctl_, logger_, update_cfg(cfg_ipt_, account, pwd), pcc_, client).first;
        BOOST_ASSERT(channel->is_open());
        //  handle dispatch errors
        channel->suspend(
            std::chrono::seconds(15) + std::chrono::milliseconds(stash_.size() * 120),
            "connect",
            std::bind(&bus::log_dispatch_error, &bus_, std::placeholders::_1, std::placeholders::_2));
        stash_.lock(channel);
        CYNG_LOG_INFO(logger_, "[cluster] " << stash_.size() << " push tasks stashed");

        return channel->get_id();
    }

    void cluster::remove_iec_meter(cyng::key_t key) {
        store_.access(
            [&](cyng::table const *tbl_iec, cyng::table const *tbl_meter) {
                auto const rec = tbl_iec->lookup(key);
                if (!rec.empty()) {

                    auto const host = rec.value("host", "");
                    auto const port = rec.value<std::uint16_t>("port", 0);

                    auto const rec_meter = tbl_meter->lookup(key);
                    if (!rec_meter.empty()) {
                        auto const name = rec_meter.value("meter", "");
                        auto const task_name = make_task_name(host, port);
                        ctl_.get_registry().dispatch(
                            task_name,
                            "remove.meter",
                            std::bind(&bus::log_dispatch_error, &bus_, std::placeholders::_1, std::placeholders::_2),
                            name);

                        //
                        //  stop push task
                        //
                        CYNG_LOG_WARNING(logger_, "[cluster] stop push task " << name);
                        stash_.stop(name); //  unlock and stop
                    }
                }
            },
            cyng::access::read("meterIEC"),
            cyng::access::read("meter"));
    }

    void cluster::update_delay(std::uint32_t counter) {
        delay_ += std::chrono::seconds((counter + 1) * 5);
        if (delay_ > std::chrono::seconds(300)) {
            delay_ = std::chrono::seconds(0);
        }
    }

    //
    //	bus interface
    //
    cyng::mesh *cluster::get_fabric() { return &fabric_; }
    cfg_sink_interface *cluster::get_cfg_sink_interface() { return nullptr; }
    cfg_data_interface *cluster::get_cfg_data_interface() { return nullptr; }

    void cluster::on_login(bool success) {
        if (success) {
            CYNG_LOG_INFO(logger_, "[cluster] join complete");

            //
            //	subscribe table "meterIEC" and "meter"
            //
            auto slot = std::static_pointer_cast<cyng::slot_interface>(db_);
            db_->init(slot);
            //  start with first table
            auto const table_name = db_->get_first_table();
            CYNG_LOG_TRACE(logger_, "[cluster] subscribe table " << table_name);
            bus_.req_subscribe(table_name);

        } else {
            CYNG_LOG_ERROR(logger_, "[cluster] joining failed");
        }
    }

    void cluster::on_disconnect(std::string msg) {
        CYNG_LOG_WARNING(logger_, "[cluster] disconnect: " << msg);
        auto slot = std::static_pointer_cast<cyng::slot_interface>(db_);
        db_->disconnect(slot);
    }

    void
    cluster::db_res_insert(std::string table_name, cyng::key_t key, cyng::data_t data, std::uint64_t gen, boost::uuids::uuid tag) {

        std::reverse(key.begin(), key.end());
        std::reverse(data.begin(), data.end());

        //
        //	start/update client
        //

        if (db_) {
            if (boost::algorithm::equals(table_name, "gwIEC")) {
                //
                //  an IEC gateway was added - add all available IEC meters
                //
                CYNG_LOG_INFO(logger_, "[cluster] check gateway: " << data);
                cyng::record rec(db_->get_meta(table_name), key, data, gen);
                check_gateway(rec);
            } else if (boost::algorithm::equals(table_name, "meterIEC")) {
                //
                //  an IEC meter was added - add to task if available
                //
                CYNG_LOG_INFO(logger_, "[cluster] check IEC meter: " << data);
                cyng::record rec(db_->get_meta(table_name), key, data, gen);
                check_iec_meter(rec);
            }
            db_->res_insert(table_name, key, data, gen, tag);
        }
    }

    void cluster::db_res_trx(std::string table_name, bool trx) {
        CYNG_LOG_INFO(logger_, "[cluster] trx: " << table_name << (trx ? " start" : " commit"));
        if (!trx) {
            auto const next = db_->get_next_table(table_name);
            if (!next.empty()) {
                CYNG_LOG_TRACE(logger_, "[cluster] subscribe table " << next);
                bus_.req_subscribe(next);
            }
        }
    }

    void
    cluster::db_res_update(std::string table_name, cyng::key_t key, cyng::attr_t attr, std::uint64_t gen, boost::uuids::uuid tag) {

        std::reverse(key.begin(), key.end());

        CYNG_LOG_TRACE(logger_, "[cluster] update: " << table_name << " - " << key);

        if (db_)
            db_->res_update(table_name, key, attr, gen, tag);
    }

    void cluster::db_res_remove(std::string table_name, cyng::key_t key, boost::uuids::uuid tag) {

        std::reverse(key.begin(), key.end());

        CYNG_LOG_TRACE(logger_, "[cluster] remove: " << table_name << " - " << key);

        if (db_) {
            if (boost::algorithm::equals(table_name, "meterIEC")) {
                //
                //  remove from task
                //
                remove_iec_meter(key);
            } else if (boost::algorithm::equals(table_name, "gwIEC")) {
                //
                //  stop task
                //
                remove_iec_gw(key);
            }

            //
            //  remove from cache
            //
            db_->res_remove(table_name, key, tag);
        }
    }

    void cluster::remove_iec_gw(cyng::key_t key) {

        if (db_) {
            db_->cache_.access(
                [&](cyng::table *tbl_meter) {
                    auto const rec = tbl_meter->lookup(key);
                    if (!rec.empty()) {
                        auto const host = rec.value("host", "");
                        auto const port = rec.value<std::uint16_t>("port", 0);

                        //
                        //	construct task name
                        //
                        auto const task_name = make_task_name(host, port);

                        ctl_.get_registry().dispatch(
                            task_name,
                            "shutdown",
                            std::bind(&bus::log_dispatch_error, &bus_, std::placeholders::_1, std::placeholders::_2));
#ifdef _DEBUG
                        std::this_thread::sleep_for(std::chrono::milliseconds(200));
#endif
                        ctl_.get_registry().stop(task_name);
                    }
                },
                cyng::access::write("gwIEC"));
        }
    }

    void cluster::db_res_clear(std::string table_name, boost::uuids::uuid tag) {

        CYNG_LOG_TRACE(logger_, "[cluster] clear: " << table_name);

        if (db_) {
            if (boost::algorithm::equals(table_name, "gwIEC")) {
                //
                //  ToDo: stop all tasks
                //
            } else if (boost::algorithm::equals(table_name, "meterIEC")) {
                //
                //  ToDo: stop all tasks
                //
            }
            db_->res_clear(table_name, tag);
        }
    }

    std::string make_task_name(std::string host, std::uint16_t port) { return config::dependend_key::build_name(host, port); }

    ipt::toggle::server_vec_t update_cfg(ipt::toggle::server_vec_t cfg, std::string const &account, std::string const &pwd) {

        ipt::toggle::server_vec_t r;
        std::transform(std::begin(cfg), std::end(cfg), std::back_inserter(r), [&](ipt::server const &srv) {
            //
            //	substitute account and password
            //
            return ipt::server(
                srv.host_, srv.service_, account, pwd, srv.sk_, srv.scrambled_, static_cast<int>(srv.monitor_.count()));
        });

        return r;
    }

} // namespace smf
