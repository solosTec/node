/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <tasks/persistence.h>

#include <smf/config/persistence.h>
#include <smf/obis/defs.h>
#include <smf/sml/event.h>
#include <storage.h>

#include <cyng/db/session.h>
#include <cyng/log/record.h>
#include <cyng/sql/sql.hpp>
#include <cyng/store/slot.h>

#include <boost/algorithm/string.hpp>

namespace smf {
    persistence::persistence(cyng::channel_weak wp, cyng::controller &ctl, cyng::logger logger, cfg &config, storage &s)
        : sigs_{
            std::bind(&persistence::insert, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5), //    "db.insert"
            std::bind(&persistence::modify, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5), //    "db.modify"
            std::bind(&persistence::remove, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), // "db.remove"
            std::bind(&persistence::clear, this, std::placeholders::_1, std::placeholders::_2), // "db.clear"
            std::bind(&persistence::trx, this, std::placeholders::_1, std::placeholders::_2), // "db.trx"
            std::bind(&persistence::power_return, this), // "oplog.power.return"
            std::bind(&persistence::authorized, this, std::placeholders::_1), // 
            std::bind(&persistence::stop, this, std::placeholders::_1)
        }
        , channel_(wp)
        , logger_(logger)
        , cfg_(config)
        , storage_(s)
        , distributor_(logger, ctl, config) {

        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names(
                {"db.insert", "db.modify", "db.remove", "db.clear", "db.trx", "oplog.power.return", "oplog.authorized"});
            CYNG_LOG_TRACE(logger_, "task [" << sp->get_name() << "] created");
        }

        connect();
        CYNG_LOG_INFO(logger_, "[persistence] ready");
    }

    void persistence::stop(cyng::eod) { CYNG_LOG_INFO(logger_, "persistence stopped"); }

    void persistence::connect() {

        CYNG_LOG_INFO(logger_, "[persistence] connecting");
        connect("cfg");
        connect("meterMBus");
        connect("dataCollector");
        connect("dataMirror");
        connect("pushOps");
        connect("pushRegister");
        connect("mirror");
        connect("mirrorData");
    }

    void persistence::connect(std::string name) {
        cfg_.get_cache().connect_only(name, cyng::make_slot(channel_));
        CYNG_LOG_TRACE(logger_, "[persistence] table " << name << " is connected");
    }

    void persistence::power_return() {

        storage_.generate_op_log(
            cfg_.get_status_word(),
            sml::LOG_CODE_09,  //	0x00100023 - power return
            OBIS_PEER_SCM,     //	peer: source is SCM
            cfg_.get_srv_id(), //	server ID
            "",                //	target
            0,                 //	nr
            "power return");   //	description
    }

    void persistence::authorized(bool success) {
        storage_.generate_op_log(
            cfg_.get_status_word(),
            (success ? sml::LOG_CODE_61 : sml::LOG_CODE_65),
            OBIS_PEER_ADDRESS_WANGSM, //	source is WANGSM
            cfg_.get_srv_id(),        //	server ID
            "",                       //	target
            0,                        //	nr
            "IP-T access");           //	description
    }

    void persistence::insert(cyng::table const *tbl, cyng::key_t key, cyng::data_t data, std::uint64_t gen, boost::uuids::uuid) {

        BOOST_ASSERT_MSG(!key.empty(), "[persistence] empty key");
        // CYNG_LOG_TRACE(logger_, "[persistence] insert " << tbl->meta().get_name());
        if (boost::algorithm::equals(tbl->meta().get_name(), "cfg")) {
            CYNG_LOG_TRACE(logger_, "[persistence] " << tbl->meta().get_name() << "- " << key.at(0) << " := " << data.at(0));
            if (!storage_.cfg_insert(key.at(0), data.at(0))) {
                CYNG_LOG_WARNING(logger_, "[persistence] insert " << tbl->meta().get_name() << " <" << key.at(0) << "> failed");
            }
            //} else if (boost::algorithm::equals(tbl->meta().get_name(), "meter")) {
            //    //  convert data
        } else {
            //
            // use default mechanism
            //
            auto const meta = get_sql_meta_data(tbl->meta().get_name());
            if (!config::persistent_insert(meta, storage_.db_, key, data, gen)) {
                CYNG_LOG_ERROR(logger_, "[persistence] insert " << tbl->meta().get_name() << ": " << key.at(0) << " failed");
            }
        }
    }

    /**
     * signal an modify event
     */
    void persistence::modify(cyng::table const *tbl, cyng::key_t key, cyng::attr_t attr, std::uint64_t gen, boost::uuids::uuid) {

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
            auto const meta = get_sql_meta_data(tbl->meta().get_name());
            CYNG_LOG_TRACE(logger_, "modify " << meta.get_name() << "[" << attr.first << "] = " << attr.second);
            if (!config::persistent_update(meta, storage_.db_, key, attr, gen)) {
                CYNG_LOG_ERROR(logger_, "[persistence] modify " << meta.get_name() << ": " << key.at(0) << " failed");
            }
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
            auto const meta = get_sql_meta_data(tbl->meta().get_name());
            CYNG_LOG_TRACE(logger_, "remove " << meta.get_name());
            if (!config::persistent_remove(meta, storage_.db_, key)) {
                CYNG_LOG_ERROR(logger_, "[persistence] remove " << meta.get_name() << ": " << key.at(0) << " failed");
            }
        }
    }

    /**
     * signal an clear event
     */
    void persistence::clear(cyng::table const *tbl, boost::uuids::uuid) {

        auto const meta = get_sql_meta_data(tbl->meta().get_name());
        CYNG_LOG_TRACE(logger_, "clear " << meta.get_name());
        if (!config::persistent_clear(meta, storage_.db_)) {
            CYNG_LOG_ERROR(logger_, "[persistence] clear " << meta.get_name() << " failed");
        }
    }

    void persistence::trx(cyng::table const *tbl, bool) { CYNG_LOG_TRACE(logger_, "trx " << tbl->meta().get_name()); }

} // namespace smf
