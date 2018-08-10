/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#ifndef NODE_IPT_STRESS_SENDER_H
#define NODE_IPT_STRESS_SENDER_H

#include <smf/ipt/bus.h>
#include <smf/ipt/config.h>
#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/policy.h>
#include <random>

namespace node
{
	namespace ipt
	{
		class sender
		{
		public:
			using msg_0 = std::tuple<std::uint16_t, std::string>;
			using msg_1 = std::tuple<>;
			using msg_2 = std::tuple<sequence_type, std::string>;
			using msg_3 = std::tuple<sequence_type, std::uint32_t, std::uint32_t, cyng::buffer_t>;
			using msg_4 = std::tuple<sequence_type, bool, std::uint32_t>;
			using msg_5 = std::tuple<cyng::buffer_t>;
			using msg_6 = std::tuple<std::string>;
			using signatures_t = std::tuple<msg_0, msg_1, msg_2, msg_3, msg_4, msg_5, msg_6>;

		public:
			sender(cyng::async::base_task* bt
				, cyng::logging::log_ptr
				, master_config_t const& cfg);
			cyng::continuation run();
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

			/**
			 * @brief slot [3]
			 *
			 * push data
			 */
			cyng::continuation process(sequence_type, std::uint32_t, std::uint32_t, cyng::buffer_t const&);

			/**
			 * @brief slot [4]
			 *
			 * register target response
			 */
			cyng::continuation process(sequence_type, bool, std::uint32_t);

			/**
			 * @brief slot [5]
			 *
			 * receive data
			 */
			cyng::continuation process(cyng::buffer_t const&);

			/**
			 * @brief slot [6]
			 *
			 * open connection to receiver
			 */
			cyng::continuation process(std::string const&);

		private:
			//void task_resume(cyng::context& ctx);
			void reconfigure(cyng::context& ctx);
			void reconfigure_impl();

			cyng::vector_t ipt_req_login_public() const;
			cyng::vector_t ipt_req_login_scrambled() const;

			void res_open_connection(cyng::context& ctx);

		private:
			cyng::async::base_task& base_;
			bus::shared_type bus_;
			cyng::logging::log_ptr logger_;

			/**
			 * managing redundant ipt master configurations
			 */
			const redundancy	config_;

			enum {
				TASK_STATE_INITIAL_,
				TASK_STATE_AUTHORIZED_,
				TASK_STATE_CONNECTED_,
			} task_state_;

			// First create an instance of an engine.
			std::random_device rnd_device_;
			// Specify the engine and distribution.
			std::mt19937 mersenne_engine_;
		};
	}
}

#endif
