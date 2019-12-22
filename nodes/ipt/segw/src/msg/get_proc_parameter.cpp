/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "get_proc_parameter.h"
#include "config_ipt.h"
#include "config_sensor_params.h"
#include "../segw.h"
#include "../cache.h"
#include "../storage.h"

#include <smf/sml/protocol/generator.h>
#include <smf/sml/obis_io.h>
#include <smf/sml/obis_db.h>
#include <smf/shared/db_cfg.h>

#include <cyng/io/serializer.h>
#include <cyng/sys/ntp.h>
#include <cyng/sys/ip.h>
#include <cyng/sys/dns.h>
#include <cyng/sys/memory.h>
#include <cyng/numeric_cast.hpp>
#include <cyng/intrinsics/buffer.h>

#if BOOST_OS_WINDOWS
#include <cyng/scm/mgr.h>
#endif


namespace node
{
	namespace sml
	{
		get_proc_parameter::get_proc_parameter(cyng::logging::log_ptr logger
			, res_generator& sml_gen
			, cache& cfg
			, storage& db
			, node::ipt::config_ipt& config_ipt
			, config_sensor_params& config_sensor_params)
		: logger_(logger)
			, sml_gen_(sml_gen)
			, cache_(cfg)
			, storage_(db)
			, config_ipt_(config_ipt)
			, config_sensor_params_(config_sensor_params)
		{}

		void get_proc_parameter::generate_response(obis code
			, std::string trx
			, cyng::buffer_t srv_id
			, std::string user
			, std::string pwd)
		{
			switch (code.to_uint64()) {
			case CODE_CLASS_OP_LOG_STATUS_WORD:	//	OBIS_CLASS_OP_LOG_STATUS_WORD
				class_op_log_status_word(trx, srv_id);
				break;
			case CODE_ROOT_DEVICE_IDENT:	//	0x8181C78201FF
				code_root_device_ident(trx, srv_id);
				break;
			case CODE_ROOT_DEVICE_TIME:	//	0x8181C78810FF
				code_root_device_time(trx, srv_id);
				break;
			case CODE_ROOT_NTP:	//	0x8181C78801FF
				code_root_ntp(trx, srv_id);
				break;
			case CODE_ROOT_ACCESS_RIGHTS:	//	0x81818160FFFF
				code_root_access_rights(trx, srv_id);
				break;
			case CODE_ROOT_CUSTOM_INTERFACE:	//	0x8102000700FF
				code_root_custom_interface(trx, srv_id);
				break;
			case CODE_ROOT_CUSTOM_PARAM:	//	0x8102000710FF
				code_root_custom_param(trx, srv_id);
				break;
			case CODE_ROOT_WAN:	//	0x8104000610FF
				code_root_wan(trx, srv_id);
				break;
			case CODE_ROOT_GSM:	//	0x8104020700FF
				code_root_gsm(trx, srv_id);
				break;
			case CODE_ROOT_GPRS_PARAM:	//	0x81040D0700FF
				code_root_gprs_param(trx, srv_id);
				break;
			case CODE_ROOT_IPT_STATE:	//	0x81490D0600FF
				code_root_ipt_state(trx, srv_id);
				break;
			case CODE_ROOT_IPT_PARAM:	//	0x81490D0700FF
				code_root_ipt_param(trx, srv_id);
				break;
			case CODE_ROOT_W_MBUS_STATUS:	//	0x81060F0600FF
				code_root_w_mbus_status(trx, srv_id);
				break;
			case CODE_IF_wMBUS:	//	0x8106190700FF
				code_if_wmbus(trx, srv_id);
				break;
			case CODE_ROOT_LAN_DSL:	//	0x81480D0600FF
				code_root_lan_dsl(trx, srv_id);
				break;
			case CODE_IF_LAN_DSL:	//	0x8148170700FF
				code_if_lan_dsl(trx, srv_id);
				break;
			case CODE_ROOT_MEMORY_USAGE:	//	0x0080800010FF
				code_root_memory_usage(trx, srv_id);
				break;
			case CODE_ROOT_ACTIVE_DEVICES:	//	0x81811106FFFF
				code_root_active_devices(trx, srv_id);
				break;
			case CODE_ROOT_VISIBLE_DEVICES:	//	0x81811006FFFF
				code_root_visible_devices(trx, srv_id);
				break;
			case CODE_ROOT_DEVICE_INFO:	//	0x81811206FFFF
				code_root_device_info(trx, srv_id);
				break;
			case CODE_ROOT_SENSOR_PARAMS:	//	0x8181C78600FF
				config_sensor_params_.get_proc_params(trx, srv_id);
				break;
			case CODE_ROOT_DATA_COLLECTOR:	//	 0x8181C78620FF (Datenspiegel)
				code_root_data_collector(trx, srv_id);
				break;
			case CODE_IF_1107:	//	0x8181C79300FF
				code_if_1107(trx, srv_id);
				break;
			case CODE_STORAGE_TIME_SHIFT:	//	0x0080800000FF
				storage_time_shift(trx, srv_id);
				break;
			case CODE_PUSH_OPERATIONS:	//	0x8181C78A01FF
				push_operations(trx, srv_id);
				break;
			case CODE_LIST_SERVICES:	//	0x990000000004
				list_services(trx, srv_id);
				break;
			case CODE_ACTUATORS:	//	0x0080801100FF
				actuators(trx, srv_id);
				break;
			case CODE_CODE_IF_EDL:	//	0x81050D0700FF - M-Bus EDL (RJ10)
				code_if_edl(trx, srv_id);
				break;
			case CODE_CLASS_MBUS:	//	0x00B000020000
				class_mbus(trx, srv_id);
				break;
			default:
				CYNG_LOG_ERROR(logger_, "sml.get.proc.parameter.request - unknown OBIS code "
					<< to_string(code)
					<< " / "
					<< cyng::io::to_hex(code.to_buffer()));
				break;
			}
		}

		void get_proc_parameter::class_op_log_status_word(std::string trx, cyng::buffer_t srv_id)
		{
			auto const word = cache_.get_status_word();
			CYNG_LOG_DEBUG(logger_, "status word: " << word);

			auto msg = sml_gen_.empty_get_proc_param_response(trx, srv_id, OBIS_CLASS_OP_LOG_STATUS_WORD);

			append_get_proc_response(msg, {
				OBIS_CLASS_OP_LOG_STATUS_WORD,
				}, make_value(static_cast<std::uint32_t>(word)));

			//
			//	append to message queue
			//
			sml_gen_.append(std::move(msg));
		}

		void get_proc_parameter::code_root_device_ident(std::string trx, cyng::buffer_t srv_id)
		{
			//
			//	get values from "_Config" table
			//
			auto const manufacturer = cache_.get_cfg(OBIS_DATA_MANUFACTURER.to_str(), std::string(""));
			auto const serial = cache_.get_cfg<std::uint32_t>(OBIS_SERIAL_NR.to_str(), 10000000u);

			sml_gen_.get_proc_parameter_device_id(trx
				, srv_id	//	server id
				, manufacturer
				, cache_.get_srv_id()
				, "VSES-1KW-221-1F0"
				, serial);
		}

		void get_proc_parameter::code_root_device_time(std::string trx, cyng::buffer_t srv_id)
		{
			auto const now = std::chrono::system_clock::now();
			std::int32_t const tz = 60;
			bool const sync_active = true;

			sml_gen_.get_proc_device_time(trx
				, srv_id	//	server id
				, now
				, tz
				, sync_active);
		}

		void get_proc_parameter::code_root_ntp(std::string trx, cyng::buffer_t srv_id)
		{
			//
			//	get all configured NTP servers
			//
			auto const ntp_servers = cyng::sys::get_ntp_servers();

			//const std::string ntp_primary = "pbtime1.pbt.de", ntp_secondary = "pbtime2.pbt.de", ntp_tertiary = "pbtime3.pbt.de";
			const std::uint16_t ntp_port = 123;	// 123;
			const bool ntp_active = true;
			const std::uint16_t ntp_tz = 3600;

			auto msg = sml_gen_.empty_get_proc_param_response(trx, srv_id, OBIS_ROOT_NTP);

			append_get_proc_response(msg, { OBIS_ROOT_NTP, OBIS_CODE_NTP_PORT }, make_value(ntp_port));
			append_get_proc_response(msg, { OBIS_ROOT_NTP, OBIS_CODE_NTP_ACTIVE }, make_value(ntp_active));
			append_get_proc_response(msg, { OBIS_ROOT_NTP, OBIS_CODE_NTP_TZ }, make_value(ntp_tz));

			for (std::uint8_t nr = 0; nr < ntp_servers.size(); ++nr) {
				auto srv = ntp_servers.at(nr);
				append_get_proc_response(msg, { OBIS_ROOT_NTP, OBIS_CODE_NTP_SERVER, make_obis(0x81, 0x81, 0xC7, 0x88, 0x02, nr + 1) }, make_value(srv));
			}

			sml_gen_.append(std::move(msg));
		}

		void get_proc_parameter::code_root_access_rights(std::string trx, cyng::buffer_t srv_id)
		{
			//
			//	ToDo: implement
			//
			CYNG_LOG_WARNING(logger_, "sml.get.proc.parameter.request - OBIS_CODE_ROOT_ACCESS_RIGHTS not implemented yet");

			sml_gen_.empty(trx, srv_id, OBIS_ROOT_ACCESS_RIGHTS);
		}

		void get_proc_parameter::code_root_custom_interface(std::string trx, cyng::buffer_t srv_id)
		{
			CYNG_LOG_WARNING(logger_, "sml.get.proc.parameter.request - OBIS_CODE_ROOT_CUSTOM_INTERFACE not implemented yet");
			sml_gen_.empty(trx, srv_id, OBIS_ROOT_CUSTOM_INTERFACE);
		}

		void get_proc_parameter::code_root_custom_param(std::string trx, cyng::buffer_t srv_id)
		{
			CYNG_LOG_WARNING(logger_, "sml.get.proc.parameter.request - OBIS_CODE_ROOT_CUSTOM_PARAM not implemented yet");
			sml_gen_.empty(trx, srv_id, OBIS_ROOT_CUSTOM_PARAM);
		}

		void get_proc_parameter::code_root_wan(std::string trx, cyng::buffer_t srv_id)
		{
			CYNG_LOG_WARNING(logger_, "sml.get.proc.parameter.request - OBIS_OBIS_ROOT_WAN not implemented yet");
			sml_gen_.empty(trx, srv_id, OBIS_ROOT_WAN);	
		}

		void get_proc_parameter::code_root_gsm(std::string trx, cyng::buffer_t srv_id)
		{
			CYNG_LOG_WARNING(logger_, "sml.get.proc.parameter.request - OBIS_ROOT_GSM not implemented yet");
			sml_gen_.empty(trx, srv_id, OBIS_ROOT_GSM);
		}

		void get_proc_parameter::code_root_gprs_param(std::string trx, cyng::buffer_t srv_id)
		{
			CYNG_LOG_WARNING(logger_, "sml.get.proc.parameter.request - OBIS_ROOT_GPRS_PARAM not implemented yet");
			sml_gen_.empty(trx, srv_id, OBIS_ROOT_GPRS_PARAM);			
		}

		void get_proc_parameter::code_root_ipt_state(std::string trx, cyng::buffer_t srv_id)
		{
			//
			//	get TCP/IP endpoint from _Config table
			//
			auto rep = cache_.get_cfg<boost::asio::ip::tcp::endpoint>("remote.ep", boost::asio::ip::tcp::endpoint());
			auto lep = cache_.get_cfg<boost::asio::ip::tcp::endpoint>("local.ep", boost::asio::ip::tcp::endpoint());

			CYNG_LOG_TRACE(logger_, "remote endpoint " << rep);
			CYNG_LOG_TRACE(logger_, "local endpoint " << lep);

			//
			//	generate response
			//
			sml_gen_.get_proc_parameter_ipt_state(trx
				, srv_id
				, rep
				, lep);
		}

		void get_proc_parameter::code_root_ipt_param(std::string trx, cyng::buffer_t srv_id)
		{
			//
			//	IP-T access parameters
			//
			config_ipt_.get_proc_params(trx, srv_id);
		}

		void get_proc_parameter::code_root_w_mbus_status(std::string trx, cyng::buffer_t srv_id)
		{
			//
			//	wireless M-Bus adapter
			//
			sml_gen_.get_proc_w_mbus_status(trx
				, srv_id
				//, "RC1180-MBUS3"	// manufacturer of w-mbus adapter
				, "solos::Tec"	// manufacturer of w-mbus adapter
				, cyng::make_buffer({ 0xA8, 0x15, 0x34, 0x83, 0x40, 0x04, 0x01, 0x31 })	//	adapter id (EN 13757-3/4)
				, NODE_VERSION	//	firmware version of adapter
				, "2.00"	//	hardware version of adapter
			);
		}

		void get_proc_parameter::code_if_wmbus(std::string trx, cyng::buffer_t srv_id)
		{
			//
			//	wireless M-Bus interface
			//
			
			cache_.read_table("_Cfg", [&](cyng::store::table const* tbl_cfg) {
				sml_gen_.get_proc_w_mbus_if(trx
					, srv_id
					, tbl_cfg->lookup(cfg_key({ OBIS_IF_wMBUS, OBIS_W_MBUS_PROTOCOL }), "val")	//	protocol
					, tbl_cfg->lookup(cfg_key({ OBIS_IF_wMBUS, OBIS_W_MBUS_MODE_S }), "val")	//	duration in seconds
					, tbl_cfg->lookup(cfg_key({ OBIS_IF_wMBUS, OBIS_W_MBUS_MODE_T }), "val")	//	duration in seconds
					, tbl_cfg->lookup(cfg_key({ OBIS_IF_wMBUS, OBIS_W_MBUS_REBOOT }), "val")	//	duration in seconds
					, tbl_cfg->lookup(cfg_key({ OBIS_IF_wMBUS, OBIS_W_MBUS_POWER }), "val")	//	transmision power (transmission_power)
					, tbl_cfg->lookup(cfg_key({ OBIS_IF_wMBUS, OBIS_W_MBUS_INSTALL_MODE }), "val")
				);
			});
		}

		void get_proc_parameter::code_root_lan_dsl(std::string trx, cyng::buffer_t srv_id)
		{
			//CYNG_LOG_WARNING(logger_, "sml.get.proc.parameter.request - OBIS_ROOT_LAN_DSL not implemented yet");

			//	81 48 0D 06 00 FF
			auto msg = sml_gen_.empty_get_proc_param_response(trx, srv_id, OBIS_ROOT_LAN_DSL);

			//	81 48 17 07 00 00
			auto rep = cache_.get_cfg<boost::asio::ip::tcp::endpoint>("remote.ep", boost::asio::ip::tcp::endpoint());
			auto lep = cache_.get_cfg<boost::asio::ip::tcp::endpoint>("local.ep", boost::asio::ip::tcp::endpoint());

			CYNG_LOG_TRACE(logger_, "remote endpoint " << rep.address().to_string());
			CYNG_LOG_TRACE(logger_, "local endpoint " << lep.address().to_string());

			boost::asio::ip::address wan = cyng::sys::get_WAN_address(rep.address().to_string());

			if (wan.is_v4()) {
				append_get_proc_response(msg, {
					OBIS_ROOT_LAN_DSL,
					OBIS_CODE_IF_LAN_ADDRESS
					}, make_value(cyng::swap_num(wan.to_v4().to_uint())));
			}
			else {
				auto v6 = wan.to_v6().to_bytes();
				append_get_proc_response(msg, {
					OBIS_ROOT_LAN_DSL,
					OBIS_CODE_IF_LAN_ADDRESS
					}, make_value(cyng::make_buffer(v6)));
			}

			//	81 48 17 07 01 00
			append_get_proc_response(msg, {
				OBIS_ROOT_LAN_DSL,
				OBIS_CODE_IF_LAN_SUBNET_MASK
				}, make_value(0xFFFFFF));	//	255.255.255.0 (fix!)

			//	81 48 17 07 02 00
			append_get_proc_response(msg, {
				OBIS_ROOT_LAN_DSL,
				OBIS_CODE_IF_LAN_GATEWAY
				}, make_value(0x0101A8C0));	//	192.168.1.1 (fix!)

			//
			//	DNS server
			//
			auto const dns = cyng::sys::get_dns_servers();
			switch (dns.size()) {
			case 0:
				break;
			case 1:
				append_get_proc_response(msg, {
					OBIS_ROOT_LAN_DSL,
					OBIS_CODE_IF_LAN_DNS_PRIMARY
					}, make_value(cyng::swap_num(dns.at(0).to_v4().to_uint())));
				append_get_proc_response(msg, {
					OBIS_ROOT_LAN_DSL,
					OBIS_CODE_IF_LAN_DNS_SECONDARY
					}, make_value(0u));
				append_get_proc_response(msg, {
					OBIS_ROOT_LAN_DSL,
					OBIS_CODE_IF_LAN_DNS_TERTIARY
					}, make_value(0u));
				break;
			case 2:
				append_get_proc_response(msg, {
					OBIS_ROOT_LAN_DSL,
					OBIS_CODE_IF_LAN_DNS_PRIMARY
					}, make_value(cyng::swap_num(dns.at(0).to_v4().to_uint())));
				append_get_proc_response(msg, {
					OBIS_ROOT_LAN_DSL,
					OBIS_CODE_IF_LAN_DNS_SECONDARY
					}, make_value(cyng::swap_num(dns.at(1).to_v4().to_uint())));
				append_get_proc_response(msg, {
					OBIS_ROOT_LAN_DSL,
					OBIS_CODE_IF_LAN_DNS_TERTIARY
					}, make_value(0u));
				break;
			default:
				append_get_proc_response(msg, {
					OBIS_ROOT_LAN_DSL,
					OBIS_CODE_IF_LAN_DNS_PRIMARY
					}, make_value(cyng::swap_num(dns.at(0).to_v4().to_uint())));
				append_get_proc_response(msg, {
					OBIS_ROOT_LAN_DSL,
					OBIS_CODE_IF_LAN_DNS_SECONDARY
					}, make_value(cyng::swap_num(dns.at(1).to_v4().to_uint())));
				append_get_proc_response(msg, {
					OBIS_ROOT_LAN_DSL,
					OBIS_CODE_IF_LAN_DNS_TERTIARY
					}, make_value(cyng::swap_num(dns.at(1).to_v4().to_uint())));
			}

			//
			//	append to message queue
			//
			sml_gen_.append(std::move(msg));
		}

		void get_proc_parameter::code_if_lan_dsl(std::string trx, cyng::buffer_t srv_id)
		{
			//	81 48 17 07 00 FF
			auto msg = sml_gen_.empty_get_proc_param_response(trx, srv_id, OBIS_IF_LAN_DSL);

			//
			//	81 48 00 00 00 00 - computer name 
			//
			append_get_proc_response(msg, {
				OBIS_IF_LAN_DSL,
				OBIS_COMPUTER_NAME
				}, make_value(boost::asio::ip::host_name()));

			//	81 48 00 32 02 01 - LAN_DHCP_ENABLED
			append_get_proc_response(msg, {
				OBIS_IF_LAN_DSL,
				OBIS_LAN_DHCP_ENABLED
				}, make_value(true));

			//
			//	append to message queue
			//
			sml_gen_.append(std::move(msg));
		}

		void get_proc_parameter::code_root_memory_usage(std::string trx, cyng::buffer_t srv_id)
		{
			CYNG_LOG_INFO(logger_, "get_used_physical_memory " << cyng::sys::get_used_physical_memory());
			CYNG_LOG_INFO(logger_, "get_total_physical_memory " << cyng::sys::get_total_physical_memory());
			CYNG_LOG_INFO(logger_, "get_used_virtual_memory " << cyng::sys::get_used_virtual_memory());
			CYNG_LOG_INFO(logger_, "get_total_virtual_memory " << cyng::sys::get_total_virtual_memory());

			//	OBIS_ROOT_MEMORY_USAGE
			const std::uint8_t mirror = cyng::sys::get_used_virtual_memory_in_percent()
				, tmp = cyng::sys::get_used_physical_memory_in_percent();

			sml_gen_.get_proc_mem_usage(trx
				, srv_id	//	server id
				, mirror
				, tmp);

		}

		void get_proc_parameter::code_root_active_devices(std::string trx, cyng::buffer_t srv_id)
		{
			//	81 81 11 06 FF FF
			auto msg = sml_gen_.empty_get_proc_param_response(trx, srv_id, OBIS_ROOT_ACTIVE_DEVICES);

			//append_get_proc_response(msg, { OBIS_CODE_ROOT_NTP, OBIS_CODE_NTP_PORT }, make_value(ntp_port));

			std::uint8_t 
				quant{ 1 }, //	outer loop counter (0x01 - 0xFA)
				store{ 1 }	//	inner loop counter (0x01 - 0xFE)
			;

			auto const now = std::chrono::system_clock::now();

			//
			//	list all active M-Bus devices
			//

			cyng::buffer_t tmp;
			cache_.loop("_DeviceMBUS", [&](cyng::table::record const& rec)->bool {
				//
				//	set server ID
				//
				append_get_proc_response(msg, {
					OBIS_ROOT_ACTIVE_DEVICES,
					make_obis(0x81, 0x81, 0x11, 0x06, quant, 0xFF),
					make_obis(0x81, 0x81, 0x11, 0x06, quant, store),
					OBIS_SERVER_ID
					}, make_value(cyng::value_cast(rec["serverID"], tmp)));

				//
				//	device class
				//
				append_get_proc_response(msg, {
					OBIS_ROOT_ACTIVE_DEVICES,
					make_obis(0x81, 0x81, 0x11, 0x06, quant, 0xFF),
					make_obis(0x81, 0x81, 0x11, 0x06, quant, store),
					OBIS_DEVICE_CLASS
					}, make_value(cyng::value_cast<std::string>(rec["class"], "")));

				//
				//	timestamp (last seen)
				//
				append_get_proc_response(msg, {
					OBIS_ROOT_ACTIVE_DEVICES,
					make_obis(0x81, 0x81, 0x11, 0x06, quant, 0xFF),
					make_obis(0x81, 0x81, 0x11, 0x06, quant, store),
					OBIS_CURRENT_UTC
					}, make_value(cyng::value_cast(rec["lastSeen"], now)));

				//
				//	update counter
				//
				++store;
				if (store == 0xFE) {
					++quant;
					store = 0x01;
				}

				return true;	//	continue
			});

			CYNG_LOG_TRACE(logger_, "code_root_active_devices: "
				<< cyng::io::to_str(msg));

			//
			//	append to message queue
			//
			sml_gen_.append(std::move(msg));
		}

		void get_proc_parameter::code_root_visible_devices(std::string trx, cyng::buffer_t srv_id)
		{
			//	81 81 10 06 FF FF
			auto msg = sml_gen_.empty_get_proc_param_response(trx, srv_id, OBIS_ROOT_VISIBLE_DEVICES);

			std::uint8_t
				quant{ 1 }, //	outer loop counter (0x01 - 0xFA)
				store{ 1 }	//	inner loop counter (0x01 - 0xFE)
			;

			auto const now = std::chrono::system_clock::now();

			//
			//	list all active M-Bus devices
			//
			cyng::buffer_t tmp;
			cache_.loop("_DeviceMBUS", [&](cyng::table::record const& rec) {

				//
				//	set server ID
				//
				append_get_proc_response(msg, {
					OBIS_ROOT_VISIBLE_DEVICES,
					make_obis(0x81, 0x81, 0x10, 0x06, quant, 0xFF),
					make_obis(0x81, 0x81, 0x10, 0x06, quant, store),
					OBIS_SERVER_ID
					}, make_value(cyng::value_cast(rec["serverID"], tmp)));

				//
				//	device class
				//
				append_get_proc_response(msg, {
					OBIS_ROOT_VISIBLE_DEVICES,
					make_obis(0x81, 0x81, 0x10, 0x06, quant, 0xFF),
					make_obis(0x81, 0x81, 0x10, 0x06, quant, store),
					OBIS_DEVICE_CLASS
					}, make_value(cyng::value_cast<std::string>(rec["class"], "")));

				//
				//	timestamp (last seen)
				//
				append_get_proc_response(msg, {
					OBIS_ROOT_VISIBLE_DEVICES,
					make_obis(0x81, 0x81, 0x10, 0x06, quant, 0xFF),
					make_obis(0x81, 0x81, 0x10, 0x06, quant, store),
					OBIS_CURRENT_UTC
					}, make_value(cyng::value_cast(rec["lastSeen"], now)));

				//
				//	update counter
				//
				++store;
				if (store == 0xFE) {
					++quant;
					store = 0x01;
				}

				return true;	//	continue
			});

			//
			//	append to message queue
			//
			sml_gen_.append(std::move(msg));

		}

		void get_proc_parameter::code_root_device_info(std::string trx, cyng::buffer_t srv_id)
		{
			//	see ZDUE-MUC_Anwenderhandbuch_V2_5.pdf (Chapter 21.19)
			CYNG_LOG_WARNING(logger_, "sml.get.proc.parameter.request - OBIS_ROOT_DEVICE_INFO not implemented yet");
			sml_gen_.empty(trx, srv_id, OBIS_ROOT_DEVICE_INFO);
		}

		void get_proc_parameter::code_root_data_collector(std::string trx, cyng::buffer_t srv_id)
		{
			//
			//	Comes up when clicked "Datenspiegel"
			//
			//	81 81 C7 86 20 FF - table "data.collector"
			//
			auto msg = sml_gen_.empty_get_proc_param_response(trx, srv_id, OBIS_ROOT_DATA_COLLECTOR);

			cache_.read_tables("data.collector", "readout", [&](cyng::store::table const* tbl_dc, cyng::store::table const* tbl_ro) {

				std::uint8_t nr{ 1 };	//	data collector index
				tbl_dc->loop([&](cyng::table::record const& rec) {

					//
					//	81 81 C7 86 21 FF - active
					//
					append_get_proc_response(msg, {
						OBIS_ROOT_DATA_COLLECTOR,
						make_obis(0x81, 0x81, 0xC7, 0x86, 0x20, nr),
						OBIS_DATA_COLLECTOR_ACTIVE
						}, make_value(rec["active"]));

					//
					//	81 81 C7 86 22 FF - Einträge
					//
					append_get_proc_response(msg, {
						OBIS_ROOT_DATA_COLLECTOR,
						make_obis(0x81, 0x81, 0xC7, 0x86, 0x20, nr),
						OBIS_DATA_COLLECTOR_SIZE
						}, make_value(rec["maxSize"]));

					//
					//	81 81 C7 87 81 FF  - Registerperiode (seconds)
					//
					append_get_proc_response(msg, {
						OBIS_ROOT_DATA_COLLECTOR,
						make_obis(0x81, 0x81, 0xC7, 0x86, 0x20, nr),
						OBIS_DATA_COLLECTOR_PERIOD
						}, make_value(rec["period"]));

					//
					//	81 81 C7 8A 83 FF - profile
					//
					append_get_proc_response(msg, {
						OBIS_ROOT_DATA_COLLECTOR,
						make_obis(0x81, 0x81, 0xC7, 0x86, 0x20, nr),
						OBIS_PROFILE
						}, make_value(rec["period"]));

					//
					//	collect all available OBIS codes for this meter from "readout" table
					//
					std::uint8_t idx{ 1 };	//	OBIS counter
					tbl_ro->loop([&](cyng::table::record const& rec) {

						//
						//	extract server/meter ID from record
						//
						cyng::buffer_t srv;
						srv = cyng::value_cast(rec["serverID"], srv);

						//
						//	ToDo: use only matching records
						//
						if (srv == srv_id) {
						}

						obis const code(cyng::value_cast(rec["OBIS"], srv));

						append_get_proc_response(msg, {
							OBIS_ROOT_DATA_COLLECTOR,
							make_obis(0x81, 0x81, 0xC7, 0x86, 0x20, nr),
							OBIS_PROFILE,
							make_obis(0x81, 0x81, 0xC7, 0x8A, 0x23, idx)
							}, make_value(code));

						//
						//	update OBIS counter
						//
						++idx;

						return true;	//	continue
					});

					//
					//	update data collector index
					//
					++nr;
					return true;	//	continue
				});

			});

			//
			//	append to message queue
			//
			sml_gen_.append(std::move(msg));

		}

		void get_proc_parameter::code_if_1107(std::string trx, cyng::buffer_t srv_id)
		{
			auto msg = sml_gen_.empty_get_proc_param_response(trx, srv_id, OBIS_IF_1107);

			//
			//	Configuration of IEC interface (wired)
			//	deliver IEC 62056-21 (aka interface 1107) configuration and all defined meters
			//
			cache_.read_tables("_Cfg", "iec62056-21-devices", [&](cyng::store::table const* tbl_cfg, cyng::store::table const* tbl_dev) {

				CYNG_LOG_INFO(logger_, tbl_dev->size() << " IEC devices");

				//
				//	81 81 C7 93 01 FF - if true 1107 interface active otherwise SML interface active
				//
				;
				append_get_proc_response(msg, {
					OBIS_IF_1107,
					OBIS_IF_1107_ACTIVE
					}, make_value(get_config_value(tbl_cfg, OBIS_IF_1107_ACTIVE.to_str(), false)));

				//
				//	81 81 C7 93 02 FF - Loop timeout in seconds
				//
				append_get_proc_response(msg, {
					OBIS_IF_1107,
					OBIS_IF_1107_LOOP_TIME
					}, make_value(get_config_obj(tbl_cfg, OBIS_IF_1107_LOOP_TIME.to_str())));

				//
				//	81 81 C7 93 03 FF - Retry count
				//
				append_get_proc_response(msg, {
					OBIS_IF_1107,
					OBIS_IF_1107_RETRIES
					}, make_value(get_config_obj(tbl_cfg, OBIS_IF_1107_RETRIES.to_str())));

				//
				//	81 81 C7 93 04 FF - Minimal answer timeout(300)
				//
				append_get_proc_response(msg, {
					OBIS_IF_1107,
					OBIS_IF_1107_MIN_TIMEOUT
					}, make_value(get_config_obj(tbl_cfg, OBIS_IF_1107_MIN_TIMEOUT.to_str())));

				//
				//	81 81 C7 93 05 FF - Maximal answer timeout(5000)
				//
				append_get_proc_response(msg, {
					OBIS_IF_1107,
					OBIS_IF_1107_MAX_TIMEOUT
					}, make_value(get_config_obj(tbl_cfg, OBIS_IF_1107_MAX_TIMEOUT.to_str())));

				//
				//	81 81 C7 93 06 FF - Maximum data bytes(10240)
				//
				append_get_proc_response(msg, {
					OBIS_IF_1107,
					OBIS_IF_1107_MAX_DATA_RATE
					}, make_value(get_config_obj(tbl_cfg, OBIS_IF_1107_MAX_DATA_RATE.to_str())));

				//
				//	81 81 C7 93 08 FF - Protocol mode(A ... D)
				//
				append_get_proc_response(msg, {
					OBIS_IF_1107,
					OBIS_IF_1107_PROTOCOL_MODE
					}, make_value(get_config_obj(tbl_cfg, OBIS_IF_1107_PROTOCOL_MODE.to_str())));

				//
				//	81 81 C7 93 09 FF - Liste der abzufragenden 1107 Zähler
				//
				std::uint8_t nr{ 1 };	//	device number
				tbl_dev->loop([&](cyng::table::record const& rec)->bool {

					//
					//	81 81 C7 93 0A FF - meter id
					//
					append_get_proc_response(msg, {
						OBIS_IF_1107,
						OBIS_IF_1107_METER_LIST,
						make_obis(0x81, 0x81, 0xC7, 0x93, 0x09, nr),
						OBIS_IF_1107_METER_ID
						}, make_value(rec["meterID"]));

					//
					//	81 81 C7 93 0B FF - baudrate
					//
					append_get_proc_response(msg, {
						OBIS_IF_1107,
						OBIS_IF_1107_METER_LIST,
						make_obis(0x81, 0x81, 0xC7, 0x93, 0x09, nr),
						OBIS_IF_1107_BAUDRATE
						}, make_value(rec["baudrate"]));

					//
					//	81 81 C7 93 0C FF - address
					//
					append_get_proc_response(msg, {
						OBIS_IF_1107,
						OBIS_IF_1107_METER_LIST,
						make_obis(0x81, 0x81, 0xC7, 0x93, 0x09, nr),
						OBIS_IF_1107_ADDRESS
						}, make_value(rec["address"]));

					//
					//	81 81 C7 93 0D FF - P1
					//
					append_get_proc_response(msg, {
						OBIS_IF_1107,
						OBIS_IF_1107_METER_LIST,
						make_obis(0x81, 0x81, 0xC7, 0x93, 0x09, nr),
						OBIS_IF_1107_P1
						}, make_value(rec["p1"]));

					//
					//	81 81 C7 93 0E FF - W5
					//
					append_get_proc_response(msg, {
						OBIS_IF_1107,
						OBIS_IF_1107_METER_LIST,
						make_obis(0x81, 0x81, 0xC7, 0x93, 0x09, nr),
						OBIS_IF_1107_W5
						}, make_value(rec["w5"]));

					//
					//	update device number
					//
					++nr;
					return true;	//	continue
				});

				//
				//	81 81 C7 93 10 FF - auto activation
				//
				append_get_proc_response(msg, {
					OBIS_IF_1107,
					OBIS_IF_1107_AUTO_ACTIVATION
					}, make_value(get_config_value(tbl_cfg, OBIS_IF_1107_AUTO_ACTIVATION.to_str(), false)));

				//
				//	81 81 C7 93 11 FF - time grid of load profile readout in seconds
				//
				append_get_proc_response(msg, {
					OBIS_IF_1107,
					OBIS_IF_1107_TIME_GRID
					}, make_value(get_config_obj(tbl_cfg, OBIS_IF_1107_TIME_GRID.to_str())));

				//
				//	81 81 C7 93 13 FF - time sync in seconds
				//
				append_get_proc_response(msg, {
					OBIS_IF_1107,
					OBIS_IF_1107_TIME_SYNC
					}, make_value(get_config_obj(tbl_cfg, OBIS_IF_1107_TIME_SYNC.to_str())));

				//
				//	81 81 C7 93 14 FF - seconds
				//
				append_get_proc_response(msg, {
					OBIS_IF_1107,
					OBIS_IF_1107_MAX_VARIATION
					}, make_value(get_config_obj(tbl_cfg, OBIS_IF_1107_MAX_VARIATION.to_str())));

			});

			//
			//	append to message queue
			//
			sml_gen_.append(std::move(msg));

		}

		void get_proc_parameter::storage_time_shift(std::string trx, cyng::buffer_t srv_id)
		{
			auto msg = sml_gen_.empty_get_proc_param_response(trx, srv_id, OBIS_STORAGE_TIME_SHIFT);

			append_get_proc_response(msg, {
				OBIS_STORAGE_TIME_SHIFT,
				OBIS_CODE(00, 80, 80, 00, 01, FF)
				}, make_value(0u));

			//
			//	append to message queue
			//
			sml_gen_.append(std::move(msg));
		}

		void get_proc_parameter::push_operations(std::string trx, cyng::buffer_t srv_id)
		{
			auto msg = sml_gen_.empty_get_proc_param_response(trx, srv_id, OBIS_PUSH_OPERATIONS);

			//
			//	81 81 C7 8A 01 FF - service push
			//	read from SQLite database table "TOpLog"
			//
			/*
			storage_.loop("TPushOps", [&](cyng::table::record const& rec)->bool {

				cyng::buffer_t id;
				id = cyng::value_cast(rec["serverID"], id);

				//
				//	only matching records
				//
				if (srv_id == id) {

					auto const nr = cyng::numeric_cast<std::uint8_t>(rec["idx"], 1);

					//
					//	81 81 C7 8A 02 FF - push interval in seconds
					//
					append_get_proc_response(msg, {
						OBIS_PUSH_OPERATIONS,
						make_obis(0x81, 0x81, 0xC7, 0x8A, 0x01, nr),
						OBIS_PUSH_INTERVAL
						}, make_value(rec["interval"]));

					//
					//	81 81 C7 8A 03 FF - push delay in seconds
					//
					append_get_proc_response(msg, {
						OBIS_PUSH_OPERATIONS,
						make_obis(0x81, 0x81, 0xC7, 0x8A, 0x01, nr),
						OBIS_PUSH_DELAY
						}, make_value(rec["delay"]));

					//
					//	81 47 17 07 00 FF - target name
					//
					append_get_proc_response(msg, {
						OBIS_PUSH_OPERATIONS,
						make_obis(0x81, 0x81, 0xC7, 0x8A, 0x01, nr),
						OBIS_CODE_PUSH_TARGET
						}, make_value(rec["target"]));

					//	push service: 
					//	* 81 81 C7 8A 21 FF == IP-T
					//	* 81 81 C7 8A 22 FF == SML client address
					//	* 81 81 C7 8A 23 FF == KNX ID 
					append_get_proc_response(msg, {
						OBIS_PUSH_OPERATIONS,
						make_obis(0x81, 0x81, 0xC7, 0x8A, 0x01, nr),
						OBIS_PUSH_SERVICE
						}, make_value(OBIS_PUSH_SERVICE_IPT));

					//	push source: 
					//	* 81 81 C7 8A 42 FF == profile (PUSH_SOURCE_PROFILE)
					//	* 81 81 C7 8A 43 FF == installation parameters (PUSH_SOURCE_INSTALL)
					//	* 81 81 C7 8A 44 FF == list of visible sensors/actors (PUSH_SOURCE_SENSOR_LIST)
					append_get_proc_response(msg, {
						OBIS_PUSH_OPERATIONS,
						make_obis(0x81, 0x81, 0xC7, 0x8A, 0x01, nr),
						OBIS_PUSH_SOURCE
						}, make_value(OBIS_PUSH_SOURCE_PROFILE));

				}

				return true;	//	continue
			});
			*/

			//
			//	append to message queue
			//
			sml_gen_.append(std::move(msg));

		}

		void get_proc_parameter::list_services(std::string trx, cyng::buffer_t srv_id)
		{
			auto msg = sml_gen_.empty_get_proc_param_response(trx, srv_id, OBIS_LIST_SERVICES);

#if BOOST_OS_WINDOWS
			scm::mgr m(false);	//	no admin required
			auto r = m.get_active_services();

			std::size_t counter{ 0 };
			std::stringstream ss;
			for (auto const& srv : r) {
				ss
					<< srv.name_
					<< ':'
					//<< srv.name_
					<< srv.display_name
					<< ';'
					;
				++counter;
				//	problems with handling larger strings
				if (counter > 36)	break;
			}

			auto data = ss.str();
			//std::replace(data.begin(), data.end(), ' ', '-');
			CYNG_LOG_INFO(logger_, "active services: " << data);

			append_get_proc_response(msg, {
				OBIS_LIST_SERVICES,
		}, make_value(data));

#else
			append_get_proc_response(msg, {
				OBIS_LIST_SERVICES,
				}, make_value("smfService:smfConfiguration-001;"));
#endif

			//
			//	append to message queue
			//
			sml_gen_.append(std::move(msg));
		}

		void get_proc_parameter::actuators(std::string trx, cyng::buffer_t srv_id)
		{
			sml_gen_.get_proc_actuators(trx, srv_id);
		}

		void get_proc_parameter::code_if_edl(std::string trx, cyng::buffer_t srv_id)
		{
			CYNG_LOG_WARNING(logger_, "sml.get.proc.parameter.request - OBIS_CODE_IF_EDL not implemented yet");
			sml_gen_.empty(trx, srv_id, OBIS_CODE_IF_EDL);
		}

		void get_proc_parameter::class_mbus(std::string trx, cyng::buffer_t srv_id)
		{
			CYNG_LOG_WARNING(logger_, "sml.get.proc.parameter.request - OBIS_CLASS_MBUS not implemented yet");
			sml_gen_.empty(trx, srv_id, OBIS_CLASS_MBUS);
			//constexpr static obis	DEFINE_OBIS_CODE(00, B0, 00, 02, 00, 01, CLASS_MBUS_RO_INTERVAL);	//	readout interval in seconds % 3600 (33 36 30 30)
			//constexpr static obis	DEFINE_OBIS_CODE(00, B0, 00, 02, 00, 02, CLASS_MBUS_SEARCH_INTERVAL);	//	search interval in seconds % 0 (30)
			//constexpr static obis	DEFINE_OBIS_CODE(00, B0, 00, 02, 00, 03, CLASS_MBUS_SEARCH_DEVICE);	//	search device now and by restart	% True(54 72 75 65)
			//constexpr static obis	DEFINE_OBIS_CODE(00, B0, 00, 02, 00, 04, CLASS_MBUS_AUTO_ACTICATE);	//	automatic activation of meters     % False(46 61 6C 73 65)
			//constexpr static obis	DEFINE_OBIS_CODE(00, B0, 00, 02, 00, 05, CLASS_MBUS_BITRATE);		//	used baud rates(bitmap) % 82 (38 32)

		}

	}	//	sml
}

