/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_IPT_SEGW_STORAGE_H
#define NODE_IPT_SEGW_STORAGE_H

#include <cyng/store/db.h>
#include <vector>

namespace node
{
	using meta_db_t = std::vector<cyng::table::meta_table_ptr>;

	/**
	 * create a map with all table meta data.
	 */
	meta_db_t create_db_meta_data();

	/**
	 * 
	 */

}
#endif
