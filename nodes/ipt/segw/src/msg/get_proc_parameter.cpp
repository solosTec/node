/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "get_proc_parameter.h"
#include "config_ipt.h"
#include "config_sensor_params.h"
#include "config_data_collector.h"
#include "config_security.h"
#include "config_access.h"
#include "config_iec.h"
#include "config_broker.h"
#include "config_customer_if.h"
#include "../segw.h"
#include "../cache.h"
#include "../storage.h"

#include <smf/sml/protocol/generator.h>
#include <smf/sml/obis_io.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/intrinsics/obis_factory.hpp>
#include <smf/shared/db_cfg.h>

#include <cyng/io/serializer.h>
#include <cyng/sys/ntp.h>
#include <cyng/sys/ip.h>
#include <cyng/sys/dns.h>
#include <cyng/sys/memory.h>
#include <cyng/numeric_cast.hpp>
#include <cyng/buffer_cast.h>
#include <cyng/intrinsics/buffer.h>
#include <cyng/parser/chrono_parser.h>
#include <cyng/rnd.h>
#include <cyng/io/io_buffer.h>

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
			, node::ipt::config_ipt& ipt
			, config_sensor_params& sensor_params
			, config_data_collector& data_collector
			, config_security& security
			, config_access& access
			, config_iec& iec
			, config_broker& broker
			, config_customer_if& customer)
		: logger_(logger)
			, sml_gen_(sml_gen)
			, cache_(cfg)
			, storage_(db)
			, config_ipt_(ipt)
			, config_sensor_params_(sensor_params)
			, config_data_collector_(data_collector)
			, config_security_(security)
			, config_access_(access)
			, config_iec_(iec)
			, config_broker_(broker)
			, config_customer_if_(customer)
		{}

		cyng::tuple_t get_proc_parameter::generate_response(obis_path_t path
			, std::string trx
			, cyng::buffer_t srv_id
			, std::string user
			, std::string pwd)
		{
			BOOST_ASSERT(!path.empty());

			switch (path.front().to_uint64()) {
			case CODE_CLASS_OP_LOG_STATUS_WORD:	//	OBIS_CLASS_OP_LOG_STATUS_WORD
				return class_op_log_status_word(trx, srv_id);
			case CODE_ROOT_DEVICE_IDENT:	//	0x8181C78201FF
				return code_root_device_ident(trx, srv_id);
			case CODE_ROOT_DEVICE_TIME:	//	0x8181C78810FF
				return code_root_device_time(trx, srv_id);
			case CODE_ROOT_NTP:	//	0x8181C78801FF
				return code_root_ntp(trx, srv_id);
			case CODE_ROOT_ACCESS_RIGHTS:	//	81 81 81 60 FF FF
				return config_access_.get_proc_params(trx, srv_id, path);
			case CODE_ROOT_CUSTOM_INTERFACE:	//	81 02 00 07 00 FF
				return config_customer_if_.get_proc_params(trx, srv_id);
			case CODE_ROOT_CUSTOM_PARAM:	//	0x8102000710FF
				return config_customer_if_.get_ip_address(trx, srv_id);
			case CODE_ROOT_NMS:
				return config_customer_if_.get_nms(trx, srv_id);
			case CODE_ROOT_WAN:	//	0x8104000610FF
				return code_root_wan(trx, srv_id);
			case CODE_ROOT_GSM:	//	0x8104020700FF
				return code_root_gsm(trx, srv_id);
			case CODE_ROOT_GPRS_PARAM:	//	0x81040D0700FF
				return code_root_gprs_param(trx, srv_id);
			case CODE_ROOT_IPT_STATE:	//	0x81490D0600FF
				return code_root_ipt_state(trx, srv_id);
			case CODE_ROOT_IPT_PARAM:	//	0x81490D0700FF
				return config_ipt_.get_proc_params(trx, srv_id);
			case CODE_ROOT_W_MBUS_STATUS:	//	0x81060F0600FF
				return code_root_w_mbus_status(trx, srv_id);
			case CODE_IF_wMBUS:	//	0x8106190700FF
				return code_if_wmbus(trx, srv_id);
			case CODE_ROOT_LAN_DSL:	//	0x81480D0600FF
				return code_root_lan_dsl(trx, srv_id);
			case CODE_IF_LAN_DSL:	//	0x8148170700FF
				return code_if_lan_dsl(trx, srv_id);
			case CODE_ROOT_MEMORY_USAGE:	//	0x0080800010FF
				return code_root_memory_usage(trx, srv_id);
			case CODE_ROOT_VISIBLE_DEVICES:	//	81 81 10 06 FF FF
				return code_root_visible_devices(trx, srv_id);
			case CODE_ROOT_ACTIVE_DEVICES:	//	81 81 11 06 FF FF
				return code_root_active_devices(trx, srv_id);
			case CODE_ROOT_DEVICE_INFO:	//	81 81 12 06 FF FF
				return code_root_device_info(trx, srv_id);
			case CODE_ROOT_SENSOR_PARAMS:	//	81 81 C7 86 00 FF
				return config_sensor_params_.get_proc_params(trx, srv_id);
			case CODE_ROOT_DATA_COLLECTOR:	//	 0x8181C78620FF (Datenspiegel)
				return config_data_collector_.get_proc_params(trx, srv_id);
			case CODE_IF_1107:	//	0x8181C79300FF
				return config_iec_.get_proc_params(trx, srv_id);
			case CODE_STORAGE_TIME_SHIFT:	//	0x0080800000FF
				return storage_time_shift(trx, srv_id);
			case CODE_ROOT_PUSH_OPERATIONS:	//	0x8181C78A01FF
				return config_data_collector_.get_push_operations(trx, srv_id);
			case CODE_LIST_SERVICES:	//	0x990000000004
				return list_services(trx, srv_id);
			case CODE_ACTUATORS:	//	0x0080801100FF
				return actuators(trx, srv_id);
			case CODE_IF_EDL:	//	0x81050D0700FF - M-Bus EDL (RJ10)
				return code_if_edl(trx, srv_id);
			case CODE_CLASS_MBUS:	//	0x00B000020000
				return class_mbus(trx, srv_id);
			case CODE_ROOT_SECURITY:	//	00 80 80 01 00 FF
				return config_security_.get_proc_params(trx, srv_id);
			case CODE_ROOT_BROKER:
				return config_broker_.get_proc_params(trx, srv_id);
			case CODE_ROOT_SERIAL:
				return config_broker_.get_proc_params_port(trx, srv_id);
			default:
				CYNG_LOG_ERROR(logger_, "sml.get.proc.parameter.request - unknown OBIS code "
					<< path.front().to_str()
					<< " / "
					<< to_hex(path, ':'));
				break;
			}
			return sml_gen_.empty_get_proc_param(trx, srv_id, path.front());
		}

		cyng::tuple_t get_proc_parameter::class_op_log_status_word(std::string trx, cyng::buffer_t srv_id)
		{
			auto const word = cache_.get_status_word();
			CYNG_LOG_DEBUG(logger_, "status word: " << word);

			//auto msg = sml_gen_.empty_get_proc_param(trx, srv_id, OBIS_CLASS_OP_LOG_STATUS_WORD);

			//merge_msg(msg, {
			//	OBIS_CLASS_OP_LOG_STATUS_WORD,
			//	}, make_value(static_cast<std::uint32_t>(word)));

			//
			//	append to message queue
			//
			return sml_gen_.get_status_word(trx, srv_id, static_cast<std::uint32_t>(word));
		}

		cyng::tuple_t get_proc_parameter::code_root_device_ident(std::string trx, cyng::buffer_t srv_id)
		{
			//
			//	get values from "_Config" table
			//
			auto const manufacturer = cache_.get_cfg(OBIS_DATA_MANUFACTURER.to_str(), std::string(""));
			auto const serial = cache_.get_cfg<std::uint32_t>(OBIS_SERIAL_NR.to_str(), 10000000u);

			return sml_gen_.get_proc_parameter_device_id(trx
				, srv_id	//	server id
				, manufacturer
				, cache_.get_srv_id()
				, "VSES-1KW-221-1F0"
				, serial);
		}

		cyng::tuple_t get_proc_parameter::code_root_device_time(std::string trx, cyng::buffer_t srv_id)
		{
			auto const now = std::chrono::system_clock::now();
			std::int32_t const tz = 60;
			bool const sync_active = true;

			return sml_gen_.get_proc_device_time(trx
				, srv_id	//	server id
				, now
				, tz
				, sync_active);
		}

		cyng::tuple_t get_proc_parameter::code_root_ntp(std::string trx, cyng::buffer_t srv_id)
		{
			//
			//	get all configured NTP servers
			//
			auto const ntp_servers = cyng::sys::get_ntp_servers();

			//const std::string ntp_primary = "pbtime1.pbt.de", ntp_secondary = "pbtime2.pbt.de", ntp_tertiary = "pbtime3.pbt.de";
			const std::uint16_t ntp_port = 123;	// 123;
			const bool ntp_active = true;
			const std::uint16_t ntp_tz = 3600;

			auto msg = sml_gen_.empty_get_proc_param(trx, srv_id, OBIS_ROOT_NTP);

			merge_msg(msg, { OBIS_ROOT_NTP, OBIS_CODE_NTP_PORT }, make_value(ntp_port));
			merge_msg(msg, { OBIS_ROOT_NTP, OBIS_CODE_NTP_ACTIVE }, make_value(ntp_active));
			merge_msg(msg, { OBIS_ROOT_NTP, OBIS_CODE_NTP_TZ }, make_value(ntp_tz));

			for (std::uint8_t nr = 0; nr < ntp_servers.size(); ++nr) {
				auto srv = ntp_servers.at(nr);
				merge_msg(msg, { OBIS_ROOT_NTP, OBIS_CODE_NTP_SERVER, make_obis(0x81, 0x81, 0xC7, 0x88, 0x02, nr + 1) }, make_value(srv));
			}

			return msg;
		}

		cyng::tuple_t get_proc_parameter::code_root_wan(std::string trx, cyng::buffer_t srv_id)
		{
			CYNG_LOG_WARNING(logger_, "sml.get.proc.parameter.request - OBIS_OBIS_ROOT_WAN not implemented yet");
			return sml_gen_.empty_get_proc_param(trx, srv_id, OBIS_ROOT_WAN);
		}

		cyng::tuple_t get_proc_parameter::code_root_gsm(std::string trx, cyng::buffer_t srv_id)
		{
			CYNG_LOG_WARNING(logger_, "sml.get.proc.parameter.request - OBIS_ROOT_GSM not implemented yet");
			return sml_gen_.empty_get_proc_param(trx, srv_id, OBIS_ROOT_GSM);
		}

		cyng::tuple_t get_proc_parameter::code_root_gprs_param(std::string trx, cyng::buffer_t srv_id)
		{
			CYNG_LOG_WARNING(logger_, "sml.get.proc.parameter.request - OBIS_ROOT_GPRS_PARAM not implemented yet");
			return sml_gen_.empty_get_proc_param(trx, srv_id, OBIS_ROOT_GPRS_PARAM);
		}

		cyng::tuple_t get_proc_parameter::code_root_ipt_state(std::string trx, cyng::buffer_t srv_id)
		{
			//
			//	get TCP/IP endpoint from _Config table
			//
			auto const rep = cache_.get_cfg(build_cfg_key({ sml::OBIS_ROOT_IPT_PARAM }, "ep.remote"), boost::asio::ip::tcp::endpoint());
			auto const lep = cache_.get_cfg(build_cfg_key({ sml::OBIS_ROOT_IPT_PARAM }, "ep.local"), boost::asio::ip::tcp::endpoint());

			CYNG_LOG_TRACE(logger_, "remote endpoint " << rep);
			CYNG_LOG_TRACE(logger_, "local endpoint " << lep);

			//
			//	generate response
			//
			return sml_gen_.get_proc_parameter_ipt_state(trx
				, srv_id
				, rep
				, lep);
		}

		cyng::tuple_t get_proc_parameter::code_root_w_mbus_status(std::string trx, cyng::buffer_t srv_id)
		{
			//
			//	wireless M-Bus adapter
			//
			return sml_gen_.get_proc_w_mbus_status(trx
				, srv_id
				//, "RC1180-MBUS3"	// manufacturer of w-mbus adapter
				, "solos::Tec"	// manufacturer of w-mbus adapter
				, cyng::make_buffer({ 0xA8, 0x15, 0x34, 0x83, 0x40, 0x04, 0x01, 0x31 })	//	adapter id (EN 13757-3/4)
				, NODE_VERSION	//	firmware version of adapter
				, "2.00"	//	hardware version of adapter
			);
		}

		cyng::tuple_t get_proc_parameter::code_if_wmbus(std::string trx, cyng::buffer_t srv_id)
		{
			//
			//	wireless M-Bus interface
			//
			auto msg = sml_gen_.empty_get_proc_param(trx, srv_id, OBIS_IF_wMBUS);

			cache_.read_table("_Cfg", [&](cyng::store::table const* tbl_cfg) {

				//
				//	This was necessary for a transition period and mybe no longer required
				//
				std::uint32_t reboot = 86400u;	//	seconds
				auto const obj = get_obj(tbl_cfg, cfg_key({ OBIS_IF_wMBUS, OBIS_W_MBUS_REBOOT }));
				if (obj.get_class().tag() == cyng::TC_STRING) {
					auto const r = cyng::parse_timespan_seconds(cyng::value_cast<std::string>(obj, "24:00:0.000000"));
					if (r.second) {
						reboot = static_cast<std::uint32_t>(r.first.count());
					}
				}

				merge_msg(msg, {
					OBIS_IF_wMBUS,
					OBIS_W_MBUS_REBOOT
					}, make_value(reboot));

				merge_msg(msg, {
					OBIS_IF_wMBUS,
					OBIS_W_MBUS_PROTOCOL
					}, make_value(get_obj(tbl_cfg, cfg_key({ OBIS_IF_wMBUS, OBIS_W_MBUS_PROTOCOL }))));

				merge_msg(msg, {
					OBIS_IF_wMBUS,
					OBIS_W_MBUS_MODE_S
					}, make_value(get_obj(tbl_cfg, cfg_key({ OBIS_IF_wMBUS, OBIS_W_MBUS_MODE_S }))));

				merge_msg(msg, {
					OBIS_IF_wMBUS,
					OBIS_W_MBUS_MODE_T
					}, make_value(get_obj(tbl_cfg, cfg_key({ OBIS_IF_wMBUS, OBIS_W_MBUS_MODE_T }))));

				merge_msg(msg, {
					OBIS_IF_wMBUS,
					OBIS_W_MBUS_POWER
					}, make_value(get_obj(tbl_cfg, cfg_key({ OBIS_IF_wMBUS, OBIS_W_MBUS_POWER }))));

				merge_msg(msg, {
					OBIS_IF_wMBUS,
					OBIS_W_MBUS_INSTALL_MODE
					}, make_value(get_obj(tbl_cfg, cfg_key({ OBIS_IF_wMBUS, OBIS_W_MBUS_INSTALL_MODE }))));
			});

			return msg;
		}

		cyng::tuple_t get_proc_parameter::code_root_lan_dsl(std::string trx, cyng::buffer_t srv_id)
		{
			//CYNG_LOG_WARNING(logger_, "sml.get.proc.parameter.request - OBIS_ROOT_LAN_DSL not implemented yet");

			//	81 48 0D 06 00 FF
			auto msg = sml_gen_.empty_get_proc_param(trx, srv_id, OBIS_ROOT_LAN_DSL);

			//	81 48 17 07 00 00
			auto rep = cache_.get_cfg<boost::asio::ip::tcp::endpoint>("remote.ep", boost::asio::ip::tcp::endpoint());
			auto lep = cache_.get_cfg<boost::asio::ip::tcp::endpoint>("local.ep", boost::asio::ip::tcp::endpoint());

			CYNG_LOG_TRACE(logger_, "remote endpoint " << rep.address().to_string());
			CYNG_LOG_TRACE(logger_, "local endpoint " << lep.address().to_string());

			boost::asio::ip::address wan = cyng::sys::get_WAN_address(rep.address().to_string());

			if (wan.is_v4()) {
				merge_msg(msg, {
					OBIS_ROOT_LAN_DSL,
					OBIS_CODE_IF_LAN_ADDRESS
					}, make_value(cyng::swap_num(wan.to_v4().to_uint())));
			}
			else {
				auto v6 = wan.to_v6().to_bytes();
				merge_msg(msg, {
					OBIS_ROOT_LAN_DSL,
					OBIS_CODE_IF_LAN_ADDRESS
					}, make_value(cyng::make_buffer(v6)));
			}

			//	81 48 17 07 01 00
			merge_msg(msg, {
				OBIS_ROOT_LAN_DSL,
				OBIS_CODE_IF_LAN_SUBNET_MASK
				}, make_value(0xFFFFFF));	//	255.255.255.0 (fix!)

			//	81 48 17 07 02 00
			merge_msg(msg, {
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
				merge_msg(msg, {
					OBIS_ROOT_LAN_DSL,
					OBIS_CODE_IF_LAN_DNS_PRIMARY
					}, make_value(cyng::swap_num(dns.at(0).to_v4().to_uint())));
				merge_msg(msg, {
					OBIS_ROOT_LAN_DSL,
					OBIS_CODE_IF_LAN_DNS_SECONDARY
					}, make_value(0u));
				merge_msg(msg, {
					OBIS_ROOT_LAN_DSL,
					OBIS_CODE_IF_LAN_DNS_TERTIARY
					}, make_value(0u));
				break;
			case 2:
				merge_msg(msg, {
					OBIS_ROOT_LAN_DSL,
					OBIS_CODE_IF_LAN_DNS_PRIMARY
					}, make_value(cyng::swap_num(dns.at(0).to_v4().to_uint())));
				merge_msg(msg, {
					OBIS_ROOT_LAN_DSL,
					OBIS_CODE_IF_LAN_DNS_SECONDARY
					}, make_value(cyng::swap_num(dns.at(1).to_v4().to_uint())));
				merge_msg(msg, {
					OBIS_ROOT_LAN_DSL,
					OBIS_CODE_IF_LAN_DNS_TERTIARY
					}, make_value(0u));
				break;
			default:
				merge_msg(msg, {
					OBIS_ROOT_LAN_DSL,
					OBIS_CODE_IF_LAN_DNS_PRIMARY
					}, make_value(cyng::swap_num(dns.at(0).to_v4().to_uint())));
				merge_msg(msg, {
					OBIS_ROOT_LAN_DSL,
					OBIS_CODE_IF_LAN_DNS_SECONDARY
					}, make_value(cyng::swap_num(dns.at(1).to_v4().to_uint())));
				merge_msg(msg, {
					OBIS_ROOT_LAN_DSL,
					OBIS_CODE_IF_LAN_DNS_TERTIARY
					}, make_value(cyng::swap_num(dns.at(1).to_v4().to_uint())));
			}

			//
			//	append to message queue
			//
			return msg;
		}

		cyng::tuple_t get_proc_parameter::code_if_lan_dsl(std::string trx, cyng::buffer_t srv_id)
		{
			//	81 48 17 07 00 FF
			auto msg = sml_gen_.empty_get_proc_param(trx, srv_id, OBIS_IF_LAN_DSL);

			//
			//	81 48 00 00 00 00 - computer name 
			//
			merge_msg(msg, {
				OBIS_IF_LAN_DSL,
				OBIS_COMPUTER_NAME
				}, make_value(boost::asio::ip::host_name()));

			//	81 48 00 32 02 01 - LAN_DHCP_ENABLED
			merge_msg(msg, {
				OBIS_IF_LAN_DSL,
				OBIS_LAN_DHCP_ENABLED
				}, make_value(true));

			//
			//	append to message queue
			//
			return msg;
		}

		cyng::tuple_t get_proc_parameter::code_root_memory_usage(std::string trx, cyng::buffer_t srv_id)
		{
			CYNG_LOG_INFO(logger_, "get_used_physical_memory " << cyng::sys::get_used_physical_memory());
			CYNG_LOG_INFO(logger_, "get_total_physical_memory " << cyng::sys::get_total_physical_memory());
			CYNG_LOG_INFO(logger_, "get_used_virtual_memory " << cyng::sys::get_used_virtual_memory());
			CYNG_LOG_INFO(logger_, "get_total_virtual_memory " << cyng::sys::get_total_virtual_memory());

			//	OBIS_ROOT_MEMORY_USAGE
			const std::uint8_t mirror = cyng::sys::get_used_virtual_memory_in_percent()
				, tmp = cyng::sys::get_used_physical_memory_in_percent();

			return sml_gen_.get_proc_mem_usage(trx
				, srv_id	//	server id
				, mirror
				, tmp);

		}

		cyng::tuple_t get_proc_parameter::code_root_active_devices(std::string trx, cyng::buffer_t srv_id)
		{
			//	81 81 11 06 FF FF
			auto msg = sml_gen_.empty_get_proc_param(trx, srv_id, OBIS_ROOT_ACTIVE_DEVICES);

			//merge_msg(msg, { OBIS_CODE_ROOT_NTP, OBIS_CODE_NTP_PORT }, make_value(ntp_port));

			std::uint8_t 
				quant{ 1 }, //	outer loop counter (0x01 - 0xFA)
				store{ 0 }	//	inner loop counter (0x01 - 0xFE)
			;

			auto const now = std::chrono::system_clock::now();

			//
			//	list all active M-Bus devices
			//

			cyng::buffer_t tmp;
			cache_.loop("_DeviceMBUS", [&](cyng::table::record const& rec)->bool {

				//
				//	update counter
				//
				++store;
				if (store == 0xFE) {
					++quant;
					store = 0x01;
				}

				//
				//	set server ID
				//
				auto const srv_id = cyng::to_buffer(rec["serverID"]);
				merge_msg(msg, {
					OBIS_ROOT_ACTIVE_DEVICES,
					make_obis(0x81, 0x81, 0x11, 0x06, quant, 0xFF),
					make_obis(0x81, 0x81, 0x11, 0x06, quant, store),
					OBIS_SERVER_ID
					}, make_value(srv_id));

				CYNG_LOG_TRACE(logger_, "list active device "
					<< (((quant - 1) * 256) + store)
					<< ": "
					<< cyng::io::to_hex(srv_id));

				//
				//	device class
				//
				merge_msg(msg, {
					OBIS_ROOT_ACTIVE_DEVICES,
					make_obis(0x81, 0x81, 0x11, 0x06, quant, 0xFF),
					make_obis(0x81, 0x81, 0x11, 0x06, quant, store),
					OBIS_DEVICE_CLASS
					}, make_value(cyng::value_cast<std::string>(rec["class"], "")));

				//
				//	timestamp (last seen)
				//
				merge_msg(msg, {
					OBIS_ROOT_ACTIVE_DEVICES,
					make_obis(0x81, 0x81, 0x11, 0x06, quant, 0xFF),
					make_obis(0x81, 0x81, 0x11, 0x06, quant, store),
					OBIS_CURRENT_UTC
					}, make_value(cyng::value_cast(rec["lastSeen"], now)));

				return true;	//	continue
			});

			CYNG_LOG_TRACE(logger_, "ROOT_ACTIVE_DEVICES: "
				<< (((quant - 1) * 256) + store)
				<< "/"
				<< msg.size()
				<< " - "
				<< cyng::io::to_type(msg));

			//
			//	append to message queue
			//
			return msg;
		}

		cyng::tuple_t get_proc_parameter::code_root_visible_devices(std::string trx, cyng::buffer_t srv_id)
		{
			//	81 81 10 06 FF FF
			auto msg = sml_gen_.empty_get_proc_param(trx, srv_id, OBIS_ROOT_VISIBLE_DEVICES);

			std::uint8_t
				quant{ 1 }, //	outer loop counter (0x01 - 0xFA)
				store{ 0 }	//	inner loop counter (0x01 - 0xFE)
			;

			auto const now = std::chrono::system_clock::now();

			//
			//	list all visible M-Bus devices
			//
			cache_.loop("_DeviceMBUS", [&](cyng::table::record const& rec) {

				//
				//	update counter
				//
				++store;
				if (store == 0xFE) {
					++quant;
					store = 0x01;
				}

				//
				//	set server ID
				//
				auto const srv_id = cyng::to_buffer(rec["serverID"]);
				merge_msg(msg, {
					OBIS_ROOT_VISIBLE_DEVICES,
					make_obis(0x81, 0x81, 0x10, 0x06, quant, 0xFF),
					make_obis(0x81, 0x81, 0x10, 0x06, quant, store),
					OBIS_SERVER_ID
				}, make_value(srv_id));

				CYNG_LOG_TRACE(logger_, "list visible device "
					<< (((quant - 1) * 256) + store)
					<< ": "
					<< cyng::io::to_hex(srv_id));
				
				//
				//	device class
				//
				merge_msg(msg, {
					OBIS_ROOT_VISIBLE_DEVICES,
					make_obis(0x81, 0x81, 0x10, 0x06, quant, 0xFF),
					make_obis(0x81, 0x81, 0x10, 0x06, quant, store),
					OBIS_DEVICE_CLASS
					}, make_value(cyng::value_cast<std::string>(rec["class"], "")));

				//
				//	timestamp (last seen)
				//
				merge_msg(msg, {
					OBIS_ROOT_VISIBLE_DEVICES,
					make_obis(0x81, 0x81, 0x10, 0x06, quant, 0xFF),
					make_obis(0x81, 0x81, 0x10, 0x06, quant, store),
					OBIS_CURRENT_UTC
					}, make_value(cyng::value_cast(rec["lastSeen"], now)));


				return true;	//	continue
			});


			CYNG_LOG_TRACE(logger_, "ROOT_VISIBLE_DEVICES: "
				<< (((quant - 1) * 256) + store)
				<< "/"
				<< msg.size()
				<< " - "
				<< cyng::io::to_type(msg));

			//
			//	append to message queue
			//
			return msg;
		}

		cyng::tuple_t get_proc_parameter::code_root_device_info(std::string trx, cyng::buffer_t srv_id)
		{
			//	see ZDUE-MUC_Anwenderhandbuch_V2_5.pdf (Chapter 21.19)
			CYNG_LOG_WARNING(logger_, "sml.get.proc.parameter.request - OBIS_ROOT_DEVICE_INFO not implemented yet");
			return sml_gen_.empty_get_proc_param(trx, srv_id, OBIS_ROOT_DEVICE_INFO);
		}

		cyng::tuple_t get_proc_parameter::storage_time_shift(std::string trx, cyng::buffer_t srv_id)
		{
			auto msg = sml_gen_.empty_get_proc_param(trx, srv_id, OBIS_STORAGE_TIME_SHIFT);

			auto sts = cache_.get_cfg<std::int32_t>(sml::OBIS_STORAGE_TIME_SHIFT.to_str(), 0);
			merge_msg(msg, {
				OBIS_STORAGE_TIME_SHIFT,
				OBIS_CODE(00, 80, 80, 00, 01, FF)
				}, make_value(sts));

			//
			//	append to message queue
			//
			return msg;
		}

		cyng::tuple_t get_proc_parameter::list_services(std::string trx, cyng::buffer_t srv_id)
		{
			auto msg = sml_gen_.empty_get_proc_param(trx, srv_id, OBIS_LIST_SERVICES);

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

			merge_msg(msg, {
				OBIS_LIST_SERVICES,
		}, make_value(data));

#else
			merge_msg(msg, {
				OBIS_LIST_SERVICES,
				}, make_value("smfService:smfConfiguration-001;"));
#endif

			//
			//	append to message queue
			//
			return msg;
		}

		cyng::tuple_t get_proc_parameter::actuators(std::string trx, cyng::buffer_t srv_id)
		{
			return sml_gen_.get_proc_actuators(trx, srv_id);
		}

		cyng::tuple_t get_proc_parameter::code_if_edl(std::string trx, cyng::buffer_t srv_id)
		{
			//sml_gen_.empty(trx, srv_id, OBIS_IF_EDL);
			auto msg = sml_gen_.empty_get_proc_param(trx, srv_id, OBIS_IF_EDL);

			//
			//	protocol (always 1)
			//
			merge_msg(msg, {
				OBIS_IF_EDL,
				OBIS_IF_EDL_PROTOCOL
				}, make_value(1u));

			//
			//	baudrate
			//
			merge_msg(msg, {
				OBIS_IF_EDL,
				OBIS_IF_EDL_BAUDRATE
				}, make_value(0u));	//	auto

			//
			//	append to message queue
			//
			return msg;
		}

		cyng::tuple_t get_proc_parameter::class_mbus(std::string trx, cyng::buffer_t srv_id)
		{
			auto msg = sml_gen_.empty_get_proc_param(trx, srv_id, OBIS_CLASS_MBUS);

			//
			//	readout interval 
			//
			auto const ro_interval = cache_.get_cfg(build_cfg_key({ OBIS_CLASS_MBUS, OBIS_CLASS_MBUS_RO_INTERVAL }), std::chrono::seconds(7200u));
			merge_msg(msg, {
				OBIS_CLASS_MBUS,
				OBIS_CLASS_MBUS_RO_INTERVAL
				}, make_value(ro_interval));


			//
			//	search interval
			//
			auto const search_interval = cache_.get_cfg(build_cfg_key({ OBIS_CLASS_MBUS, OBIS_CLASS_MBUS_SEARCH_INTERVAL }), std::chrono::seconds(0));
			merge_msg(msg, {
				OBIS_CLASS_MBUS,
				OBIS_CLASS_MBUS_SEARCH_INTERVAL
				}, make_value(search_interval));

			//
			//	[bool] search device now and by restart
			//
			auto const search = cache_.get_cfg(build_cfg_key({ OBIS_CLASS_MBUS, OBIS_CLASS_MBUS_SEARCH_DEVICE }), true);
			merge_msg(msg, {
				OBIS_CLASS_MBUS,
				OBIS_CLASS_MBUS_SEARCH_DEVICE
				}, make_value(search));

			//
			//	[bool] automatic activation of meters
			//
			auto const auto_activate = cache_.get_cfg(build_cfg_key({ OBIS_CLASS_MBUS, OBIS_CLASS_MBUS_AUTO_ACTICATE }), false);
			merge_msg(msg, {
				OBIS_CLASS_MBUS,
				OBIS_CLASS_MBUS_AUTO_ACTICATE
				}, make_value(auto_activate));

			//
			//	[u32] baudrate
			//
			auto const baudrate = cache_.get_cfg(build_cfg_key({ OBIS_CLASS_MBUS, OBIS_CLASS_MBUS_BITRATE }), 2400u);
			merge_msg(msg, {
				OBIS_CLASS_MBUS,
				OBIS_CLASS_MBUS_BITRATE
				}, make_value(baudrate));

			//
			//	append to message queue
			//
			return msg;
		}

	}	//	sml
}

