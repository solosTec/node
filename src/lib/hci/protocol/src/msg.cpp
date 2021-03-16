/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/hci/msg.h>

#include <cyng/obj/buffer_cast.hpp>

#include <bitset>

namespace smf
{
	namespace hci
	{
		msg::msg()
			: header_ctrl_ep_{ 0 }
			, header_msg_id_{ 0 }
			, header_length_{ 0 }
			, payload_()
			, rssi_{ 0 }
			, time_stamp_()
			, fcs_()
		{}

		bool msg::has_crc16() const {
			std::bitset<8> const bs(header_ctrl_ep_);
			return bs.test(7);
		}

		bool msg::has_rssi() const {
			std::bitset<8> const bs(header_ctrl_ep_);
			return bs.test(6);
		}

		bool msg::has_timestamp() const {
			std::bitset<8> const bs(header_ctrl_ep_);
			return bs.test(5);
		}

		cyng::buffer_t msg::get_payload() const {
			return payload_;
		}

		bool msg::is_complete() const {
			return get_length() + 1 == payload_.size();
		}

		double msg::get_dBm() const {
			return (has_rssi())
				? (-120.0 + (0.75 * rssi_))
				: 0;
		}

		std::uint32_t msg::get_time_stamp() const {
			return (has_timestamp())
				? cyng::to_numeric<std::uint32_t>(time_stamp_)
				: 0u
				;
		}

		std::int16_t msg::get_crc() const {
			return (has_crc16())
				? cyng::to_numeric<std::int16_t>(fcs_)
				: 0u
				;
		}

		std::ostream& operator<<(std::ostream& os, msg const& m) {
			os
				<< "\""
				<< +m.get_length()
				<< " bytes, ep: "
				<< +m.header_ctrl_ep_	//!<	Control and Endpoint field
				<< ", id: "
				<< +m.header_msg_id_	//!<	message ID field
				<< "\":hci"
				;
			return os;
		}

	}
}


