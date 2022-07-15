#include <tasks/push.h>

#include <smf/ipt/bus.h>
#include <smf/obis/db.h>
#include <smf/obis/defs.h>
#include <smf/obis/profile.h>

#include <cyng/db/storage.h>
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
        , trx_(7)
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
            CYNG_LOG_INFO(
                logger_,
                "[push] channel " << target << "#" << count << " is open - channel: " << channel << ", source: " << source);

            //
            //  ToDo: collect data
            //  ToDo: send data
            //
            collect_data(channel, source);

            //
            //  close channel
            //
            bus_.close_channel(channel, channel_);

        } else {
            CYNG_LOG_WARNING(logger_, "[push] open channel failed");
        }
    }

    void push::collect_data(std::uint32_t channel, std::uint32_t source) {

        trx_.reset();

        sml::messages_t messages;

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
                    auto const target = rec.value("target", "");

                    CYNG_LOG_TRACE(
                        logger_,
                        "[push] looking for data of meter: " << meter_ << " with profile " << profile << " (" << target << ")");
                    //
                    //  lookup in table "mirror" for readout data
                    //
                    tbl_mirror->loop([&, this](cyng::record &&rec_mirror, std::size_t) -> bool {
                        auto const mirror_meter = rec_mirror.value("meterID", meter_);
                        auto const mirror_profile = rec_mirror.value("profile", OBIS_PROFILE);
                        auto const decrypted = rec_mirror.value("decrypted", false);

                        if ((meter_ == mirror_meter) && (profile == mirror_profile)) {
                            CYNG_LOG_TRACE(
                                logger_,
                                "[push] match - meter: " << meter_ << "/" << mirror_meter << ", profile: " << profile << "/"
                                                         << mirror_profile);
                        } else {
                            CYNG_LOG_DEBUG(
                                logger_,
                                "[push] no match - meter: " << meter_ << "/" << mirror_meter << ", profile: " << profile << "/"
                                                            << mirror_profile);
                        }
                        if (!decrypted) {
                            CYNG_LOG_WARNING(logger_, "[push] collect data of meter " << mirror_meter << " are still encrypted");
                        }

                        if ((meter_ == mirror_meter) && (profile == mirror_profile) && decrypted) {

                            auto const ops = rec_mirror.value<std::uint32_t>("secIdx", 0u);

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

                            //
                            //  generate payload - SML_GetList.Res
                            //
                            messages.push_back(convert_to_payload(tbl_mirror_data, pk, ops, regs));
                        }

                        return true;
                    });

                } else {
                    CYNG_LOG_ERROR(
                        logger_,
                        "[push] no record for " << meter_ << "#" << +nr_ << " in table \"" << tbl_ops->meta().get_name() << "\"");
                }
            },
            cyng::access::read("pushOps"),
            cyng::access::read("pushRegister"),
            cyng::access::read("mirror"),
            cyng::access::read("mirrorData"));

        //
        //  convert to SML
        //  and send payload
        //
        auto buffer = sml::to_sml(messages);
        bus_.transmit(std::make_pair(channel, source), buffer);
    }

    cyng::tuple_t push::convert_to_payload(
        cyng::table const *tbl_mirror_data,
        cyng::key_t const &pk,
        std::uint32_t ops,
        cyng::obis_path_t const &regs) {

        // <meterID: 01e61e571406213603><profile: 8181c78611ff><idx: 18720><register: 0100000102ff>[reading: 177][type:
        // 000c][scaler: 0][unit: 7][status: 00000000]
        //
        // <meterID: 01e61e571406213603><profile: 8181c78611ff><idx: 18720><register: 0100020000ff>[reading: 1632093][type:
        // 000c][scaler: -2][unit: d][status: 00000000]
        //
        // <meterID: 01e61e571406213603><profile: 8181c78611ff><idx: 18720><register: 0100000902ff>[reading:
        // 2022-06-30T02:00:00+0200][type: 0010][scaler: 0][unit: ff][status: 00000000]

        cyng::tuple_t data;
        for (auto const &reg : regs) {
            //
            //  build key for table "mirrorData"
            //
            auto const pk_mirror_data = cyng::extend_key(pk, reg);
            BOOST_ASSERT(pk_mirror_data.size() == 4);

            auto const rec_mirror_data = tbl_mirror_data->lookup(pk_mirror_data);
            if (!rec_mirror_data.empty()) {
                CYNG_LOG_INFO(logger_, "[push] data: " << rec_mirror_data.to_string());

                //
                // generate valListEntry
                // example:
                //  77                                        valListEntry(Sequence):
                //    070100000102FF                          objName: 01 00 00 01 02 FF
                //    6200                                    status: 0
                //    01                                      valTime: not set
                //    6207                                    unit: 7
                //    5200                                    scaler: 0
                //    530163                                  value: 355
                //    01                                      valueSignature: not set
                //
                auto const reg = rec_mirror_data.value("register", OBIS_PROFILE);
                auto const scaler = rec_mirror_data.value<std::uint8_t>("scaler", 0u);
                auto const unit = rec_mirror_data.value<std::uint8_t>("unit", 0u);
                auto const status = rec_mirror_data.value<std::uint32_t>("status", 0u);
                auto const type = rec_mirror_data.value<std::uint16_t>("type", 0u);
                auto const val = rec_mirror_data.value("reading", "");
                auto const obj = cyng::restore(val, type);

                data.push_back(cyng::make_object(sml::list_entry(
                    reg,
                    status, // status
                    unit,   // unit
                    scaler, // scaler
                    obj)    // value
                                                 ));
            }
        }

        //
        //  generate payload - SML_GetList.Res
        //
        sml::response_generator gen;
        auto const trx = *++trx_;
        return gen.get_list(
            trx,
            cyng::buffer_t{}, // null
            meter_,           // meter id
            cyng::obis{},     // => null
            data,             // data
            ops               // seconds index (operation counter)
        );

        // return payload;
    }
    cyng::obis_path_t push::get_registers(cyng::table const *tbl) {
        BOOST_ASSERT(boost::algorithm::equals(tbl->meta().get_name(), "pushRegister"));

        cyng::obis_path_t regs;

        //  doesn't work with GNU 8.4.0
        // tbl->loop<cyng::buffer_t, std::uint8_t, std::uint8_t, cyng::obis>(
        //    [&](cyng::buffer_t meter, std::uint8_t nr, std::uint8_t idx, cyng::obis reg) -> bool {
        //        if (meter == meter_ && nr == nr_) {
        //            regs.push_back(reg);
        //        }
        //        return true;
        //    });

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
