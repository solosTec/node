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
#include <smf/sml/protocol/parser.h>
#include <smf/sml/status.h>
#include "../kernel.h"
#include "../executor.h"

#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/policy.h>

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
			using msg_3 = std::tuple<sequence_type, std::uint32_t, std::uint32_t, cyng::buffer_t>;
			using msg_4 = std::tuple<sequence_type, bool, std::uint32_t>;
			using msg_5 = std::tuple<cyng::buffer_t>;
			using msg_6 = std::tuple<sequence_type, bool, std::string>;
			using msg_7 = std::tuple<sequence_type>;
			using msg_8 = std::tuple<sequence_type, response_type, std::uint32_t, std::uint32_t, std::uint16_t, std::size_t>;
			using msg_9 = std::tuple<sequence_type, response_type, std::uint32_t>;

			using signatures_t = std::tuple<msg_0, msg_1, msg_2, msg_3, msg_4, msg_5, msg_6, msg_7, msg_8, msg_9>;

		public:
			network(cyng::async::base_task* bt
				, cyng::logging::log_ptr
				, node::sml::status& status_word
				, cyng::store::db& config_db
				, boost::uuids::uuid tag
				, master_config_t const& cfg
				, std::string account
				, std::string pwd
				, std::string manufacturer
				, std::string model
				, cyng::mac48 mac);
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
			 * incoming data
			 */
			cyng::continuation process(cyng::buffer_t const& data);

			/**
			 * @brief slot [6]
			 *
			 * deregister target response
			 */
			cyng::continuation process(sequence_type, bool, std::string const&);

			/**
			 * @brief slot [7]
			 *
			 * open connection closed
			 */
			cyng::continuation process(sequence_type);

			/**
			 * @brief slot [8]
			 *
			 * open push channel response
			 * @param seq ipt sequence
			 * @param res channel open response
			 * @param channel channel id
			 * @param source source id
			 * @param status channel status
			 * @param count number of targets reached
			 */
			cyng::continuation process(sequence_type seq
				, response_type res
				, std::uint32_t channel
				, std::uint32_t source
				, std::uint16_t status
				, std::size_t count);

			/**
			 * @brief slot [9]
			 *
			 * register consumer.
			 * open push channel response
			 * @param seq ipt sequence
			 * @param res channel open response
			 * @param channel channel id
			 */
			cyng::continuation process(sequence_type seq
				, response_type res
				, std::uint32_t channel);

		private:
			void reconfigure(cyng::context& ctx);
			void insert_seq_open_channel_rel(cyng::context& ctx);
			void reconfigure_impl();

		private:
			cyng::async::base_task& base_;
			bus::shared_type bus_;
			cyng::logging::log_ptr logger_;

			/**
			 * managing redundant ipt master configurations
			 */
			const redundancy	config_;

			/**
			 * SML parser
			 */
			node::sml::parser 	parser_;

			/**
			 * Provide core functions of an SML gateway
			 */
			node::sml::kernel core_;

			/**
			 * Apply changed configurations
			 */
			sml::executor exec_;

			/**
			 * maintain relation between sequence and open push channel request
			 */
			std::map<sequence_type, std::pair<std::size_t, std::string>>	seq_open_channel_map_;

		};
	}
}

#endif