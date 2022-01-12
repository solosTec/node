#include <session.h>
#include <tasks/proxy.h>

#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/channel.h>

#include <iostream>

namespace smf {

    proxy::proxy(cyng::channel_weak wp
		, cyng::logger logger
		, std::shared_ptr<ipt_session> iptsp, bus &cluster_bus)
    : state_(state::INITIAL)
	, sigs_{ 
        std::bind(&proxy::cfg_backup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
        std::bind(&proxy::stop, this, std::placeholders::_1),
	}
	, channel_(wp)
	, logger_(logger)
	, iptsp_(iptsp)
    , cluster_bus_(cluster_bus)
	{
        BOOST_ASSERT(iptsp_);
        auto sp = wp.lock();
        if (sp) {
            sp->set_channel_names({"cfg.backup"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    proxy::~proxy() {
#ifdef _DEBUG_IPT
        std::cout << "proxy(~)" << std::endl;
#endif
    }

    void proxy::cfg_backup(std::string name, std::string pwd, std::string id) {
        CYNG_LOG_TRACE(logger_, "[proxy] config backup " << name << '@' << id);

        //
        // * login
        // * send SML requests to get current configuration
        // * parse SML responses
        // * update "connection" table
        // * update some progress information
        // * write SML data into config table (setup node)
        // * ready
        //
    }

    void proxy::stop(cyng::eod) { CYNG_LOG_INFO(logger_, "[proxy] stop"); }

} // namespace smf
