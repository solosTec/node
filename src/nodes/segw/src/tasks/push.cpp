#include <tasks/push.h>

#include <smf/ipt/bus.h>
#include <smf/obis/defs.h>
#include <smf/obis/profile.h>

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
            std::bind(&push::init, this),	//	0
            std::bind(&push::run, this),	//	1
            std::bind(
                    &push::on_channel_open,
                    this,
                    std::placeholders::_1,
                    std::placeholders::_2,
                    std::placeholders::_3,
                    std::placeholders::_4),
            std::bind(
            &push::on_channel_close,
            this,
            std::placeholders::_1,
            std::placeholders::_2), // "on.channel.close"
            std::bind(&push::stop, this, std::placeholders::_1)	
		}	
		, channel_(wp)
		, logger_(logger)
        , bus_(bus)
        , cfg_(config)
        , key_(key)
	{
        BOOST_ASSERT(key.size() == 2);

        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"init", "run", "on.channel.open", "on.channel.close"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "#" << sp->get_id() << "] created");
        }
    }

    void push::stop(cyng::eod) { CYNG_LOG_INFO(logger_, "[push] stopped"); }

    void push::init() {

        auto const now = std::chrono::system_clock::now();

        auto sp = channel_.lock();
        BOOST_ASSERT_MSG(sp, "push task already stopped");
        if (sp) {

            cfg_.get_cache().access(
                [&, this](cyng::table const *tbl) {
                    auto const rec = tbl->lookup(key_);
                    if (!rec.empty()) {
                        auto const interval = rec.value("interval", std::chrono::seconds(0));
                        auto const delay = rec.value("delay", std::chrono::seconds(0));
                        auto const profile = rec.value("profile", OBIS_PROFILE);
                        auto const target = rec.value("target", "");

                        //
                        //  Calculate initial start time dependent from profile type.
                        //  ToDo: consider delay
                        //

                        auto const next_push = sml::next(interval, profile, now);
                        BOOST_ASSERT_MSG(next_push > now, "negative time span");

                        if (next_push > now) {
                            auto const span = std::chrono::duration_cast<std::chrono::minutes>(next_push - now);
                            sp->suspend(span, "run");
                            CYNG_LOG_TRACE(logger_, "[push] " << target << " at " << next_push);
                        } else {
                            sp->suspend(interval, "run");
                            CYNG_LOG_WARNING(logger_, "[push] " << target << " - init: " << now + interval);
                        }
                    } else {
                        CYNG_LOG_ERROR(logger_, "[push] no table record for " << key_);
                    }
                },
                cyng::access::read("pushOps"));
        }
    }

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

                    auto sp = channel_.lock();
                    if (sp) {
                        sp->suspend(interval, "run");
                        CYNG_LOG_TRACE(logger_, "[push] " << target << " - next: " << std::chrono::system_clock::now() + interval);
                    }
                } else {
                    CYNG_LOG_ERROR(logger_, "[push] no table record for " << key_);
                }
            },
            cyng::access::read("pushOps"));

        if (!target.empty()) {
            if (bus_.is_authorized()) {
                //
                //  ToDo: collect data
                //  open push channel
                if (open_channel(ipt::push_channel(target, "", "", "", "", 30u))) {
                    //  ToDo: send data
                    //  ToDo: close push channel
                    //
                }

            } else {
                CYNG_LOG_WARNING(logger_, "[push] IP-T bus not authorized: " << target);
            }
        } else {
            CYNG_LOG_WARNING(logger_, "[push] no push target defined");
        }
    }

    bool push::open_channel(ipt::push_channel &&pc) { return bus_.open_channel(pc, channel_); }

    void push::on_channel_open(bool success, std::string meter, std::uint32_t channel, std::uint32_t source) {
        if (success) {
            CYNG_LOG_INFO(logger_, "[push] channel is open");

            //
            //  send data
            //

            //
            //  close channel
            //
            bus_.close_channel(channel, channel_);

        } else {
            CYNG_LOG_WARNING(logger_, "[push] open channel failed");
        }
    }

    void push::on_channel_close(bool success, std::uint32_t channel) {
        if (success) {
            CYNG_LOG_INFO(logger_, "[push] channel " << channel << " is closed");
        } else {
            CYNG_LOG_WARNING(logger_, "[push] close channel " << channel << " failed");
        }
    }

} // namespace smf
