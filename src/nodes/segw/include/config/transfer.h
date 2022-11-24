/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_CONFIG_TRANSFER_H
#define SMF_SEGW_CONFIG_TRANSFER_H

#include <cfg.h>

#include <cyng/db/session.h>

#include <fstream>
#include <vector>

namespace smf {

    /**
     * Read a JSON configuration and store it in the config database.
     * Transfer configuration data available through the DOM reader
     * into the database.
     *
     * Precondition: Table "TCfg" must exist.

     */
    void read_json_config(cyng::db::session &db, cyng::object &&cfg);

    void read_net(cyng::db::statement_ptr stmt_insert, cyng::db::statement_ptr stmt_select, cyng::param_map_t &&pmap);
    void read_ipt_config(cyng::db::statement_ptr stmt_insert, cyng::db::statement_ptr stmt_select, cyng::vector_t &&vec);
    void read_ipt_params(cyng::db::statement_ptr stmt_insert, cyng::db::statement_ptr stmt_select, cyng::param_map_t &&pmap);
    void read_hardware(cyng::db::statement_ptr stmt_insert, cyng::db::statement_ptr stmt_select, cyng::param_map_t &&pmap);
    void read_hardware_adapter(cyng::db::statement_ptr stmt_insert, cyng::db::statement_ptr stmt_select, cyng::param_map_t &&pmap);
    void read_sml(cyng::db::statement_ptr stmt_insert, cyng::db::statement_ptr stmt_select, cyng::param_map_t &&pmap);
    void read_nms(cyng::db::statement_ptr stmt_insert, cyng::db::statement_ptr stmt_select, cyng::param_map_t &&pmap);
    void read_lnm(cyng::db::statement_ptr stmt_insert, cyng::db::statement_ptr stmt_select, cyng::vector_t &&vec);
    void read_gpio(cyng::db::statement_ptr stmt_insert, cyng::db::statement_ptr stmt_select, cyng::param_map_t &&pmap);

    void read_broker(
        cyng::db::statement_ptr stmt_insert,
        cyng::db::statement_ptr stmt_select,
        std::size_t counter,
        cyng::vector_t &&vec);
    void read_listener(
        cyng::db::statement_ptr stmt_insert,
        cyng::db::statement_ptr stmt_select,
        std::size_t counter,
        cyng::param_map_t &&pmap);
    void read_cache(
        cyng::db::statement_ptr stmt_insert,
        cyng::db::statement_ptr stmt_select,
        std::size_t counter,
        cyng::param_map_t &&pmap);
    void read_blocklist(
        cyng::db::statement_ptr stmt_insert,
        cyng::db::statement_ptr stmt_select,
        std::size_t counter,
        cyng::param_map_t &&pmap);
    void read_virtual_meter(
        cyng::db::statement_ptr stmt_insert,
        cyng::db::statement_ptr stmt_select,
        std::size_t counter,
        cyng::param_map_t &&pmap);

    /**
     * write a JSON configuration with the content of the config database.
     */
    void write_json_config(cyng::db::session &db, cyng::object &&cfg, std::filesystem::path file_name, cyng::param_map_t const &s);
    void write_json_config(cyng::db::session &db, std::ofstream &, cyng::param_map_t const &s);
    cyng::param_map_t read_config(cyng::db::session &db, cyng::param_map_t const &s);
    void insert_value(std::vector<std::string> const &, cyng::param_map_t *, cyng::object);
    cyng::param_map_t transform_config(cyng::param_map_t &&);

} // namespace smf

#endif
