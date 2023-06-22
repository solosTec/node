#include <tasks/cluster.h>

#include <session.h>

#include <cyng/log/record.h>
#include <cyng/net/server_factory.hpp>
#include <cyng/obj/util.hpp>
#include <cyng/task/channel.h>

#include <iostream>

namespace smf {

    cluster::cluster(cyng::channel_weak wp
		, cyng::controller& ctl
		, boost::uuids::uuid tag
		, std::uint32_t query
		, std::string const& node_name
		, cyng::logger logger
		, toggle::server_vec_t&& tgl
		, ipt::scramble_key const& sk
		, std::chrono::minutes watchdog
		, std::chrono::seconds timeout
        , cyng::mac48 client_id)
	: sigs_{ 
		std::bind(&cluster::connect, this),
		//std::bind(&ipt_server::listen, &server_, std::placeholders::_1),
		std::bind(&cyng::net::server_proxy::listen<boost::asio::ip::tcp::endpoint>, &server_proxy_, std::placeholders::_1),
		std::bind(&cluster::stop, this, std::placeholders::_1),
	}
		, channel_(wp)
		, ctl_(ctl)
		, logger_(logger)
		, fabric_(ctl)
		, bus_(ctl, logger, std::move(tgl), node_name, tag, CONFIG_PROVIDER, this)
        , server_proxy_()
        , session_counter_{0}
	{
        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"connect", "listen"});
            CYNG_LOG_INFO(logger_, "cluster task " << tag << " started");
            //
            //  create the IP-T server
            //
            cyng::net::server_factory srvf(ctl);
            server_proxy_ = srvf.create_proxy<boost::asio::ip::tcp::socket, 2048>(
                [this](boost::system::error_code ec) {
                    if (!ec) {
                        CYNG_LOG_INFO(logger_, "listen callback " << ec.message());
                    } else {
                        CYNG_LOG_WARNING(logger_, "listen callback " << ec.message());
                    }
                },
                [=, this](boost::asio::ip::tcp::socket socket) {
                    CYNG_LOG_INFO(logger_, "new session #" << session_counter_ << ": " << socket.remote_endpoint());

                    auto sp = std::shared_ptr<ipt_session>(
                        new ipt_session(std::move(socket), logger_, bus_, fabric_, sk, query, client_id), [this](ipt_session *s) {
                            //
                            //	update cluster state
                            //
                            s->logout();
                            s->stop();

                            //
                            //	update session counter
                            //
                            --session_counter_;
                            CYNG_LOG_TRACE(logger_, "session(s) running: " << session_counter_);

                            //
                            //	remove session
                            //
                            delete s;
                        });

                    if (sp) {

                        //
                        //	start session
                        //
                        sp->start(timeout);

                        //
                        //	update session counter
                        //
                        ++session_counter_;
                    }
                });

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

    void cluster::connect() { bus_.start(); }

    //
    //	bus interface
    //
    cyng::mesh *cluster::get_fabric() { return &fabric_; }
    cfg_db_interface *cluster::get_cfg_db_interface() { return nullptr; };
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
