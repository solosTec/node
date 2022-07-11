#include <tasks/push.h>

#include <smf/ipt/bus.h>
#include <smf/obis/db.h>
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
        , cyng::buffer_t meter
        , std::uint8_t nr)
		: sigs_{
            std::bind(&push::init, this),	//	0
            std::bind(&push::run, this),	//	1
            std::bind(
                &push::on_channel_open,
                this,
                std::placeholders::_1,
                std::placeholders::_2,
                std::placeholders::_3,
                std::placeholders::_4,
                std::placeholders::_5),// "on.channel.open"
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
        , meter_(meter)
        , nr_(nr)
	{
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
                    auto const rec = tbl->lookup(cyng::key_generator(meter_, nr_));
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
                        CYNG_LOG_ERROR(logger_, "[push] no table record for " << meter_ << "#" << +nr_);
                    }
                },
                cyng::access::read("pushOps"));
        }
    }

    void push::run() {

        std::string target;

        auto sp = channel_.lock();
        if (sp) {

            cfg_.get_cache().access(
                [&, this](cyng::table const *tbl) {
                    auto const rec = tbl->lookup(cyng::key_generator(meter_, nr_));
                    if (!rec.empty()) {
                        auto const interval = rec.value("interval", std::chrono::seconds(0));
                        auto const delay = rec.value("delay", std::chrono::seconds(0));
                        auto const profile = rec.value("profile", OBIS_PROFILE);
                        target = rec.value("target", "");

                        sp->suspend(interval, "run");
                        CYNG_LOG_TRACE(logger_, "[push] " << target << " - next: " << std::chrono::system_clock::now() + interval);
                    } else {
                        CYNG_LOG_ERROR(logger_, "[push] no record for " << meter_ << "#" << +nr_ << " in table \"pushOps\"");
                    }
                },
                cyng::access::read("pushOps"));

            if (!target.empty()) {
                if (bus_.is_authorized()) {
                    //
                    //  open push channel
                    if (!open_channel(ipt::push_channel(target, "", "", "", "", 30u))) {
                        CYNG_LOG_WARNING(logger_, "[push] IP-T cannot open push channel: " << target);
                    }

                } else {
                    CYNG_LOG_WARNING(logger_, "[push] IP-T bus not authorized: " << target);
                }
            } else {
                CYNG_LOG_WARNING(logger_, "[push] no push target defined");
            }
        } else {
            //  already stopped
        }
    }

    bool push::open_channel(ipt::push_channel &&pc) { return bus_.open_channel(pc, channel_); }

    void push::on_channel_open(bool success, std::uint32_t channel, std::uint32_t source, std::uint32_t count, std::string target) {
        if (success) {
            CYNG_LOG_INFO(logger_, "[push] channel is open");

            //
            //  ToDo: collect data
            //
            collect_data();

            //
            //  ToDo: send data
            //

            //
            //  close channel
            //
            bus_.close_channel(channel, channel_);

        } else {
            CYNG_LOG_WARNING(logger_, "[push] open channel failed");
        }
    }

    void push::collect_data() {
        cfg_.get_cache().access(
            [&, this](
                cyng::table const *tbl_ops,
                cyng::table const *tbl_regs,
                cyng::table const *tbl_mirror,
                cyng::table const *tbl_mirror_data) {
                auto const rec = tbl_ops->lookup(cyng::key_generator(meter_, nr_));
                if (!rec.empty()) {
                    // auto const interval = rec.value("interval", std::chrono::seconds(0));
                    auto const profile = rec.value("profile", OBIS_PROFILE);
                    // auto const target = rec.value("target", "");

                    //
                    //  lookup in table "mirror" for readout data
                    //
                    tbl_mirror->loop([&, this](cyng::record &&rec_mirror, std::size_t) -> bool {
                        auto const mirror_meter = rec_mirror.value("meterID", meter_);
                        auto const mirror_profile = rec_mirror.value("profile", OBIS_PROFILE);
                        auto const decrypted = rec_mirror.value("decrypted", false);

                        if ((meter_ == mirror_meter) && (profile == mirror_profile) && decrypted) {

                            auto const pk = rec_mirror.key();
                            BOOST_ASSERT(pk.size() == 3);

                            //
                            // get all registers to push
                            //
                            auto const regs = get_registers(tbl_regs);
                            CYNG_LOG_INFO(
                                logger_,
                                "[push] collect data of meter: " << meter_ << ", profile: " << obis::get_name(profile) << " with "
                                                                 << regs.size() << " entries");
                            for (auto const &reg : regs) {
                                //
                                //  build key for table "mirrorData"
                                //
                                auto const pk_mirror_data = cyng::extend_key(pk, reg);
                                BOOST_ASSERT(pk_mirror_data.size() == 4);
                            }
                        }

                        return true;
                    });

                } else {
                    CYNG_LOG_ERROR(logger_, "[push] no record for " << meter_ << "#" << +nr_ << " in table \"pushOps\"");
                }
            },
            cyng::access::read("pushOps"),
            cyng::access::read("pushRegister"),
            cyng::access::read("mirror"),
            cyng::access::read("mirrorData"));
    }

    cyng::obis_path_t push::get_registers(cyng::table const *tbl) {
        BOOST_ASSERT(boost::algorithm::equals(tbl->meta().get_name(), "pushRegister"));

        cyng::obis_path_t regs;
        tbl->loop([&, this](cyng::record &&rec, std::size_t) -> bool {
            auto const meter = rec.value("meterID", meter_);
            auto const nr = rec.value<std::uint8_t>("nr", 0u);
            if (meter == meter_ && nr == nr_) {
                auto const reg = rec.value("register", OBIS_PROFILE);
                regs.push_back(reg);
            }

            return true;
        });
        return regs;
    }

    void push::on_channel_close(bool success, std::uint32_t channel) {
        if (success) {
            CYNG_LOG_INFO(logger_, "[push] channel " << channel << " is closed");
        } else {
            CYNG_LOG_WARNING(logger_, "[push] close channel " << channel << " failed");
        }
    }

} // namespace smf
