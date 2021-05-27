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
		, rnd_delay_(10u, 240u)
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
        // clients_.stop();
        // clients_.clear();
        bus_.stop();
    }

    void cluster::connect() {
        //
        //	join cluster
        //
        bus_.start();
    }

    bool cluster::check_client(cyng::record const &rec) {

        auto const host = rec.value("host", "");
        auto const port = rec.value<std::uint16_t>("port", 0);
        auto const interval = rec.value("interval", std::chrono::seconds(900));
        bool found = false;

        std::size_t counter{0};
        store_.access(
            [&](cyng::table const *tbl_iec, cyng::table const *tbl_meter) {
                //
                //	lookup meter
                //
                auto const meter = tbl_meter->lookup(rec.key());
                if (!meter.empty()) {

                    //
                    //	update "found" flag
                    //
                    found = true;

                    //
                    //	get meter name
                    //
                    auto const name = meter.value("meter", "no-meter");

                    //
                    //	search for duplicate ip adresses
                    //
                    tbl_iec->loop([&](cyng::record const &rec, std::size_t) -> bool {
                        auto const cmp_host = rec.value("host", "");
                        auto const cmp_port = rec.value<std::uint16_t>("port", 0);

                        if (boost::algorithm::equals(cmp_host, host) && cmp_port == port) {
                            ++counter;
                        }

                        return true;
                    });

                    //
                    //	construct task name
                    //
                    auto task = make_task_name(host, port);

                    //
                    //	start/update clients
                    //
                    if (counter != 0) {
                        CYNG_LOG_INFO(logger_, "[db] address with multiple meters " << counter - 1 << ": " << host << ':' << port);
                        ctl_.get_registry().dispatch(task, "add.meter", name);

                    } else {

                        auto const delay = rnd_delay_();
                        CYNG_LOG_TRACE(logger_, "[cluster] start client " << host << ':' << port << " in " << delay << " seconds");

                        //
                        //	start client
                        //
                        auto channel = ctl_.create_named_channel_with_ref<client>(task, ctl_, bus_, logger_, out_, rec.key(), name);
                        BOOST_ASSERT(channel->is_open());

                        //
                        //  start all clients with a random delay between 10 and 240 seconds
                        //
                        channel->suspend(
                            std::chrono::seconds(delay), "start", cyng::make_tuple(host, std::to_string(port), interval));
                        //
                        //  client stays alive since using a timer with a reference to the task
                        //
                    }
                } else {
                    CYNG_LOG_WARNING(logger_, "[db] address " << host << ':' << port << " without meter configuration");
                    bus_.sys_msg(cyng::severity::LEVEL_WARNING, "[iec]", host, ":", port, "without meter configuration");
                }
            },
            cyng::access::read("meterIEC"),
            cyng::access::read("meter"));

        return found;
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
            db_->loop([this](cyng::meta_store const &m) { bus_.req_subscribe(m.get_name()); });

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
            if (boost::algorithm::equals(table_name, "meterIEC")) {
                cyng::record rec(db_->get_meta_iec(), key, data, gen);

                CYNG_LOG_INFO(logger_, "[cluster] check client: " << data);
                check_client(rec);
            }
            db_->res_insert(table_name, key, data, gen, tag);
        }
    }

    void cluster::db_res_trx(std::string table_name, bool trx) {
        CYNG_LOG_INFO(logger_, "[cluster] trx: " << table_name << (trx ? " start" : " commit"));
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
