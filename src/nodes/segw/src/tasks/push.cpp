#include <tasks/push.h>

#include <profiles.h>
#include <smf/ipt/bus.h>
#include <smf/obis/defs.h>

#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>

#include <iostream>

namespace smf {
    push::push(cyng::channel_weak wp
		, cyng::logger logger
        , ipt::bus& bus
        , cfg& config
        , cyng::key_t key)
		: sigs_{
            std::bind(&push::run, this),	//	0
            std::bind(&push::stop, this, std::placeholders::_1)	//	1
		}	
		, channel_(wp)
		, logger_(logger)
        , bus_(bus)
        , cfg_(config)
        , key_(key)
	{
        BOOST_ASSERT(key.size() == 2);

        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"run"});
            CYNG_LOG_TRACE(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    void push::stop(cyng::eod) { CYNG_LOG_INFO(logger_, "[push] stopped"); }

    void push::run() {

        std::string target;

        cfg_.get_cache().access(
            [&, this](cyng::table const *tbl) {
                auto const rec = tbl->lookup(key_);
                if (!rec.empty()) {
                    auto const interval = rec.value("interval", std::chrono::seconds(0));
                    auto const delay = rec.value("delay", std::chrono::seconds(0));
                    auto const profile = rec.value("profile", OBIS_PROFILE);
                    target = rec.value("target", "");

                    //
                    //	recalibrate start time dependent from profile type
                    // ToDo: consider delay
                    //
                    auto const span = smf::next(interval, profile) - std::chrono::system_clock::now();
                    BOOST_ASSERT_MSG(span.count() > 0, "negative time span");

                    auto sp = channel_.lock();
                    if (sp) {
                        sp->suspend(span, "run");
                        CYNG_LOG_TRACE(logger_, "[push] " << target << " - next: " << std::chrono::system_clock::now() + span);
                    }
                } else {
                    CYNG_LOG_ERROR(logger_, "[push] no table record for " << key_);
                }
            },
            cyng::access::read("pushOps"));

        if (!target.empty()) {
            if (bus_.is_authorized()) {
                //
                //  open push channel
                //

            } else {
                CYNG_LOG_WARNING(logger_, "[push] IP-T bus not authorized: " << target);
            }
        } else {
        }
    }
} // namespace smf
