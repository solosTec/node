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

namespace node
{
	namespace ipt
	{
		class bus : public std::enable_shared_from_this<bus>
		{
		public:
			using shared_type = std::shared_ptr<bus>;

		public:
			bus(cyng::async::mux& mux
				, cyng::logging::log_ptr logger
				, boost::uuids::uuid tag
				, scramble_key const&
				, std::size_t tsk
				, std::string const& model);

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

		private:
			void do_read();

			void ipt_res_login(cyng::context& ctx, bool scrambled);
			void ipt_res_register_push_target(cyng::context& ctx);
			void ipt_res_deregister_push_target(cyng::context& ctx);
			void ipt_req_transmit_data(cyng::context& ctx);
			void ipt_req_open_connection(cyng::context& ctx);
			void ipt_req_close_connection(cyng::context& ctx);

			void ipt_req_protocol_version(cyng::context& ctx);
			void ipt_req_software_version(cyng::context& ctx);
			void ipt_req_device_id(cyng::context& ctx);
			void ipt_req_net_stat(cyng::context& ctx);
			void ipt_req_ip_statistics(cyng::context& ctx);
			void ipt_req_dev_auth(cyng::context& ctx);
			void ipt_req_dev_time(cyng::context& ctx);

			void ipt_req_transfer_pushdata(cyng::context& ctx);

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
			std::array<char, NODE_PREFERRED_BUFFER_SIZE> buffer_;

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
				STATE_SHUTDOWN_,
			} state_;

		};

		/**
		 * factory method for cluster bus
		 */
		bus::shared_type bus_factory(cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, boost::uuids::uuid tag
			, scramble_key const& sk
			, std::size_t tsk
			, std::string const& model);
	}
}

#endif
