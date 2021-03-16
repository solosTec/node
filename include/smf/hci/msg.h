/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_HCI_MSG_H
#define SMF_HCI_MSG_H


#include <cyng/obj/intrinsics/buffer.h>

#include <type_traits>
#include <ostream>

namespace smf
{
	namespace hci
	{
		class parser;
		class msg;
		std::ostream& operator<<(std::ostream& os, msg const& m);

		/**
		 * [0] SOF
		 * [1-3] header (ctrl field, endpoint id, msg id, length)
		 * [4..] payload
		 * [+4] timestamp
		 * [+1] RSSI
		 * [+2] FCS
		 */
		class msg
		{
			friend class parser;
			friend std::ostream& operator<<(std::ostream&, msg const&);

		public:
			msg();
			msg(msg const&) = default;

			constexpr std::uint8_t get_length() const {
				return header_length_;
			}

			bool has_crc16() const;
			bool has_rssi() const;
			bool has_timestamp() const;

			cyng::buffer_t get_payload() const;

			/**
			 * @return true if full payload size 
			 * is reached.
			 */
			bool is_complete() const;

			/**
			 * 98=-45.3dBm
			 * 91=-49.3dBm
			 * 81=-57.9dBm
			 * approximation f(x)=3/4
			 * 
			 * @return RSSI as cBm
			 */
			double get_dBm() const;

			std::uint32_t get_time_stamp() const;

			std::int16_t get_crc() const;

		private:
			std::uint8_t header_ctrl_ep_;	//!<	Control and Endpoint field
			std::uint8_t header_msg_id_;	//!<	message ID field
			std::uint8_t header_length_;	//!<	length in bytes

			cyng::buffer_t payload_;
			std::uint8_t rssi_;
			cyng::buffer_t time_stamp_;
			cyng::buffer_t fcs_;
		};

	}
}


#endif
