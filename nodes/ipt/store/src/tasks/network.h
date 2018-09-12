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
#include "../processors/sml_processor.h"
#include "../processors/iec_processor.h"
#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/policy.h>
#include <boost/uuid/random_generator.hpp>

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

			using msg_10 = std::tuple<std::string, std::size_t>;
			using msg_11 = std::tuple<std::string, std::uint64_t, boost::uuids::uuid>;
			using signatures_t = std::tuple<msg_0, msg_1, msg_2, msg_3, msg_4, msg_5, msg_6, msg_7, msg_8, msg_9, msg_10, msg_11>;

		public:
			network(cyng::async::base_task* bt
				, cyng::logging::log_ptr
				, master_config_t const& cfg
				, std::map<std::string, std::string> const& targets);
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
			cyng::continuation process(cyng::buffer_t const&);

			/**
			 * @brief slot [6]
			 *
			 * push target deregistered
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

			/**
			 * @brief slot [10]
			 *
			 * register consumer.
			 * @param type consumer type
			 * @param tid task id
			 */
			cyng::continuation process(std::string type, std::size_t tid);

			/**
			 * @brief slot [11]
			 *
			 * remove line
			 */
			cyng::continuation process(std::string, std::uint64_t line, boost::uuids::uuid tag);

		private:
			//void task_resume(cyng::context& ctx);
			void reconfigure(cyng::context& ctx);
			void reconfigure_impl();

			void insert_rel(cyng::context& ctx);

			cyng::vector_t ipt_req_login_public() const;
			cyng::vector_t ipt_req_login_scrambled() const;

			void register_targets();

			void distribute(std::uint32_t channel
				, std::uint32_t source
				, std::string const& protocol
				, std::string const& target
				, cyng::buffer_t const& data);

			/**
			 * Distribute raw data.
			 *
			 * @return count of found consumer tasks.
			 */
			std::size_t distribute_raw_all(std::uint32_t channel
				, std::uint32_t source
				, std::string const& protocol
				, std::string const& target
				, cyng::buffer_t const& data);

			std::size_t distribute_raw(std::uint32_t channel
				, std::uint32_t source
				, std::string protocol
				, std::string const& target
				, cyng::buffer_t const& data);

			std::size_t distribute_sml(std::uint32_t channel
				, std::uint32_t source
				, std::string const& target
				, cyng::buffer_t const& data);

			std::size_t distribute_iec(std::uint32_t channel
				, std::uint32_t source
				, std::string const& target
				, cyng::buffer_t const& data);

			/**
			 * @return vector with all consumers of the specified protocol
			 */
			std::vector<std::size_t> get_consumer(std::string protocol);

		private:
			cyng::async::base_task& base_;
			bus::shared_type bus_;
			cyng::logging::log_ptr logger_;

			/**
			 * managing redundant ipt master configurations
			 */
			const redundancy	config_;

			/**
			 * target/protocol relation
			 * target => protocol
			 */
			const std::map<std::string, std::string> targets_;

			/**
			 * maintain relation between sequence and registered target
			 */
			std::map<sequence_type, std::string>	seq_target_map_;

			/**
			 * maintain relation between channel and protocol
			 * channel => [protocol, target]
			 */
			std::map<std::uint32_t, std::pair<std::string, std::string>>	channel_protocol_map_;

			/**
			 * consumer map: protocol => task id
			 */
			std::multimap<std::string, std::size_t>	consumers_;

			/**
			 * line => parser relation.
			 * Each line has it's own parser instance.
			 */
			std::map<std::uint64_t, sml_processor>	sml_lines_;
			std::map<std::uint64_t, iec_processor>	iec_lines_;

			/**
			 * produce random UUIDs for parser VMs
			 */
			boost::uuids::random_generator rng_;

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
