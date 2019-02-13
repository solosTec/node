/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_DB_GATEWAY_H
#define NODE_DB_GATEWAY_H

namespace cyng {
	namespace store {
		class db;
	}
}
#include <cyng/table/meta_interface.h>

 /** @file db_meta.h 
  * Similar to db_schemes.h but with focus on gateway tables to save disk space.
  *
  * Since both implementation using the same interface they cannot be used in the same
  * complation unit!
  */

namespace node 
{
	/**
	 * Provide meta data of the specified table. If table name
	 * unknown the return value is an empty pointer.
	 */
	cyng::table::meta_table_ptr create_meta(std::string);

	/**
	 * create a table with the specified name.
	 */
	bool create_table(cyng::store::db&, std::string);

}

#endif

