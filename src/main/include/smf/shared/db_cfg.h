/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_DB_CFG_H
#define NODE_DB_CFG_H

#if defined NODE_DB_SCHEMES_H
#error cannot mix db_schemes.h and db_meta.h
#endif

namespace cyng {
	namespace store {
		class db;
		class table;
	}
}

#include <cyng/table/meta_interface.h>
#include <cyng/value_cast.hpp>

namespace node 
{

	/**
	 * Get value from "_Config" table.
	 * Set e read lock on "_Config" table.
	 */
	cyng::object get_config(cyng::store::db& db, std::string key);
	cyng::object get_config(cyng::store::table const* tbl, std::string key);

	/**
	 * Convinience function to get a config value of a specific type
	 */
	template <typename T>
	T get_config_typed(cyng::store::db& db, std::string key, T def) {
		return cyng::value_cast<T>(get_config(db, key), def);
	}

	/**
	 * Convinience function to get a config value of a specific type
	 */
	template <typename T>
	T get_config_typed(cyng::store::table const* tbl, std::string key, T def) {
		return cyng::value_cast<T>(get_config(tbl, key), def);
	}

	/**
	 * Failes if a records with the same key already exists.
	 */
	template <typename T>
	bool insert_config(cyng::store::db& db, std::string key, T&& val, boost::uuids::uuid tag)
	{
		return db.insert("_Config"
			, cyng::table::key_generator(key)
			, cyng::table::data_generator(std::move(val))
			, 1u
			, tag);
	}

	/**
	 * Failes if a records with the same key already exists.
	 */
	template <typename T>
	bool insert_config(cyng::store::table* tbl, std::string key, T&& val, boost::uuids::uuid tag)
	{
		if (boost::algorithm::equals(tbl->meta().get_name(), "_Config")) {
			return tbl->insert(cyng::table::key_generator(key)
				, cyng::table::data_generator(std::move(val))
				, 1u
				, tag);
		}
		return false;
	}

	template <typename T>
	bool update_config(cyng::store::db& db, std::string key, T&& val, boost::uuids::uuid tag)
	{
		return db.update("_Config"
			, cyng::table::key_generator(key)
			, cyng::table::data_generator(std::move(val))
			, 1u	//	only needed for insert operations
			, tag);
	}

	template <typename T>
	bool update_config(cyng::store::table* tbl, std::string key, T&& val, boost::uuids::uuid tag)
	{
		if (boost::algorithm::equals(tbl->meta().get_name(), "_Config")) {
			return tbl->update(cyng::table::key_generator(key)
				, cyng::table::data_generator(std::move(val))
				, 1u	//	only needed for insert operations
				, tag);
		}
		return false;
	}

}

#endif

