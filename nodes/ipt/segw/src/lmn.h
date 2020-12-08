/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */
#ifndef NODE_LMN_H
#define NODE_LMN_H

#include <cfg_broker.h>

#include <smf/mbus/decoder.h>

#include <cyng/async/mux.h>
#include <cyng/vm/controller.h>
#include <cyng/table/table_fwd.h>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/random_generator.hpp>

namespace node
{
	//
	//	forward declaration
	//
	class cache;
	class cfg_wmbus;
	class cfg_mbus;

	/**
	 * Read configuration and starts all required
	 * tasks to read data from serial ports and connects
	 * this data stream to a parser.
	 */
	class lmn
	{
	public:
		lmn(cyng::async::mux&
			, cyng::logging::log_ptr logger
			, cache&
			, boost::uuids::uuid tag);

		/**
		 * Open serial and wireless communication ports
		 */
		void start();

	private:
		void start_lmn_wired();
		void start_lmn_wireless(cfg_wmbus const&);

		std::size_t start_wireless_sender(cfg_wmbus const&, std::size_t tsk_status);

		/**
		 * @param init initialization message
		 */
		std::pair<std::size_t, bool> start_lmn_port_wireless(std::size_t	//	status task
			, cyng::buffer_t&& init);
		std::pair<std::size_t, bool> start_lmn_port_wired(std::size_t);	//	status task

		/**
		 * Start RS485 wired M-Bus interface manager
		 */
		std::pair<std::size_t, bool> start_rs485_mgr(std::size_t, std::chrono::seconds);

		/**
		 * Start RS485 IEC interface manager
		 */
		std::pair<std::size_t, bool> start_iec_mgr(std::size_t, std::chrono::seconds);

		/**
		 * Starts the wireless M-Bus receiver. This could be the wireless mBus parse
		 * or a server connected via TCP/IP.
		 * @return a list of receiver tasks. The first one is the wireless M-Bus parser
		 */
		void start_wireless_mbus_receiver(bool profile, server_list_t&&);
		void start_wired_mbus_receiver(bool profile, server_list_t&&);

		/**
		 * incoming wireless M-Bus data
		 */
		void wmbus_push_frame(cyng::context& ctx);
		void sml_msg(cyng::context& ctx);
		void sml_eom(cyng::context& ctx);
		void sml_get_list_response(cyng::context& ctx);

		void mbus_ack(cyng::context& ctx);
		void mbus_frame_short(cyng::context& ctx);
		void mbus_frame_ctrl(cyng::context& ctx);
		void mbus_frame_long(cyng::context& ctx);

		void sig_ins_cfg(cyng::store::table const*
			, cyng::table::key_type const&
			, cyng::table::data_type const&
			, std::uint64_t
			, boost::uuids::uuid);
		void sig_del_cfg(cyng::store::table const*, cyng::table::key_type const&, boost::uuids::uuid);
		void sig_clr_cfg(cyng::store::table const*, boost::uuids::uuid);
		void sig_mod_cfg(cyng::store::table const*
			, cyng::table::key_type const&
			, cyng::attr_t const&
			, std::uint64_t
			, boost::uuids::uuid);

		void cb_wmbus_meter(cyng::buffer_t const& srv_id
			, std::uint8_t status
			, std::uint8_t aes_mode
			, cyng::crypto::aes_128_key const& aes);

		void cb_wmbus_data(cyng::buffer_t const& srv_id
			, cyng::buffer_t const& data
			, std::uint8_t status
			, boost::uuids::uuid pk);

		void cb_wmbus_value(cyng::buffer_t const& srv_id
			, cyng::object const& val
			, std::uint8_t scaler
			, mbus::units unit
			, sml::obis code
			, boost::uuids::uuid pk);

	private:
		cyng::logging::log_ptr logger_;
		cyng::async::mux& mux_;
		cyng::controller vm_;
		cache& cache_;
		wmbus::decoder decoder_;

		std::size_t serial_mgr_;
		std::size_t radio_port_;	//	[1] wireless LMN task (M-Bus)
		std::size_t serial_port_;	//	[2] wired LMN task (rs485)

		/**
		 * distribute incoming data to receiver (optionally parsed)
		 */
		std::size_t radio_distributor_;	

		boost::uuids::random_generator_mt19937 uuidgen_;
	};

}
#endif