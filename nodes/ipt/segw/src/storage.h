/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_SEGW_STORAGE_H
#define NODE_SEGW_STORAGE_H

#include <smf/sml/obis_io.h>

#include <cyng/store/db.h>
#include <cyng/sql.h>
#include <cyng/db/session_pool.h>
#include <cyng/dom/reader.h>

namespace node
{
	cyng::table::meta_table_ptr create_meta_load_profile(std::string name);
	cyng::table::meta_table_ptr create_meta_data_storage(std::string name);

	/**
	 * create a map with all table meta data.
	 */
	cyng::table::meta_vec_t create_storage_meta_data();

	/**
	 * create all required SQL tables
	 */
	bool init_storage(cyng::param_map_t&& cfg);

	/**
	 * Transfer configuration data available through the DOM reader
	 * into the database. 
	 *
	 * Precondition: Table "TCfg" must exist.
	 */
	bool transfer_config_to_storage(cyng::param_map_t&& cfg, cyng::reader<cyng::object> const& dom);

	/**
	 * Table TCfg
	 * Initialize a configuration record
	 */
	bool init_config_record(cyng::db::session&, std::string const& key, cyng::object obj);

	/**
	 * helper function to build a key for table TCfg
	 */
	std::string build_cfg_key(sml::obis_path path);

	/**
	 * helper function to build a key for table TCfg
	 */
	std::string build_cfg_key(sml::obis_path path, std::string leaf);

	/**
	 * manage SQL tables
	 */
	class storage
	{
		friend bool init_storage(cyng::param_map_t&&);
		friend bool init_config_record(cyng::db::session&, std::string const&, cyng::object);

	public:
		using loop_f = std::function<bool(cyng::table::record const&)>;

	public:
		storage(boost::asio::io_service&, cyng::db::connection_type);

		/**
		 * start connection pool
		 */
		bool start(cyng::param_map_t cfg);

		void generate_op_log(std::uint64_t status
			, std::uint32_t evt
			, sml::obis code
			, cyng::buffer_t srv
			, std::string target
			, std::uint8_t nr
			, std::string description);

		/**
		 * loop over all records in the specified table
		 */
		void loop(std::string name, loop_f);

		/**
		 * Update a single value.
		 *
		 * @param tbl table name
		 * @param key key value(s)
		 * @param col column to update
		 * @param obj value
		 */
		bool update(std::string tbl
			, cyng::table::key_type const& key
			, std::string col
			, cyng::object obj
			, std::uint64_t gen);

		bool insert(std::string tbl
			, cyng::table::key_type const& key
			, cyng::table::data_type const& body
			, std::uint64_t gen
			, boost::uuids::uuid source);

		bool remove(std::string tbl
			, cyng::table::key_type const& key
			, boost::uuids::uuid source);

	private:
		/**
		 * build up meta data
		 */
		static cyng::table::meta_map_t get_meta_map();

		/**
		 * create command for specified table
		 */
		cyng::sql::command create_cmd(std::string, cyng::sql::dialect);

	private:
		static cyng::table::meta_map_t const mm_;
		cyng::db::session_pool pool_;
	};


}
#endif
