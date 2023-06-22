#include <tasks/cluster.h>

#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/channel.h>

#include <iostream>

#include <boost/bind/bind.hpp>

#ifdef _DEBUG_DNS
#include <cyng/io/hex_dump.hpp>
#include <iostream>
#include <sstream>
#endif

namespace smf {

    cluster::cluster(cyng::channel_weak wp
		, cyng::controller& ctl
		, boost::uuids::uuid tag
		, std::string const& node_name
		, cyng::logger logger
		, toggle::server_vec_t&& tgl
        , boost::asio::ip::udp::endpoint ep)
	: sigs_{ 
		std::bind(&cluster::connect, this),
		std::bind(&cluster::stop, this, std::placeholders::_1),
	}
		, channel_(wp)
		, ctl_(ctl)
		, logger_(logger)
		, fabric_(ctl)
		, bus_(ctl, logger, std::move(tgl), node_name, tag, NO_FEATURES, this)
        , server_(ctl_, logger, ep)
	{
        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"connect"});
            server_.start();
            CYNG_LOG_INFO(logger_, "cluster task " << tag << " started");
        } else {
            CYNG_LOG_FATAL(logger_, "cluster task " << tag << " failed");
        }
    }

    cluster::~cluster() {
#ifdef _DEBUG_DNS
        std::cout << "cluster(~)" << std::endl;
#endif
    }

    void cluster::stop(cyng::eod) {
        CYNG_LOG_WARNING(logger_, "stop cluster bus(" << bus_.get_tag() << ")");
        server_.stop();
        bus_.stop();
    }

    void cluster::connect() { bus_.start(); }

    //
    //	bus interface
    //
    cyng::mesh *cluster::get_fabric() { return &fabric_; }
    cfg_db_interface *cluster::get_cfg_db_interface() { return nullptr; }
    cfg_sink_interface *cluster::get_cfg_sink_interface() { return nullptr; }
    cfg_data_interface *cluster::get_cfg_data_interface() { return nullptr; }

    void cluster::on_login(bool success) {
        if (success) {
            CYNG_LOG_INFO(logger_, "cluster join complete");
        } else {
            CYNG_LOG_ERROR(logger_, "joining cluster failed");
        }
    }

    void cluster::on_disconnect(std::string msg) { CYNG_LOG_WARNING(logger_, "[cluster] disconnect: " << msg); }

} // namespace smf
