/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_IPT_SEGW_CACHE_H
#define NODE_IPT_SEGW_CACHE_H

#include "segw.h"
#include <smf/sml/status.h>
#include <smf/ipt/config.h>

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
	 * read a configuration object from table "_Cfg"
	 */
	cyng::object get_obj(cyng::store::table const* tbl, cyng::table::key_type&&);

	/**
	 * read a configuration value from table "_Cfg"
	 */
	template <typename T >
	T get_cfg(cyng::store::table const* tbl, cyng::table::key_type&& key, T def) {
		return cyng::value_cast(get_obj(tbl, std::move(key)), def);
	}

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
		std::uint64_t get_status_word() const;

		/**
		 * not thread safe.
		 *
		 * @return true if STATUS_BIT_NOT_AUTHORIZED_IPT is false
		 */
		bool is_authorized() const;

		/**
		 * get configured server ID (OBIS_SERVER_ID:)
		 */
		cyng::buffer_t get_srv_id();

		/**
		 * read a configuration object from table "_Cfg"
		 */
		cyng::object get_obj(std::string name);

		/**
		 * read a configuration value from table "_Cfg"
		 */
		template <typename T >
		T get_cfg(std::string name, T def) {
			return cyng::value_cast(get_obj(name), def);
		}

		/**
		 * Solves the problem with "defs".
		 * The non-template function wins.
		 */
		std::string get_cfg(std::string name, const char* def) {
			return cyng::value_cast(get_obj(name), def);
		}

		/**
		 * set/insert a configuration value
		 */
		template <typename T >
		bool set_cfg(std::string name, T val) {
			return merge_cfg(name, cyng::make_object(val));
		}

		template <typename T >
		bool set_cfg(std::initializer_list<std::string> slist, T val) {
			return merge_cfg(build_cfg_key(slist), cyng::make_object(val));
		}

		/**
		 * set/insert a configuration value
		 */
		bool merge_cfg(std::string name, cyng::object&& obj);

		void read_table(std::string const&, std::function<void(cyng::store::table const*)>) const;
		void read_tables(std::string const&, std::string const&, std::function<void(cyng::store::table const*, cyng::store::table const*)>) const;
		void write_table(std::string const&, std::function<void(cyng::store::table*)>);
		void write_tables(std::string const&, std::string const&, std::function<void(cyng::store::table*, cyng::store::table*)>);
		void clear_table(std::string const&);

		/**
		 * Convinience function to loop over one table with read access
		 */
		void loop(std::string const&, std::function<bool(cyng::table::record const&)>);

		/**
		 * Search "_DeviceMBUS" table for specified server ID
		 * and return the AES key.
		 * If the server was not found an empty key returns.
		 */
		cyng::crypto::aes_128_key get_aes_key(cyng::buffer_t);

		/**
		 * Lookup "_DeviceMBUS" table and insert device
		 * if not found
		 *
		 * @return true if device was inserted (new device)
		 */
		bool update_device_table(cyng::buffer_t dev_id
			, std::string manufacturer
			, std::uint32_t status
			, std::uint8_t version
			, std::uint8_t media
			, cyng::crypto::aes_128_key aes_key
			, boost::uuids::uuid tag);

		/**
		 * Direct access to memory database.
		 * Be careful.
		 */
		cyng::store::db& get_db();

		/**
		 * @return the target name of the specified push op
		 */
		std::string get_target(cyng::buffer_t const& srv_id, std::uint8_t nr);

		/**
		 * set lower bound time stamp index in table "_TPushOps"
		 */
		bool update_ts_index(cyng::buffer_t const& srv_id, std::uint8_t nr, std::uint64_t tsidx);

		/**
		 * The complete list of meters requires a virtual, unique index.
		 */
		cyng::buffer_t get_meter_by_id(std::uint8_t) const;

		/**
		 * This method reverses the ordering of the vector. If the vector
		 * contains the server/gateway ID it will be moved to the end 
		 * of the vector.
		 */
		std::vector<cyng::buffer_t> reshuffle(std::vector<cyng::buffer_t>&&) const;

	private:
		/**
		 * build up meta data
		 */
		static cyng::table::meta_map_t get_meta_map();

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

		/**
		 * fast access to OBIS_SERVER_ID: (81 81 C7 82 04 FF)
		 */
		cyng::buffer_t server_id_;

		/**
		 * cached status word
		 */
		sml::status_word_t status_word_;
	};

	/**
	 * @return the specified configuration object
	 */
	cyng::object get_config_obj(cyng::store::table const* tbl, std::string name);

	/**
	 * @return the specified configuration value
	 */
	template <typename T >
	T get_config_value(cyng::store::table const* tbl, std::string name, T def) {
		return cyng::value_cast<T>(get_config_obj(tbl, name), def);
	}

	/**
	 * Collects all meters that are available for the specified user.
	 * The vector has to provide a special ordering with the gateway
	 * as last element.
	 */
	std::vector<cyng::buffer_t> collect_meters_of_user(cyng::store::table const* tbl, std::uint8_t);
}
#endif
