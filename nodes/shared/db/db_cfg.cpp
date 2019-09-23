/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */


#include <smf/shared/db_cfg.h>
#include <cyng/table/meta.hpp>
#include <cyng/store/db.h>
#include <boost/algorithm/string.hpp>

namespace node
{
	cyng::object get_config(cyng::store::db& db, std::string key)
	{
		cyng::object obj;
		db.access([&obj, key](cyng::store::table const* tbl_cfg)->void {
			obj = get_config(tbl_cfg, key);
			}, cyng::store::read_access("_Config"));
		return obj;
	}

	cyng::object get_config(cyng::store::table const* tbl, std::string key)
	{
		//
		//	maybe other tables are coming
		//
		if (boost::algorithm::equals(tbl->meta().get_name(), "_Config")) {
			auto const rec = tbl->lookup(cyng::table::key_generator(key));
			if (!rec.empty())	return rec["value"];
		}
		return cyng::make_object();
	}

	bool insert_config(cyng::store::db& db, std::string key, cyng::object obj, boost::uuids::uuid tag)
	{
		return false;
	}


	//bool update_config(cyng::store::db& db, std::string key, cyng::object obj)
	//{
	//	return false;
	//}
	//bool update_config(cyng::store::table const* tbl, std::string key, cyng::object obj)
	//{
	//	return false;
	//}
}



