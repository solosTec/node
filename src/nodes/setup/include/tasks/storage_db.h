/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SETUP_TASK_STORAGE_DB_H
#define SMF_SETUP_TASK_STORAGE_DB_H

#include <smf/config/schemes.h>

#include <cyng/obj/intrinsics/eod.h>
#include <cyng/log/logger.h>
#include <cyng/task/task_fwd.h>
#include <cyng/db/session.h>
#include <cyng/store/db.h>

#include <tuple>
#include <functional>
#include <string>
#include <memory>

#include <boost/uuid/uuid.hpp>

namespace smf {

	class storage_db
	{
		template <typename T >
		friend class cyng::task;

		using signatures_t = std::tuple<
			std::function<void(void)>,
			std::function<void(cyng::eod)>
		>;

	public:
		storage_db(std::weak_ptr<cyng::channel>
			, cyng::controller&
			, boost::uuids::uuid tag
			, cyng::store& cache
			, cyng::logger logger
			, cyng::param_map_t&& cfg);
		~storage_db();


		void stop(cyng::eod);

	private:
		void open();
		void init_store();
		void init_store(cyng::meta_store);
		void load_store();
		void load_store(cyng::meta_sql const&);

	private:
		signatures_t sigs_;
		std::weak_ptr<cyng::channel> channel_;
		cyng::controller& ctl_;
		boost::uuids::uuid const tag_;
		cyng::logger logger_;
		cyng::db::session db_;
		cyng::store& store_;

		config::store_map store_map_;

	};

	bool init_storage(cyng::db::session& db);
	std::vector< cyng::meta_sql > get_sql_meta_data();
	void generate_access_rights(cyng::db::session& db, std::string const& user);
	void generate_random_devs(cyng::db::session& db, std::uint32_t count);
}

#endif
