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
#include "../sml_reader.h"
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
			using signatures_t = std::tuple<msg_0, msg_1, msg_2, msg_3, msg_4, msg_5, msg_6>;

		public:
			network(cyng::async::base_task* bt
				, cyng::logging::log_ptr
				, master_config_t const& cfg);
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

		private:
			void task_resume(cyng::context& ctx);
			void reconfigure(cyng::context& ctx);
			void insert_rel(cyng::context& ctx);
			void reconfigure_impl();
			void register_targets();

			void sml_msg(cyng::context& ctx);
			void sml_eom(cyng::context& ctx);

			void sml_public_open_request(cyng::context& ctx);
			void sml_public_close_request(cyng::context& ctx);
			void sml_get_proc_parameter_request(cyng::context& ctx);

		private:
			cyng::async::base_task& base_;
			bus::shared_type bus_;
			cyng::logging::log_ptr logger_;
			const master_config_t	config_;
			std::size_t master_;	//!< IP-T master

			/**
			 * SML parser
			 */
			node::sml::parser 	parser_;

			/**
			 * read SML tree and generate commands
			 */
			node::sml::sml_reader reader_;

			/**
			 * maintain relation between sequence and registered target
			 */
			std::map<sequence_type, std::string>	seq_target_map_;
			std::map<std::uint32_t, std::string>	channel_target_map_;

		};
	}
}

#endif