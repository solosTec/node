/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_IEC_62056_TASK_CLIENT_H
#define NODE_IEC_62056_TASK_CLIENT_H

#include <NODE_project_info.h>
#include <smf/iec/parser.h>
#include <smf/sml/protocol/generator.h>

#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/policy.h>
#include <cyng/store/store_fwd.h>
#include <cyng/table/key.hpp>
#include <cyng/vm/controller.h>

namespace node
{

	class client
	{
		using read_buffer_t = std::array<char, NODE::PREFERRED_BUFFER_SIZE>;

	public:
		using msg_0 = std::tuple<std::chrono::system_clock::time_point>;
		using signatures_t = std::tuple<msg_0>;

	public:
		client(cyng::async::base_task* bt
			, cyng::controller& cluster
			, boost::uuids::uuid tag
			, cyng::logging::log_ptr
			, cyng::store::db&
			, boost::asio::ip::tcp::endpoint
			, std::string const& meter
			, cyng::buffer_t const& srv_id
			, cyng::table::key_type const&
			, bool client_login
			, bool verbose
			, std::string const& target);
		cyng::continuation run();
		void stop(bool shutdown);

		/**
		 * @brief slot [0]
		 *
		 * query meter data
		 */
		cyng::continuation process(std::chrono::system_clock::time_point);

	private:
		/**
		 * connect to server
		 */
		void do_connect(const boost::asio::ip::tcp::resolver::results_type& endpoints);
		void do_read();
		void do_write();
		void reset_write_buffer();

		void data_start(cyng::context& ctx);
		void data_line(cyng::context& ctx);
		void data_bcc(cyng::context& ctx);
		void data_eof(cyng::context& ctx);

		void client_res_login(cyng::context& ctx);
		void client_res_open_push_channel(cyng::context& ctx);
		void client_res_close_push_channel(cyng::context& ctx);
		void client_res_transfer_push_data(cyng::context& ctx);

		void authorize_client();
		void query_metering_data();

		std::chrono::seconds get_monitor() const;

	private:
		cyng::async::base_task& base_;

		/**
		 * communication bus to master
		 */
		cyng::controller& cluster_;
		cyng::controller vm_;
		cyng::logging::log_ptr logger_;

		/**
		 * global data cache
		 */
		cyng::store::db& cache_;

		boost::asio::ip::tcp::endpoint const ep_;

		std::string const meter_;
		cyng::buffer_t const srv_id_;
		cyng::table::key_type const key_;
		bool const client_login_;
		std::string const target_;

		/**
		 * connection socket
		 */
		boost::asio::ip::tcp::socket socket_;

		/**
		 * IEC parser
		 */
		iec::parser parser_;

		/**
		 * Buffer for incoming data.
		 */
		read_buffer_t buffer_read_;
		std::deque<cyng::buffer_t>	buffer_write_;

		enum class internal_state {
			FAILURE,
			SHUTDOWN,
			INITIAL,
			WAIT,	//	for login response
			AUTHORIZED,	//	check connection state with socket
			READING,	//	readout is running
		} state_;

		bool target_available_;		//	push target
		std::uint32_t channel_;		//	push target channel
		std::uint32_t source_;		//	push target source

		cyng::tuple_t period_list_;	//	all results
		sml::trx trx_;


	};
	
}

#endif
