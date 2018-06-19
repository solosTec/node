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
		, boost::asio::ip::tcp::endpoint
		, std::uint64_t global_config);

	void insert_msg(cyng::store::db&
		, cyng::logging::severity
		, std::string const&
		, boost::uuids::uuid tag);

	void insert_msg(cyng::store::table* tbl
		, cyng::logging::severity
		, std::string const&
		, boost::uuids::uuid tag);

	cyng::table::record connection_lookup(cyng::store::table* tbl, cyng::table::key_type&& key);
	bool connection_erase(cyng::store::table* tbl, cyng::table::key_type&& key, boost::uuids::uuid tag);

	/**
	 * Define configuration bits
	 */
	enum system_config : std::uint64_t
	{
		SMF_CONNECTION_AUTO_LOGIN	= (1ull << 0),
		SMF_CONNECTION_AUTO_ENABLED = (1ull << 1),
		SMF_CONNECTION_SUPERSEDED	= (1ull << 2),
		SMF_GENERATE_TIME_SERIES	= (1ull << 3),
	};

	bool is_connection_auto_login(std::uint64_t);
	bool is_connection_auto_enabled(std::uint64_t);
	bool is_connection_superseed(std::uint64_t);
	bool is_generate_time_series(std::uint64_t);

	bool set_connection_auto_login(std::atomic<std::uint64_t>&, bool);
	bool set_connection_auto_enabled(std::atomic<std::uint64_t>&, bool);
	bool set_connection_superseed(std::atomic<std::uint64_t>&, bool);
	bool set_generate_time_series(std::atomic<std::uint64_t>&, bool);
}

#endif

