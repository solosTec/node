/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/ipt/transpiler.h>
#include <smf/ipt.h>

#include <cyng/vm/generator.hpp>

namespace smf
{
	namespace ipt
	{
		scramble_key to_sk(cyng::buffer_t const& data, std::size_t offset) {
			BOOST_ASSERT(scramble_key::size() == data.size() - offset);
			scramble_key::key_type key;
			for (std::size_t idx = 0; idx < scramble_key::size(); ++idx) {
				key.at(idx) = data.at(idx + offset);
				if (idx + offset >= data.size())	break;
			}
			return key;
		}

		std::tuple<response_t, std::uint16_t, std::string> ctrl_res_login(cyng::buffer_t&& data) {

			//	response
			//	watchdog
			//	redirect
			return read<response_t, std::uint16_t, std::string>(std::move(data));
		}

		std::tuple<std::string, std::string> ctrl_req_login_public(cyng::buffer_t&& data) {

			return read<std::string, std::string>(std::move(data));

		}

		std::tuple<std::string, std::string, scramble_key> ctrl_req_login_scrambled(cyng::buffer_t&& data) {
			
			return read<std::string, std::string, scramble_key>(std::move(data));

		}

		std::string app_res_software_version(cyng::buffer_t&& data) {
			return cyng::to_string_nil(data, 0);
		}
		std::string app_res_device_identifier(cyng::buffer_t&& data) {
			return cyng::to_string_nil(data, 0);
		}
		std::string ctrl_req_deregister_target(cyng::buffer_t&& data) {
			return cyng::to_string_nil(data, 0);
		}
		std::uint16_t ctrl_res_unknown_cmd(cyng::buffer_t&& data) {
			return cyng::to_numeric_be<std::uint16_t >(data, 0);
		}

		std::tuple<std::uint8_t, std::uint32_t> ctrl_res_register_target(cyng::buffer_t&& data) {

			return read<std::uint8_t, std::uint32_t>(std::move(data));
		}


		std::tuple<std::string, std::uint16_t, std::uint8_t> ctrl_req_register_target(cyng::buffer_t&& data) {

			return read<std::string, std::uint16_t, std::uint8_t>(std::move(data));

		}

		std::tuple<std::string, std::string, std::string, std::string, std::string, std::uint16_t> tp_req_open_push_channel(cyng::buffer_t&& data) {

			return read<std::string, std::string, std::string, std::string, std::string, std::uint16_t>(std::move(data));

		}

		std::uint32_t ctrl_req_close_push_channel(cyng::buffer_t&& data) {
			return cyng::to_numeric_be<std::uint32_t >(data, 0);
		}

		std::tuple<std::uint8_t, std::uint32_t, std::uint32_t, std::uint16_t, std::uint8_t, std::uint8_t, std::uint32_t> tp_res_open_push_channel(cyng::buffer_t&& data) {

			return read <std::uint8_t, std::uint32_t, std::uint32_t, std::uint16_t, std::uint8_t, std::uint8_t, std::uint32_t>(std::move(data));
		}

		std::tuple <std::uint32_t, std::uint32_t, std::uint8_t, std::uint8_t, cyng::buffer_t> tp_req_pushdata_transfer(cyng::buffer_t&& data) {

			return read <std::uint32_t, std::uint32_t, std::uint8_t, std::uint8_t, cyng::buffer_t>(std::move(data));
		}



	}	//	ipt
}



