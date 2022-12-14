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
        BOOST_ASSERT(sml::is_profile(profile));
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
        auto const now = cyng::make_utc_date();
        auto sp = channel_.lock();
        BOOST_ASSERT_MSG(sp, "store task already stopped");
        if (sp) {
            auto const interval = sml::interval_time(now, profile_);

            sp->suspend(interval, "run");

            CYNG_LOG_TRACE(logger_, "[store] run " << obis::get_name(profile_) << " at " << now + interval);
        }
    }

    void store::run() {

        auto sp = channel_.lock();
        BOOST_ASSERT_MSG(sp, "store task already stopped");
        if (sp) {
            auto const now = cyng::make_utc_date();
            auto const interval = sml::interval_time(now, profile_);
            sp->suspend(interval, "run");
            CYNG_LOG_TRACE(
                logger_, "[store] " << obis::get_name(profile_) << " - next: " << std::chrono::system_clock::now() + interval);

            //
            //  transfer readout data for all defined profiles
            //
            auto const stats = transfer(now);
            CYNG_LOG_INFO(
                logger_, "[store] " << obis::get_name(profile_) << " - readouts: " << stats.first << ", records: " << stats.second);
        }
    }

    std::pair<std::size_t, std::size_t> store::transfer(cyng::date now) {
        std::size_t meta_count = 0, data_count = 0;

        cfg_.get_cache().access(
            [&, this](
                cyng::table const *tbl_data_col,
                cyng::table const *tbl_ro,
                cyng::table const *tbl_ro_data,
                cyng::table *tbl_mirror,
                cyng::table *tbl_mirror_data) {
                tbl_data_col->loop([&, this](cyng::record &&rec, std::size_t) -> bool {
                    auto const profile = rec.value("profile", profile_);
                    BOOST_ASSERT(sml::is_profile(profile));
                    auto const active = rec.value("active", false);

                    if (!active) {
                        CYNG_LOG_WARNING(logger_, "[store] transfer " << obis::get_name(profile_) << " is deactivated");
                    }

                    if ((profile == profile_) && active) {
                        //
                        //  transfer from "readout" + "readoutData" => "mirror" + "mirrorData"
                        //
                        auto const meter = rec.value<cyng::buffer_t>("meterID", {});
                        BOOST_ASSERT(!meter.empty());
                        auto const max_size = rec.value<std::uint32_t>("maxSize", 0);
                        CYNG_LOG_TRACE(logger_, "[store] transfer " << obis::get_name(profile_) << ", meter: " << meter);

                        auto const pk = cyng::key_generator(meter);
                        auto const rec_ro = tbl_ro->lookup(pk);
                        if (!rec_ro.empty()) {

                            //
                            //  transfer "readout" => "mirror"
                            //
                            auto const received = rec_ro.value("received", now);
                            auto const idx = sml::to_index(received, profile_);
                            //  "meterID", "profile", "idx"
                            if (idx.second) {
                                auto const pk_mirror = cyng::extend_key(pk, profile_, idx.first);
                                BOOST_ASSERT_MSG(pk_mirror.size() == 3, "wrong key size for table mirror");
                                if (tbl_mirror->insert(pk_mirror, rec_ro.data(), 1, cfg_.get_tag())) {
                                    ++meta_count;

                                    tbl_ro_data->loop([&, this](cyng::record &&rec_ro_data, std::size_t) -> bool {
                                        auto const meter_ro = rec_ro_data.value("meterID", meter);
#ifdef _DEBUG
                                        CYNG_LOG_DEBUG(
                                            logger_,
                                            "[store] transfer " << obis::get_name(profile_) << ", meter: " << meter << "/"
                                                                << meter_ro);
                                    //{
                                    //    auto const data = rec_ro_data.data();
                                    //    for (auto const &obj : data) {
                                    //        // BOOST_ASSERT(obj); //  null values not allowed
                                    //        if (obj) {
                                    //            CYNG_LOG_DEBUG(logger_, "[store] transfer " << obj);
                                    //        } else {
                                    //            CYNG_LOG_ERROR(logger_, "[store] transfer null value: " << data);
                                    //        }
                                    //    }
                                    //}
#endif
                                        if (meter_ro == meter) {
                                            //
                                            //  transfer "readoutData" => "mirrorData"
                                            //
                                            auto const reg = rec_ro_data.value("register", OBIS_METER_ADDRESS);
                                            //  "meterID", "profile", "idx", "register"
                                            auto const pk_mirror_data = cyng::extend_key(pk_mirror, reg);
                                            BOOST_ASSERT_MSG(pk_mirror_data.size() == 4, "wrong key size for table mirrorData");
                                            CYNG_LOG_TRACE(
                                                logger_,
                                                "[store] transfer " << obis::get_name(profile_) << ", meter: " << meter
                                                                    << ", register: " << reg);
                                            if (tbl_mirror_data->insert(pk_mirror_data, rec_ro_data.data(), 1, cfg_.get_tag())) {
                                                ++data_count;
                                            } else {
                                                CYNG_LOG_ERROR(
                                                    logger_,
                                                    "[store] transfer " << obis::get_name(profile_) << ", meter: " << meter
                                                                        << ", register: " << reg << " failed");
                                            }
                                        }
                                        return true;
                                    });

                                    if (data_count == 0) {
                                        //
                                        //  There is an entry in table "readout" but no data in "readoutData"
                                        //
                                        CYNG_LOG_WARNING(
                                            logger_,
                                            "[store] transfer " << obis::get_name(profile_) << ", meter: " << meter
                                                                << " has no data");
                                    }

                                } else {
                                    CYNG_LOG_ERROR(
                                        logger_,
                                        "[store] transfer " << obis::get_name(profile_) << ", meter: " << meter << " failed");
                                }
                            } else { // invalid receiving time
                                CYNG_LOG_ERROR(
                                    logger_,
                                    "[store] transfer " << obis::get_name(profile_) << ", meter: " << meter
                                                        << " with invalid receiving time: " << received);
                            }
                        } else {
                            //  no readout data
                        }
                    }
                    return true;
                });
            },
            cyng::access::read("dataCollector"),
            cyng::access::read("readout"),
            cyng::access::read("readoutData"),
            cyng::access::write("mirror"),
            cyng::access::write("mirrorData"));

        return {meta_count, data_count};
    }
} // namespace smf
