/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_STORAGE_FUNCTIONS_H
#define SMF_SEGW_STORAGE_FUNCTIONS_H

#include <cyng/db/session.h>
#include <cyng/log/logger.h>
#include <cyng/store/meta.h>

namespace smf {

    std::vector<cyng::meta_store> get_store_meta_data();
    std::vector<cyng::meta_sql> get_sql_meta_data();

    /**
     * get the SQL meta data of the specified in-memory store.
     */
    cyng::meta_sql get_sql_meta_data(std::string);

    cyng::meta_store get_store_cfg();
    cyng::meta_store get_store_oplog();
    cyng::meta_store get_store_meter_mbus();
    cyng::meta_store get_store_data_collector();
    /**
     * OBIS codes from the data collector
     */
    cyng::meta_store get_store_data_mirror();
    cyng::meta_store get_store_push_ops();
    cyng::meta_store get_store_push_register();

    /**
     * Contains latest readout data.
     */
    cyng::meta_store get_store_readout();
    cyng::meta_store get_store_readout_data();

    cyng::meta_sql get_table_cfg();
    cyng::meta_sql get_table_oplog();
    cyng::meta_sql get_table_meter_mbus();
    cyng::meta_sql get_table_meter();
    cyng::meta_sql get_table_data_collector();
    cyng::meta_sql get_table_data_mirror();
    cyng::meta_sql get_table_push_ops();
    cyng::meta_sql get_table_push_register();

    /**
     * create all tables defined in get_sql_meta_data()
     */
    bool init_storage(cyng::db::session &);

    /**
     * Clear table "TCfg"
     */
    void clear_config(cyng::db::session &);
    void list_config(cyng::db::session &);
    bool set_config_value(cyng::db::session &, std::string const &path, std::string const &value, std::string const &type);
    bool add_config_value(cyng::db::session &, std::string const &path, std::string const &value, std::string const &type);
    bool del_config_value(cyng::db::session &, std::string const &path);
    bool set_nms_mode(cyng::db::session &db, std::string const &mode);

    bool alter_table(cyng::db::session &db, std::string const &table);
    bool recreate_table(cyng::db::session &db, cyng::meta_sql const &m);

    /**
     * split string by "/" and return the first section
     * by default
     */
    std::string get_section(std::string, std::size_t = 0);

    bool insert_config_record(
        cyng::db::statement_ptr stmt_insert,
        cyng::db::statement_ptr stmt_select,
        std::string key,
        cyng::object obj,
        std::string desc);
    bool insert_config_record(cyng::db::statement_ptr stmt, cyng::object const &, cyng::object obj);
    bool update_config_record(cyng::db::statement_ptr stmt, cyng::object const &key, cyng::object obj);
    bool remove_config_record(cyng::db::statement_ptr stmt, cyng::object const &key);
    cyng::object read_config_record(cyng::db::statement_ptr stmt, cyng::object const &key);
} // namespace smf

#endif
