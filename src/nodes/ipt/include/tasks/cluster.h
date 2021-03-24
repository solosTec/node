/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_IPT_TASK_CLUSTER_H
#define SMF_IPT_TASK_CLUSTER_H

#include <ipt_server.h>

#include <smf/cluster/bus.h>
#include <smf/ipt/scramble_key.h>

#include <cyng/obj/intrinsics/eod.h>
#include <cyng/log/logger.h>
#include <cyng/task/task_fwd.h>
#include <cyng/vm/mesh.h>

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
			std::function<void(boost::asio::ip::tcp::endpoint)>,
			std::function<void(cyng::eod)>
		>;

	public:
		cluster(std::weak_ptr<cyng::channel>
			, cyng::controller&
			, boost::uuids::uuid tag
			, std::string const& node_name
			, cyng::logger
			, toggle::server_vec_t&&
			, ipt::scramble_key const& sk
			, std::chrono::minutes watchdog
			, std::chrono::seconds timeout);
		~cluster();

		void connect();

		void stop(cyng::eod);

	private:
		//
		//	bus interface
		//
		virtual cyng::mesh* get_fabric() override;
		virtual void on_login(bool) override;
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

	private:
		signatures_t sigs_;
		std::weak_ptr<cyng::channel> channel_;
		cyng::controller& ctl_;
		boost::uuids::uuid const tag_;
		cyng::logger logger_;
		cyng::mesh fabric_;
		/**
		 * cluster bus
		 */
		bus	bus_;
		ipt_server server_;
	};

}

#endif
