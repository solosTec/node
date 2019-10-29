/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_IPT_SEGW_CACHE_H
#define NODE_IPT_SEGW_CACHE_H

#include <smf/sml/status.h>

#include <cyng/store/db.h>
#include <cyng/io/serializer.h>

#include <boost/uuid/uuid.hpp>
#include <boost/algorithm/string.hpp>

namespace node
{

	/**
	 * create a map with all table meta data.
	 */
	cyng::table::meta_vec_t create_cache_meta_data();

	/**
	 * create all required cache tables
	 */
	void init_cache(cyng::store::db&);

	/**
	 * manage cached tables
	 */
	class bridge;
	class cache
	{
		friend class bridge;
		friend void init_cache(cyng::store::db&);

	public:
		cache(cyng::store::db&, boost::uuids::uuid tag);

		/**
		 * @return itentity/source tag
		 */
		boost::uuids::uuid const get_tag() const;

		/**
		 * atomic change of status word
		 */
		void set_status_word(sml::status_bits, bool);

		/**
		 * read status word from "_Cfg" cache 
		 */
		std::uint64_t get_status_word();

		/**
		 * read a configuration value
		 */
		template <typename T >
		T get_cfg(std::string name, T def) {
			return cyng::value_cast(db_.get_value("_Cfg", name, std::string("val")), def);
		}

		/**
		 * set/insert a configuration value
		 */
		template <typename T >
		bool set_cfg(std::string name, T val) {
			return merge_cfg(name, cyng::io::to_str(cyng::make_object(val)));
		}

		bool merge_cfg(std::string name, std::string&& val);

	private:
		/**
		 * build up meta data
		 */
		static cyng::table::meta_map_t get_meta_map();

		/**
		 * @return the specified configuration value
		 */
		template <typename T >
		T get_config_value(cyng::store::table* tbl, std::string name, T def) {

			if (boost::algorithm::equals(tbl->meta().get_name(), "_Cfg")) {
				auto const rec = tbl->lookup(cyng::table::key_generator(name));
				if (!rec.empty())	return cyng::value_cast<T>(rec["val"], def);
			}
			return def;
		}

		/**
		 * Insert/update a configuration value
		 */
		template <typename T >
		bool set_config_value(cyng::store::table* tbl, std::string name, T val) {

			if (boost::algorithm::equals(tbl->meta().get_name(), "_Cfg")) {
				return tbl->merge(cyng::table::key_generator(name)
					, cyng::table::data_generator(std::move(val))
					, 1u	//	only needed for insert operations
					, tag_);
			}
			return false;
		}

	private:
		static cyng::table::meta_map_t const mm_;

		/**
		 * global data cache
		 */
		cyng::store::db& db_;

		/**
		 * source tag
		 */
		boost::uuids::uuid const tag_;
	};


}
#endif
