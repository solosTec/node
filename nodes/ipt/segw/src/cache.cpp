/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "cache.h"

#include <cyng/table/meta.hpp>

#include <boost/core/ignore_unused.hpp>

namespace node
{

	cache::cache(cyng::store::db& db, boost::uuids::uuid tag)
		: db_(db)
		, tag_(tag)
	{
		//
		//	prepare table _Cfg
		//	status word, etc...
		//
	}

	boost::uuids::uuid const cache::get_tag() const
	{
		return tag_;
	}

	cyng::table::meta_map_t cache::get_meta_map()
	{
		auto vec = create_cache_meta_data();
		cyng::table::meta_map_t mm;
		for (auto tbl : vec) {
			mm.emplace(tbl->get_name(), tbl);
		}
		return mm;
	}

	void cache::set_status_word(sml::status_bits code, bool b)
	{
		//
		//	atomic status update
		//
		db_.access([&](cyng::store::table* tbl) {

			//
			//	get current status word
			//
			auto word = get_config_value(tbl, "status.word", sml::status::get_initial_value());

			//
			//	set/remove flag
			//
			node::sml::status status(word);
			status.set_flag(code, b);

			//
			//	write back to cache
			//
			set_config_value(tbl, "status.word", word);

		}, cyng::store::write_access("_Cfg"));
	}

	std::uint64_t cache::get_status_word()
	{
		return cyng::value_cast(db_.get_value("_Cfg", "status.word", std::string("val")), sml::status::get_initial_value());
	}

	bool cache::merge_cfg(std::string name, cyng::object obj)
	{
		bool r{ false };
		db_.access([&](cyng::store::table* tbl) {

			r = tbl->merge(cyng::table::key_generator(name)
				, cyng::table::data_generator(obj)
				, 1u	//	only needed for insert operations
				, tag_);

		}, cyng::store::write_access("_Cfg"));

		return r;
	}

	//
	//	initialize static member
	//
	cyng::table::meta_map_t const cache::mm_(cache::get_meta_map());

	cyng::table::meta_vec_t create_cache_meta_data()
	{
		//
		//	SQL table scheme
		//
		cyng::table::meta_vec_t vec
		{
			//
			//	Configuration table
			//
			cyng::table::make_meta_table<1, 1>("_Cfg",
			{ "path"	//	OBIS path, ':' separated values
			, "val"		//	value
			},
			{ cyng::TC_STRING
			, cyng::TC_STRING	//	may vary
			},
			{ 128
			, 256
			})
		};

		return vec;
	}

	void init_cache(cyng::store::db& db)
	{
		//
		//	create all tables
		//
		for (auto const& tbl : cache::mm_) {
			db.create_table(tbl.second);
		}
	}

}
