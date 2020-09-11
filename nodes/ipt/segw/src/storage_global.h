/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2020 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_SEGW_STORAGE_GLOABL_H
#define NODE_SEGW_STORAGE_GLOABL_H

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
	bool init_storage(cyng::param_map_t&& cfg, cyng::object mac);

	/**
	 * Transfer configuration data available through the DOM reader
	 * into the database. 
	 *
	 * Precondition: Table "TCfg" must exist.
	 */
	bool transfer_config_to_storage(cyng::param_map_t&& cfg, cyng::reader<cyng::object> const& dom);

	/**
	 * Read the specified profile data and print it on screen
	 */
	bool dump_profile_data(cyng::param_map_t&& cfg, cyng::reader<cyng::object> const& dom, std::uint32_t profile);

	/**
	 * List all configured PushOps
	 */
	bool dump_push_ops(cyng::param_map_t&& cfg, cyng::reader<cyng::object> const& dom);

	/**
	 * List all visible and activated devices
	 */
	bool dump_devices(cyng::param_map_t&& cfg, cyng::reader<cyng::object> const& dom);

	/**
	 * List configuration to console
	 */
	bool list_config_data(std::ostream& os, cyng::param_map_t&& cfg, cyng::reader<cyng::object> const& dom);

	/**
	 * Clear table "TCfg"
	 *
	 * @return count of affected rows.
	 */
	int clear_config_from_storage(cyng::param_map_t&& cfg, cyng::reader<cyng::object> const& dom);

	/**
	 * Table TCfg
	 * Initialize a configuration record
	 */
	bool init_config_record(cyng::db::session&, std::string const& key, cyng::object obj);

	/**
	 * @return an SQL select for the specified profile to get
	 * all data.
	 */
	std::string get_sql_select_all(std::uint32_t profile);

	/**
	 * @return an SQL select for the specified profile to get
	 * all data from a specified time range
	 */
	std::string get_sql_select(std::uint32_t profile
		, std::chrono::system_clock::time_point start
		, std::chrono::system_clock::time_point end);


	/**
	 * Takes the time index and calculate the related time point
	 * depending from the specified profile.
	 */
	std::chrono::system_clock::time_point time_index_to_time_point(std::uint32_t profile
		, std::uint64_t tsidx);

	/**
	 * @return profile name
	 */
	std::string get_profile_name(std::uint32_t profile);

	/**
	 * @return first section of the string
	 */
	std::string get_first_section(std::string, char sep);

	/**
	 * Assume that obj is of type mac48 ("00:ff:90:98:57:56")
	 */
	cyng::buffer_t to_gateway_srv_id(cyng::object obj);

	void insert_priv(cyng::db::statement_ptr stmt
		, std::uint8_t nr
		, cyng::buffer_t srv_id
		, sml::obis code
		, std::uint8_t priv);

	bool set_value(cyng::param_map_t&& cfg, std::vector<std::string> const&, bool value);
	bool set_value(cyng::param_map_t&& cfg, std::vector<std::string> const&, std::uint32_t value);
	bool set_value(cyng::param_map_t&& cfg, std::vector<std::string> const&, std::uint16_t value);
	bool set_value(cyng::param_map_t&& cfg, std::vector<std::string> const&, std::string value);
	bool set_value(cyng::param_map_t&& cfg, std::vector<std::string> const&, std::chrono::minutes value);
	bool set_value(cyng::param_map_t&& cfg, std::vector<std::string> const&, std::chrono::seconds value);
	bool set_value(cyng::param_map_t&& cfg, std::vector<std::string> const&, cyng::object value);

}
#endif
