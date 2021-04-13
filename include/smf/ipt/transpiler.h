/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_IPT_BUS_TRANSPILER_H
#define SMF_IPT_BUS_TRANSPILER_H

#include <smf/ipt/header.h>
#include <smf/ipt/codes.h>
#include <smf/ipt/scramble_key.h>

#include <cyng/obj/intrinsics/buffer.h>
#include <cyng/obj/intrinsics/container.h>
#include <cyng/obj/util.hpp>
#include <cyng/obj/buffer_cast.hpp>

#include <tuple>

namespace smf
{
	namespace ipt	
	{
		[[nodiscard]]
		scramble_key to_sk(cyng::buffer_t const& data, std::size_t offset);

		namespace {

			//
			//	numerics
			//
			template <typename T>
			std::size_t read_policy(T& value, cyng::buffer_t const& data, std::size_t offset) {
				value = cyng::to_numeric_be<T>(data, offset);
				return offset + sizeof(T);
			}

			//
			//	strings
			//
			std::size_t read_policy(std::string& value, cyng::buffer_t const& data, std::size_t offset) {
				value = cyng::to_string_nil(data, offset);
				return offset + value.size() + 1;
			}

			//
			//	scramble key
			//
			std::size_t read_policy(scramble_key& value, cyng::buffer_t const& data, std::size_t offset) {
				value = to_sk(data, offset);
				return offset + scramble_key::size();
			}
			
			//
			//	buffer data
			//
			std::size_t read_policy(cyng::buffer_t& value, cyng::buffer_t const& data, std::size_t offset) {
				auto const size = cyng::to_numeric_be<std::uint32_t>(data, offset);
				std::copy_n(data.begin() + offset, size, std::back_inserter(value));
				return offset + value.size() + sizeof(size);
			}

			template <std::size_t N, typename ...Args>
			struct read_impl
			{
				static std::size_t convert(cyng::buffer_t const& data, std::tuple<Args...>& tpl, std::size_t offset) {

					if constexpr (N > 0) {
						offset = read_impl<N - 1, Args...>::convert(data, tpl, offset);
					}
					using T = typename std::tuple_element<N, std::tuple<Args...>>::type;
					return read_policy(std::get<N>(tpl), data, offset);
				}
			};

			template <typename ...Args>
			struct read_impl<0, Args...>
			{
				static std::size_t convert(cyng::buffer_t const& data, std::tuple<Args...>& tpl, std::size_t offset) {

					using T = typename std::tuple_element<0, std::tuple<Args...>>::type;
					return read_policy(std::get<0>(tpl), data, offset);
				}
			};

		}

		template <typename ...Args>
		auto read(cyng::buffer_t&& data) -> std::tuple<Args...> {
			std::tuple<Args...> r;

			constexpr std::size_t upper_bound = sizeof...(Args) - 1;

			auto const size = read_impl<upper_bound, Args...>::convert(data, r, 0);
			BOOST_ASSERT(size == data.size());
			return r;
		}

		std::tuple<response_t, std::uint16_t, std::string> ctrl_res_login(cyng::buffer_t&& data);
		std::tuple<std::string, std::string> ctrl_req_login_public(cyng::buffer_t&& data);
		std::tuple<std::string, std::string, scramble_key> ctrl_req_login_scrambled(cyng::buffer_t&& data);

		std::string app_res_software_version(cyng::buffer_t&& data);
		std::string app_res_device_identifier(cyng::buffer_t&& data);

		/**
		 * @return target name, packet size, windows size
		 */
		std::tuple<std::string, std::uint16_t, std::uint8_t> ctrl_req_register_target(cyng::buffer_t&& data);
		std::string ctrl_req_deregister_target(cyng::buffer_t&& data);

		/**
		 * @return response, channel
		 */
		std::tuple<std::uint8_t, std::uint32_t> ctrl_res_register_target(cyng::buffer_t&& data);

		/**
		 * @return target name, account, number, version, device id, timout
		 */
		std::tuple<std::string, std::string, std::string, std::string, std::string, std::uint16_t> tp_req_open_push_channel(cyng::buffer_t&& data);
		std::uint32_t ctrl_req_close_push_channel(cyng::buffer_t&& data);

		/**
		 * @return response code, channel, source, packet size, window size, status, target count
		 */
		std::tuple<std::uint8_t, std::uint32_t, std::uint32_t, std::uint16_t, std::uint8_t, std::uint8_t, std::uint32_t> tp_res_open_push_channel(cyng::buffer_t&& data);

		/**
		 * @return channel, source, status, block and data
		 */
		std::tuple <std::uint32_t, std::uint32_t, std::uint8_t, std::uint8_t, cyng::buffer_t> tp_req_pushdata_transfer(cyng::buffer_t&& data);

		/**
		 * @return msisdn
		 */
		std::string tp_req_open_connection(cyng::buffer_t&& data);

		/**
		 * @return response code
		 */
		response_t tp_res_open_connection(cyng::buffer_t&& data);

		/**
		 * @return unknown command as u16 value
		 */
		std::uint16_t ctrl_res_unknown_cmd(cyng::buffer_t&& data);


	}	//	ipt
}


#endif
