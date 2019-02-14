/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_DB_SCHEMES_H
#define NODE_DB_SCHEMES_H

#if defined NODE_DB_GATEWAY_H
#error cannot mix db_schemes.h and db_meta.h
#endif

namespace cyng {
	namespace store {
		class db;
	}
}

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

