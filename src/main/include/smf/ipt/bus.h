/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_BUS_H
#define NODE_IPT_BUS_H

#include <NODE_project_info.h>
#include <smf/ipt/parser.h>
#include <smf/ipt/serializer.h>
#include <cyng/async/mux.h>
#include <cyng/log.h>
#include <cyng/vm/controller.h>
#include <array>
#include <chrono>

namespace node
{
	namespace ipt
	{
		/**
		 * Implementation of an IP-T client. 
		 * The bus send messages to it's host to signal different IP-T events.
		 * <ol>
		 * <li>0 - successful authorized</li>
		 * <li>1 - connection to master lost</li>
		 * <li>2 - incoming call (open connection request)</li>
		 * <li>3 - push data received</li>
		 * <li>4 - push target registered response</li>
		 * <li>5 - data received</li>
		 * <li>6 - push target deregistered response</li>
		 * <li>7 - open connection closed</li>
		 * <li>8 - open push channel</li>
		 * <li>9 - push channel closed</li>
		 * <li>10 - connection open response</li>
		 * </ol>
		 */
		class bus : public std::enable_shared_from_this<bus>
		{
		public:
			enum ipt_events : std::size_t  {

				//	control layer
				IPT_EVENT_AUTHORIZED,	//	 [0] 0x4001/0x4002: response login
				IPT_EVENT_CONNECTION_TO_MASTER_LOST, //	[1]

				IPT_EVENT_PUSH_TARGET_REGISTERED,		//	[2] 0x4005: push target registered response
				IPT_EVENT_PUSH_TARGET_DEREREGISTERED,	//	[3] 0x4006: push target deregistered response

				//	transport layer
				IPT_EVENT_PUSH_CHANNEL_OPEN,	//	[4] 0x1000: push channel open response
				IPT_EVENT_PUSH_CHANNEL_CLOSED,	//	[5] 0x1001: push channel close response
				IPT_EVENT_INCOMING_CALL,		//	[6] 0x9003: connection open request 
				IPT_EVENT_CONNECTION_OPEN,		//	[7] 0x1003: connection open response
				IPT_EVENT_CONNECTION_CLOSED,	//	[8] 0x9004/0x1004: connection close request/response
				IPT_EVENT_PUSH_DATA_RECEIVED,	//	[9] 0x9002: push data transfer request

				IPT_EVENT_INCOMING_DATA,	//	[10] transmit data (if connected)
			};

		public:
			using shared_type = std::shared_ptr<bus>;

		public:
			bus(cyng::async::mux& mux
				, cyng::logging::log_ptr logger
				, boost::uuids::uuid tag
				, scramble_key const&
				, std::size_t tsk
				, std::string const& model
				, std::size_t retries);

			bus(bus const&) = delete;
			bus& operator=(bus const&) = delete;

			void start();
            void stop();

			/**
			 * @return true if online with master
			 */
			bool is_online() const;

			/**
			 * @return true if watchdog is requested
			 */
			bool has_watchdog() const;
			std::uint16_t get_watchdog() const;

			boost::asio::ip::tcp::endpoint local_endpoint() const;
			boost::asio::ip::tcp::endpoint remote_endpoint() const;

			/**
			 * send connection open request and starts a monitor tasks
			 * to detect timeouts.
			 */
			void req_connection_open(std::string const& number, std::chrono::seconds d);

			/**
			 * send connection close request and starts a monitor tasks
			 * to detect timeouts.
			 */
			void req_connection_close(std::chrono::seconds d);

			/**
			 * send connection open response.
			 * @param seq ipt sequence
			 * @param accept true if request is accepted
			 */
			void res_connection_open(sequence_type seq, bool accept);

			/**
			 * @return a textual description of the bus/connection state
			 */
			std::string get_state() const;

		private:
			void do_read();

			void ipt_res_login(cyng::context& ctx, bool scrambled);
			void ipt_res_register_push_target(cyng::context& ctx);
			void ipt_res_deregister_push_target(cyng::context& ctx);
			void ipt_res_open_channel(cyng::context& ctx);
			void ipt_res_close_channel(cyng::context& ctx);
			void ipt_res_transfer_push_data(cyng::context& ctx);
			void ipt_req_transmit_data(cyng::context& ctx);
			void ipt_req_open_connection(cyng::context& ctx);
			void ipt_res_open_connection(cyng::context& ctx);
			void ipt_req_close_connection(cyng::context& ctx);
			void ipt_res_close_connection(cyng::context& ctx);

			void ipt_req_protocol_version(cyng::context& ctx);
			void ipt_req_software_version(cyng::context& ctx);
			void ipt_req_device_id(cyng::context& ctx);
			void ipt_req_net_stat(cyng::context& ctx);
			void ipt_req_ip_statistics(cyng::context& ctx);
			void ipt_req_dev_auth(cyng::context& ctx);
			void ipt_req_dev_time(cyng::context& ctx);

			void ipt_req_transfer_pushdata(cyng::context& ctx);

			void store_relation(cyng::context& ctx);

		public:
			/**
			 * Interface to cyng VM
			 */
			cyng::controller vm_;

		private:
			/**
			 * connection socket
			 */
			boost::asio::ip::tcp::socket socket_;

			/**
			 * Buffer for incoming data.
			 */
			std::array<char, NODE::PREFERRED_BUFFER_SIZE> buffer_;

			cyng::async::mux& mux_;

			/**
			 * The logger instance
			 */
			cyng::logging::log_ptr logger_;

            /**
             * Owner task
             */
			const std::size_t task_;

			/**
			 * Parser for binary cyng data stream (from cluster master)
			 */
			parser 	parser_;

			/**
			 * wrapper class to serialize and send
			 * data and code.
			 */
			serializer		serializer_;

			/**
			 * device id - response device id request
			 */
			const std::string model_;

			/**
			 * connection open retries
			 */
			const std::size_t retries_;

			/** 
			 * watchdog in minutes
			 */
			std::uint16_t watchdog_;

			/**
			 * session state
			 */
			enum {
				STATE_ERROR_,
				STATE_INITIAL_,
				STATE_AUTHORIZED_,
				STATE_CONNECTED_,
				STATE_WAIT_FOR_OPEN_RESPONSE_,
				STATE_WAIT_FOR_CLOSE_RESPONSE_,
				STATE_SHUTDOWN_,
			} state_;

			/**
			 * bookkeeping of ip-t sequence to task relation
			 */
			std::map<sequence_type, std::size_t>	task_db_;

		};

		/**
		 * factory method for cluster bus
		 */
		bus::shared_type bus_factory(cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, boost::uuids::uuid tag
			, scramble_key const& sk
			, std::size_t tsk
			, std::string const& model
			, std::size_t retries);

		/**
		 * Combine two 32-bit integers into one 64-bit integer
		 */
		std::uint64_t build_line(std::uint32_t channel, std::uint32_t source);

	}
}

#endif
