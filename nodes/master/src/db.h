/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_MASTER_DB_H
#define NODE_MASTER_DB_H

#include <cyng/log.h>
#include <cyng/store/db.h>
#include <boost/uuid/uuid.hpp>

namespace node 
{
	void init(cyng::logging::log_ptr logger
		, cyng::store::db&
		, boost::uuids::uuid tag
		, std::string country_code
		, std::string language_code
		, boost::asio::ip::tcp::endpoint
		, std::uint64_t global_config
		, boost::filesystem::path stat_dir
		, std::uint64_t max_messages
		, std::uint64_t max_events);

	void insert_msg(cyng::store::db&
		, cyng::logging::severity
		, std::string const&
		, boost::uuids::uuid tag);

	void insert_msg(cyng::store::table* tbl
		, cyng::logging::severity
		, std::string const&
		, boost::uuids::uuid tag
		, std::uint64_t max_messages);

	void insert_ts_event(cyng::store::db&
		, boost::uuids::uuid tag
		, std::string const& account
		, std::string const& evt
		, cyng::object);

	void insert_ts_event(cyng::store::table* tbl
		, boost::uuids::uuid tag
		, std::string const& account
		, std::string const& evt
		, cyng::object
		, std::uint64_t max_events);

	void insert_lora_uplink(cyng::store::db& db
		, std::chrono::system_clock::time_point tp
		, cyng::mac64 devEUI
		, std::uint16_t FPort
		, std::uint32_t FCntUp
		, std::uint32_t ADRbit
		, std::uint32_t MType
		, std::uint32_t FCntDn
		, std::string const& customerID
		, std::string const& payload
		, boost::uuids::uuid tag
		, boost::uuids::uuid origin);

	void insert_lora_uplink(cyng::store::table* tbl
		, std::chrono::system_clock::time_point tp
		, cyng::mac64 devEUI
		, std::uint16_t FPort
		, std::uint32_t FCntUp
		, std::uint32_t ADRbit
		, std::uint32_t MType
		, std::uint32_t FCntDn
		, std::string const& customerID
		, std::string const& payload
		, boost::uuids::uuid tag
		, boost::uuids::uuid origin
		, std::uint64_t max_messages);

	cyng::table::record connection_lookup(cyng::store::table* tbl, cyng::table::key_type&& key);
	bool connection_erase(cyng::store::table* tbl, cyng::table::key_type&& key, boost::uuids::uuid tag);

	/**
	 * Get value from "_Config" table. 
	 * Set e read lock on "_Config" table. 
	 */
	cyng::object get_config(cyng::store::db& db, std::string key);
	cyng::object get_config(cyng::store::table const* tbl, std::string key);

	/**
	 * Define configuration bits
	 */
	enum system_config : std::uint64_t
	{
		SMF_CONNECTION_AUTO_LOGIN	= (1 << 0),
		SMF_CONNECTION_AUTO_ENABLED = (1 << 1),
		SMF_CONNECTION_SUPERSEDED	= (1 << 2),
		SMF_GENERATE_TIME_SERIES	= (1 << 3),
		SMF_GENERATE_CATCH_METERS	= (1 << 4),
		SMF_GENERATE_CATCH_LORA		= (1 << 5),
	};

	bool is_connection_auto_login(std::uint64_t);
	bool is_connection_auto_enabled(std::uint64_t);
	bool is_connection_superseed(std::uint64_t);
	bool is_generate_time_series(std::uint64_t);
	bool is_catch_meters(std::uint64_t);
	bool is_catch_lora(std::uint64_t);

	bool set_connection_auto_login(std::atomic<std::uint64_t>&, bool);
	bool set_connection_auto_enabled(std::atomic<std::uint64_t>&, bool);
	bool set_connection_superseed(std::atomic<std::uint64_t>&, bool);
	bool set_generate_time_series(std::atomic<std::uint64_t>&, bool);
	bool set_catch_meters(std::atomic<std::uint64_t>&, bool);
	bool set_catch_lora(std::atomic<std::uint64_t>&, bool);

	/** @brief lookup meter
	 * 
	 * "ident" + "gw" have to be unique
	 *
	 * @return true if the specified meter on this gateway exists.
	 * @note This method is inherently slow.
	 * @todo optimize
	 */
	cyng::table::record lookup_meter(cyng::store::table const* tbl, std::string const& ident, boost::uuids::uuid gw_tag);
}

#endif

