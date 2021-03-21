/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_MAIN_DB_H
#define SMF_MAIN_DB_H

#include <smf/config/kv_store.h>

#include <smf/config/schemes.h>

#include <cyng/log/logger.h>

#include <boost/uuid/random_generator.hpp>

namespace smf {

	class db
	{

	public:
		db(cyng::store& cache, cyng::logger, boost::uuids::uuid tag);
		~db();

		/**
		 * fill store map and create all tables
		 */
		void init(cyng::param_map_t const&);

		/**
		 * Add a new cluster node into table "cluster"
		 */
		bool insert_cluster_member(boost::uuids::uuid
			, std::string class_name
			, cyng::version
			, boost::asio::ip::tcp::endpoint
			, cyng::pid);

	private:
		void set_start_values(cyng::param_map_t const& session_cfg);

	private:
		cyng::store& cache_;
		cyng::logger logger_;
		kv_store cfg_;
		config::store_map store_map_;
		boost::uuids::random_generator uuid_gen_;
	};

	/**
	 * @return vector of all required meta data
	 */
	std::vector< cyng::meta_store > get_store_meta_data();

}

#endif
