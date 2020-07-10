/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_BUS_H
#define NODE_IPT_BUS_H

#include <smf/ipt/bus_interface.h>
#include <smf/ipt/scramble_key.h>
#include <smf/ipt/config.h>
#include <smf/ipt/parser.h>
#include <smf/ipt/serializer.h>

#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/vm/controller.h>

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
		class bus : public bus_interface 
		{
		public:
			bus(cyng::logging::log_ptr logger
				, cyng::async::mux&
				, boost::uuids::uuid tag
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
			bool is_connected() const;

			/**
			 * @return true if watchdog is requested
			 */
			bool has_watchdog() const;
			//std::uint16_t get_watchdog() const;

			boost::asio::ip::tcp::endpoint local_endpoint() const;
			boost::asio::ip::tcp::endpoint remote_endpoint() const;

			/**
			 * Send a login request (public/scrambled)
			 * 
			 * @return true if bus/session is not authorized yet and login request
			 * was emitted.
			 */
			bool req_login(master_record);

			/**
			 * send connection open request and starts a monitor tasks
			 * to detect timeouts.
			 *
			 * @return false if bus is in authorized state
			 */
			bool req_connection_open(std::string const& number, std::chrono::seconds d);

			/**
			 * send connection close request and starts a monitor tasks
			 * to detect timeouts.
			 *
			 * @return false if bus is not in connected state
			 */
			bool req_connection_close(std::chrono::seconds d);

			/**
			 * send connection open response.
			 * @param seq ipt sequence
			 * @param accept true if request is accepted
			 */
			void res_connection_open(sequence_type seq, bool accept);

			/**
			 * send a register push target request with an timeout
			 */
			void req_register_push_target(std::string const& name
				, std::uint16_t packet_size
				, std::uint8_t window_size
				, std::chrono::seconds);

			/**
			 * send a channel open request with an timeout
			 *
			 * @param tsk task to send response event, if tsk == NO_TASK.
			 * @return true if bus is online
			 */
			bool req_channel_open(std::string const& target
				, std::string const& account
				, std::string const& msisdn
				, std::string const& version
				, std::string const& device
				, std::uint16_t time_out
				, std::size_t tsk);

			bool req_channel_close(std::uint32_t channel);

			/**
			 * transfer push data in "chunks"
			 */
			channel_response req_transfer_push_data(std::uint32_t channel
				, std::uint32_t source
				, cyng::buffer_t const& data
				, std::size_t tsk);

			/**
			 * @return a textual description of the bus/connection state
			 */
			std::string get_state() const;

		private:
			void do_read();

			void ipt_res_login(cyng::context& ctx, bool scrambled);
			void ipt_res_register_push_target(cyng::context& ctx);
			void ipt_res_deregister_push_target(cyng::context& ctx);
			void ipt_req_register_push_target(cyng::context& ctx);
			void ipt_req_deregister_push_target(cyng::context& ctx);
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
			void remove_relation(cyng::context& ctx);

			/**
			 * lookup channel db and test if data size exceeds 
			 * allowed package size.
			 */
			std::pair<std::uint16_t, bool> test_channel_size(std::uint32_t, std::size_t) const;

		protected:
			/**
			 * The logger instance
			 */
			cyng::logging::log_ptr logger_;

			/**
			 * Instance of CYNG VM
			 */
			cyng::controller vm_;

		private:
			/**
			 * task scheduler
			 */
			cyng::async::mux& mux_;

			/**
			 * connection socket
			 */
			boost::asio::ip::tcp::socket socket_;

			/**
			 * Buffer for incoming data.
			 */
			std::array<char, NODE::PREFERRED_BUFFER_SIZE> buffer_;

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
			std::string const model_;

			/**
			 * connection open retries
			 */
			std::size_t const retries_;

			/** 
			 * watchdog in minutes
			 */
			std::uint16_t watchdog_;

			/**
			 * session state
			 */
			enum state {
				STATE_ERROR_,
				STATE_INITIAL_,
				STATE_AUTHORIZED_,
				STATE_CONNECTED_,
				STATE_WAIT_FOR_OPEN_RESPONSE_,
				STATE_WAIT_FOR_CLOSE_RESPONSE_,
				STATE_SHUTDOWN_,
				STATE_HALTED_
			};
			std::atomic<state>	state_;

			/**
			 * Check if transition to new state is valid.
			 */
			bool transition(state evt);

			/**
			 * bookkeeping of ip-t sequence to task relation
			 */
			std::map<sequence_type, std::size_t>	task_db_;

			/**
			 * manage package size of push channels
			 */
			std::map<std::uint32_t, std::uint16_t>	channel_db_;
		};
	}
}

#endif
