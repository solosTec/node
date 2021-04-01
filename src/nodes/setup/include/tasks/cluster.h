/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SETUP_TASK_CLUSTER_H
#define SMF_SETUP_TASK_CLUSTER_H

#include <smf/cluster/bus.h>

#include <cyng/obj/intrinsics/eod.h>
#include <cyng/log/logger.h>
#include <cyng/task/task_fwd.h>
#include <cyng/vm/mesh.h>
#include <cyng/store/db.h>

#include <tuple>
#include <functional>
#include <string>
#include <memory>

#include <boost/uuid/uuid.hpp>

namespace smf {

	class cluster : private bus_interface
	{
		template <typename T >
		friend class cyng::task;

		using signatures_t = std::tuple<
			std::function<void(void)>,
			//std::function<void(int)>,
			std::function<void(cyng::eod)>
		>;

	public:
		cluster(cyng::channel_weak
			, cyng::controller&
			, boost::uuids::uuid tag
			, std::string const& node_name
			, cyng::logger
			, toggle::server_vec_t&&
			, std::string storage_type
			, cyng::param_map_t&& cfg_db);
		~cluster();

		void connect();

		void stop(cyng::eod);

	private:
		//
		//	bus interface
		//
		virtual cyng::mesh* get_fabric() override;
		virtual void on_login(bool) override;
		virtual void on_disconnect(std::string msg) override;
		virtual void db_res_insert(std::string
			, cyng::key_t  key
			, cyng::data_t  data
			, std::uint64_t gen
			, boost::uuids::uuid tag) override;
		virtual void db_res_trx(std::string
			, bool) override;
		virtual void db_res_update(std::string
			, cyng::key_t key
			, cyng::attr_t attr
			, std::uint64_t gen
			, boost::uuids::uuid tag) override;

		virtual void db_res_remove(std::string
			, cyng::key_t key
			, boost::uuids::uuid tag) override;

		virtual void db_res_clear(std::string
			, boost::uuids::uuid tag) override;

		/**
		 * Upload table cache to main node
		 */
		void upload(std::string const& table_name);

	private:
		signatures_t sigs_;
		cyng::channel_weak channel_;	//	self
		cyng::controller& ctl_;
		boost::uuids::uuid const tag_;
		cyng::logger logger_;
		cyng::mesh fabric_;
		bus	bus_;
		cyng::store store_;
		cyng::channel_ptr storage_;
	};

	cyng::channel_ptr start_data_store(cyng::controller& ctl
		, cyng::logger logger
		, bus&
		, cyng::store& cache
		, std::string const& storage_type
		, cyng::param_map_t&&);


}

#endif
