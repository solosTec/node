/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/obis/defs.h>
#include <smf/sml/event.h>
#include <storage.h>
#include <tasks/persistence.h>

#include <cyng/log/record.h>
#include <cyng/store/slot.h>

#include <boost/algorithm/string.hpp>

namespace smf {
    persistence::persistence(cyng::channel_weak wp, cyng::controller &ctl, cyng::logger logger, cfg &config, storage &s)
        : sigs_{std::bind(&persistence::insert, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5), std::bind(&persistence::modify, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5), std::bind(&persistence::remove, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), std::bind(&persistence::clear, this, std::placeholders::_1, std::placeholders::_2), std::bind(&persistence::trx, this, std::placeholders::_1, std::placeholders::_2), std::bind(&persistence::power_return, this), std::bind(&persistence::stop, this, std::placeholders::_1)}
        , channel_(wp)
        , logger_(logger)
        , cfg_(config)
        , storage_(s)
        , distributor_(logger, ctl, config) {
        auto sp = channel_.lock();
        if (sp) {
            sp->set_channel_names({"db.insert", "db.modify", "db.remove", "db.clear", "db.trx", "power-return"});
            CYNG_LOG_TRACE(logger_, "task [" << sp->get_name() << "] created");
        }

        connect();
        CYNG_LOG_INFO(logger_, "[persistence] ready");
    }

    void persistence::stop(cyng::eod) { CYNG_LOG_INFO(logger_, "persistence stopped"); }

    void persistence::connect() {

        cfg_.get_cache().connect_only("cfg", cyng::make_slot(channel_));
        CYNG_LOG_INFO(logger_, "[persistence] connected");
    }

    void persistence::power_return() {

        storage_.generate_op_log(
            cfg_.get_status_word(),
            sml::LOG_CODE_09,  //	0x00100023 - power return
            OBIS_PEER_SCM,     //	source is SCM
            cfg_.get_srv_id(), //	server ID
            "",                //	target
            0,                 //	nr
            "power return");   //	description
    }

    void persistence::insert(cyng::table const *tbl, cyng::key_t key, cyng::data_t data, std::uint64_t gen, boost::uuids::uuid) {

        CYNG_LOG_TRACE(logger_, "insert " << tbl->meta().get_name());
        if (boost::algorithm::equals(tbl->meta().get_name(), "cfg")) {
            CYNG_LOG_TRACE(logger_, key.at(0) << " := " << data.at(0));
            if (!storage_.cfg_insert(key.at(0), data.at(0))) {
                CYNG_LOG_WARNING(logger_, "[persistence] insert " << tbl->meta().get_name() << " <" << key.at(0) << "> failed");
            }
        } else {
            // use default mechanism
        }
    }

    /**
     * signal an modify event
     */
    void persistence::modify(cyng::table const *tbl, cyng::key_t key, cyng::attr_t attr, std::uint64_t, boost::uuids::uuid) {

        if (boost::algorithm::equals(tbl->meta().get_name(), "cfg")) {
            BOOST_ASSERT(key.size() == 1);
            auto const name = cyng::to_string(key.at(0));
            //  don't store the following values:
            // listener/1/taskIdIPv4
            if (boost::algorithm::ends_with(name, "taskIdIPv4")) {
                return;
            }

            //
            //  make changes permanent
            //
            CYNG_LOG_TRACE(logger_, name << " = " << attr.second);
            if (storage_.cfg_update(key.at(0), attr.second)) {
                distributor_.update(name, attr.second);
            } else {
                CYNG_LOG_WARNING(logger_, "[persistence] update " << tbl->meta().get_name() << " failed");
            }
        } else {
            // use default mechanism
            CYNG_LOG_TRACE(logger_, "modify " << tbl->meta().get_name());
        }
    }

    /**
     * signal an remove event
     */
    void persistence::remove(cyng::table const *tbl, cyng::key_t key, boost::uuids::uuid) {

        CYNG_LOG_TRACE(logger_, "remove " << tbl->meta().get_name());
        if (boost::algorithm::equals(tbl->meta().get_name(), "cfg")) {
            CYNG_LOG_TRACE(logger_, "remove config entry: " << key.at(0));
            if (!storage_.cfg_remove(key.at(0))) {
                CYNG_LOG_WARNING(logger_, "[persistence] remove " << tbl->meta().get_name() << " failed");
            }
        } else {
            // use default mechanism
            CYNG_LOG_TRACE(logger_, "modify " << tbl->meta().get_name());
        }
    }

    /**
     * signal an clear event
     */
    void persistence::clear(cyng::table const *tbl, boost::uuids::uuid) {

        CYNG_LOG_TRACE(logger_, "clear " << tbl->meta().get_name());
    }

    void persistence::trx(cyng::table const *tbl, bool) { CYNG_LOG_TRACE(logger_, "trx " << tbl->meta().get_name()); }

} // namespace smf
