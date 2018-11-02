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
		class network : public bus
		{
		public:

			//	[0] register data consumer
			using msg_0 = std::tuple<std::string, std::size_t>;

			//	[1] remove data consumer
			using msg_1 = std::tuple<std::string, std::uint64_t, boost::uuids::uuid>;

			using signatures_t = std::tuple<msg_0, msg_1>;

		public:
			network(cyng::async::base_task* bt
				, cyng::logging::log_ptr
				, boost::uuids::uuid tag
				, bool log_pushdata
				, redundancy const& cfg
				, std::map<std::string, std::string> const& targets);
			cyng::continuation run();
			void stop();

			/**
			 * @brief event 0x4001/0x4002: response login
			 *
			 * sucessful network login
			 *
			 * @param watchdog watchdog timer in minutes
			 * @param redirect URL to reconnect
			 */
			virtual void on_login_response(std::uint16_t, std::string) override;

			/**
			 * @brief event IP-T connection lost
			 *
			 * connection lost / reconnect
			 */
			virtual void on_logout() override;

			/**
			 * @brief event 0x4005: push target registered response
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
			 * @brief event 0x4006: push target deregistered response
			 *
			 * deregister target response
			 */
			virtual void on_res_deregister_target(sequence_type, bool, std::string const&) override;

			/**
			 * @brief event 0x1000: push channel open response
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
			 * @brief event 0x1001: push channel close response
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
			 * @brief event 0x9003: connection open request 
			 *
			 * incoming call
			 *
			 * @return true if request is accepted - otherwise false
			 */
			virtual bool on_req_open_connection(sequence_type, std::string const& number) override;

			/**
			 * @brief event 0x1003: connection open response
			 *
			 * @param seq ipt sequence
			 * @param success true if connection open request was accepted
			 */
			virtual cyng::buffer_t on_res_open_connection(sequence_type seq, bool success) override;

			/**
			 * @brief event 0x9004/0x1004: connection close request/response
			 *
			 * open connection closed
			 */
			virtual void on_req_close_connection(sequence_type) override;
			virtual void on_res_close_connection(sequence_type) override;

			/**
			 * @brief event 0x9002: push data transfer request
			 *
			 * push data
			 */
			virtual void on_req_transfer_push_data(sequence_type, std::uint32_t, std::uint32_t, cyng::buffer_t const&) override;

			/**
			 * @brief event transmit data (if connected)
			 *
			 * incoming data
			 */
			virtual cyng::buffer_t on_transmit_data(cyng::buffer_t const&) override;

			/**
			 * @brief slot [0] - message from costumer to register (STORE_EVENT_REGISTER_CONSUMER)
			 *
			 * add consumer
			 */
			cyng::continuation process(std::string, std::size_t tid);

			/**
			 * @brief slot [1] - remove processor (STORE_EVENT_REMOVE_PROCESSOR)
			 *
			 * remove line
			 */
			cyng::continuation process(std::string, std::uint64_t line, boost::uuids::uuid);

		private:
			void reconfigure(cyng::context& ctx);
			void reconfigure_impl();

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

			/**
			 * write all incoming push data into separate files
			 */
			void log_push_data(std::uint32_t source
				, std::uint32_t target
				, std::string const&
				, cyng::buffer_t const& data);

			void test_line_activity();
			void test_line_activity_SML();
			void test_line_activity_IEC();

		private:
			cyng::async::base_task& base_;
			cyng::logging::log_ptr logger_;

			/**
			 * write all incoming push data into separate files
			 */
			const bool log_pushdata_;

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
			boost::uuids::random_generator uidgen_;

		};
	}
}

#endif
