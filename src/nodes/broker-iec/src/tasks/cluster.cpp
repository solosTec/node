/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/client.h>
#include <tasks/cluster.h>

#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/channel.h>

#include <iostream>

namespace smf {

    cluster::cluster(cyng::channel_weak wp
		, cyng::controller& ctl
		, boost::uuids::uuid tag
		, std::string const& node_name
		, cyng::logger logger
		, toggle::server_vec_t&& cfg
		, bool login
		, std::filesystem::path out)
	: sigs_{ 
		std::bind(&cluster::connect, this),
		std::bind(&cluster::stop, this, std::placeholders::_1),
	}
		, channel_(wp)
		, ctl_(ctl)
		, tag_(tag)
		, logger_(logger)
		, out_(out)
		, fabric_(ctl)
		, bus_(ctl.get_ctx(), logger, std::move(cfg), node_name, tag, this)
		, store_()
		, db_(std::make_shared<db>(store_, logger_, tag_, channel_))
        , delay_(0)
	{
        auto sp = channel_.lock();
        if (sp) {
            std::size_t idx{0};
            sp->set_channel_name("connect", idx++);
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] started");
        }
    }

    void cluster::stop(cyng::eod) {
        CYNG_LOG_WARNING(logger_, "[cluster] stop task(" << tag_ << ")");
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
            task_name, ctl_, bus_, logger_, out_, rec.key(), connect_counter, failure_counter);
        BOOST_ASSERT(channel->is_open());

        //
        // find all meters and calculate the smallest time interval.
        //
        std::uint32_t counter{0};
        store_.access(
            [&](cyng::table const *tbl_iec, cyng::table const *tbl_meter) {
                tbl_iec->loop([&](cyng::record const &rec_iec, std::size_t) -> bool {
                    auto const host_iec = rec_iec.value("host", "");
                    auto const port_iec = rec_iec.value<std::uint16_t>("port", 0);

                    if (boost::algorithm::equals(host_iec, host) && port_iec == port) {

                        //
                        //	lookup meter
                        //
                        auto const meter = tbl_meter->lookup(rec_iec.key());
                        if (!meter.empty()) {

                            //
                            //	get meter name
                            //
                            auto const name = meter.value("meter", "no-meter");

                            //
                            //  update meter counter
                            //
                            ++counter;

                            CYNG_LOG_INFO(
                                logger_, "[db] " << task_name << " add meter " << name << " - " << counter << "/" << meter_counter);
                            channel->dispatch("add.meter", name, rec_iec.key());

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
            cyng::access::read("meter"));

        //
        //  start all clients with a random delay between 10 and 300 seconds
        //
        channel->suspend(
            std::chrono::seconds(delay_) + std::chrono::seconds(12), //  +12 sec offset
            "start",
            cyng::make_tuple(host, std::to_string(port), interval));

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
        //  update delay
        //
        update_delay(counter);

        return counter;
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
    void cluster::on_login(bool success) {
        if (success) {
            CYNG_LOG_INFO(logger_, "[cluster] join complete");

            //
            //	subscribe table "meterIEC" and "meter"
            //
            auto slot = std::static_pointer_cast<cyng::slot_interface>(db_);
            db_->init(slot);
            //  start with first table
            bus_.req_subscribe("meter");

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
                cyng::record rec(db_->get_meta(table_name), key, data, gen);

                CYNG_LOG_INFO(logger_, "[cluster] check gateway: " << data);
                check_gateway(rec);
            }
            db_->res_insert(table_name, key, data, gen, tag);
        }
    }

    void cluster::db_res_trx(std::string table_name, bool trx) {
        CYNG_LOG_INFO(logger_, "[cluster] trx: " << table_name << (trx ? " start" : " commit"));
        if (!trx) {
            auto const next = db_->get_next_table(table_name);
            if (!next.empty()) {
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

        if (db_)
            db_->res_remove(table_name, key, tag);
    }

    void cluster::db_res_clear(std::string table_name, boost::uuids::uuid tag) {

        CYNG_LOG_TRACE(logger_, "[cluster] clear: " << table_name);

        if (db_)
            db_->res_clear(table_name, tag);
    }

    std::string make_task_name(std::string host, std::uint16_t port) {
        std::stringstream ss;
        ss << host << ':' << port;
        return ss.str();
    }

} // namespace smf
