/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_MBUS_BROKER_SESSION_H
#define NODE_MBUS_BROKER_SESSION_H

#include <NODE_project_info.h>
#include <cache.h>

#include <smf/mbus/parser.h>
#include <smf/mbus/decoder.h>

#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/vm/controller.h>
#include <cyng/store/store_fwd.h>

#include <boost/uuid/random_generator.hpp>

namespace node
{

	class session : public std::enable_shared_from_this<session>
	{
		using read_buffer_t = std::array<char, NODE::PREFERRED_BUFFER_SIZE>;

	public:
		session(boost::asio::ip::tcp::socket socket
			, cyng::logging::log_ptr
			, cyng::controller& cluster
			, boost::uuids::uuid tag
			, cyng::store::db&
			, bool session_login
			, bool session_auto_insert);
		virtual ~session();

	public:
		void start();

	private:
		void do_read();
		void process_data(cyng::buffer_t&&);
		void process_login(cyng::buffer_t&&);

		void decode(wmbus::header const& h, cyng::buffer_t const& data);
		void insert_meter(wmbus::header const& h);

		void cb_meter_decoded(cyng::buffer_t const& srv_id
			, std::uint8_t status
			, std::uint8_t aes_mode
			, cyng::crypto::aes_128_key const& aes);

		void cb_data_decoded(cyng::buffer_t const& srv_id
			, cyng::buffer_t const& data
			, std::uint8_t status
			, boost::uuids::uuid pk);

		void cb_value_decoded(cyng::buffer_t const& srv_id
			, cyng::object const& val
			, std::uint8_t scaler
			, mbus::units unit
			, sml::obis code
			, boost::uuids::uuid pk);

		void sml_msg(cyng::context& ctx);
		void sml_get_list_response(cyng::context& ctx);

	private:
		boost::asio::ip::tcp::socket socket_;
		cyng::logging::log_ptr logger_;
		cyng::controller& cluster_;	//!< cluster bus VM
		cyng::controller vm_;	//!< session VM
		cache cache_;
		bool const session_login_;
		bool const session_auto_insert_;

		/**
		 * Buffer for incoming data.
		 */
		read_buffer_t buffer_;

		/**
		 * authorization state
		 */
		bool authorized_;

		/**
		 * temporary data buffer
		 */
		cyng::buffer_t data_;

		/**
		 * in/out byte counter
		 */
		std::uint64_t rx_, sx_;

		/**
		 * M-Bus parser
		 */
		wmbus::parser parser_;

		/**
		 * generate PKs for TMeter table
		 */
		boost::uuids::random_generator_mt19937 uuidgen_;

		/**
		 * payload decoder
		 */
		wmbus::decoder decoder_;
	};
	
}

#endif
