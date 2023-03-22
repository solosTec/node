/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Sylko Olzscher
 *
 */
#ifndef SMF_SESSION_GATEKEEPER_HPP
#define SMF_SESSION_GATEKEEPER_HPP

#include <smf/cluster/bus.h>

#include <cyng/log/logger.h>
#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/channel.h>
#include <cyng/task/controller.h>
#include <cyng/task/task_fwd.h>

#include <iostream>

#include <boost/uuid/uuid.hpp>

namespace smf {

    /**
     * @tparam S session type
     */
    template <typename S> class gatekeeper {
        template <typename U> friend class cyng::task;

        using signatures_t = std::tuple<
            std::function<void()>,         // timeout
            std::function<void(cyng::eod)> // stop
            >;
        using session_t = S;

      public:
        gatekeeper(cyng::channel_weak wp, cyng::logger logger, std::shared_ptr<S> s, bus &cluster_bus)
        : sigs_{ 
		    std::bind(&gatekeeper::timeout, this),
		    std::bind(&gatekeeper::stop, this, std::placeholders::_1),
        }
        , channel_(wp)
        , logger_(logger)
        , sp_(s)
        , cluster_bus_(cluster_bus)
        {
            BOOST_ASSERT(sp_);
            auto sp = wp.lock();
            if (sp) {
                sp->set_channel_names({"timeout"});
                CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
            }
        }
        ~gatekeeper() = default;

        void stop(cyng::eod) { CYNG_LOG_INFO(logger_, "[gatekeeper] stop"); }

      private:
        void timeout() {
            CYNG_LOG_WARNING(logger_, "[gatekeeper] timeout");
            //  this will stop this task too
            if (sp_) {
                cluster_bus_.sys_msg(cyng::severity::LEVEL_WARNING, "session gatekeeper timeout", sp_->get_remote_endpoint());
                sp_->stop();
            }
        }

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;
        cyng::logger logger_;
        std::shared_ptr<session_t> sp_;
        bus &cluster_bus_;
    };

} // namespace smf

#endif
