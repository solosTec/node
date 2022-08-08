/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/iec_db_writer.h>

#include <smf/config/schemes.h>

#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/sql/sql.hpp>
#include <cyng/task/channel.h>

#include <iostream>

#include <boost/algorithm/string.hpp>

namespace smf {

    iec_db_writer::iec_db_writer(cyng::channel_weak wp, cyng::controller &ctl, cyng::logger logger, cyng::db::session db)
    : sigs_{			
            std::bind(&iec_db_writer::open, this, std::placeholders::_1),
			std::bind(&iec_db_writer::store, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
			std::bind(&iec_db_writer::commit, this),
			std::bind(&iec_db_writer::stop, this, std::placeholders::_1),
        }
        , channel_(wp)
        , ctl_(ctl)
        , logger_(logger)
        , db_(db)
        , uidgen_()
        , id_() {

        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"open", "store", "commit"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    bool iec_db_writer::init_storage(cyng::db::session db) {
        BOOST_ASSERT(db.is_alive());

        try {

            //
            //	start transaction
            //
            cyng::db::transaction trx(db);

            auto const vec = get_sql_meta_data();
            for (auto const &m : vec) {
                auto const sql = cyng::sql::create(db.get_dialect(), m).to_string();
                if (!db.execute(sql)) {
                    std::cerr << "***error: " << sql << std::endl;
                } else {
                    std::cerr << "***info : " << sql << std::endl;
                }
            }

            return true;
        } catch (std::exception const &ex) {
            boost::ignore_unused(ex);
            std::cerr << "***error: " << ex.what() << std::endl;
        }
        return false;
    }

    void iec_db_writer::stop(cyng::eod) {}
    void iec_db_writer::open(std::string id) { id_ = id; }
    void iec_db_writer::store(cyng::obis code, std::string value, std::string unit) {}
    void iec_db_writer::commit() { CYNG_LOG_TRACE(logger_, "[iec.db.writer] commit \"" << id_ << "\""); }

    std::vector<cyng::meta_store> iec_db_writer::get_store_meta_data() {
        return {config::get_store_iec_readout(), config::get_store_iec_readout_data()};
    }
    std::vector<cyng::meta_sql> iec_db_writer::get_sql_meta_data() {
        return {config::get_table_iec_readout(), config::get_table_iec_readout_data()};
    }

} // namespace smf
