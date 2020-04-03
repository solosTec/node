/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_TASK_NETWORK_H
#define NODE_SEGW_TASK_NETWORK_H

#include <smf/ipt/bus.h>
#include <smf/sml/protocol/parser.h>
#include "../router.h"

#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/policy.h>

namespace node
{
	class bridge;
	namespace ipt
	{
		class network : public bus
		{
		public:
			using signatures_t = std::tuple<>;

		public:
			network(cyng::async::base_task* btp
				, cyng::logging::log_ptr logger
				, bridge& b
				, std::string account
				, std::string pwd
				, bool accept_all
				, boost::uuids::uuid tag);
			cyng::continuation run();
			void stop(bool shutdown);

			/**
			 * @brief slot [0] 0x4001/0x4002: response login
			 *
			 * sucessful network login
			 *
			 * @param watchdog watchdog timer in minutes
			 * @param redirect URL to reconnect
			 */
			virtual void on_login_response(std::uint16_t, std::string) override;

			/**
			 * @brief slot [1]
			 *
			 * connection lost / reconnect
			 */
			virtual void on_logout() override;

			/**
			 * @brief slot [2] - 0x4005: push target registered response
			 *
			 * register target response
			 *
			 * @param seq ipt sequence
			 * @param success success flag
			 * @param channel channel id
			 * @param target target name (empty when if request not triggered by bus::req_register_push_target())
			 */
			virtual void on_res_register_target(sequence_type seq
				, bool success
				, std::uint32_t channel
				, std::string target) override;

			/**
			 * @brief slot [3] - 0x4006: push target deregistered response
			 *
			 * deregister target response
			 */
			virtual void on_res_deregister_target(sequence_type, bool, std::string const&) override;

			/**
			 * @brief slot [4] - 0x1000: push channel open response
			 *
			 * open push channel response
			 * 
			 * @param seq ipt sequence
			 * @param success success flag
			 * @param channel channel id
			 * @param source source id
			 * @param status channel status
			 * @param count number of targets reached
			 */
			virtual void on_res_open_push_channel(sequence_type seq
				, bool success
				, std::uint32_t channel
				, std::uint32_t source
				, std::uint16_t status
				, std::size_t count) override;

			/**
			 * @brief slot [5] - 0x1001: push channel close response
			 *
			 * register consumer.
			 * open push channel response
			 * @param seq ipt sequence
			 * @param bool success flag
			 * @param channel channel id
			 * @param res response name
			 */
			virtual void on_res_close_push_channel(sequence_type seq
				, bool success
				, std::uint32_t channel) override;

			/**
			 * @brief slot [6] - 0x9003: connection open request 
			 *
			 * incoming call
			 *
			 * @return true if request is accepted - otherwise false
			 */
			virtual bool on_req_open_connection(sequence_type, std::string const& number) override;

			/**
			 * @brief slot [7] - 0x1003: connection open response
			 *
			 * @param seq ipt sequence
			 * @param success true if connection open request was accepted
			 */
			virtual cyng::buffer_t on_res_open_connection(sequence_type seq, bool success) override;

			/**
			 * @brief slot [8] - 0x9004/0x1004: connection close request/response
			 *
			 * open connection closed
			 */
			virtual void on_req_close_connection(sequence_type) override;
			virtual void on_res_close_connection(sequence_type) override;

			/**
			 * @brief slot [9] - 0x9002: push data transfer request
			 *
			 * push data
			 */
			virtual void on_req_transfer_push_data(sequence_type, std::uint32_t, std::uint32_t, cyng::buffer_t const&) override;

			/**
			 * @brief slot [10] - transmit data (if connected)
			 *
			 * incoming data
			 */
			virtual cyng::buffer_t on_transmit_data(cyng::buffer_t const&) override;

		private:
			/** 
			 * Callback when resolving IP adress failed (no IP connect)
			 */
			void reconfigure(cyng::context& ctx);

			void reconfigure_impl();

			/**
			 * load and start all configured push ops
			 */
			void load_push_ops();

			/**
			 * Start a push task. Record has to be from "TPushOps"/"_PushOps"
			 */
			std::size_t start_task_push(cyng::table::record const&, cyng::buffer_t profile);

			std::size_t start_task_push(cyng::buffer_t srv_id
				, std::uint8_t nr
				, cyng::buffer_t profile
				, std::uint32_t interval
				, std::uint32_t delay
				, std::string target);

		private:
			cyng::async::base_task& base_;
			cyng::logging::log_ptr logger_;

			/**
			 * configuration management
			 */
			bridge& bridge_;
			cache& cache_;

			/**
			 * SQL database
			 */
			storage& storage_;

			/**
			 * SML parser
			 */
			node::sml::parser 	parser_;

			/**
			 * message dispatcher
			 */
			router router_;

			/**
			 * maintain relation between sequence and open push channel request
			 */
			//std::map<sequence_type, std::pair<std::size_t, std::string>>	seq_open_channel_map_;

			/**
			 * GPIO to signal IP-T online state
			 */
			std::size_t task_gpio_;


		};
	}
}

#endif
