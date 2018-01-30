/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#ifndef NODE_IPT_STORE_TASK_NETWORK_H
#define NODE_IPT_STORE_TASK_NETWORK_H

#include <smf/ipt/bus.h>
#include <smf/ipt/config.h>
#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/policy.h>
//#include <cyng/store/db.h>

namespace node
{
	namespace ipt
	{
		class network
		{
		public:
			using msg_0 = std::tuple<std::uint16_t, std::string>;
			using msg_1 = std::tuple<>;
			using msg_2 = std::tuple<sequence_type, std::string>;
			using signatures_t = std::tuple<msg_0, msg_1, msg_2>;

		public:
			network(cyng::async::base_task* bt
				, cyng::logging::log_ptr
				, std::vector<std::size_t> const&
				, master_config_t const& cfg
				, std::vector<std::string> const& targets);
			void run();
			void stop();

			/**
			 * @brief slot [0]
			 *
			 * sucessful network login
			 */
			cyng::continuation process(std::uint16_t, std::string);

			/**
			 * @brief slot [1]
			 *
			 * reconnect
			 */
			cyng::continuation process();

			/**
			 * @brief slot [2]
			 *
			 * incoming call
			 */
			cyng::continuation process(sequence_type, std::string const& number);

		private:
			void task_resume(cyng::context& ctx);
			void reconfigure(cyng::context& ctx);
			void reconfigure_impl();

			cyng::vector_t ipt_req_login_public() const;
			cyng::vector_t ipt_req_login_scrambled() const;

			void register_targets();

		private:
			cyng::async::base_task& base_;
			bus::shared_type bus_;
			cyng::logging::log_ptr logger_;
			const std::size_t storage_tsk_;
			const master_config_t	config_;
			const std::vector<std::string> targets_;
			std::size_t master_;

		};
	}
}

#endif