/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_TASK_BRIDGE_H
#define SMF_SEGW_TASK_BRIDGE_H

#include <cfg.h>
#include <storage_functions.h>
#include <storage.h>
#include <router.h>
#include <config/cfg_lmn.h>
#include <sml/server.h>
#include <nms/server.h>
#include <redirector/server.h>

#include <smf/ipt/config.h>

#include <cyng/obj/intrinsics/eod.h>
#include <cyng/log/logger.h>
#include <cyng/store/db.h>
#include <cyng/vm/mesh.h>
#include <cyng/task/task_fwd.h>

namespace smf {

	/**
	 * manage segw lifetime
	 */
	class bridge
	{
		template <typename T >
		friend class cyng::task;

		using signatures_t = std::tuple<
			std::function<void(cyng::eod)>,
			std::function<void()>
		>;

	public:
		bridge(std::weak_ptr<cyng::channel>
			, cyng::controller& ctl
			, cyng::logger
			, cyng::db::session);

	private:
		void stop(cyng::eod);
		void start();

		void init_data_cache();
		void load_config_data();
		void init_cache_persistence();

		void load_configuration();

		void init_gpio();

		void init_virtual_meter();

		void init_sml_server();
		void init_nms_server();

		/**
		 * start reading from serial ports
		 */
		void init_lmn_ports();
		void init_lmn_port(lmn_type);

		void init_ipt_bus();

		void init_broker_clients();
		void init_broker_clients(lmn_type);

		void init_redirectors();
		void init_redirector(lmn_type);

	private:
		signatures_t sigs_;
		std::weak_ptr<cyng::channel> channel_;
		cyng::controller& ctl_;
		cyng::logger logger_;
		cyng::db::session db_;
		storage storage_;
		cyng::store cache_;
		cfg cfg_;
		cyng::mesh fabric_;
		router	router_;	//	SML router
		//ipt::bus	bus_;
		nms::server	nms_;
		sml::server	sml_;
		std::array<rdr::server, 2>	rdr_;

	};
}

#endif