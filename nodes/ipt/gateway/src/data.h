/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_SML_DATAL_H
#define NODE_SML_DATAL_H


#include <smf/sml/defs.h>
#include <smf/mbus/header.h>

#include <cyng/log.h>
#include <cyng/vm/controller.h>
#include <cyng/store/db.h>

namespace node
{
	namespace sml
	{
		/**
		 * Process incoming meter data
		 */
		class data
		{
		public:
			data(cyng::logging::log_ptr 
				, cyng::controller& vm
				, cyng::store::db& config_db);

		private:
			/**
			 *	callback from wireless LMN
			 */
			void wmbus_push_frame(cyng::context& ctx);

			/**
			 * data from SML parser after receiving a 0x7F frame (short SML header)
			 */
			void sml_get_list_response(cyng::context& ctx);

			/**
			 * store received data
			 */
			void store(cyng::context& ctx);

			void update_device_table(cyng::buffer_t const& dev_id
				, std::string const& manufacturer
				, std::uint8_t version
				, std::uint8_t media
				, std::uint8_t frame_type
				, cyng::crypto::aes_128_key
				, boost::uuids::uuid tag);

			/**
			 * Applied from master with CI = 0x53, 0x55, 0x5B, 0x5F, 0x60, 0x64, 0x6Ch, 0x6D
			 * Applied from slave with CI = 0x68, 0x6F, 0x72, 0x75, 0x7C, 0x7E, 0x9F
			 */
			void read_frame_header_long(cyng::context& ctx, cyng::buffer_t const&, cyng::buffer_t const&);
			void read_frame_header_short(cyng::context& ctx, cyng::buffer_t const&, cyng::buffer_t const&);

			void read_variable_data_block(cyng::buffer_t const&, header_short&);

			/**
			 * frame type 0x7F
			 */
			void read_frame_header_short_sml(cyng::context& ctx, cyng::buffer_t const&, cyng::buffer_t const&);


			bool decode_data(boost::uuids::uuid tag, std::uint8_t aes_mode, cyng::buffer_t const& server_id, header_long&);
			bool decode_data(boost::uuids::uuid tag, std::uint8_t aes_mode, cyng::buffer_t const& server_id, header_short&);
			bool decode_data_mode_5(boost::uuids::uuid tag, std::uint8_t aes_mode, cyng::buffer_t const& server_id, header_long&);
			bool decode_data_mode_5(boost::uuids::uuid tag, std::uint8_t aes_mode, cyng::buffer_t const& server_id, header_short&);

		private:
			cyng::logging::log_ptr logger_;

			/**
			 * configuration db
			 */
			cyng::store::db& config_db_;

			/**
			 * last/current data set of SML data
			 */
			cyng::param_map_t params_;

		};

	}	//	sml
}

#endif
