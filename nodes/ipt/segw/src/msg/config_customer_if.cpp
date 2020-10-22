/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "config_customer_if.h"
#include "../cache.h"
#include "../segw.h"

#include <smf/sml/protocol/generator.h>
#include <smf/sml/obis_db.h>

#include <cyng/numeric_cast.hpp>
#include <cyng/buffer_cast.h>

namespace node
{
	namespace sml
	{
		config_customer_if::config_customer_if(cyng::logging::log_ptr logger
			, res_generator& sml_gen
			, cache& cfg)
		: logger_(logger)
			, sml_gen_(sml_gen)
			, cache_(cfg)
		{}

		void config_customer_if::get_proc_params(std::string trx, cyng::buffer_t srv_id) const
		{
			//	00 80 80 01 00 FF
			auto msg = sml_gen_.empty_get_proc_param(trx, srv_id, OBIS_ROOT_CUSTOM_INTERFACE);

			//	example
			//81 02 00 07 00 FF                Not set
			//   81 02 00 07 01 FF             1 (31 )
			//   81 02 17 07 00 01             3842091200 (33 38 34 32 30 39 31 32 30 30 )
			//   81 02 17 07 01 01             16777215 (31 36 37 37 37 32 31 35 )
			//   81 02 17 07 00 02             3842091200 (33 38 34 32 30 39 31 32 30 30 )
			//   81 02 17 07 01 02             16777215 (31 36 37 37 37 32 31 35 )
			//   81 02 00 07 02 FF             False (46 61 6C 73 65 )
			//   81 02 00 07 02 01             16777215 (31 36 37 37 37 32 31 35 )
			//   81 02 00 07 02 02             0 (30 )
			//   81 02 00 07 02 03             0 (30 )
			//   81 02 00 07 02 04             1677830336 (31 36 37 37 38 33 30 33 33 36 )
			//   81 02 00 07 02 05             3338774720 (33 33 33 38 37 37 34 37 32 30 )

			//
			//	81 02 00 07 01 FF - CUSTOM_IF_IP_REF
			//	[u8] 0 == manual, 1 == DHCP
			//
			append_get_proc_response(msg, {
				OBIS_ROOT_CUSTOM_INTERFACE,
				OBIS_CUSTOM_IF_IP_REF
				}, make_value<std::uint8_t>(0));

			//
			//	81 02 17 07 00 01 - CUSTOM_IF_IP_ADDRESS_1
			//	192.168.0.200 on windows
			//
			auto const addr_1 = boost::asio::ip::make_address("192.168.0.200");			
			append_get_proc_response(msg, {
				OBIS_ROOT_CUSTOM_INTERFACE,
				OBIS_CUSTOM_IF_IP_ADDRESS_1
				}, make_value(addr_1));

			//	OBIS_CODE_DEFINITION(81, 02, 17, 07, 00, 02, CUSTOM_IF_IP_ADDRESS_2);	//	[IPv4/IPv6] second manual set IP address
			try {
				auto const addr_2 = boost::asio::ip::make_address("172.16.0.254");
				append_get_proc_response(msg, {
					OBIS_ROOT_CUSTOM_INTERFACE,
					OBIS_CUSTOM_IF_IP_ADDRESS_2
					}, make_value(addr_2));
			}
			catch (std::exception const& ex) {
				CYNG_LOG_WARNING(logger_, "get ROOT_CUSTOM_INTERFACE:CUSTOM_IF_IP_ADDRESS_2: " << ex.what());
				append_get_proc_response(msg, {
					OBIS_ROOT_CUSTOM_INTERFACE,
					OBIS_CUSTOM_IF_IP_ADDRESS_2
					}, make_value<std::uint32_t>(0));
			}

			//
			//	OBIS_CODE_DEFINITION(81, 02, 17, 07, 01, 01, CUSTOM_IF_IP_MASK_1);	//	[IPv4/IPv6] 
			//
			auto const mask = boost::asio::ip::make_address("255.255.255.0");
			append_get_proc_response(msg, {
				OBIS_ROOT_CUSTOM_INTERFACE,
				OBIS_CUSTOM_IF_IP_MASK_1
				}, make_value(mask));

			//
			//	OBIS_CODE_DEFINITION(81, 02, 00, 07, 02, FF, CUSTOM_IF_DHCP);	//	[bool] if true use a DHCP server
			//
			append_get_proc_response(msg, {
				OBIS_ROOT_CUSTOM_INTERFACE,
				OBIS_CUSTOM_IF_DHCP
				}, make_value(false));

			//	OBIS_CODE_DEFINITION(81, 02, 00, 07, 02, 01, CUSTOM_IF_DHCP_LOCAL_IP_MASK);	//	[IPv4/IPv6] 
			append_get_proc_response(msg, {
				OBIS_ROOT_CUSTOM_INTERFACE,
				OBIS_CUSTOM_IF_DHCP_LOCAL_IP_MASK
				}, make_value(boost::asio::ip::make_address("255.255.0.0")));

			//	OBIS_CODE_DEFINITION(81, 02, 00, 07, 02, 02, CUSTOM_IF_DHCP_DEFAULT_GW);	//	[IPv4/IPv6] 
			auto const gw = boost::asio::ip::make_address_v4("192.168.0.1");
			append_get_proc_response(msg, {
				OBIS_ROOT_CUSTOM_INTERFACE,
				OBIS_CUSTOM_IF_DHCP_DEFAULT_GW
				}, make_value(gw));

			//	OBIS_CODE_DEFINITION(81, 02, 00, 07, 02, 03, CUSTOM_IF_DHCP_DNS);	//	[IPv4/IPv6] 
			auto const dns = boost::asio::ip::make_address_v4("1.1.1.1");
			append_get_proc_response(msg, {
				OBIS_ROOT_CUSTOM_INTERFACE,
				OBIS_CUSTOM_IF_DHCP_DNS
				}, make_value(dns));

			//	OBIS_CODE_DEFINITION(81, 02, 00, 07, 02, 04, CUSTOM_IF_DHCP_START_ADDRESS);	//	[IPv4/IPv6] 
			auto const dhscp_start = boost::asio::ip::make_address_v4("192.168.0.2");
			append_get_proc_response(msg, {
				OBIS_ROOT_CUSTOM_INTERFACE,
				OBIS_CUSTOM_IF_DHCP_START_ADDRESS
				}, make_value(dhscp_start));

			//	OBIS_CODE_DEFINITION(81, 02, 00, 07, 02, 05, CUSTOM_IF_DHCP_END_ADDRESS);	//	[IPv4/IPv6] 
			auto const dhscp_end = boost::asio::ip::make_address_v4("192.168.0.254");
			append_get_proc_response(msg, {
				OBIS_ROOT_CUSTOM_INTERFACE,
				OBIS_CUSTOM_IF_DHCP_END_ADDRESS
				}, make_value(dhscp_end));



			//
			//	append to message queue
			//
			sml_gen_.append(std::move(msg));

		}

		void config_customer_if::get_ip_address(std::string trx, cyng::buffer_t srv_id) const
		{
			auto msg = sml_gen_.empty_get_proc_param(trx, srv_id, OBIS_ROOT_CUSTOM_PARAM);

			auto const rep = cache_.get_cfg(build_cfg_key({ sml::OBIS_ROOT_CUSTOM_PARAM }, "ep.remote"), boost::asio::ip::tcp::endpoint());

			append_get_proc_response(msg, {
				OBIS_ROOT_CUSTOM_PARAM,
				OBIS_CUSTOM_IF_IP_ADDRESS
				}, make_value(rep.address()));

			//
			//	append to message queue
			//
			sml_gen_.append(std::move(msg));

		}

		void config_customer_if::set_params(obis_path_t const& path
			, cyng::buffer_t srv_id
			, cyng::object const& obj)
		{

		}


	}	//	sml
}

