/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/ipt/parser.h>
#include <smf/ipt/codes.h>

namespace smf
{
	namespace ipt	
	{
		parser::parser(scramble_key const& sk)
			: state_(state::HEADER)
			, def_sk_(sk)
			, scrambler_()
			, buffer_()
			, header_()
		{
			buffer_.reserve(HEADER_SIZE);
			header_.reset(INVALID_SEQ);
		}

		void parser::put(char c) {

			switch (state_) {
			case state::STREAM:
				state_ = state_stream(c);
				break;
			case state::ESC:
				state_ = state_esc(c);
				break;
			case state::HEADER:
				state_ = state_header(c);
				break;
			case state::DATA:
				state_ = state_data(c);
				break;
			default:
				BOOST_ASSERT_MSG(false, "invalid parser state");
				break;
			}
		}

		parser::state parser::state_stream(char c) {
			if (c == ESCAPE_SIGN)	return state::ESC;
			buffer_.push_back(c);
			return state_;
		}
		parser::state parser::state_esc(char c) {
			if (c == ESCAPE_SIGN) {
				buffer_.push_back(c);
				return state::STREAM;
			}
			return state::HEADER;
		}
		parser::state parser::state_header(char c) {
			buffer_.push_back(c);
			if (buffer_.size() == HEADER_SIZE) {
				header_ = to_header(buffer_);
				buffer_.clear();

				if (has_data(header_)) {

					//
					//	collect data
					//
					buffer_.reserve(size(header_));
					return state::DATA;

				}
				else {

					//
					//	ToDo: command complete
					//

					buffer_.reserve(HEADER_SIZE);
					return state::HEADER;
				}
			}
			return state_;
		}
		parser::state parser::state_data(char c) {
			buffer_.push_back(c);
			if (buffer_.size() == size(header_)) {

				//
				//	command complete
				//
				read_command();
				buffer_.clear();
				buffer_.reserve(HEADER_SIZE);
				return state::HEADER;
			}
			return state_;
		}

		void parser::read_command() {
			switch (static_cast<code>(header_.command_)) {
			case code::TP_REQ_OPEN_PUSH_CHANNEL:
				break;
			case code::TP_RES_OPEN_PUSH_CHANNEL:
				break;
			case code::TP_REQ_CLOSE_PUSH_CHANNEL:
				break;
			case code::TP_RES_CLOSE_PUSH_CHANNEL:
				break;
			case code::TP_REQ_PUSHDATA_TRANSFER:
				break;
			case code::TP_RES_PUSHDATA_TRANSFER:
				break;
			case code::TP_REQ_OPEN_CONNECTION:
				break;
			case code::TP_RES_OPEN_CONNECTION:
				break;
				//case code::TP_REQ_CLOSE_CONNECTION:
				//	parser_state_ = tp_req_close_connection();
				//	break;
			case code::TP_RES_CLOSE_CONNECTION:
				break;
				//	open stream channel
				//TP_REQ_OPEN_STREAM_CHANNEL = 0x9006,
				//TP_RES_OPEN_STREAM_CHANNEL = 0x1006,

				//	close stream channel
				//TP_REQ_CLOSE_STREAM_CHANNEL = 0x9007,
				//TP_RES_CLOSE_STREAM_CHANNEL = 0x1007,

				//	stream channel data transfer
				//TP_REQ_STREAMDATA_TRANSFER = 0x9008,
				//TP_RES_STREAMDATA_TRANSFER = 0x1008,

			case code::APP_RES_PROTOCOL_VERSION:
				break;
			case code::APP_RES_SOFTWARE_VERSION:
				break;
			case code::APP_RES_DEVICE_IDENTIFIER:
				break;
			case code::APP_RES_NETWORK_STATUS:
				break;
			case code::APP_RES_IP_STATISTICS:
				break;
			case code::APP_RES_DEVICE_AUTHENTIFICATION:
				break;
			case code::APP_RES_DEVICE_TIME:
				break;
			case code::APP_RES_PUSH_TARGET_NAMELIST:
				break;
			case code::APP_RES_PUSH_TARGET_ECHO:
				break;
			case code::APP_RES_TRACEROUTE:
				break;

			case code::CTRL_REQ_LOGIN_PUBLIC:
				break;
			case code::CTRL_REQ_LOGIN_SCRAMBLED:
				scrambler_ = def_sk_.key();
				break;
			case code::CTRL_RES_LOGIN_PUBLIC:
				break;
			case code::CTRL_RES_LOGIN_SCRAMBLED:
				break;

			case code::CTRL_REQ_LOGOUT:
				break;
			case code::CTRL_RES_LOGOUT:
				break;

			case code::CTRL_REQ_REGISTER_TARGET:
				break;
			case code::CTRL_RES_REGISTER_TARGET:
				break;
			case code::CTRL_REQ_DEREGISTER_TARGET:
				break;
			case code::CTRL_RES_DEREGISTER_TARGET:
				break;

				//	stream source register
				//CTRL_REQ_REGISTER_STREAM_SOURCE = 0xC00B,
				//CTRL_RES_REGISTER_STREAM_SOURCE = 0x400B,

				//	stream source deregister
				//CTRL_REQ_DEREGISTER_STREAM_SOURCE = 0xC00C,
				//CTRL_RES_DEREGISTER_STREAM_SOURCE = 0x400C,

			case code::UNKNOWN:
				break;

			default:
				break;
			}
		}

		/**
		 * Probe if parsing is completed and
		 * inform listener.
		 */
		void parser::post_processing() {
			if (state_ == state::STREAM) {

				//
				//	ToDo: transmit data
				//
			}

		}

		void parser::set_sk(scramble_key const& sk)
		{
			scrambler_ = sk.key();
		}

		void parser::reset(scramble_key const& sk)
		{
			clear();
			def_sk_ = sk.key();
		}

		void parser::clear()
		{
			state_ = state::HEADER;
			header_.reset(INVALID_SEQ);
			scrambler_.reset();
			buffer_.clear();
		}

	}	//	ipt	
}	
