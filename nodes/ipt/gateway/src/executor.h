/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_SML_EXECUTOR_H
#define NODE_SML_EXECUTOR_H


#include <smf/sml/defs.h>
#include <smf/sml/status.h>
#include <smf/ipt/bus.h>
#include <cyng/intrinsics/mac.h>
#include <cyng/log.h>
#include <cyng/store/db.h>
#include <cyng/async/mux.h>
#include <boost/uuid/random_generator.hpp>

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
				, status&
				, cyng::store::db& config_db
				, node::ipt::bus::shared_type bus
				, boost::uuids::uuid tag
				, cyng::mac48 mac);

			/**
			 * Generate op log entry
			 */
			void ipt_access(bool, std::string);

		private:
			void subscribe(std::string const& name);
			void init_db(boost::uuids::uuid tag, cyng::mac48 mac);

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
			cyng::logging::log_ptr logger_;
			cyng::async::mux& mux_;

			/**
			 * global state
			 */
			status& status_word_;

			/**
			 * configuration db
			 */
			cyng::store::db& config_db_;

			/**
			 * ipt bus
			 */
			node::ipt::bus::shared_type bus_;

			/**
			 * table subscriptions
			 */
			cyng::store::subscriptions_t	subscriptions_;

			boost::uuids::random_generator rgn_;

		};

	}	//	sml
}

#endif