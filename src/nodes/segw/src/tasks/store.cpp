#include <tasks/store.h>

#include <smf/ipt/bus.h>
#include <smf/obis/db.h>
#include <smf/obis/defs.h>
#include <smf/obis/profile.h>

#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>

#include <iostream>

namespace smf {
    store::store(cyng::channel_weak wp
		, cyng::logger logger
        , ipt::bus& bus
        , cfg& config
        , cyng::obis profile)
	: sigs_{
            std::bind(&store::init, this),	//	0
            std::bind(&store::run, this),	//	0
            std::bind(&store::stop, this, std::placeholders::_1)	//	1
		}	
		, channel_(wp)
		, logger_(logger)
        , bus_(bus)
        , cfg_(config)
        , profile_(profile)
	{
        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"init", "run"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "#" << sp->get_id() << "] created");
        }
    }

    void store::stop(cyng::eod) { CYNG_LOG_INFO(logger_, "[store] stopped"); }

    void store::init() {
        //
        //  calculate initial start time
        //
        auto const now = std::chrono::system_clock::now();
        // std::chrono::minutes interval(1);
        auto sp = channel_.lock();
        BOOST_ASSERT_MSG(sp, "store task already stopped");
        if (sp) {
            auto const interval = sml::interval_time(profile_);
            auto const next_push = sml::next(interval, profile_, now);
            BOOST_ASSERT_MSG(next_push > now, "negative time span");

            auto const span = std::chrono::duration_cast<std::chrono::minutes>(next_push - now);
            sp->suspend(span, "run");

            CYNG_LOG_TRACE(
                logger_, "[store] " << obis::get_name(profile_) << " - next: " << std::chrono::system_clock::now() + span);
        }
    }

    void store::run() {

        auto sp = channel_.lock();
        BOOST_ASSERT_MSG(sp, "store task already stopped");
        if (sp) {
            auto const interval = sml::interval_time(profile_);
            sp->suspend(interval, "run");
            CYNG_LOG_TRACE(
                logger_, "[store] " << obis::get_name(profile_) << " - next: " << std::chrono::system_clock::now() + interval);
        }

        ;
    }
} // namespace smf
