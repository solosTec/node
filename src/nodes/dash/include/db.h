/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_DASH_HTTP_DB_H
#define SMF_DASH_HTTP_DB_H

#include <smf/config/schemes.h>
#include <smf/config/kv_store.h>

#include <cyng/store/db.h>
#include <cyng/log/logger.h>

namespace smf {

	class db
	{
	public:
		/**
		 * releation between table name, channel name and table size channel
		 */
		struct rel {
			rel(std::string, std::string, std::string);
			rel() = default;
			rel(rel const&) = default;

			bool empty() const;

			std::string const table_;
			std::string const channel_;
			std::string const counter_;
		};

	public:
		db(cyng::store& cache
			, cyng::logger
			, boost::uuids::uuid tag);

		void init(std::uint64_t max_upload_size
			, std::string const& nickname
			, std::chrono::seconds timeout);

		void db_res_subscribe(std::string table_name
			, cyng::key_t  key
			, cyng::data_t  data
			, std::uint64_t gen
			, boost::uuids::uuid tag);

		void db_res_update(std::string table_name
			, cyng::key_t key
			, cyng::attr_t attr
			, std::uint64_t gen
			, boost::uuids::uuid tag);

		void db_res_remove(std::string table_name
			, cyng::key_t  key
			, boost::uuids::uuid tag);

		/**
		 * search a relation record by table name
		 */
		rel by_table(std::string const& name) const;

		/**
		 * search a relation record by channel name
		 */
		rel by_channel(std::string const& name) const;

		/**
		 * search a relation record by counter name
		 */
		rel by_counter(std::string const& name) const;

		/**
		 * loop over all rel-entries
		 */
		void loop_rel(std::function<void(rel const&)>) const;

	private:
		void set_start_values(std::uint64_t max_upload_size
			, std::string const& nickname
			, std::chrono::seconds timeout);

	public:
		kv_store cfg_;
		cyng::store& cache_;

	private:
		cyng::logger logger_;
		config::store_map store_map_;

		using array_t = std::array<rel, 9>;
		static array_t const rel_;

	};

	/**
	 * @return vector of all required meta data
	 */
	std::vector< cyng::meta_store > get_store_meta_data();

}

#endif
