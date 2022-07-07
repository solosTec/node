#include <tasks/store.h>

#include <smf/ipt/bus.h>
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

    void store::init() { auto const now = std::chrono::system_clock::now(); }

    void store::run() {

        // std::string target;

        // cfg_.get_cache().access(
        //     [&, this](cyng::table const *tbl) {
        //         auto const rec = tbl->lookup(key_);
        //         if (!rec.empty()) {
        //             auto const interval = rec.value("interval", std::chrono::seconds(0));
        //             auto const delay = rec.value("delay", std::chrono::seconds(0));
        //             auto const profile = rec.value("profile", OBIS_PROFILE);
        //             target = rec.value("target", "");

        //            auto sp = channel_.lock();
        //            if (sp) {
        //                sp->suspend(interval, "run");
        //                CYNG_LOG_TRACE(logger_, "[store] " << target << " - next: " << std::chrono::system_clock::now() +
        //                interval);
        //            }
        //        } else {
        //            CYNG_LOG_ERROR(logger_, "[store] no table record for " << key_);
        //        }
        //    },
        //    cyng::access::read("pushOps"));

        // if (!target.empty()) {
        //     if (bus_.is_authorized()) {
        //         //
        //         //  ToDo: open push channel
        //         //

        //    } else {
        //        CYNG_LOG_WARNING(logger_, "[store] IP-T bus not authorized: " << target);
        //    }
        //} else {
        //}
    }
} // namespace smf
