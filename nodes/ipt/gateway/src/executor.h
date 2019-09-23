/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_SML_EXECUTOR_H
#define NODE_SML_EXECUTOR_H


#include <smf/sml/defs.h>
#include <smf/ipt/bus.h>
#include <cyng/log.h>
#include <cyng/store/db.h>
#include <cyng/async/mux.h>

namespace node
{
	namespace sml
	{
		/**
		 * Apply changed configurations
		 */
		class executor
		{
		public:
			executor(cyng::logging::log_ptr
				, cyng::async::mux& mux
				, cyng::store::db& config_db
				, cyng::controller& vm);

			/**
			 * Generate op log entry
			 */
			void ipt_access(bool, std::string);

			/**
			 * Monitor and update wireless LMN status
			 */
			void start_wireless_lmn(bool);

		private:
			void subscribe(std::string const& name);
			void init_db(boost::uuids::uuid tag);

			void sig_ins(cyng::store::table const*
				, cyng::table::key_type const&
				, cyng::table::data_type const&
				, std::uint64_t
				, boost::uuids::uuid);
			void sig_del(cyng::store::table const*, cyng::table::key_type const&, boost::uuids::uuid);
			void sig_clr(cyng::store::table const*, boost::uuids::uuid);
			void sig_mod(cyng::store::table const*
				, cyng::table::key_type const&
				, cyng::attr_t const&
				, std::uint64_t
				, boost::uuids::uuid);

		private:
			/**
			 * global logger
			 */
			cyng::logging::log_ptr logger_;

			/**
			 * task manager
			 */
			cyng::async::mux& mux_;

			/**
			 * configuration db
			 */
			cyng::store::db& config_db_;

			/**
			 * execution engine
			 */
			cyng::controller& vm_;

			/**
			 * table subscriptions
			 */
			cyng::store::subscriptions_t	subscriptions_;

		};

	}	//	sml
}

#endif