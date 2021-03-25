/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/ipt/transpiler.h>
#include <smf/ipt.h>

#include <cyng/obj/util.hpp>
#include <cyng/obj/buffer_cast.hpp>
#include <cyng/vm/generator.hpp>

namespace smf
{
	namespace ipt
	{
		std::tuple<response_t, std::uint16_t, std::string> ctrl_res_login(cyng::buffer_t&& data) {

			//	response
			//	watchdog
			//	redirect
			return {
				cyng::to_numeric_be<response_t>(data),
				cyng::to_numeric_be<std::uint16_t>(data, sizeof(response_t)),
				cyng::to_string_nil(data, sizeof(response_t) + sizeof(std::uint16_t))
			};
		}

		std::tuple<std::string, std::string> ctrl_req_login_public(cyng::buffer_t&& data) {
			return {
				std::string(data.data()),
				""
			};

		}

		std::tuple<std::string, std::string, scramble_key> ctrl_req_login_scrambled(cyng::buffer_t&& data) {
			return {
				std::string(data.data()),
				"",
				scramble_key::null_scramble_key_
			};


		}

		cyng::deque_t gen_instructions(header const& h, cyng::buffer_t&& data) {

			switch (static_cast<code>(h.command_)) {
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
				//scrambler_ = def_sk_.key();
				break;
			case code::CTRL_RES_LOGIN_PUBLIC:
				//return ctrl_res_login_public(std::move(data));
				break;
			case code::CTRL_RES_LOGIN_SCRAMBLED:
				//return ctrl_res_login_scrambled(std::move(data));
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

			return cyng::make_deque();
		}

	}	//	ipt
}



