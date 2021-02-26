/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/hci/parser.h>

namespace smf
{
	namespace hci
	{
		parser::parser(cb_t cb)
			: state_(state::SOF)
			, cb_(cb)
			, msg_()
		{}

		void parser::put(char c) {

			switch (state_) {
			case state::SOF:
				state_ = state_sof(c);
				break;
			case state::HEADER_CTRL_EP:			//!<	Control and Enpoint field
				state_ = state_header_ctl_ep(c);
				break;
			case state::HEADER_MSG_ID:
				state_ = state_header_msg_id(c);
				break;
			case state::HEADER_LENGTH:
				state_ = state_header_length(c);
				break;
			case state::PAYLOAD:
				state_ = state_payload(c);
				break;
			case state::TIME_STAMP:
				state_ = state_timestamp(c);
				break;
			case state::RSSI:
				state_ = state_rssi(c);
				break;
			case state::FCS:
				state_ = state_fcs(c);
				break;

			default:
				break;
			}
		}

		parser::state parser::state_sof(char c) {
			return (sof_c == c)
				? state::HEADER_CTRL_EP
				: state_
				;
		}

		parser::state parser::state_header_ctl_ep(char c) {
			msg_.header_ctrl_ep_ = static_cast<std::uint8_t>(c);
			return state::HEADER_MSG_ID;
		}
		parser::state parser::state_header_msg_id(char c) {
			msg_.header_msg_id_ = static_cast<std::uint8_t>(c);
			return state::HEADER_LENGTH;
		}
		parser::state parser::state_header_length(char c) {
			msg_.header_length_ = static_cast<std::uint8_t>(c);
			msg_.payload_.clear();
			msg_.payload_.reserve(msg_.get_length());
			msg_.time_stamp_.clear();
			msg_.fcs_.clear();
			if (msg_.get_length() != 0) {
				//	is part of the payload
				msg_.payload_.push_back(c);
				return state::PAYLOAD;
			}
			//	This happens after sending an initilaization string
			return state::FCS;
		}

		parser::state parser::state_payload(char c) {
			msg_.payload_.push_back(c);
			if (msg_.is_complete()) {
				if (msg_.has_timestamp())	return state::TIME_STAMP;
				else if (msg_.has_rssi())	return state::RSSI;
				else if (msg_.has_crc16())	return state::FCS;
				else {
					cb_(msg_);	//	complete
					return state::SOF;
				}
			}
			return state_;
		}

		parser::state parser::state_timestamp(char c) {
			msg_.time_stamp_.push_back(c);
			if (msg_.time_stamp_.size() == 4) {
				if (msg_.has_rssi())	return state::RSSI;
				else if (msg_.has_crc16())	return state::FCS;
				else {
					cb_(msg_);	//	complete
					return state::SOF;
				}
			}
			return state_;
		}

		parser::state parser::state_rssi(char c) {
			msg_.rssi_ = static_cast<std::uint8_t>(c);
			if  (! msg_.has_crc16())	{
				cb_(msg_);	//	complete
				return state::SOF;
			}
			return state::FCS;
		}

		parser::state parser::state_fcs(char c) {
			msg_.time_stamp_.push_back(c);
			if (msg_.time_stamp_.size() == 2) {
				cb_(msg_);	//	complete
				return state::SOF;
			}
			return state_;
		}

	}
}


