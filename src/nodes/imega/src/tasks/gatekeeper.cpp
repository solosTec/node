#include <tasks/gatekeeper.h>

#include <session.h>

#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/channel.h>

#include <iostream>

namespace smf {

    gatekeeper::gatekeeper(cyng::channel_weak wp
		, cyng::logger logger
		, std::shared_ptr<imega_session> imegasp, bus &cluster_bus)
	: sigs_{ 
		std::bind(&gatekeeper::timeout, this),
		std::bind(&gatekeeper::stop, this, std::placeholders::_1),
	}
	, channel_(wp)
	, logger_(logger)
	, imegasp_(imegasp)
    , cluster_bus_(cluster_bus)
	{
        BOOST_ASSERT(imegasp_);
        auto sp = wp.lock();
        if (sp) {
            sp->set_channel_names({"timeout"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    gatekeeper::~gatekeeper() {
#ifdef _DEBUG_MODEM
        std::cout << "gatekeeper(~)" << std::endl;
#endif
    }

    void gatekeeper::timeout() {
        CYNG_LOG_WARNING(logger_, "[gatekeeper] timeout");
        //  this will stop this task too
        if (imegasp_) {
            cluster_bus_.sys_msg(cyng::severity::LEVEL_WARNING, "modem gatekeeper timeout", imegasp_->get_remote_endpoint());
            imegasp_->stop();
        }
    }

    void gatekeeper::stop(cyng::eod) { CYNG_LOG_INFO(logger_, "[gatekeeper] stop"); }

} // namespace smf
