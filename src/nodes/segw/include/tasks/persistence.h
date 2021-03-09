/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_TASK_PERSISTENCE_H
#define SMF_SEGW_TASK_PERSISTENCE_H

#include <cfg.h>
#include <distributor.h>

#include <cyng/log/logger.h>
#include <cyng/store/db.h>
//#include <cyng/db/session.h>

#include <cyng/task/task_fwd.h>

namespace smf {

	class storage;

	/**
	 * make configuration changes persistent
	 */
	class persistence
	{
		template <typename T >
		friend class cyng::task;

		using signatures_t = std::tuple<
			std::function<void(cyng::eod)>,
			std::function<void(cyng::table const*
				, cyng::key_t
				, cyng::data_t
				, std::uint64_t
				, boost::uuids::uuid)>,
			std::function<void(cyng::table const*
				, cyng::key_t
				, cyng::attr_t
				, std::uint64_t
				, boost::uuids::uuid)>,
			std::function<void(cyng::table const*
				, cyng::key_t
				, boost::uuids::uuid)>,
			std::function<void(cyng::table const*
				, boost::uuids::uuid)>,
			std::function<void()>	//	power_return
		>;

	public:
		persistence(std::weak_ptr<cyng::channel>
			, cyng::controller& ctl
			, cyng::logger
			, cfg&
			, storage&);

	private:
		void stop(cyng::eod);
		void connect();

		/**
		 * place power return message into OBIS log
		 */
		void power_return();

		void insert(cyng::table const*
			, cyng::key_t
			, cyng::data_t
			, std::uint64_t
			, boost::uuids::uuid);

		/**
		 * signal an modify event
		 */
		void modify(cyng::table const*
			, cyng::key_t
			, cyng::attr_t
			, std::uint64_t
			, boost::uuids::uuid);

		/**
		 * signal an remove event
		 */
		void remove(cyng::table const*
			, cyng::key_t
			, boost::uuids::uuid);

		/**
		 * signal an clear event
		 */
		void clear(cyng::table const*
			, boost::uuids::uuid);


	private:
		signatures_t sigs_;
		std::weak_ptr<cyng::channel> channel_;
		cyng::logger logger_;
		cfg& cfg_;
		storage& storage_;
		distributor distributor_;
	};
}

#endif