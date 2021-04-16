/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_DASH_NOTIFIER_H
#define SMF_DASH_NOTIFIER_H

#include <cyng/store/slot_interface.h>
#include <cyng/log/logger.h>

namespace smf {

	class db;
	class http_server;
	class notifier : public cyng::slot_interface
	{

	public:
		notifier(db& data
			, http_server&
			, cyng::logger);

	public:
		/**
		 * insert
		 */
		virtual bool forward(cyng::table const*
			, cyng::key_t const&
			, cyng::data_t const&
			, std::uint64_t
			, boost::uuids::uuid) override;

		/**
		 * update
		 */
		virtual bool forward(cyng::table const* tbl
			, cyng::key_t const& key
			, cyng::attr_t const& attr
			, std::uint64_t gen
			, boost::uuids::uuid tag) override;

		/**
		 * remove
		 */
		virtual bool forward(cyng::table const* tbl
			, cyng::key_t const& key
			, boost::uuids::uuid tag) override;

		/**
		 * clear
		 */
		virtual bool forward(cyng::table const*
			, boost::uuids::uuid) override;

		/**
		 * transaction
		 */
		virtual bool forward(cyng::table const*
			, bool) override;


	private:
		void update_meter_online_state(cyng::key_t const& key, bool);
		void update_gw_online_state(cyng::key_t const& key, bool);
		void update_meter_iec_online_state(cyng::key_t const& key, bool);
		void update_meter_wmbus_online_state(cyng::key_t const& key, bool);

	private:
		db& db_;
		http_server& http_server_;
		cyng::logger logger_;
	};


}

#endif
