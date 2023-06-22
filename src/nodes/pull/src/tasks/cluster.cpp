#include <tasks/cluster.h>
#include <tasks/storage_db.h>

#include <cyng/log/record.h>
#include <cyng/net/server_factory.hpp>
#include <cyng/obj/util.hpp>
#include <cyng/task/channel.h>

#include <iostream>

namespace smf {

    cluster::cluster(cyng::channel_weak wp
		, cyng::controller& ctl
		, boost::uuids::uuid tag
		, std::string const& node_name
		, cyng::logger logger
		, toggle::server_vec_t&& tgl
        , cyng::param_map_t &&cfg_db)
	: sigs_{ 
		std::bind(&cluster::connect, this),
		std::bind(&cluster::stop, this, std::placeholders::_1),
	}
		, channel_(wp)
		, ctl_(ctl)
		, logger_(logger)
		, fabric_(ctl)
		, bus_(ctl, logger, std::move(tgl), node_name, tag, CONFIG_PROVIDER, this)
        , db_dispatcher_(this, start_data_store(ctl, logger, bus_, std::move(cfg_db)))
        {

        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"connect", "listen"});
            CYNG_LOG_INFO(logger_, "cluster task " << tag << " started");
        } else {
            CYNG_LOG_FATAL(logger_, "cluster task " << tag << " failed");
        }
    }

    cluster::~cluster() {
#ifdef _DEBUG_IPT
        std::cout << "cluster(~)" << std::endl;
#endif
    }

    void cluster::stop(cyng::eod) {
        CYNG_LOG_WARNING(logger_, "stop cluster bus(" << bus_.get_tag() << ")");
        bus_.stop();
    }

    void cluster::connect() {
        //
        //  open database
        //
        db_dispatcher_.open();

        //
        //  join cluster
        //
        bus_.start();
    }

    //
    //	bus interface
    //
    cyng::mesh *cluster::get_fabric() { return &fabric_; }
    cfg_db_interface *cluster::get_cfg_db_interface() { return &db_dispatcher_; };
    cfg_sink_interface *cluster::get_cfg_sink_interface() { return nullptr; }
    cfg_data_interface *cluster::get_cfg_data_interface() { return nullptr; }

    void cluster::on_login(bool success) {
        if (success) {
            CYNG_LOG_INFO(logger_, "cluster join complete");
            db_dispatcher_.subscribe();

        } else {
            CYNG_LOG_ERROR(logger_, "joining cluster failed");
        }
    }

    void cluster::on_disconnect(std::string msg) { CYNG_LOG_WARNING(logger_, "[cluster] disconnect: " << msg); }

    cyng::channel_ptr start_data_store(cyng::controller &ctl, cyng::logger logger, bus &cluster_bus, cyng::param_map_t &&cfg) {
        // storage_db(
        //     cyng::channel_weak,
        //     cyng::controller &,
        //     bus &,
        //     cyng::store & cache,
        //     cyng::logger logger,
        //     cyng::param_map_t && cfg,
        //     std::set<std::string> &&);

        return ctl.create_named_channel_with_ref<storage_db>("storage", ctl, cluster_bus, logger, std::move(cfg)).first;
    }

    cluster::db_dispatcher::db_dispatcher(cluster *cluster_ptr, cyng::channel_ptr cp)
        : cluster_(cluster_ptr)
        , storage_db_(cp) {}

    void cluster::db_dispatcher::db_res_insert(
        std::string name,
        cyng::key_t key,
        cyng::data_t data,
        std::uint64_t gen,
        boost::uuids::uuid tag) {

        std::reverse(key.begin(), key.end());
        std::reverse(data.begin(), data.end());

        storage_db_->dispatch(
            "insert",
            std::bind(&bus::log_dispatch_error, &cluster_->bus_, std::placeholders::_1, std::placeholders::_2),
            name,
            key,
            data,
            gen,
            tag);
    }

    void cluster::db_dispatcher::db_res_trx(std::string name, bool start) {}
    void cluster::db_dispatcher::db_res_update(
        std::string name,
        cyng::key_t key,
        cyng::attr_t attr,
        std::uint64_t gen,
        boost::uuids::uuid tag) {
        std::reverse(key.begin(), key.end());
    }
    void cluster::db_dispatcher::db_res_remove(std::string name, cyng::key_t key, boost::uuids::uuid tag) {
        std::reverse(key.begin(), key.end());
    }
    void cluster::db_dispatcher::db_res_clear(std::string name, boost::uuids::uuid tag) {}

    void cluster::db_dispatcher::subscribe() {
        storage_db_->dispatch(
            "subscribe", std::bind(&bus::log_dispatch_error, &cluster_->bus_, std::placeholders::_1, std::placeholders::_2));
    }

    void cluster::db_dispatcher::open() {
        storage_db_->dispatch(
            "open", std::bind(&bus::log_dispatch_error, &cluster_->bus_, std::placeholders::_1, std::placeholders::_2));
    }

} // namespace smf
