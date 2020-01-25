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
	cyng::table::meta_table_ptr create_profile_meta(std::string name);
	cyng::table::meta_table_ptr create_profile_storage(std::string name);


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

		/**
		 * Update a single parameter.
		 *
		 * @param tbl table name
		 * @param key key value(s)
		 * @param param parameter to update
		 */
		bool update(std::string tbl
			, cyng::table::key_type const& key
			, cyng::param_t const& param
			, std::uint64_t gen);

		/**
		 * Update a complete record
		 *
		 * @param tbl table name
		 * @param key key value(s)
		 * @param col column to update
		 * @param obj value
		 */
		bool update(std::string tbl
			, cyng::table::key_type const& key
			, cyng::table::data_type const& body
			, std::uint64_t gen);

		/**
		 * Insert a record
		 *
		 * @param tbl table name
		 * @param key key value(s)
		 * @param body body value(s)
		 * @param gen generation
		 * @param source data source
		 */
		bool insert(std::string tbl
			, cyng::table::key_type const& key
			, cyng::table::data_type const& body
			, std::uint64_t gen);

		bool remove(std::string tbl
			, cyng::table::key_type const& key);

		std::uint64_t count(std::string tbl);

		bool exists(std::string tbl
			, cyng::table::key_type const& key);

		/**
		 * meta data of 1 minute profile
		 */
		void merge_profile_meta_8181C78610FF(cyng::buffer_t srv_id
			, std::uint64_t minutes
			, std::chrono::system_clock::time_point ts
			, std::uint32_t status);

		/**
		 * meta data of 15 minute profile
		 */
		void merge_profile_meta_8181C78611FF(cyng::buffer_t srv_id
			, std::uint64_t quarters	//	quarter hours
			, std::chrono::system_clock::time_point ts
			, std::uint32_t status);

		/**
		 * meta data of 60 minute profile
		 */
		void merge_profile_meta_8181C78612FF(cyng::buffer_t srv_id
			, std::uint64_t hours
			, std::chrono::system_clock::time_point ts
			, std::uint32_t status);

		/**
		 * readout data of 15 minute profile
		 */
		void merge_profile_storage_8181C78611FF(cyng::buffer_t srv_id
			, std::uint64_t hours
			, cyng::buffer_t code
			, cyng::object val
			, cyng::object scaler
			, cyng::object unit
			, cyng::object type);

		/**
		 * readout data of 60 minute profile
		 */
		void merge_profile_storage_8181C78612FF(cyng::buffer_t srv_id
			, std::uint64_t hours
			, cyng::buffer_t code
			, cyng::object val
			, cyng::object scaler
			, cyng::object unit
			, cyng::object type);

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
