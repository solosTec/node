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

	 std::vector< cyng::meta_store > get_store_meta_data();
	 std::vector< cyng::meta_sql > get_sql_meta_data();

	 cyng::meta_store get_store_cfg();
	 cyng::meta_store get_store_oplog();

	 cyng::meta_sql get_table_cfg();
	 cyng::meta_sql get_table_oplog();

	 /**
	  * create all tables defined in get_sql_meta_data()
	  */
	 bool init_storage(cyng::db::session&);

	 /**
	  * Transfer configuration data available through the DOM reader
	  * into the database.
	  *
	  * Precondition: Table "TCfg" must exist.
	  */
	 void transfer_config(cyng::db::session&, cyng::object&& cfg);
	 void transfer_ipt_config(cyng::db::statement_ptr stmt, cyng::vector_t&& vec);
	 void transfer_ipt_params(cyng::db::statement_ptr stmt, cyng::param_map_t&& pmap);
	 void transfer_sml(cyng::db::statement_ptr stmt, cyng::param_map_t&& pmap);
	 void transfer_nms(cyng::db::statement_ptr stmt, cyng::param_map_t&& pmap);
	 void transfer_lnm(cyng::db::statement_ptr stmt, cyng::vector_t&& vec);
	 void transfer_gpio(cyng::db::statement_ptr stmt, cyng::param_map_t&& pmap);

	 /**
	  * Clear table "TCfg"
	  *
	  * @return count of affected rows.
	  */
	 void clear_config(cyng::db::session&);

	 bool insert_config_record(cyng::db::statement_ptr stmt, std::string key, cyng::object obj, std::string desc);
}

#endif