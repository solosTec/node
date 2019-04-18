/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_STORE_TASK_PUSH_OPS_H
#define NODE_IPT_STORE_TASK_PUSH_OPS_H

#include <smf/ipt/bus.h>
#include <smf/sml/status.h>
#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/store/db.h>

namespace node
{
	namespace ipt
	{
		class push_ops
		{
		public:
			using msg_0 = std::tuple<bool, std::uint32_t, std::uint32_t, std::uint16_t, std::size_t, std::string>;
			using signatures_t = std::tuple<msg_0>;

		public:
			push_ops(cyng::async::base_task* bt
				, cyng::logging::log_ptr
				, node::sml::status& status_word
				, cyng::store::db& config_db
				, cyng::controller& vm
				, cyng::table::key_type const&
				, boost::uuids::uuid tag);
			cyng::continuation run();
			void stop(bool shutdown);

			/**
			 * @brief slot [0]
			 *
			 * open push channel response
			 */
			cyng::continuation process(bool success
				, std::uint32_t channel
				, std::uint32_t source
				, std::uint16_t status
				, std::size_t count
				, std::string target);


		private:
			void set_tp();
			void push();

		private:
			cyng::async::base_task& base_;
			cyng::logging::log_ptr logger_;
			
			/**
			 * global state
			 */
			node::sml::status& status_word_;

			/**
			 * configuration db
			 */
			cyng::store::db& config_db_;

			/**
			 * VM
			 */
			cyng::controller& vm_;

			cyng::table::key_type key_;

			boost::uuids::uuid const tag_;

			enum {
				TASK_STATE_INITIAL_,
				TASK_STATE_RUNNING_,
			} state_;
		};
	}
}

#endif
