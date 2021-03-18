/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_MAIN_DB_H
#define SMF_MAIN_DB_H

#include <cfg.h>

#include <smf/config/schemes.h>

#include <cyng/log/logger.h>

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


	private:
		void set_start_values(cyng::param_map_t const& session_cfg);

	private:
		cyng::store& cache_;
		cyng::logger logger_;
		cfg cfg_;	
		config::store_map store_map_;
		
	};

	/**
	 * @return vector of all required meta data
	 */
	std::vector< cyng::meta_store > get_store_meta_data();

}

#endif
