/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_STORE_TASK_PUSH_OPS_H
#define NODE_IPT_STORE_TASK_PUSH_OPS_H

#include <smf/ipt/bus.h>
#include <cyng/log.h>
#include <cyng/async/mux.h>
//#include <cyng/table/record.h>
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
				, cyng::store::db& config_db
				, node::ipt::bus::shared_type bus
				, cyng::table::key_type const&
				, boost::uuids::uuid tag);
			cyng::continuation run();
			void stop();

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

			//void task_resume(cyng::context& ctx);
			//void reconfigure(cyng::context& ctx);
			//void insert_rel(cyng::context& ctx);
			//void reconfigure_impl();
			//void register_targets();

		private:
			cyng::async::base_task& base_;

			/**
			 * configuration db
			 */
			cyng::store::db& config_db_;

			/**
			 * ipt bus
			 */
			bus::shared_type bus_;

			cyng::logging::log_ptr logger_;
			cyng::table::key_type key_;

			const boost::uuids::uuid tag_;

			enum {
				TASK_STATE_INITIAL_,
				TASK_STATE_RUNNING_,
			} state_;
		};
	}
}

#endif