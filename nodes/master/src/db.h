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
	void create_tables(cyng::logging::log_ptr logger
		, cyng::store::db&
		, boost::uuids::uuid tag);

	void insert_msg(cyng::store::table* tbl
		, cyng::logging::severity
		, std::string const&
		, boost::uuids::uuid tag
		, std::uint64_t max_messages);

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

	bool insert_wmbus_uplink(cyng::store::db& db
		, std::chrono::system_clock::time_point tp
		, std::string const& srv_id
		, std::uint8_t medium
		, std::string  manufacturer
		, std::uint8_t frame_type
		, std::string const& payload
		, boost::uuids::uuid tag
		, boost::uuids::uuid origin);

	bool insert_wmbus_uplink(cyng::store::table* tbl
		, std::chrono::system_clock::time_point tp
		, std::string const& srv_id
		, std::uint8_t medium
		, std::string  manufacturer
		, std::uint8_t frame_type
		, std::string const& payload
		, boost::uuids::uuid tag
		, boost::uuids::uuid origin
		, std::uint64_t max_messages);

	bool insert_iec_uplink(cyng::store::db& db
		, std::chrono::system_clock::time_point tp
		, std::string const& evt
		, boost::asio::ip::tcp::endpoint
		, boost::uuids::uuid tag
		, boost::uuids::uuid origin);

	bool insert_iec_uplink(cyng::store::table* tbl
		, std::chrono::system_clock::time_point tp
		, std::string const& evt
		, boost::asio::ip::tcp::endpoint
		, boost::uuids::uuid tag
		, boost::uuids::uuid origin
		, std::uint64_t max_messages);

	cyng::table::record connection_lookup(cyng::store::table* tbl, cyng::table::key_type&& key);
	bool connection_erase(cyng::store::table* tbl, cyng::table::key_type&& key, boost::uuids::uuid tag);


	/** @brief lookup meter
	 * 
	 * "ident" + "gw" have to be unique
	 *
	 * @return true if the specified meter on this gateway exists.
	 * @note This method is inherently slow.
	 * @todo optimize
	 */
	cyng::table::record lookup_meter(cyng::store::table const* tbl, std::string const& ident, boost::uuids::uuid gw_tag);

	/**
	 * manage cached tables
	 */
	class cache
	{
		/**
		 * Define configuration bits
		 */
		enum system_config : std::uint64_t
		{
			SMF_CONNECTION_AUTO_LOGIN = (1 << 0),
			SMF_CONNECTION_AUTO_ENABLED = (1 << 1),
			SMF_CONNECTION_SUPERSEDED = (1 << 2),
			SMF_GW_CACHE_ENABLED = (1 << 3),
			SMF_GENERATE_TIME_SERIES = (1 << 4),
			SMF_GENERATE_CATCH_METERS = (1 << 5),
			SMF_GENERATE_CATCH_LORA = (1 << 6),
		};

	public:
		struct tbl_descr {
			std::string const name_;
			bool const custom_;
			inline tbl_descr(std::string name, bool custom)
				: name_(name)
				, custom_(custom)
			{}
		};


		/**
		 * List of all used table names
		 */
		const static std::array<tbl_descr, 23>	tables_;

	public:
		cache(cyng::store::db&, boost::uuids::uuid tag);

		/**
		 * initialize cache with some basic data like
		 * version ids, startup time, etc.
		 */
		void init();

		/**
		 * Initialize sys_conf_
		 */
		void init_sys_cfg(
			bool auto_login,
			bool auto_enabled,
			bool supersede,
			bool gw_cache,
			bool generate_time_series,
			bool catch_meters,
			bool catch_lora
		);

		/**
		 * @return itentity/source tag
		 */
		boost::uuids::uuid const get_tag() const;

		void read_table(std::string const&, std::function<void(cyng::store::table const*)>);
		void read_tables(std::string const&, std::string const&, std::function<void(cyng::store::table const*, cyng::store::table const*)>);
		void write_table(std::string const&, std::function<void(cyng::store::table*)>);
		void write_tables(std::string const&, std::string const&, std::function<void(cyng::store::table*, cyng::store::table*)>);
		void clear_table(std::string const&);

		/**
		 * Convinience function to loop over one table with read access
		 */
		void loop(std::string const&, std::function<bool(cyng::table::record const&)>);


		bool is_connection_auto_login() const;
		bool is_connection_auto_enabled() const;
		bool is_connection_superseed() const;
		bool is_gw_cache_enabled() const;

		/**
		 * @return true if generating time series is on.
		 */
		bool is_generate_time_series() const;
		bool is_catch_meters() const;
		bool is_catch_lora() const;

		bool set_connection_auto_login(bool);
		bool set_connection_auto_enabled(bool);
		bool set_connection_superseed(bool);
		bool set_gw_cache_enabled(bool);

		/**
		 * Turn generating time series on or off
		 *
		 * @return previous value
		 */
		bool set_generate_time_series(bool);
		bool set_catch_meters(bool);
		bool set_catch_lora(bool);

		/**
		 * read a configuration value from table "_Cfg"
		 */
		template <typename T >
		T get_cfg(std::string name, T def) {
			return cyng::value_cast(db_.get_value("_Config", std::string("value"), name), def);
		}

		/**
		 * set/insert a configuration value
		 */
		template <typename T >
		bool set_cfg(std::string name, T val) {
			return merge_cfg(name, cyng::make_object(val));
		}

		/**
		 * set/insert a configuration value
		 */
		bool merge_cfg(std::string name, cyng::object&& obj);

		/**
		 * insert master record into cluster table
		 */
		void create_master_record(boost::asio::ip::tcp::endpoint);

		void insert_msg(cyng::logging::severity
			, std::string const&
			, boost::uuids::uuid tag);

		/**
		 * @return cluster heartbeat in seconds
		 */
		std::chrono::seconds get_cluster_hartbeat();

		/**
		 * read max number of statistic events from config table
		 */
		std::uint64_t get_max_events();

		/**
		 *	read max number of logging messages from config table
		 */
		std::uint64_t get_max_messages();

		/**
		 *	read max number of LoRa uplink records
		 */
		std::uint64_t get_max_LoRa_records();

		/**
		 *	read max number of wireless M-Bus uplink records
		 */
		std::uint64_t get_max_wMBus_records();

		/**
		 *	read max number of IEC uplink records
		 */
		std::uint64_t get_max_IEC_records();

	public:
		/**
		 * global data cache
		 */
		cyng::store::db& db_;

	private:
		/**
		 * source tag
		 */
		boost::uuids::uuid const tag_;

		/**
		 * system wide configuration flags (kind of bit vector)
		 * @see enum system_config
		 */
		std::atomic<std::uint64_t> sys_conf_;

	};
}

#endif

