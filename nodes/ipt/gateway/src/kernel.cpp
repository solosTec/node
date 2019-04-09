/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "kernel.h"
#include <smf/sml/protocol/message.h>
#include <smf/sml/protocol/value.hpp>
#include <smf/sml/obis_db.h>
#include <smf/sml/srv_id_io.h>
#include <smf/sml/obis_io.h>
#include <smf/sml/protocol/parser.h>
#include <smf/mbus/defs.h>
#include <smf/mbus/header.h>
#include <smf/mbus/variable_data_block.h>
#include <smf/mbus/aes.h>

#include <cyng/io/serializer.h>
#include <cyng/vm/generator.h>
#include <cyng/tuple_cast.hpp>
#include <cyng/numeric_cast.hpp>
#include <cyng/sys/memory.h>
#include <cyng/io/swap.h>
#include <cyng/io/io_buffer.h>
#include <cyng/util/slice.hpp>

//#ifdef SMF_IO_DEBUG
#include <cyng/io/hex_dump.hpp>
//#endif

#include <boost/algorithm/string.hpp>

namespace node
{
	namespace sml
	{

		kernel::kernel(cyng::logging::log_ptr logger
			, cyng::controller& vm
			, status& status_word
			, cyng::store::db& config_db
			, node::ipt::redundancy const& cfg
			, bool server_mode
			, std::string account
			, std::string pwd, std::string manufacturer
			, std::string model
			, std::uint32_t serial
			, cyng::mac48 mac
			, bool accept_all)
		: status_word_(status_word)
			, logger_(logger)
			, config_db_(config_db)
			, cfg_ipt_(cfg)
			, server_mode_(server_mode)
			, account_(account)
			, pwd_(pwd)
			, manufacturer_(manufacturer)
			, model_(model)
			, serial_(serial)
			, accept_all_(accept_all)
			, server_id_(to_gateway_srv_id(mac))
			, reader_()
			, sml_gen_()
		{
			reset();

			//
			//	SML transport
			//
			vm.register_function("sml.msg", 2, std::bind(&kernel::sml_msg, this, std::placeholders::_1));
			vm.register_function("sml.eom", 2, std::bind(&kernel::sml_eom, this, std::placeholders::_1));
			vm.register_function("sml.log", 1, [this](cyng::context& ctx){
				const cyng::vector_t frame = ctx.get_frame();
				CYNG_LOG_INFO(logger_, "sml.log - " << cyng::value_cast<std::string>(frame.at(0), ""));
			});
			
			//
			//	SML data
			//
			vm.register_function("sml.public.open.request", 8, std::bind(&kernel::sml_public_open_request, this, std::placeholders::_1));
			vm.register_function("sml.public.close.request", 3, std::bind(&kernel::sml_public_close_request, this, std::placeholders::_1));
			vm.register_function("sml.public.close.response", 3, std::bind(&kernel::sml_public_close_response, this, std::placeholders::_1));

			vm.register_function("sml.get.proc.parameter.request", 8, std::bind(&kernel::sml_get_proc_parameter_request, this, std::placeholders::_1));
			vm.register_function("sml.get.profile.list.request", 10, std::bind(&kernel::sml_get_profile_list_request, this, std::placeholders::_1));

			vm.register_function("sml.set.proc.push.interval", 7, std::bind(&kernel::sml_set_proc_push_interval, this, std::placeholders::_1));
			vm.register_function("sml.set.proc.push.delay", 7, std::bind(&kernel::sml_set_proc_push_delay, this, std::placeholders::_1));
			vm.register_function("sml.set.proc.push.name", 7, std::bind(&kernel::sml_set_proc_push_name, this, std::placeholders::_1));

			vm.register_function("sml.set.proc.ipt.param.address", 4, std::bind(&kernel::sml_set_proc_ipt_param_address, this, std::placeholders::_1));
			vm.register_function("sml.set.proc.ipt.param.port.target", 4, std::bind(&kernel::sml_set_proc_ipt_param_port_target, this, std::placeholders::_1));
			vm.register_function("sml.set.proc.ipt.param.user", 4, std::bind(&kernel::sml_set_proc_ipt_param_user, this, std::placeholders::_1));
			vm.register_function("sml.set.proc.ipt.param.pwd", 4, std::bind(&kernel::sml_set_proc_ipt_param_pwd, this, std::placeholders::_1));

			vm.register_function("sml.set.proc.activate", 7, std::bind(&kernel::sml_set_proc_activate, this, std::placeholders::_1));
			vm.register_function("sml.set.proc.deactivate", 7, std::bind(&kernel::sml_set_proc_deactivate, this, std::placeholders::_1));
			vm.register_function("sml.set.proc.delete", 7, std::bind(&kernel::sml_set_proc_delete, this, std::placeholders::_1));

			vm.register_function("sml.set.proc.if1107.param", 7, std::bind(&kernel::sml_set_proc_if1107_param, this, std::placeholders::_1));
			vm.register_function("sml.set.proc.if1107.device", 7, std::bind(&kernel::sml_set_proc_if1107_device, this, std::placeholders::_1));

			vm.register_function("sml.set.proc.mbus.param", 7, std::bind(&kernel::sml_set_proc_mbus_param, this, std::placeholders::_1));
			//vm.register_function("sml.set.proc.mbus.install.mode", 6, std::bind(&kernel::sml_set_proc_mbus_install_mode, this, std::placeholders::_1));
			//vm.register_function("sml.set.proc.mbus.s.mode", 6, std::bind(&kernel::sml_set_proc_mbus_smode, this, std::placeholders::_1));
			//vm.register_function("sml.set.proc.mbus.t.mode", 6, std::bind(&kernel::sml_set_proc_mbus_tmode, this, std::placeholders::_1));
			//vm.register_function("sml.set.proc.mbus.protocol", 6, std::bind(&kernel::sml_set_proc_mbus_protocol, this, std::placeholders::_1));
			
			vm.register_function("sml.get.list.request", 9, std::bind(&kernel::sml_get_list_request, this, std::placeholders::_1));
			vm.register_function("sml.get.list.response", 9, std::bind(&kernel::sml_get_list_response, this, std::placeholders::_1));


			//
			//	callback from wireless LMN
			//
			vm.register_function("wmbus.push.frame", 7, std::bind(&kernel::wmbus_push_frame, this, std::placeholders::_1));

		}

		void kernel::reset()
		{
			//
			//	reset reader
			//
			sml_gen_.reset();
			reader_.reset();
		}

		void kernel::sml_msg(cyng::context& ctx)
		{
			//	[{31383035323232323535353231383235322D31,0,0,{0100,{null,005056C00008,3230313830353232323235353532,0500153B02297E,6F70657261746F72,6F70657261746F72,null}},9c91},0]
			//	[{31383035323232323535353231383235322D32,1,0,{0500,{0500153B02297E,6F70657261746F72,6F70657261746F72,{810060050000},null}},f8b5},1]
			//	[{31383035323232323535353231383235322D33,2,0,{0500,{0500153B02297E,6F70657261746F72,6F70657261746F72,{8181C78201FF},null}},4d37},2]
			//	[{31383035323232323535353231383235322D34,0,0,{0200,{null}},e6e8},3]
			//
			const cyng::vector_t frame = ctx.get_frame();
			const std::size_t idx = cyng::value_cast<std::size_t>(frame.at(1), 0);
			//CYNG_LOG_TRACE(logger_, "sml.msg - " << cyng::io::to_str(frame));
			CYNG_LOG_INFO(logger_, "sml.msg #" << idx);

			//
			//	get message body
			//
			cyng::tuple_t msg;
			msg = cyng::value_cast(frame.at(0), msg);
			CYNG_LOG_INFO(logger_, "sml.msg " << cyng::io::to_str(frame));

			//
			//	add generated instruction vector
			//
			ctx.queue(reader_.read(msg, idx));
		}

		void kernel::sml_eom(cyng::context& ctx)
		{
			//[4aa5,4]
			//
			//	* CRC
			//	* message counter
			//
			const cyng::vector_t frame = ctx.get_frame();
			const std::size_t idx = cyng::value_cast<std::size_t>(frame.at(1), 0);
			//CYNG_LOG_TRACE(logger_, "sml.eom - " << cyng::io::to_str(frame));
			CYNG_LOG_INFO(logger_, "sml.eom #" << idx);

			//
			//	build SML message frame
			//
			//cyng::buffer_t buf = boxing(msg_);
			cyng::buffer_t buf = sml_gen_.boxing();

#ifdef SMF_IO_DEBUG
			cyng::io::hex_dump hd;
			std::stringstream ss;
			hd(ss, buf.begin(), buf.end());
			CYNG_LOG_TRACE(logger_, "response:\n" << ss.str());
#endif

			//
			//	transfer data
			//	"stream.serialize"
			//
			if (server_mode_)
			{
				ctx.queue(cyng::generate_invoke("stream.serialize", buf));
			}
			else
			{
				ctx.queue(cyng::generate_invoke("ipt.transfer.data", buf));
			}

			ctx.queue(cyng::generate_invoke("stream.flush"));

			reset();
		}

		void kernel::sml_public_open_request(cyng::context& ctx)
		{
			//	[34481794-6866-4776-8789-6f914b4e34e7,180301091943374505-1,0,005056c00008,00:15:3b:02:23:b3,20180301092332,operator,operator]
			//	[874c991e-cfc0-4348-83ab-064f59363229,190131160312334117-1,0,005056C00008,0500FFB04B94F8,20190131160312,operator,operator]
			//
			//	* pk
			//	* transaction id
			//	* SML message id
			//	* client ID - MAC from client
			//	* server ID - target server/gateway
			//	* request file id
			//	* username
			//	* password
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,	//	[0] pk
				std::string,		//	[1] trx
				std::size_t,		//	[2] SML message id
				cyng::buffer_t,		//	[3] client ID
				cyng::buffer_t,		//	[4] server ID
				std::string,		//	[5] request file id
				std::string,		//	[6] username
				std::string			//	[7] password
			>(frame);

			//	sml.public.open.request - trx: 190131160312334117-1, msg id: 0, client id: , server id: , file id: 20190131160312, user: operator, pwd: operator
			CYNG_LOG_INFO(logger_, "sml.public.open.request - trx: "
				<< std::get<1>(tpl)
				<< ", msg id: "
				<< std::get<2>(tpl)
				<< ", client id: "
				<< cyng::io::to_hex(std::get<3>(tpl))
				<< ", server id: "
				<< cyng::io::to_hex(std::get<4>(tpl))
				<< ", file id: "
				<< std::get<5>(tpl)
				<< ", user: "
				<< std::get<6>(tpl)
				<< ", pwd: "
				<< std::get<7>(tpl))
				;

			if (!accept_all_) {

				//
				//	test server ID
				//
				if (!boost::algorithm::equals(server_id_, std::get<4>(tpl))) {

					sml_gen_.attention_msg(frame.at(1)	// trx
						, std::get<4>(tpl)	//	server ID
						, OBIS_ATTENTION_NO_SERVER_ID.to_buffer()
						, "wrong server id"
						, cyng::tuple_t());

					CYNG_LOG_WARNING(logger_, "sml.public.open.request - wrong server ID: "
						<< cyng::io::to_hex(std::get<4>(tpl)))
						;
					return;
				}
				//
				//	test login credentials
				//
				if (!boost::algorithm::equals(account_, std::get<6>(tpl)) ||
					!boost::algorithm::equals(pwd_, std::get<7>(tpl))) {

					sml_gen_.attention_msg(frame.at(1)	// trx
						, std::get<4>(tpl)	//	server ID
						, OBIS_ATTENTION_NOT_AUTHORIZED.to_buffer()
						, "login failed"
						, cyng::tuple_t());

					CYNG_LOG_WARNING(logger_, "sml.public.open.request - login failed: "
						<< std::get<6>(tpl)
						<< ":"
						<< std::get<7>(tpl))
						;
					return;
				}
			}

			//
			//	linearize and set CRC16
			//	append to current SML message
			//
			sml_gen_.public_open(frame.at(1)	// trx
				, frame.at(3)	//	client id
				, frame.at(5)	//	req file id
				, frame.at(4));
		}

		void kernel::sml_public_close_request(cyng::context& ctx)
		{
			//	[5fb21ea1-ac66-4a00-98b7-88edd57addb7,180523152649286995-4,3]
			//
			//	* pk
			//	* transaction id
			//	* SML message id
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_DEBUG(logger_, "sml.public.close.request " << cyng::io::to_str(frame));

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,	//	[0] pk
				std::string,		//	[1] trx
				std::size_t			//	[2] msg id
			>(frame);

			//	sml.public.close.request - trx: 180523152649286995-4, msg id:
			CYNG_LOG_INFO(logger_, "sml.public.close.request - trx: "
				<< std::get<1>(tpl)
				<< ", msg id: "
				<< std::get<2>(tpl))
				;

			//
			//	linearize and set CRC16
			//	append to current SML message
			//
			sml_gen_.public_close(frame.at(1));
		}

		void kernel::sml_public_close_response(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_DEBUG(logger_, "sml.public.close.response " << cyng::io::to_str(frame));

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,	//	[0] pk
				std::string,		//	[1] trx
				std::size_t			//	[2] msg id
			>(frame);

			//	sml.public.close.response - trx: 180523152649286995-4, msg id:
			CYNG_LOG_INFO(logger_, "sml.public.close.response - trx: "
				<< std::get<1>(tpl)
				<< ", msg id: "
				<< std::get<2>(tpl))
				;

		}

		void kernel::sml_get_proc_parameter_request(cyng::context& ctx)
		{
			//	[b5cfc8a0-0bf4-4afe-9598-ded99f71469c,180301094328243436-3,2,05:00:15:3b:02:23:b3,operator,operator,81 81 c7 82 01 ff,null]
			//	[50cfab74-eef0-4c92-89d4-46b28ab9da20,180522233943303816-2,1,00:15:3b:02:29:7e,operator,operator,81 00 60 05 00 00,null]
			//
			//	* pk
			//	* transaction id
			//	* SML message id
			//	* server ID
			//	* username
			//	* password
			//	* OBIS (parameterTreePath)
			//	* attribute (should be null)
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_TRACE(logger_, "sml.get.proc.parameter.request " << cyng::io::to_str(frame));

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,	//	[0] pk
				std::string,		//	[1] trx
				std::size_t,		//	[2] msg id
				cyng::buffer_t,		//	[3] server id
				std::string,		//	[4] user
				std::string,		//	[5] password
				cyng::buffer_t		//	[6] path (OBIS)
			>(frame);

			const obis code(std::get<6>(tpl));

			if (OBIS_CLASS_OP_LOG_STATUS_WORD == code)
			{
				std::uint64_t status = status_word_.operator std::uint64_t();
				CYNG_LOG_DEBUG(logger_, "status word: " << status);

				sml_gen_.get_proc_parameter_status_word(frame.at(1)
					, frame.at(3)
					, status);
			}
			else if (OBIS_CODE_ROOT_DEVICE_IDENT == code)
			{
				sml_gen_.get_proc_parameter_device_id(frame.at(1)
					, frame.at(3)	//	server id
					, manufacturer_
					, server_id_
					, "VSES-1KW-221-1F0"
					, serial_);
			}
			else if (OBIS_CODE_ROOT_DEVICE_TIME == code)
			{
				const std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
				const std::int32_t tz = 60;
				const bool sync_active = true;

				sml_gen_.get_proc_device_time(frame.at(1)
					, frame.at(3)	//	server id
					, now
					, tz
					, sync_active);
			}
			else if (OBIS_CODE_ROOT_NTP == code)
			{
				sml_get_proc_ntp_config(frame.at(1), frame.at(3));
			}
			else if (OBIS_CODE_ROOT_ACCESS_RIGHTS == code)
			{
				//
				//	ToDo: implement
				//
				CYNG_LOG_WARNING(logger_, "sml.get.proc.parameter.request - OBIS_CODE_ROOT_ACCESS_RIGHTS not implemented yet "
					<< cyng::io::to_hex(code.to_buffer()));
				sml_gen_.empty(frame.at(1)
					, frame.at(3)	//	server id
					, OBIS_CODE_ROOT_ACCESS_RIGHTS);
			}
			else if (OBIS_CODE_ROOT_CUSTOM_INTERFACE == code)
			{
				//
				//	ToDo: implement
				//
				CYNG_LOG_WARNING(logger_, "sml.get.proc.parameter.request - OBIS_CODE_ROOT_CUSTOM_INTERFACE not implemented yet "
					<< cyng::io::to_hex(code.to_buffer()));
				sml_gen_.empty(frame.at(1)
					, frame.at(3)	//	server id
					, OBIS_CODE_ROOT_CUSTOM_INTERFACE);
			}
			else if (OBIS_CODE_ROOT_CUSTOM_PARAM == code)
			{
				//
				//	ToDo: implement
				//
				CYNG_LOG_WARNING(logger_, "sml.get.proc.parameter.request - OBIS_CODE_ROOT_CUSTOM_PARAM not implemented yet "
					<< cyng::io::to_hex(code.to_buffer()));
				sml_gen_.empty(frame.at(1)
					, frame.at(3)	//	server id
					, OBIS_CODE_ROOT_CUSTOM_PARAM);
			}
			else if (OBIS_CODE_ROOT_WAN == code)
			{
				//
				//	ToDo: implement
				//
				CYNG_LOG_WARNING(logger_, "sml.get.proc.parameter.request - OBIS_CODE_ROOT_WAN not implemented yet "
					<< cyng::io::to_hex(code.to_buffer()));
				sml_gen_.empty(frame.at(1)
					, frame.at(3)	//	server id
					, OBIS_CODE_ROOT_WAN);
			}
			else if (OBIS_CODE_ROOT_GSM == code)
			{
				//
				//	ToDo: implement
				//
				CYNG_LOG_WARNING(logger_, "sml.get.proc.parameter.request - OBIS_CODE_ROOT_GSM not implemented yet "
					<< cyng::io::to_hex(code.to_buffer()));
				sml_gen_.empty(frame.at(1)
					, frame.at(3)	//	server id
					, OBIS_CODE_ROOT_GSM);
			}
			else if (OBIS_CODE_ROOT_IPT_STATE == code)
			{
				sml_get_proc_ipt_state(frame.at(1), frame.at(3));
			}
			else if (OBIS_CODE_ROOT_IPT_PARAM == code)
			{
				sml_gen_.get_proc_ipt_params(frame.at(1)
					, frame.at(3)	//	server id
					, cfg_ipt_);
			}
			else if (OBIS_CODE_ROOT_GPRS_PARAM == code)
			{
				//
				//	ToDo: implement
				//
				CYNG_LOG_WARNING(logger_, "sml.get.proc.parameter.request - OBIS_CODE_ROOT_GPRS_PARAM not implemented yet "
					<< cyng::io::to_hex(code.to_buffer()));
				sml_gen_.empty(frame.at(1)
					, frame.at(3)	//	server id
					, OBIS_CODE_ROOT_GPRS_PARAM);
			}
			else if (OBIS_CODE_ROOT_W_MBUS_STATUS == code)
			{
				sml_gen_.get_proc_w_mbus_status(frame.at(1)
					, frame.at(3)	//	server id
					//, "RC1180-MBUS3"	// manufacturer of w-mbus adapter
					, "solos::Tec"	// manufacturer of w-mbus adapter
					, cyng::make_buffer({ 0xA8, 0x15, 0x34, 0x83, 0x40, 0x04, 0x01, 0x31 })	//	adapter id (EN 13757-3/4)
					, NODE_VERSION	//	firmware version of adapter
					, "2.00"	//	hardware version of adapter
				);
			}
			else if (OBIS_CODE_IF_wMBUS == code)
			{
				config_db_.access([&](cyng::store::table const* tbl_cfg) {
					
					sml_gen_.get_proc_w_mbus_if(frame.at(1)
						, frame.at(3)	//	server id
						, tbl_cfg->lookup(cyng::table::key_generator(sml::OBIS_W_MBUS_PROTOCOL.to_str()), "value")	//	protocol
						, tbl_cfg->lookup(cyng::table::key_generator(sml::OBIS_W_MBUS_MODE_S.to_str()), "value")	//	duration in seconds
						, tbl_cfg->lookup(cyng::table::key_generator(sml::OBIS_W_MBUS_MODE_T.to_str()), "value")	//	duration in seconds
						, tbl_cfg->lookup(cyng::table::key_generator(sml::OBIS_W_MBUS_REBOOT.to_str()), "value")	//	duration in seconds
						, tbl_cfg->lookup(cyng::table::key_generator(sml::OBIS_W_MBUS_POWER.to_str()), "value")	//	transmision power (transmission_power)
						, tbl_cfg->lookup(cyng::table::key_generator(sml::OBIS_W_MBUS_INSTALL_MODE.to_str()), "value")
					);
				}, cyng::store::read_access("_Config"));
			}
			else if (OBIS_CODE_ROOT_LAN_DSL == code)
			{
				//
				//	ToDo: implement
				//
				CYNG_LOG_WARNING(logger_, "sml.get.proc.parameter.request - OBIS_CODE_ROOT_LAN_DSL not implemented yet");
				sml_gen_.empty(frame.at(1)
					, frame.at(3)	//	server id
					, OBIS_CODE_ROOT_LAN_DSL);
			}
			else if (OBIS_CODE_IF_LAN_DSL == code)
			{
				//
				//	ToDo: implement
				//
				CYNG_LOG_WARNING(logger_, "sml.get.proc.parameter.request - OBIS_CODE_IF_LAN_DSL not implemented yet "
					<< cyng::io::to_hex(code.to_buffer()));
				sml_gen_.empty(frame.at(1)
					, frame.at(3)	//	server id
					, OBIS_CODE_IF_LAN_DSL);
			}
			else if (OBIS_CODE_ROOT_MEMORY_USAGE == code)
			{
				CYNG_LOG_INFO(logger_, "get_used_physical_memory " << cyng::sys::get_used_physical_memory());
				CYNG_LOG_INFO(logger_, "get_total_physical_memory " << cyng::sys::get_total_physical_memory());
				CYNG_LOG_INFO(logger_, "get_used_virtual_memory " << cyng::sys::get_used_virtual_memory());
				CYNG_LOG_INFO(logger_, "get_total_virtual_memory " << cyng::sys::get_total_virtual_memory());

				//	m2m::OBIS_CODE_ROOT_MEMORY_USAGE
				const std::uint8_t mirror = cyng::sys::get_used_virtual_memory_in_percent()
					, tmp = cyng::sys::get_used_physical_memory_in_percent();

				sml_gen_.get_proc_mem_usage(frame.at(1)
					, frame.at(3)	//	server id
					, mirror
					, tmp);
			}
			else if (OBIS_CODE_ROOT_ACTIVE_DEVICES == code)
			{
				config_db_.access([&](const cyng::store::table* tbl) {

					CYNG_LOG_INFO(logger_, tbl->size() << " devices");
					sml_gen_.get_proc_active_devices(frame.at(1)
						, frame.at(3)	//	server id
						, tbl);

				}, cyng::store::read_access("mbus-devices"));
			}
			else if (OBIS_CODE_ROOT_VISIBLE_DEVICES == code)
			{
				config_db_.access([&](const cyng::store::table* tbl) {

					CYNG_LOG_INFO(logger_, tbl->size() << " devices");
					sml_gen_.get_proc_visible_devices(frame.at(1)
						, frame.at(3)	//	server id
						, tbl);

				}, cyng::store::read_access("mbus-devices"));
			}
			else if (OBIS_CODE_ROOT_DEVICE_INFO == code)
			{
				//
				//	ToDo: implement
				//
				CYNG_LOG_WARNING(logger_, "sml.get.proc.parameter.request - OBIS_CODE_ROOT_DEVICE_INFO not implemented yet "
					<< cyng::io::to_hex(code.to_buffer()));
				sml_gen_.empty(frame.at(1)
					, frame.at(3)	//	server id
					, OBIS_CODE_ROOT_DEVICE_INFO);
			}
			else if (OBIS_CODE_ROOT_SENSOR_PROPERTY == code)
			{
				config_db_.access([&](const cyng::store::table* tbl) {

					CYNG_LOG_INFO(logger_, tbl->size() << " devices");
					auto rec = tbl->lookup(cyng::table::key_generator(frame.at(3)));
					if (rec.empty())
					{
						sml_gen_.empty(frame.at(1)
							, frame.at(3)	//	server id
							, OBIS_CODE_ROOT_SENSOR_PROPERTY);
					}
					else
					{
						sml_gen_.get_proc_sensor_property(frame.at(1)
							, frame.at(3)	//	server id
							, rec);
					}
				}, cyng::store::read_access("mbus-devices"));
			}
			else if (OBIS_CODE_ROOT_DATA_COLLECTOR == code)
			{
				sml_get_proc_data_collector(frame.at(1), frame.at(3));
			}
			else if (OBIS_CODE_IF_1107 == code)
			{
				//
				//	deliver IEC 62056-21 (aka interface 1107) configuration and all defined meters
				//
				config_db_.access([&](cyng::store::table const* tbl_cfg, cyng::store::table const* tbl_dev) {

					CYNG_LOG_INFO(logger_, tbl_dev->size() << " devices");

					sml_gen_.get_proc_1107_if(frame.at(1)
						, frame.at(3)	//	server id
						, tbl_dev
						, tbl_cfg->lookup(cyng::table::key_generator(sml::OBIS_CODE_IF_1107_ACTIVE.to_str()), "value")
						, tbl_cfg->lookup(cyng::table::key_generator(sml::OBIS_CODE_IF_1107_LOOP_TIME.to_str()), "value")
						, tbl_cfg->lookup(cyng::table::key_generator(sml::OBIS_CODE_IF_1107_RETRIES.to_str()), "value")
						, tbl_cfg->lookup(cyng::table::key_generator(sml::OBIS_CODE_IF_1107_MIN_TIMEOUT.to_str()), "value")
						, tbl_cfg->lookup(cyng::table::key_generator(sml::OBIS_CODE_IF_1107_MAX_TIMEOUT.to_str()), "value")
						, tbl_cfg->lookup(cyng::table::key_generator(sml::OBIS_CODE_IF_1107_MAX_DATA_RATE.to_str()), "value")
						, tbl_cfg->lookup(cyng::table::key_generator(sml::OBIS_CODE_IF_1107_RS485.to_str()), "value")
						, tbl_cfg->lookup(cyng::table::key_generator(sml::OBIS_CODE_IF_1107_PROTOCOL_MODE.to_str()), "value")
						, tbl_cfg->lookup(cyng::table::key_generator(sml::OBIS_CODE_IF_1107_AUTO_ACTIVATION.to_str()), "value")
						, tbl_cfg->lookup(cyng::table::key_generator(sml::OBIS_CODE_IF_1107_TIME_GRID.to_str()), "value")
						, tbl_cfg->lookup(cyng::table::key_generator(sml::OBIS_CODE_IF_1107_TIME_SYNC.to_str()), "value")
						, tbl_cfg->lookup(cyng::table::key_generator(sml::OBIS_CODE_IF_1107_MAX_VARIATION.to_str()), "value")
					);


				}	, cyng::store::read_access("_Config")
					, cyng::store::read_access("iec62056-21-devices"));
			}
			else if (OBIS_CODE(00, 80, 80, 00, 00, FF) == code)
			{
				sml_gen_.get_proc_0080800000FF(frame.at(1)
					, frame.at(3)	//	server id is gateway MAC
					, 0);

			}
			else if (OBIS_PUSH_OPERATIONS == code)
			{
				//
				//	service push
				//
				config_db_.access([&](const cyng::store::table* tbl) {

					CYNG_LOG_INFO(logger_, tbl->size() << " push.ops");
					sml_gen_.get_proc_push_ops(frame.at(1)
						, frame.at(3)	//	server id
						, tbl);

				}, cyng::store::read_access("push.ops"));
			}
			//0-0:96.8.0*255 - CONFIG_OVERVIEW
			//else if (OBIS_CODE(00, 00, 60, 08, 00, FF) == code)	
			else if (OBIS_CODE(99, 00, 00, 00, 00, 04) == code)
			{
				sml_gen_.get_proc_990000000004(frame.at(1)
					, frame.at(3)	//	server id is gateway MAC
					, "smfService:smfConfiguration-001;");
			}
			else if (OBIS_ACTUATORS == code)
			{
				//
				//	generate some random values
				//
				sml_gen_.get_proc_actuators(frame.at(1)
					, frame.at(3));	//	server id

			}
			else if (OBIS_CODE_IF_EDL == code)
			{
				CYNG_LOG_WARNING(logger_, "sml.get.proc.parameter.request - OBIS_CODE_IF_EDL not implemented yet "
					<< cyng::io::to_hex(code.to_buffer()));

				sml_gen_.empty(frame.at(1)
					, frame.at(3)	//	server id
					, OBIS_CODE_IF_EDL);
			}
			else if (OBIS_CLASS_MBUS == code)
			{
				CYNG_LOG_WARNING(logger_, "sml.get.proc.parameter.request - OBIS_CLASS_MBUS not implemented yet "
					<< cyng::io::to_hex(code.to_buffer()));

				sml_gen_.empty(frame.at(1)
					, frame.at(3)	//	server id
					, OBIS_CLASS_MBUS);
			}
			else {
				CYNG_LOG_ERROR(logger_, "sml.get.proc.parameter.request - unknown OBIS code " 
					<< to_string(code) 
					<< " / " 
					<< cyng::io::to_hex(code.to_buffer()));

			}
		}


		void kernel::sml_get_profile_list_request(cyng::context& ctx) 
		{
			//	[7068ce46-00b2-4f73-b3fa-72c3c965354c,180905155846820378-2,1,005056C00008,0500FFB00BCAAE,operator,operator,2018-09-04 13:58:41.00000000,2018-09-05 13:58:41.00000000,8181C789E1FF]
			//	[cfdfbf83-b7f0-4b4e-94de-fcb196179fbb,181118193517779935-2,1,005056C00008,01A815743145050102,operator,operator,1970-01-01 00:00:00.00000000,2018-11-18 18:35:17.00000000,8181C78610FF]
			//	
			//	* pk
			//	* transaction id
			//	* SML message id
			//	* client ID
			//	* server ID
			//	* username
			//	* password
			//	* start time
			//	* end time
			//	* OBIS (requested parameter)
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_TRACE(logger_, "sml.get.profile.list.request " << cyng::io::to_str(frame));

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,	//	[0] pk
				std::string,		//	[1] trx
				std::size_t,		//	[2] msg id
				cyng::buffer_t,		//	[3] client id
				cyng::buffer_t,		//	[4] server id
				std::string,		//	[5] user
				std::string,		//	[6] password
				std::chrono::system_clock::time_point,		//	[7] start time
				std::chrono::system_clock::time_point,		//	[8] end time
				cyng::buffer_t		//	[9] path (OBIS)
			>(frame);

			const obis code(std::get<9>(tpl));

			if (OBIS_CLASS_OP_LOG == code) 	{

				//
				//	send operation logs
				//

				config_db_.access([&](cyng::store::table const* tbl) {

					CYNG_LOG_INFO(logger_, "send " << tbl->size() << " op.log(s)");

					sml_gen_.get_profile_op_logs(frame.at(1)
						, frame.at(3)	//	server id
						, std::get<7>(tpl)
						, std::get<8>(tpl)
						, tbl);
				}, cyng::store::read_access("op.log"));

			}
			else if (OBIS_PROFILE_1_MINUTE == code) {
				CYNG_LOG_WARNING(logger_, "OBIS_PROFILE_1_MINUTE not implemented yet");
			}
			else if (OBIS_PROFILE_15_MINUTE == code) {
				CYNG_LOG_WARNING(logger_, "OBIS_PROFILE_15_MINUTE not implemented yet");
			}
			else if (OBIS_PROFILE_60_MINUTE == code) {
				CYNG_LOG_WARNING(logger_, "OBIS_PROFILE_60_MINUTE not implemented yet");
			}
			else if (OBIS_PROFILE_24_HOUR == code) {
				CYNG_LOG_WARNING(logger_, "OBIS_PROFILE_24_HOUR not implemented yet");
			}
			else {
				CYNG_LOG_ERROR(logger_, "sml.get.profile.list.request - unknown profile" << to_string(code));
			}
		}

		void kernel::sml_get_proc_ntp_config(cyng::object trx, cyng::object server)
		{
			//	[fd167248-a823-46d6-ba0a-03157a28a104,1805241520113192-3,2,0500153B02297E,operator,operator]
			//
			//	* pk
			//	* transaction id
			//	* SML message id
			//	* server ID
			//	* username
			//	* password
			//	* OBIS (requested parameter)
			//const cyng::vector_t frame = ctx.get_frame();
			//CYNG_LOG_INFO(logger_, "sml.get.proc.ntp.config " << cyng::io::to_str(frame));

			//76                                                SML_Message(Sequence): 
			//  81063137303531323136353635313836333431352D32    transactionId: 170512165651863415-2
			//  6201                                            groupNo: 1
			//  6200                                            abortOnError: 0
			//  72                                              messageBody(Choice): 
			//	630501                                        messageBody: 1281 => SML_GetProcParameter_Res (0x00000501)
			//	73                                            SML_GetProcParameter_Res(Sequence): 
			//	  080500153B01EC46                            serverId: 05 00 15 3B 01 EC 46 
			//	  71                                          parameterTreePath(SequenceOf): 
			//		078181C78801FF                            path_Entry: 81 81 C7 88 01 FF 
			//	  73                                          parameterTree(Sequence): 
			//		078181C78801FF                            parameterName: 81 81 C7 88 01 FF 
			//		01                                        parameterValue: not set
			//		74                                        child_List(SequenceOf): 
			//		  73                                      tree_Entry(Sequence): 
			//			078181C78802FF                        parameterName: 81 81 C7 88 02 FF 
			//			01                                    parameterValue: not set
			//			73                                    child_List(SequenceOf): 
			//			  73                                  tree_Entry(Sequence): 
			//				078181C7880201                    parameterName: 81 81 C7 88 02 01 
			//				72                                parameterValue(Choice): 
			//				  6201                            parameterValue: 1 => smlValue (0x01)
			//				  810170746274696D65312E7074622E6465smlValue: ptbtime1.ptb.de
			//				01                                child_List: not set
			//			  73                                  tree_Entry(Sequence): 
			//				078181C7880202                    parameterName: 81 81 C7 88 02 02 
			//				72                                parameterValue(Choice): 
			//				  6201                            parameterValue: 1 => smlValue (0x01)
			//				  810170746274696D65322E7074622E6465smlValue: ptbtime2.ptb.de
			//				01                                child_List: not set
			//			  73                                  tree_Entry(Sequence): 
			//				078181C7880203                    parameterName: 81 81 C7 88 02 03 
			//				72                                parameterValue(Choice): 
			//				  6201                            parameterValue: 1 => smlValue (0x01)
			//				  810170746274696D65332E7074622E6465smlValue: ptbtime3.ptb.de
			//				01                                child_List: not set
			//		  73                                      tree_Entry(Sequence): 
			//			078181C78803FF                        parameterName: 81 81 C7 88 03 FF 
			//			72                                    parameterValue(Choice): 
			//			  6201                                parameterValue: 1 => smlValue (0x01)
			//			  627B                                smlValue: 123
			//			01                                    child_List: not set
			//		  73                                      tree_Entry(Sequence): 
			//			078181C78806FF                        parameterName: 81 81 C7 88 06 FF 
			//			72                                    parameterValue(Choice): 
			//			  6201                                parameterValue: 1 => smlValue (0x01)
			//			  4201                                smlValue: True
			//			01                                    child_List: not set
			//		  73                                      tree_Entry(Sequence): 
			//			078181C78804FF                        parameterName: 81 81 C7 88 04 FF 
			//			72                                    parameterValue(Choice): 
			//			  6201                                parameterValue: 1 => smlValue (0x01)
			//			  630E10                              smlValue: 3600
			//			01                                    child_List: not set
			//  638154                                        crc16: 33108
			//  00                                            endOfSmlMsg: 00 

			const std::string ntp_primary = "pbtime1.pbt.de", ntp_secondary = "pbtime2.pbt.de", ntp_tertiary = "pbtime3.pbt.de";
			const std::uint16_t ntp_port = 123;	// 123;
			const bool ntp_active = true;
			const std::uint16_t ntp_tz = 3600;

			sml_gen_.append_msg(message(trx	//	trx
				, 2 //, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(server	//	server id
					, OBIS_CODE_ROOT_NTP	//	path entry

					//
					//	generate get process parameter response
					//
					, child_list_tree(OBIS_CODE_ROOT_NTP, {

						//	NTP servers
						child_list_tree(OBIS_CODE(81, 81, C7, 88, 02, FF),{
							parameter_tree(OBIS_CODE(81, 81, C7, 88, 02, 01), make_value(ntp_primary)),
							parameter_tree(OBIS_CODE(81, 81, C7, 88, 02, 02), make_value(ntp_secondary)),
							parameter_tree(OBIS_CODE(81, 81, C7, 88, 02, 03), make_value(ntp_tertiary))
							}),
						parameter_tree(OBIS_CODE(81, 81, C7, 88, 03, FF), make_value(ntp_port)),	//	NTP port
						parameter_tree(OBIS_CODE(81, 81, C7, 88, 06, FF), make_value(ntp_active)),	// enabled/disabled
						parameter_tree(OBIS_CODE(81, 81, C7, 88, 04, FF), make_value(ntp_tz))	// timezone
					}))));

		}


		void kernel::sml_get_proc_ipt_state(cyng::object trx, cyng::object server)
		{
			//	[fd167248-a823-46d6-ba0a-03157a28a104,1805241520113192-3,2,0500153B02297E,operator,operator]
			//
			//	* pk
			//	* transaction id
			//	* SML message id
			//	* server ID
			//	* username
			//	* password
			//	* OBIS (requested parameter)
			//const cyng::vector_t frame = ctx.get_frame();
			//CYNG_LOG_INFO(logger_, "sml.get.proc.ipt.state " << cyng::io::to_str(frame));

			//
			//	get TCP/IP endpoint from _Config table
			//
			auto const obj_rem = config_db_.get_value("_Config", "value", std::string("remote.ep"));
			auto const obj_loc = config_db_.get_value("_Config", "value", std::string("local.ep"));

			CYNG_LOG_INFO(logger_, "remote endpoint " << cyng::io::to_str(obj_rem));

			boost::asio::ip::tcp::endpoint rep;
			rep = cyng::value_cast(obj_rem, rep);

			boost::asio::ip::tcp::endpoint lep;
			lep = cyng::value_cast(obj_loc, lep);

			//
			//	generate response
			//
			sml_gen_.get_proc_parameter_ipt_state(trx
				, server
				, rep
				, lep);
		}



		void kernel::sml_get_proc_data_collector(cyng::object trx, cyng::object server)
		{
			//	### Message ###
			//	76                                                SML_Message(Sequence): 
			//	  81063138303631393139343334353337383434352D32    transactionId: 180619194345378445-2
			//	  6201                                            groupNo: 1
			//	  6200                                            abortOnError: 0
			//	  72                                              messageBody(Choice): 
			//		630501                                        messageBody: 1281 => SML_GetProcParameter_Res (0x00000501)
			//		73                                            SML_GetProcParameter_Res(Sequence): 
			//		  0A01E61E130900163C07                        serverId: 01 E6 1E 13 09 00 16 3C 07 
			//		  71                                          parameterTreePath(SequenceOf): 
			//			078181C78620FF                            path_Entry: ____ _
			//		  73                                          parameterTree(Sequence): 
			//			078181C78620FF                            parameterName: ____ _
			//			01                                        parameterValue: not set
			//			71                                        child_List(SequenceOf): 
			//			  73                                      tree_Entry(Sequence): 
			//				078181C7862001                        parameterName: 81 81 C7 86 20 01 
			//				01                                    parameterValue: not set
			//				75                                    child_List(SequenceOf): 
			//				  73                                  tree_Entry(Sequence): 
			//					078181C78621FF                    parameterName: ____!_
			//					72                                parameterValue(Choice): 
			//					  6201                            parameterValue: 1 => smlValue (0x01)
			//					  4201                            smlValue: True
			//					01                                child_List: not set
			//				  73                                  tree_Entry(Sequence): 
			//					078181C78622FF                    parameterName: ____"_
			//					72                                parameterValue(Choice): 
			//					  6201                            parameterValue: 1 => smlValue (0x01)
			//					  6264                            smlValue: 100
			//					01                                child_List: not set
			//				  73                                  tree_Entry(Sequence): 
			//					078181C78781FF                    parameterName: ______
			//					72                                parameterValue(Choice): 
			//					  6201                            parameterValue: 1 => smlValue (0x01)
			//					  6200                            smlValue: 0
			//					01                                child_List: not set
			//				  73                                  tree_Entry(Sequence): 
			//					078181C78A83FF                    parameterName: ______
			//					72                                parameterValue(Choice): 
			//					  6201                            parameterValue: 1 => smlValue (0x01)
			//					  078181C78611FF                  smlValue: 81 81 C7 86 11 FF 
			//					01                                child_List: not set
			//				  73                                  tree_Entry(Sequence): 
			//					078181C78A23FF                    parameterName: ____#_
			//					01                                parameterValue: not set
			//					71                                child_List(SequenceOf): 
			//					  73                              tree_Entry(Sequence): 
			//						078181C78A2301                parameterName: 81 81 C7 8A 23 01 
			//						72                            parameterValue(Choice): 
			//						  6201                        parameterValue: 1 => smlValue (0x01)
			//						  070800010000FF              smlValue: 08 00 01 00 00 FF 
			//						01                            child_List: not set
			//	  63CBE4                                          crc16: 52196
			//	  00                                              endOfSmlMsg: 00 

			//	ClientId: 05 00 15 3B 02 29 7E 
			//	ServerId: 01 E6 1E 13 09 00 16 3C 07 
			//	81 81 C7 86 20 FF                Not set
			//	   81 81 C7 86 20 01             Not set
			//		  81 81 C7 86 21 FF          True (54 72 75 65 )	//	active
			//		  81 81 C7 86 22 FF          100 (31 30 30 )		//	Einträge
			//		  81 81 C7 87 81 FF          0 (30 )				//	Registerperiode
			//		  81 81 C7 8A 83 FF          ______ (81 81 C7 86 11 FF )	//	OBIS
			//		  81 81 C7 8A 23 FF          Not set
			//			 81 81 C7 8A 23 01       ______ (08 00 01 00 00 FF )	Liste von Einträgen z.B. Zählerstand Wasser

			//
			//	Comes up when clicked "Datenspiegel"
			//

			sml_gen_.append_msg(message(trx	//	trx
				, 2 //, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(server	//	server id  
					, OBIS_CODE_ROOT_DATA_COLLECTOR	//	path entry - 81 81 C7 86 20 FF 
					, child_list_tree(OBIS_CODE_ROOT_DATA_COLLECTOR, {

						//	1. entry
						child_list_tree(OBIS_CODE(81, 81, C7, 86, 20, 01),{
							parameter_tree(OBIS_CODE(81, 81, C7, 86, 21, FF), make_value(true)),	//	active
							parameter_tree(OBIS_CODE(81, 81, C7, 86, 22, FF), make_value(100)),		//	Einträge
							parameter_tree(OBIS_CODE(81, 81, C7, 87, 81, FF), make_value(0)),		//	Registerperiode
							//	15 min period
							parameter_tree(OBIS_CODE(81, 81, C7, 8A, 83, FF), make_value(OBIS_CODE(81, 81, C7, 86, 11, FF))),
							//	Liste von Einträgen z.B. 08 00 01 00 00 FF := Zählerstand Wasser
							child_list_tree(OBIS_CODE(81, 81, C7, 8A, 23, FF),{
								parameter_tree(OBIS_CODE(81, 81, C7, 8A, 23, 01), make_value(OBIS_CODE(08, 00, 01, 00, 00, FF))),
								parameter_tree(OBIS_CODE(81, 81, C7, 8A, 23, 02), make_value(OBIS_CODE(08, 00, 01, 02, 00, FF)))
								})
							}),

						}))));

		}


		void kernel::sml_set_proc_push_interval(cyng::context& ctx)
		{
			//	[a72a7054-f089-4df2-a592-3983935ba7df,180711174520423322-2,1,01A815743145050102,operator,operator,03c0]
			//
			//	* pk
			//	* transaction id
			//	* push index
			//	* server ID
			//	* username
			//	* password
			//	* push interval in seconds
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.set.proc.push.interval " << cyng::io::to_str(frame));

		}

		void kernel::sml_set_proc_push_delay(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.set.proc.push.delay " << cyng::io::to_str(frame));

		}

		void kernel::sml_set_proc_push_name(cyng::context& ctx)
		{
			//	[b5b5fe15-d43f-4d2a-953e-1e2e1d153d89,180711180023633812-3,2,01A815743145050102,operator,operator,new-target-name]
			//
			//	* pk
			//	* transaction id
			//	* [uint8] push index
			//	* server ID
			//	* username
			//	* password
			//	* [string] push target name
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.set.proc.push.name " << cyng::io::to_str(frame));

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,	//	[0] pk
				std::string,		//	[1] trx
				std::uint8_t,		//	[2] push index
				cyng::buffer_t,		//	[3] server ID
				std::string,		//	[4] username
				std::string,		//	[5] password
				std::string			//	[6] target name
			>(frame);

			config_db_.access([&](cyng::store::table* tbl) {

				CYNG_LOG_INFO(logger_, tbl->size() << " push.ops");
				cyng::table::record rec = tbl->find_first(cyng::param_t("serverID", frame.at(3)));
				if (!rec.empty()) {
					tbl->modify(rec.key(), cyng::param_t("target", frame.at(6)), std::get<0>(tpl));
				}


			}, cyng::store::write_access("push.ops"));
		}

		void kernel::sml_set_proc_ipt_param_address(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_TRACE(logger_, "sml.set.proc.ipt.param.address " << cyng::io::to_str(frame));

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,			//	[0] pk
				std::string,				//	[1] trx
				std::uint8_t,				//	[2] record index (0..1)
				cyng::buffer_t				//	[3] address
			>(frame);

			auto const idx = std::get<2>(tpl);
			if (idx < cfg_ipt_.config_.size()) {

				CYNG_LOG_INFO(logger_, " sml.set.proc.ipt.param.address["
					<< +idx
					<< "] "
					<< cfg_ipt_.config_.at(idx).host_
					<< " => "
					<< cyng::io::to_ascii(std::get<3>(tpl)));

				*const_cast<std::string*>(&const_cast<node::ipt::master_config_t&>(cfg_ipt_.config_).at(idx).host_) = cyng::io::to_ascii(std::get<3>(tpl));
			}
		}

		void kernel::sml_set_proc_ipt_param_port_target(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			//CYNG_LOG_TRACE(logger_, "sml.set.proc.ipt.param.port.target " << cyng::io::to_str(frame));

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,	//	[0] pk
				std::string,		//	[1] trx
				std::uint8_t,		//	[2] record index (1..2)
				std::uint16_t		//	[3] port
			>(frame);

			const auto idx = std::get<2>(tpl);
			if (idx < cfg_ipt_.config_.size()) {

				CYNG_LOG_INFO(logger_, " sml.set.proc.ipt.param.port.target["
					<< +idx
					<< "] "
					<< cfg_ipt_.config_.at(idx).service_
					<< " => "
					<< std::get<3>(tpl));

				*const_cast<std::string*>(&const_cast<node::ipt::master_config_t&>(cfg_ipt_.config_).at(idx).service_) = std::to_string(std::get<3>(tpl));
			}
		}

		void kernel::sml_set_proc_ipt_param_user(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			//CYNG_LOG_TRACE(logger_, "sml.set.proc.ipt.param.user " << cyng::io::to_str(frame));

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,	//	[0] pk
				std::string,		//	[1] trx
				std::uint8_t,		//	[2] record index (1..2)
				std::string			//	[3] user
			>(frame);

			const auto idx = std::get<2>(tpl);
			if (idx < cfg_ipt_.config_.size()) {

				CYNG_LOG_INFO(logger_, " sml.set.proc.ipt.param.user["
					<< +idx
					<< "] "
					<< cfg_ipt_.config_.at(idx).account_
					<< " => "
					<< std::get<3>(tpl));

				*const_cast<std::string*>(&const_cast<node::ipt::master_config_t&>(cfg_ipt_.config_).at(idx).account_) = std::get<3>(tpl);
			}
		}

		void kernel::sml_set_proc_ipt_param_pwd(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			//CYNG_LOG_TRACE(logger_, "sml.set.proc.ipt.param.pwd " << cyng::io::to_str(frame));

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,	//	[0] pk
				std::string,		//	[1] trx
				std::uint8_t,		//	[2] record index (1..2)
				std::string			//	[3] pwd
			>(frame);

			const auto idx = std::get<2>(tpl);
			if (idx < cfg_ipt_.config_.size()) {

				CYNG_LOG_INFO(logger_, " sml.set.proc.ipt.param.pwd["
					<< +idx
					<< "] "
					<< cfg_ipt_.config_.at(idx).pwd_
					<< " => "
					<< std::get<3>(tpl));

				*const_cast<std::string*>(&const_cast<node::ipt::master_config_t&>(cfg_ipt_.config_).at(idx).pwd_) = std::get<3>(tpl);
			}
		}

		void kernel::sml_set_proc_activate(cyng::context& ctx)
		{
			//	[3ad68eea-ac45-4349-96c6-d77f0a45a67d,190215220337893589-2,1,0500FFB04B94F8,operator,operator,01E61E733145040102]
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,	//	[0] pk
				std::string,		//	[1] trx
				std::uint8_t,		//	[2] record index (1..2)
				cyng::buffer_t,		//	[3] gateway id e.g 0500FFB04B94F8
				std::string,		//	[4] user name
				std::string,		//	[5] password
				cyng::buffer_t		//	[6] server/meter ID 
			>(frame);

			config_db_.access([&](cyng::store::table* tbl) {

				auto const rec = tbl->lookup(cyng::table::key_generator(std::get<6>(tpl)));
				if (!rec.empty()) {

					//
					//	set sensor/actor active
					//
					CYNG_LOG_INFO(logger_, "activate sensor: " << cyng::io::to_hex(std::get<6>(tpl)));
					tbl->modify(rec.key(), cyng::param_factory("active", true), ctx.tag());

					//
					//	send attention code ATTENTION_OK
					//
					sml_gen_.attention_msg(frame.at(1)	// trx
						, std::get<3>(tpl)	//	server ID
						, OBIS_ATTENTION_OK.to_buffer()
						, "OK"
						, cyng::tuple_t());

				}
				else {

					//
					//	send attention code OBIS_ATTENTION_NO_SERVER_ID
					//
					sml_gen_.attention_msg(frame.at(1)	// trx
						, std::get<3>(tpl)	//	server ID
						, OBIS_ATTENTION_NO_SERVER_ID.to_buffer()
						, ctx.get_name()
						, cyng::tuple_t());

					CYNG_LOG_WARNING(logger_, ctx.get_name() << " - wrong server ID: "
						<< cyng::io::to_hex(std::get<3>(tpl)))
						;
				}
			}, cyng::store::write_access("mbus-devices"));

		}

		void kernel::sml_set_proc_deactivate(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,	//	[0] pk
				std::string,		//	[1] trx
				std::uint8_t,		//	[2] record index (1..2)
				cyng::buffer_t,		//	[3] gateway id e.g 0500FFB04B94F8
				std::string,		//	[4] user name
				std::string,		//	[5] password
				cyng::buffer_t		//	[6] server/meter ID 
			>(frame);

			config_db_.access([&](cyng::store::table* tbl) {

				auto const rec = tbl->lookup(cyng::table::key_generator(std::get<6>(tpl)));
				if (!rec.empty()) {

					//
					//	set sensor/actor active
					//
					CYNG_LOG_INFO(logger_, "deactivate sensor: " << cyng::io::to_hex(std::get<6>(tpl)));
					tbl->modify(rec.key(), cyng::param_factory("active", false), ctx.tag());

					//
					//	send attention code ATTENTION_OK
					//
					sml_gen_.attention_msg(frame.at(1)	// trx
						, std::get<3>(tpl)	//	server ID
						, OBIS_ATTENTION_OK.to_buffer()
						, "OK"
						, cyng::tuple_t());

				}
				else {

					//
					//	send attention code OBIS_ATTENTION_NO_SERVER_ID
					//
					sml_gen_.attention_msg(frame.at(1)	// trx
						, std::get<3>(tpl)	//	server ID
						, OBIS_ATTENTION_NO_SERVER_ID.to_buffer()
						, ctx.get_name()
						, cyng::tuple_t());

					CYNG_LOG_WARNING(logger_, ctx.get_name() << " - wrong server ID: "
						<< cyng::io::to_hex(std::get<3>(tpl)))
						;
				}
			}, cyng::store::write_access("mbus-devices"));
		}

		void kernel::sml_set_proc_delete(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,	//	[0] pk
				std::string,		//	[1] trx
				std::uint8_t,		//	[2] record index (1..2)
				cyng::buffer_t,		//	[3] gateway id e.g 0500FFB04B94F8
				std::string,		//	[4] user name
				std::string,		//	[5] password
				cyng::buffer_t		//	[6] server/meter ID 
			>(frame);

			config_db_.access([&](cyng::store::table* tbl) {

				//
				//	remove sensor/actor
				//
				CYNG_LOG_WARNING(logger_, "delete sensor: " << cyng::io::to_hex(std::get<6>(tpl)));
				auto const key = cyng::table::key_generator(std::get<6>(tpl));
				if (tbl->erase(key, ctx.tag())) {

					//
					//	send attention code ATTENTION_OK
					//
					sml_gen_.attention_msg(frame.at(1)	// trx
						, std::get<3>(tpl)	//	server ID
						, OBIS_ATTENTION_OK.to_buffer()
						, "OK"
						, cyng::tuple_t());
				}
				else {

					//
					//	send attention code OBIS_ATTENTION_NO_SERVER_ID
					//
					sml_gen_.attention_msg(frame.at(1)	// trx
						, std::get<3>(tpl)	//	server ID
						, OBIS_ATTENTION_NO_SERVER_ID.to_buffer()
						, ctx.get_name()
						, cyng::tuple_t());

					CYNG_LOG_WARNING(logger_, ctx.get_name() << " - wrong server ID: "
						<< cyng::io::to_hex(std::get<3>(tpl)))
						;
				}
			}, cyng::store::write_access("mbus-devices"));
		}

		void kernel::sml_set_proc_if1107_param(cyng::context& ctx)
		{
			//	[eb672857-669e-41cd-8e19-0b0bb187dec4,19022815085510333-3,0500FFB04B94F8,operator,operator,8181C79305FF,138a]
			cyng::vector_t const frame = ctx.get_frame();
			CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

			config_db_.access([&](cyng::store::table* tbl) {
				cyng::buffer_t tmp;
				obis code(cyng::value_cast(frame.at(5), tmp));
				tbl->modify(cyng::table::key_generator(code.to_str()), cyng::param_t("value", frame.at(6)), ctx.tag());
			}, cyng::store::write_access("_Config"));

			//
			//	send attention code ATTENTION_OK
			//
			cyng::buffer_t tmp;
			sml_gen_.attention_msg(frame.at(1)	// trx
				, cyng::value_cast(frame.at(2), tmp)	//	server ID
				, OBIS_ATTENTION_OK.to_buffer()
				, "OK"
				, cyng::tuple_t());

		}

		void kernel::sml_set_proc_if1107_device(cyng::context& ctx)
		{
			//	 [ce4769ae-da55-4464-9a47-e65d14f1afb5,1903010920587636-2,2,0500FFB04B94F8,operator,operator,%(("8181C7930AFF":31454D4830303035353133383936),("8181C7930BFF":4b00),("8181C7930CFF":31454D4830303035353133383936),("8181C7930DFF":31323133),("8181C7930EFF":313233313233))]
			//	 [5e9e8ac8-be28-4284-ba3f-cae2d8151ace,190301092252659302-2,1,0500FFB04B94F8,operator,operator,%(("8181C7930EFF":3030303030303031))]
			cyng::vector_t const frame = ctx.get_frame();
			CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,	//	[0] pk
				std::string,		//	[1] trx
				std::uint8_t,		//	[2] record index (1..2)
				cyng::buffer_t,		//	[3] gateway id e.g 0500FFB04B94F8
				std::string,		//	[4] user name
				std::string,		//	[5] password
				cyng::param_map_t	//	[6] new/mofified parameters
			>(frame);

			config_db_.access([&](cyng::store::table* tbl) {
				if (std::get<2>(tpl) > tbl->size()) {

					//
					//	new device
					//
					cyng::buffer_t tmp;
					auto const reader = cyng::make_reader(std::get<6>(tpl));
					auto const meter = cyng::value_cast(reader.get(OBIS_CODE_IF_1107_METER_ID.to_str()), tmp);
					auto const address = cyng::value_cast(reader.get(OBIS_CODE_IF_1107_ADDRESS.to_str()), tmp);
					auto const baudrate = cyng::numeric_cast(reader.get(OBIS_CODE_IF_1107_BAUDRATE.to_str()), 9600u);
					auto const p1 = cyng::value_cast(reader.get(OBIS_CODE_IF_1107_P1.to_str()), tmp);
					auto const w5 = cyng::value_cast(reader.get(OBIS_CODE_IF_1107_W5.to_str()), tmp);

					auto const b = tbl->insert(cyng::table::key_generator(meter)
						, cyng::table::data_generator(address
							, cyng::to_str(std::chrono::system_clock::now())
							, baudrate
							, std::string(p1.begin(), p1.end())
							, std::string(w5.begin(), w5.end()))
						, 1	//	generation
						, ctx.tag());

				}
				else {

					//
					//	modify device
					//	locate device with the specified index
					//	
					//	ToDo: use an u8 data type as index
					//	Currently we cannot modify the meterId since this is the table index
					//
					auto const rec = tbl->nth_record(std::get<2>(tpl) - 1);
					for (auto const& p : std::get<6>(tpl)) {
						if (p.first == OBIS_CODE_IF_1107_ADDRESS.to_str()) {
							tbl->modify(rec.key(), cyng::param_t("address", p.second), ctx.tag());
						}
						else if (p.first == OBIS_CODE_IF_1107_BAUDRATE.to_str()) {
							tbl->modify(rec.key(), cyng::param_t("baudrate", p.second), ctx.tag());
						}
						else if (p.first == OBIS_CODE_IF_1107_P1.to_str()) {
							tbl->modify(rec.key(), cyng::param_t("p1", p.second), ctx.tag());
						}
						else if (p.first == OBIS_CODE_IF_1107_W5.to_str()) {
							tbl->modify(rec.key(), cyng::param_t("w5", p.second), ctx.tag());
						}
					}
				}
			}, cyng::store::write_access("iec62056-21-devices"));

			//
			//	send attention code ATTENTION_OK
			//
			cyng::buffer_t tmp;
			sml_gen_.attention_msg(frame.at(1)	// trx
				, cyng::value_cast(frame.at(2), tmp)	//	server ID
				, OBIS_ATTENTION_OK.to_buffer()
				, "OK"
				, cyng::tuple_t());

		}

		void kernel::sml_set_proc_mbus_param(cyng::context& ctx)
		{
			//	[828497da-d0d5-4e55-8b2d-8bd4d8c4c6e0,0343476-2,0500FFB04B94F8,OBIS,operator,operator,0001517f]
			cyng::vector_t const frame = ctx.get_frame();
			CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

			//
			//	get OBIS code
			//
			cyng::buffer_t tmp;
			obis const code(cyng::value_cast(frame.at(3), tmp));

			//
			//	update config db
			//
			config_db_.modify("_Config", cyng::table::key_generator(code.to_str()), cyng::param_t("value", frame.at(6)), ctx.tag());
		}

		void kernel::sml_get_list_request(cyng::context& ctx)
		{
			cyng::vector_t const frame = ctx.get_frame();
			CYNG_LOG_TRACE(logger_, "sml.get.list.request " << cyng::io::to_str(frame));

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,	//	[0] pk
				std::string,		//	[1] trx
				std::size_t,		//	[2] msg id
				cyng::buffer_t,		//	[3] client id
				cyng::buffer_t,		//	[4] server id
				std::string,		//	[5] reqFileId
				std::string,		//	[6] user
				std::string,		//	[7] password
				cyng::buffer_t		//	[8] path (OBIS)
			>(frame);

			const obis code(std::get<8>(tpl));

			cyng::tuple_t val_list;
			val_list.push_back(list_entry_manufacturer("solosTec"));
			val_list.push_back(list_entry(OBIS_CODE(08, 00, 01, 00, 00, FF)
				, 0	//	status
				, std::chrono::system_clock::now()
				, 13	//	m3
				, -3
				, cyng::make_object(758)));
			val_list.push_back(list_entry(OBIS_CODE(01, 00, 01, 08, 00, FF)
				, 0	//	status
				, std::chrono::system_clock::now()
				, 30	//	Wh
				, -2
				, cyng::make_object(463782u)));

			if (OBIS_LIST_CURRENT_DATA_RECORD == code) {
				CYNG_LOG_TRACE(logger_, "sml.get.list.request - current data record");
				sml_gen_.get_list(frame.at(1)	//	trx
					, std::get<3>(tpl)	//	client id
					, std::get<4>(tpl)	//	server id
					, OBIS_LIST_CURRENT_DATA_RECORD
					, make_timestamp(std::chrono::system_clock::now())
					, make_timestamp(std::chrono::system_clock::now())
					, val_list);

			}
			else {
				CYNG_LOG_WARNING(logger_, "sml.get.list.request - " << get_name(code));

			}
		}

		void kernel::sml_get_list_response(cyng::context& ctx)
		{
			//	[b583e91b-14f5-4691-808a-5b0a517eb1d6,7531511-2,0,,01E61E130900163C07,%(("08 00 01 00 00 ff":0.758),("08 00 01 02 00 ff":0.758)),null,06975265]
			cyng::vector_t const frame = ctx.get_frame();
			CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));
		}

		void kernel::wmbus_push_frame(cyng::context& ctx)
		{
			//
			//	incoming wireless M-Bus data
			//

			cyng::vector_t const frame = ctx.get_frame();
			CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));
            
			//	[01242396072000630E,HYD,63,e,0003105c,72,29436587E61EBF03B900200540C83A80A8E0668CAAB369804FBEFBA35725B34A55369C7877E42924BD812D6D]
			auto const tpl = cyng::tuple_cast<
				cyng::buffer_t,		//	[0] server id
				std::string,		//	[1] manufacturer
				std::uint8_t,		//	[2] version
				std::uint8_t,		//	[3] media
				std::uint32_t,		//	[4] device id
				std::uint8_t,		//	[5] frame_type
				cyng::buffer_t		//	[6] payload
			>(frame);

			auto const server_id = sml::from_server_id(std::get<0>(tpl));

			CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - server id: " << server_id);
			CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - manufacturer: " << std::get<1>(tpl));
			CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - version: " << +std::get<2>(tpl));
			CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - media: " << +std::get<3>(tpl) << " - " << node::mbus::get_medium_name(std::get<3>(tpl)));
			CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - device id: " << std::get<4>(tpl));
			CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - frame type: " << +std::get<5>(tpl));


			//
			//	decoding depends on frame type and AES key from
			//	device table
			//
			switch (std::get<5>(tpl)) {
			case node::mbus::FIELD_CI_HEADER_LONG:	//	0x72
				read_frame_header_long(std::get<0>(tpl), std::get<6>(tpl));
				break;
			case node::mbus::FIELD_CI_RES_SHORT_SML:	//	0x7F
				//	e.g. ED300L
				read_frame_header_short_sml(ctx, std::get<0>(tpl), std::get<6>(tpl));

				//
				// update device table
				//
				update_device_table(std::get<0>(tpl), std::get<1>(tpl), std::get<2>(tpl), std::get<3>(tpl), std::get<5>(tpl), ctx.tag());
				break;
			case node::mbus::FIELD_CI_HEADER_SHORT:	//	0x7A
				//break;
			default:
			{
				//
				//	frame type not supported
				//
				std::stringstream ss;
				ss
					<< " frame type ["
					<< std::hex
					<< std::setw(2)
					<< +std::get<5>(tpl)
					<< "] not supported - data\n"
					;

				cyng::io::hex_dump hd;
				hd(ss, std::get<6>(tpl).cbegin(), std::get<6>(tpl).cend());

				CYNG_LOG_WARNING(logger_, ctx.get_name()
					<< ss.str());
			}
				//
				// update device table
				//
				update_device_table(std::get<0>(tpl), std::get<1>(tpl), std::get<2>(tpl), std::get<3>(tpl), std::get<5>(tpl), ctx.tag());
				break;
			}

		}

		void kernel::read_frame_header_long(cyng::buffer_t const& srv_id, cyng::buffer_t const& data)
		{
			CYNG_LOG_DEBUG(logger_, "make_header_long: " << cyng::io::to_hex(data));
			
			std::pair<header_long, bool> r = make_header_long(1, data);
			if (r.second) {
				
				std::cerr << "access nr: " << std::endl;		
				CYNG_LOG_DEBUG(logger_, "access nr: " << +r.first.header().get_access_no());

				auto const server_id = r.first.get_srv_id();
				auto const manufacturer = sml::get_manufacturer_code(server_id);
				auto const mbus_status = r.first.header().get_status();

				//
				//	0, 5, 7, or 13
				//	currently only 0 (== no encryption) and 5 (== AES CBC) is supported
				//
				auto const aes_mode = r.first.header().get_mode();
				
				CYNG_LOG_DEBUG(logger_, "manufacturer: " << manufacturer);
				CYNG_LOG_DEBUG(logger_, "status      : " << +mbus_status);
				CYNG_LOG_DEBUG(logger_, "mode        : " << +aes_mode);
				

				if (aes_mode == 5) {

					//
					//	get AES key and decrypt content (AES CBC - mode 5)
					//
					config_db_.access([&](cyng::store::table const* tbl) {

						CYNG_LOG_DEBUG(logger_, "search: " 
							<< sml::from_server_id(server_id)
							<< " / "
							<< tbl->size());
						
						auto const rec = tbl->lookup(cyng::table::key_generator(srv_id));
						if (!rec.empty()) {

							cyng::crypto::aes_128_key key;
							key = cyng::value_cast(rec["aes"], key);

							//
							//	decode payload data
							//
							if (!r.first.decode(key)) {
								CYNG_LOG_WARNING(logger_, "meter "
									<< sml::from_server_id(server_id)
									<< " has invalid AES key: "
									<< cyng::io::to_hex(key.to_buffer(), ' '));
							}
						}
						else {
							CYNG_LOG_WARNING(logger_, "meter " << sml::from_server_id(server_id) << " is not configured");
						}
					}, cyng::store::read_access("mbus-devices"));
				}

				CYNG_LOG_INFO(logger_, sml::from_server_id(server_id)
					<< ": "
					<< cyng::io::to_hex(r.first.header().data(), ' '));

				//
				//	test data integrity, parse data and write into data mirror
				//
				if (r.first.header().verify_encryption()) {

					//
					//	get number of encrypted blocks
					//
					auto counter = r.first.header().get_block_counter();

					vdb_reader reader;
					std::size_t offset{ 0 };
					while (counter-- != 0) {

						//
						//	read block
						//
						offset = reader.decode(r.first.header().data(), offset);

						//
						//	store block
						//
						CYNG_LOG_INFO(logger_, "offset: "
							<< offset
							<< ", meter " 
							<< sml::from_server_id(server_id) 
							<< ", value: "
							<< cyng::io::to_str(reader.get_value())
							<< ", scaler: "
							<< +reader.get_scaler()
							<< ", unit: "
							<< get_unit_name(reader.get_unit()));
					}

				}
				else {
					CYNG_LOG_WARNING(logger_, "meter " << sml::from_server_id(server_id) << " encryption failed");
				}

			}
			else {
				CYNG_LOG_WARNING(logger_, "cannot read long header of " << sml::from_server_id(srv_id));
			}
		}

		void kernel::read_frame_header_short_sml(cyng::context& ctx, cyng::buffer_t const& srv_id, cyng::buffer_t const& data)
		{
			std::pair<header_short, bool> r = make_header_short(data);
			auto const manufacturer = sml::get_manufacturer_code(srv_id);
			
			//
			//	0, 5, 7, or 13
			//	currently only 0 (== no encryption) and 5 (== AES CBC) is supported
			//
			auto const aes_mode = r.first.get_mode();
			
			if (r.second) {
				std::cout
				<< sml::decode(manufacturer)
				<< ": "
				<< mbus::get_medium_name(sml::get_medium_code(srv_id))
				<< " - "
				<< sml::from_server_id(srv_id)
				<< std::hex
				<< ", access #"
				<< +r.first.get_access_no()
				<< ", status: "
				<< +r.first.get_status()
				<< ", mode: "
				<< std::dec
				<< +aes_mode
				<< ", counter: "
				<< +r.first.get_block_counter()
				<< std::endl
				;
				
				//
				//	encrypt data
				//
				if (aes_mode == 5) {
					
					//
					//	get AES key and decrypt content (AES CBC - mode 5)
					//
					config_db_.access([&](cyng::store::table const* tbl) {
						
						CYNG_LOG_DEBUG(logger_, "search: " 
						<< sml::from_server_id(srv_id)
						<< " / "
						<< tbl->size());
						
						auto const rec = tbl->lookup(cyng::table::key_generator(srv_id));
						if (!rec.empty()) {

							//
							//	get AES key
							//
							cyng::crypto::aes_128_key key;
							key = cyng::value_cast(rec["aes"], key);
							
							//
							//	build initialization vector
							//
							auto const iv = mbus::build_initial_vector(srv_id, r.first.get_access_no());
							
							//
							//	decode payload data
							//
							if (!r.first.decode(key, iv)) {
								CYNG_LOG_WARNING(logger_, "meter "
								<< sml::from_server_id(srv_id)
								<< " has invalid AES key: "
								<< cyng::io::to_hex(key.to_buffer(), ' '));
							}
						}
						else {
							CYNG_LOG_WARNING(logger_, "meter " << sml::from_server_id(srv_id) << " is not configured");
							
							cyng::crypto::aes_128_key key;
							key.key_ = { 0x23, 0xA8, 0x4B, 0x07, 0xEB, 0xCB, 0xAF, 0x94, 0x88, 0x95, 0xDF, 0x0E, 0x91, 0x33, 0x52, 0x0D };
							
							auto const iv = mbus::build_initial_vector(srv_id, r.first.get_access_no());
							
							//
							//	decode payload data
							//
							if (!r.first.decode(key, iv)) {
								CYNG_LOG_WARNING(logger_, "meter "
								<< sml::from_server_id(srv_id)
								<< " has invalid AES key: "
								<< cyng::io::to_hex(key.to_buffer(), ' '));
							}
							else {
								CYNG_LOG_INFO(logger_, "meter " 
									<< sml::from_server_id(srv_id) 
									<< " encrypted with default key: "
									<< cyng::io::to_hex(key.to_buffer(), ' '));
								
								CYNG_LOG_DEBUG(logger_, "meter " 
									<< sml::from_server_id(srv_id) 
									<< " encrypted data: "
									<< cyng::io::to_hex(r.first.data()));								
							}
						}
					}, cyng::store::read_access("mbus-devices"));
				}
				
				
				//
				//	start SML parser
				//
				parser sml_parser([&](cyng::vector_t&& prg) {
					//std::cout << cyng::io::to_str(prg) << std::endl;
					ctx.queue(std::move(prg));
				}, false, true);	//	not verbose, logging

				auto const sml = r.first.data();
				sml_parser.read(sml.begin(), sml.end());
			}
			else {
				CYNG_LOG_WARNING(logger_, "cannot read short header of " << sml::from_server_id(srv_id));
			}
		}

		void kernel::update_device_table(cyng::buffer_t const& dev_id
			, std::string const& manufacturer
			, std::uint8_t version
			, std::uint8_t media
			, std::uint8_t frame_type
			, boost::uuids::uuid tag)
		{
			config_db_.access([&](cyng::store::table* tbl) {

				auto const rec = tbl->lookup(cyng::table::key_generator(dev_id));
				if (rec.empty()) {

					//
					//	new device
					//
					CYNG_LOG_INFO(logger_, "insert new device: " << cyng::io::to_hex(dev_id));

					tbl->insert(cyng::table::key_generator(dev_id)
						, cyng::table::data_generator(std::chrono::system_clock::now()
							, "+++"	//	class
							, true	//	visible
							, false	//	active
							, manufacturer	//	description
							, 0ull	//	status
							, cyng::buffer_t{ 0, 0 }	//	mask
							, 26000ul	//	interval
							, cyng::make_buffer({})	//	pubKey
							, cyng::crypto::aes_128_key{}	//	 AES key 
							, ""	//	user
							, "")	//	password
						, 1	//	generation
						, tag);

				}
				else {
					CYNG_LOG_TRACE(logger_, "update device: " << cyng::io::to_hex(dev_id));
					tbl->modify(rec.key(), cyng::param_factory("lastSeen", std::chrono::system_clock::now()), tag);
				}
			}, cyng::store::write_access("mbus-devices"));
		}
	}	//	sml
}

