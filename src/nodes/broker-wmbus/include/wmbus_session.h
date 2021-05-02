/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_WMBUS_SESSION_H
#define SMF_WMBUS_SESSION_H

#include <db.h>
#include <smf/cluster/bus.h>

#include <smf/mbus/radio/parser.h>

#include <cyng/log/logger.h>
#include <cyng/io/parser/parser.h>
#include <cyng/vm/proxy.h>
#include <cyng/obj/intrinsics/pid.h>
#include <cyng/task/controller.h>

#include <memory>
#include <array>

namespace smf {

	class wmbus_session : public std::enable_shared_from_this<wmbus_session>
	{
	public:
		wmbus_session(cyng::controller& ctl
			, boost::asio::ip::tcp::socket socket
			, std::shared_ptr<db>
			, cyng::logger
			, bus&);
		~wmbus_session();

		void start(std::chrono::seconds timeout);
		void stop();

	private:
		void do_read();
		void do_write();
		void handle_write(const boost::system::error_code& ec);

		void decode(mbus::radio::header const& h
			, mbus::radio::tpl const& t
			, cyng::buffer_t const& data);
		void decode(srv_id_t id
			, std::uint8_t access_no
			, std::uint8_t frame_type
			, cyng::buffer_t const& data);

		void push_sml_data(cyng::buffer_t const& payload);
		void push_dlsm_data(cyng::buffer_t const& payload);
		void push_data(cyng::buffer_t const& payload);

	private:
		cyng::controller& ctl_;
		boost::asio::ip::tcp::socket socket_;
		cyng::logger logger_;
		bus& bus_;
		std::shared_ptr<db> db_;

		/**
		 * Buffer for incoming data.
		 */
		std::array<char, 2048>	buffer_;

		/**
		 * Buffer for outgoing data.
		 */
		std::deque<cyng::buffer_t>	buffer_write_;

		/**
		 * parser for wireless M-Bus data
		 */
		mbus::radio::parser parser_;

		/**
		 * gatekeeper
		 */
		cyng::channel_ptr gatekeeper_;

	};

}

#endif
