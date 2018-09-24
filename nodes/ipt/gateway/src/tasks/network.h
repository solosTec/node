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
			//	 [0] 0x4001/0x4002: response login
			using msg_00 = std::tuple<std::uint16_t, std::string>;
			using msg_01 = std::tuple<>;

			//	[2] 0x4005: push target registered response
			using msg_02 = std::tuple<sequence_type, bool, std::uint32_t>;

			//	[3] 0x4006: push target deregistered response
			using msg_03 = std::tuple<sequence_type, bool, std::string>;

			//	[4] 0x1000: push channel open response
			using msg_04 = std::tuple<sequence_type, bool, std::uint32_t, std::uint32_t, std::uint16_t, std::size_t>;

			//	[5] 0x1001: push channel close response
			using msg_05 = std::tuple<sequence_type, bool, std::uint32_t, std::string>;

			//	[6] 0x9003: connection open request 
			using msg_06 = std::tuple<sequence_type, std::string>;

			//	[7] 0x1003: connection open response
			using msg_07 = std::tuple<sequence_type, bool>;

			//	[8] 0x9004: connection close request
			using msg_08 = std::tuple<sequence_type, bool, std::size_t>;

			//	[9] 0x9002: push data transfer request
			using msg_09 = std::tuple<sequence_type, std::uint32_t, std::uint32_t, cyng::buffer_t>;

			//	[10] transmit data(if connected)
			using msg_10 = std::tuple<cyng::buffer_t>;

			using signatures_t = std::tuple<msg_00, msg_01, msg_02, msg_03, msg_04, msg_05, msg_06, msg_07, msg_08, msg_09, msg_10>;


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
				, std::uint32_t serial
				, cyng::mac48 mac);
			cyng::continuation run();
			void stop();

			/**
			 * @brief slot [0] 0x4001/0x4002: response login
			 *
			 * sucessful network login
			 */
			cyng::continuation process(std::uint16_t, std::string);

			/**
			 * @brief slot [1]
			 *
			 * connection lost / reconnect
			 */
			cyng::continuation process();

			/**
			 * @brief slot [2] 0x4005: push target registered response
			 *
			 * register target response
			 */
			cyng::continuation process(sequence_type, bool, std::uint32_t);

			/**
			 * @brief slot [3] 0x4006: push target deregistered response
			 *
			 * deregister target response
			 */
			cyng::continuation process(sequence_type, bool, std::string const&);

			/**
			 * @brief slot [4] 0x1000: push channel open response
			 *
			 * open push channel response
			 * @param seq ipt sequence
			 * @param success success flag
			 * @param channel channel id
			 * @param source source id
			 * @param status channel status
			 * @param count number of targets reached
			 */
			cyng::continuation process(sequence_type seq
				, bool success
				, std::uint32_t channel
				, std::uint32_t source
				, std::uint16_t status
				, std::size_t count);

			/**
			 * @brief slot [5] 0x1001: push channel close response
			 *
			 * register consumer.
			 * open push channel response
			 * @param seq ipt sequence
			 * @param bool success flag
			 * @param channel channel id
			 * @param res response name
			 */
			cyng::continuation process(sequence_type seq
				, bool success
				, std::uint32_t channel
				, std::string res);

			/**
			 * @brief slot [6] 0x9003: connection open request 
			 *
			 * incoming call
			 */
			cyng::continuation process(sequence_type, std::string const& number);

			/**
			 * @brief slot [7] 0x1003: connection open response
			 *
			 * @param seq ipt sequence
			 * @param success true if connection open request was accepted
			 */
			cyng::continuation process(sequence_type seq, bool success);

			/**
			 * @brief slot [8] 0x9004/0x1004: connection close request/response
			 *
			 * open connection closed
			 */
			cyng::continuation process(sequence_type, bool, std::size_t);

			/**
			 * @brief slot [9] 0x9002: push data transfer request
			 *
			 * push data
			 */
			cyng::continuation process(sequence_type, std::uint32_t, std::uint32_t, cyng::buffer_t const&);

			/**
			 * @brief slot [10] transmit data (if connected)
			 *
			 * receive data
			 */
			cyng::continuation process(cyng::buffer_t const&);

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

#if BOOST_COMP_GNUC
namespace cyng {
	namespace async {

		//
		//	initialize static slot names
		//
		template <>
		std::map<std::string, std::size_t> cyng::async::task<node::ipt::network>::slot_names_;
    }
}
#endif

#endif
