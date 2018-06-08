/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "kernel.h"
//#include <NODE_project_info.h>
//#include <smf/sml/crc16.h>
//#include <smf/sml/protocol/serializer.h>
#include <smf/sml/protocol/message.h>
#include <smf/sml/protocol/value.hpp>
#include <smf/sml/obis_db.h>
#include <smf/sml/srv_id_io.h>

#include <cyng/io/serializer.h>
#include <cyng/vm/generator.h>
#include <cyng/tuple_cast.hpp>
#include <cyng/sys/memory.h>

#ifdef SMF_IO_DEBUG
#include <cyng/io/hex_dump.hpp>
#endif

namespace node
{
	namespace sml
	{

		kernel::kernel(cyng::logging::log_ptr logger
			, cyng::controller& vm
			, bool server_mode
			, std::string account
			, std::string pwd, std::string manufacturer
			, std::string model
			, cyng::mac48 mac)
		: logger_(logger)
			, server_mode_(server_mode)
			, account_(account)
			, pwd_(pwd)
			, manufacturer_(manufacturer)
			, model_(model)
			, server_id_(to_gateway_srv_id(mac))
			, reader_()
			, status_(0x070202)
			, sml_gen_()
			//, msg_()
			//, group_no_(0)
		{
			reset();

			//
			//	SML transport
			//
			vm.register_function("sml.msg", 2, std::bind(&kernel::sml_msg, this, std::placeholders::_1));
			vm.register_function("sml.eom", 2, std::bind(&kernel::sml_eom, this, std::placeholders::_1));

			//
			//	SML data
			//
			vm.register_function("sml.public.open.request", 8, std::bind(&kernel::sml_public_open_request, this, std::placeholders::_1));
			vm.register_function("sml.public.close.request", 3, std::bind(&kernel::sml_public_close_request, this, std::placeholders::_1));
			vm.register_function("sml.get.proc.parameter.request", 8, std::bind(&kernel::sml_get_proc_parameter_request, this, std::placeholders::_1));
			vm.register_function("sml.get.proc.status.word", 6, std::bind(&kernel::sml_get_proc_status_word, this, std::placeholders::_1));
			vm.register_function("sml.get.proc.device.id", 6, std::bind(&kernel::sml_get_proc_device_id, this, std::placeholders::_1));
			vm.register_function("sml.get.proc.mem.usage", 6, std::bind(&kernel::sml_get_proc_mem_usage, this, std::placeholders::_1));
			vm.register_function("sml.get.proc.lan.if", 6, std::bind(&kernel::sml_get_proc_lan_if, this, std::placeholders::_1));
			vm.register_function("sml.get.proc.lan.config", 6, std::bind(&kernel::sml_get_proc_lan_config, this, std::placeholders::_1));
			vm.register_function("sml.get.proc.ntp.config", 6, std::bind(&kernel::sml_get_proc_ntp_config, this, std::placeholders::_1));
			vm.register_function("sml.get.proc.device.time", 6, std::bind(&kernel::sml_get_proc_device_time, this, std::placeholders::_1));
			vm.register_function("sml.get.proc.active.devices", 6, std::bind(&kernel::sml_get_proc_active_devices, this, std::placeholders::_1));
			vm.register_function("sml.get.proc.visible.devices", 6, std::bind(&kernel::sml_get_proc_visible_devices, this, std::placeholders::_1));
			vm.register_function("sml.get.proc.device.info", 6, std::bind(&kernel::sml_get_proc_device_info, this, std::placeholders::_1));
			vm.register_function("sml.get.proc.ipt.state", 6, std::bind(&kernel::sml_get_proc_ipt_state, this, std::placeholders::_1));
			vm.register_function("sml.get.proc.ipt.param", 6, std::bind(&kernel::sml_get_proc_ipt_param, this, std::placeholders::_1));
			vm.register_function("sml.get.proc.sensor.property", 6, std::bind(&kernel::sml_get_proc_sensor_property, this, std::placeholders::_1));
			//"sml.get.proc.access.rights"
			//"sml.get.proc.custom.interface"
			//"sml.get.proc.custom.param"
			//"sml.get.proc.wan.config"
			//"sml.get.proc.gsm.config"
			//"sml.get.proc.gprs.param"

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
			ctx.attach(reader_.read(msg, idx));
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
			cyng::buffer_t buf = sml_gen_.boxing();;

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
				ctx.attach(cyng::generate_invoke("stream.serialize", buf));
			}
			else
			{
				ctx.attach(cyng::generate_invoke("ipt.transfer.data", buf));
			}

			ctx.attach(cyng::generate_invoke("stream.flush"));

			//ctx.attach(cyng::generate_invoke("ipt.transfer.data", buf)).attach(cyng::generate_invoke("stream.flush"));
			reset();
		}

		void kernel::sml_public_open_request(cyng::context& ctx)
		{
			//	[34481794-6866-4776-8789-6f914b4e34e7,180301091943374505-1,0,005056c00008,00:15:3b:02:23:b3,20180301092332,operator,operator]
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
			CYNG_LOG_TRACE(logger_, "sml.public.open.request " << cyng::io::to_str(frame));

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,	//	[0] pk
				std::string,		//	[1] trx
				std::size_t,		//	[2] SML message id
				std::string,		//	[3] client ID
				std::string,		//	[4] server ID
				std::string,		//	[5] request file id
				std::string,		//	[6] username
				std::string			//	[7] password
			>(frame);
			CYNG_LOG_INFO(logger_, "sml.public.open.request - trx: "
				<< std::get<1>(tpl)
				<< ", msg id: "
				<< std::get<2>(tpl)
				<< ", client id: "
				<< std::get<3>(tpl)
				<< ", server id: "
				<< std::get<4>(tpl)
				<< ", file id: "
				<< std::get<5>(tpl)
				<< ", user: "
				<< std::get<6>(tpl)
				<< ", pwd: "
				<< std::get<7>(tpl))
				;

			//
			//	linearize and set CRC16
			//	append to current SML message
			//
			//BOOST_ASSERT_MSG(msg_.empty(), "pending SML data");
			sml_gen_.public_open(frame.at(1)	// trx
				, frame.at(3)	//	client id
				, frame.at(5)	//	req file id
				, frame.at(4));
			//append_msg(res_generator::public_open(frame.at(1)	// trx
			//	, frame.at(3)	//	client id
			//	, frame.at(5)	//	req file id
			//	, frame.at(4)	//	server id
			//));
		}

		void kernel::sml_public_close_request(cyng::context& ctx)
		{
			//	[5fb21ea1-ac66-4a00-98b7-88edd57addb7,180523152649286995-4,3]
			//
			//	* pk
			//	* transaction id
			//	* SML message id
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.public.close.request " << cyng::io::to_str(frame));

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
			//	* OBIS (requested parameter)
			//	* attribute (should be null)
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_WARNING(logger_, "sml.get.proc.parameter.request unknown " << cyng::io::to_str(frame));

			//CODE_ROOT_DEVICE_IDENT

		}

		void kernel::sml_get_proc_status_word(cyng::context& ctx)
		{
			//	[fd167248-a823-46d6-ba0a-03157a28a104,1805241520113192-2,1,0500153B02297E,operator,operator]
			//
			//	* pk
			//	* transaction id
			//	* SML message id
			//	* server ID
			//	* username
			//	* password
			//	* OBIS (requested parameter)
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.get.proc.status.word " << cyng::io::to_str(frame));

			//	sets the following flags:
			//	0x010070202 - 100000000000001110000001000000010
			//	fataler Fehler              : false
			//	am Funknetz angemeldet      : false
			//	Endkundenschnittstelle      : true
			//	Betriebsbereischaft erreicht: true
			//	am IP-T Server angemeldet   : true
			//	Erweiterungsschnittstelle   : true
			//	DHCP Parameter erfasst      : true
			//	Out of Memory erkannt       : false
			//	Wireless M-Bus Schnittst.   : true
			//	Ethernet-Link vorhanden     : true
			//	Zeitbasis unsicher          : rot
			//	PLC Schnittstelle           : false

			//	0x000070202 - 000000000000001110000001000000010
			//	fataler Fehler              : false
			//	am Funknetz angemeldet      : false
			//	Endkundenschnittstelle      : true
			//	Betriebsbereischaft erreicht: true
			//	am IP-T Server angemeldet   : true
			//	Erweiterungsschnittstelle   : true
			//	DHCP Parameter erfasst      : true
			//	Out of Memory erkannt       : false
			//	Wireless M-Bus Schnittst.   : true
			//	Ethernet-Link vorhanden     : true
			//	Zeitbasis unsicher          : false
			//	PLC Schnittstelle           : false

			//	see also http://www.sagemcom.com/fileadmin/user_upload/Energy/Dr.Neuhaus/Support/ZDUE-MUC/Doc_communs/ZDUE-MUC_Anwenderhandbuch_V2_5.pdf
			//	chapter Globales Statuswort

			//	bit meaning
			//	0	always 0
			//	1	always 1
			//	2-7	always 0
			//	8	1 if fatal error was detected
			//	9	1 if restart was triggered by watchdog reset
			//	10	0 if IP address is available (DHCP)
			//	11	0 if ethernet link is available
			//	12	always 0
			//	13	0 if authorized on IP-T server
			//	14	1 in case of out of memory
			//	15	always 0
			//	16	1 if Service interface is available (Kundenschnittstelle)
			//	17	1 if extension interface is available (Erweiterungs-Schnittstelle)
			//	18	1 if Wireless M-Bus interface is available
			//	19	1 if PLC is available
			//	20-31	always 0
			//	32	1 if time base is unsure

			//	32.. 28.. 24.. 20.. 16.. 12.. 8..  4..
			//	‭0001 0000 0000 0111 0000 0010 0000 0010‬ = 0x010070202

			//	81 00 60 05 00 00 = OBIS_CLASS_OP_LOG_STATUS_WORD

			//node::sml::parameter_tree(node::sml::OBIS_CLASS_OP_LOG_STATUS_WORD, node::sml::make_value(static_cast<std::uint64_t>(0x000070202)));

			//
			//	linearize and set CRC16
			//	append to current SML message
			//
			sml_gen_.get_proc_parameter_status_word(frame.at(1)
				, frame.at(3)
				, status_);

		}

		void kernel::sml_get_proc_device_id(cyng::context& ctx)
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
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.get.proc.device.id " << cyng::io::to_str(frame));

			sml_gen_.get_proc_parameter_device_id(frame.at(1)
				, frame.at(3)	//	server id
				, manufacturer_
				, server_id_
				, "VSES-1KW-221-1F0"
				, "serial.number");
		}

		void kernel::sml_get_proc_mem_usage(cyng::context& ctx)
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
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.get.proc.mem.usage " << cyng::io::to_str(frame));

			//76                                                SML_Message(Sequence): 
			//  81063137303531313135343334303434363535382D32    transactionId: 170511154340446558-2
			//  6201                                            groupNo: 1
			//  6200                                            abortOnError: 0
			//  72                                              messageBody(Choice): 
			//	630501                                        messageBody: 1281 => SML_GetProcParameter_Res (0x00000501)
			//	73                                            SML_GetProcParameter_Res(Sequence): 
			//	  080500153B01EC46                            serverId: 05 00 15 3B 01 EC 46 
			//	  71                                          parameterTreePath(SequenceOf): 
			//		070080800010FF                            path_Entry: 00 80 80 00 10 FF 
			//	  73                                          parameterTree(Sequence): 
			//		070080800010FF                            parameterName: 00 80 80 00 10 FF 
			//		01                                        parameterValue: not set
			//		72                                        child_List(SequenceOf): 
			//		  73                                      tree_Entry(Sequence): 
			//			070080800011FF                        parameterName: 00 80 80 00 11 FF 
			//			72                                    parameterValue(Choice): 
			//			  6201                                parameterValue: 1 => smlValue (0x01)
			//			  620E                                smlValue: 14 (std::uint8_t)
			//			01                                    child_List: not set
			//		  73                                      tree_Entry(Sequence): 
			//			070080800012FF                        parameterName: 00 80 80 00 12 FF 
			//			72                                    parameterValue(Choice): 
			//			  6201                                parameterValue: 1 => smlValue (0x01)
			//			  6200                                smlValue: 0
			//			01                                    child_List: not set
			//  631AA8                                          crc16: 6824
			//  00                                              endOfSmlMsg: 00 

			CYNG_LOG_INFO(logger_, "get_used_physical_memory " << cyng::sys::get_used_physical_memory());
			CYNG_LOG_INFO(logger_, "get_total_physical_memory " << cyng::sys::get_total_physical_memory());
			CYNG_LOG_INFO(logger_, "get_used_virtual_memory " << cyng::sys::get_used_virtual_memory());
			CYNG_LOG_INFO(logger_, "get_total_virtual_memory " << cyng::sys::get_total_virtual_memory());

			//	m2m::OBIS_CODE_ROOT_MEMORY_USAGE
			const std::uint8_t mirror = cyng::sys::get_used_virtual_memory_in_percent()
				, tmp = cyng::sys::get_used_physical_memory_in_percent();

			sml_gen_.append_msg(message(frame.at(1)	//	trx
				, 2 //, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(frame.at(3)	//	server id
					, OBIS_CODE(00, 80, 80, 00, 10, FF)		//	path entry

					//
					//	generate get process parameter response
					//	2 uint8 values 
					//
					, child_list_tree(OBIS_CODE(00, 80, 80, 00, 10, FF), {

						parameter_tree(OBIS_CODE(00, 80, 80, 00, 11, FF), make_value(mirror)),	//	mirror
						parameter_tree(OBIS_CODE(00, 80, 80, 00, 12, FF), make_value(tmp))	// tmp
			}))));
		}

		void kernel::sml_get_proc_lan_if(cyng::context& ctx)
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
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.get.proc.lan.if " << cyng::io::to_str(frame));

			//
			//	ToDo: implement
			//
			sml_gen_.append_msg(message(frame.at(1)	//	trx
				, 2 //, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(frame.at(3)	//	server id
					, OBIS_CODE_IF_LAN_DSL	//	path entry - 81 48 17 07 00 FF
					, empty_tree(OBIS_CODE_IF_LAN_DSL))));


		}

		void kernel::sml_get_proc_lan_config(cyng::context& ctx)
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
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.get.proc.lan.config " << cyng::io::to_str(frame));
			
			//
			//	ToDo: implement
			//
			sml_gen_.append_msg(message(frame.at(1)	//	trx
				, 2 //, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(frame.at(3)	//	server id
					, OBIS_CODE_ROOT_LAN_DSL	//	path entry - 81 48 0D 06 00 FF
					, empty_tree(OBIS_CODE_ROOT_LAN_DSL))));

		}

		void kernel::sml_get_proc_ntp_config(cyng::context& ctx)
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
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.get.proc.ntp.config " << cyng::io::to_str(frame));

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
			const std::uint16_t ntp_port = 555;	// 123;
			const bool ntp_active = true;
			const std::uint16_t ntp_tz = 3600;

			sml_gen_.append_msg(message(frame.at(1)	//	trx
				, 2 //, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(frame.at(3)	//	server id
					, obis(0x81, 0x81, 0xC7, 0x88, 0x01, 0xFF)	//	path entry

					//
					//	generate get process parameter response
					//
					, child_list_tree(obis(0x81, 0x81, 0xC7, 0x88, 0x01, 0xFF), {

						//	NTP servers
						child_list_tree(obis(0x81, 0x81, 0xC7, 0x88, 0x02, 0xFF),{
							parameter_tree(obis(0x81, 0x81, 0xC7, 0x88, 0x02, 0x01), make_value(ntp_primary)),	
							parameter_tree(obis(0x81, 0x81, 0xC7, 0x88, 0x02, 0x02), make_value(ntp_secondary)),
							parameter_tree(obis(0x81, 0x81, 0xC7, 0x88, 0x02, 0x03), make_value(ntp_tertiary))
						}),
						parameter_tree(obis(0x81, 0x81, 0xC7, 0x88, 0x03, 0xFF), make_value(ntp_port)),	//	NTP port
						parameter_tree(obis(0x81, 0x81, 0xC7, 0x88, 0x06, 0xFF), make_value(ntp_active)),	// enabled/disabled
						parameter_tree(obis(0x81, 0x81, 0xC7, 0x88, 0x04, 0xFF), make_value(ntp_tz))	// timezone
			}))));

		}

		void kernel::sml_get_proc_device_time(cyng::context& ctx)
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
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.get.proc.device.time " << cyng::io::to_str(frame));
			//76                                                SML_Message(Sequence): 
			//  81063137303531323136353635313836333431352D33    transactionId: 170512165651863415-3
			//  6202                                            groupNo: 2
			//  6200                                            abortOnError: 0
			//  72                                              messageBody(Choice): 
			//	630501                                        messageBody: 1281 => SML_GetProcParameter_Res (0x00000501)
			//	73                                            SML_GetProcParameter_Res(Sequence): 
			//	  080500153B01EC46                            serverId: 05 00 15 3B 01 EC 46 
			//	  71                                          parameterTreePath(SequenceOf): 
			//		078181C78810FF                            path_Entry: 81 81 C7 88 10 FF 
			//	  73                                          parameterTree(Sequence): 
			//		078181C78810FF                            parameterName: 81 81 C7 88 10 FF 
			//		01                                        parameterValue: not set
			//		74                                        child_List(SequenceOf): 
			//		  73                                      tree_Entry(Sequence): 
			//			07010000090B00                        parameterName: 01 00 00 09 0B 00 
			//			72                                    parameterValue(Choice): 
			//			  6204                                parameterValue: 4 => smlTime (0x04)
			//			  72                                  smlTime(Choice): 
			//				6202                              smlTime: 2 => timestamp (0x02)
			//				655915CD3C                        timestamp: 1494601020
			//			01                                    child_List: not set
			//		  73                                      tree_Entry(Sequence): 
			//			070000600800FF                        parameterName: 00 00 60 08 00 FF 
			//			72                                    parameterValue(Choice): 
			//			  6204                                parameterValue: 4 => smlTime (0x04)
			//			  72                                  smlTime(Choice): 
			//				6201                              smlTime: 1 => secIndex (0x01)
			//				6505E5765F                        secIndex: 98924127
			//			01                                    child_List: not set
			//		  73                                      tree_Entry(Sequence): 
			//			07810000090B01                        parameterName: 81 00 00 09 0B 01 
			//			72                                    parameterValue(Choice): 
			//			  6201                                parameterValue: 1 => smlValue (0x01)
			//			  550000003C                          smlValue: 60
			//			01                                    child_List: not set
			//		  73                                      tree_Entry(Sequence): 
			//			07810000090B02                        parameterName: 81 00 00 09 0B 02 
			//			72                                    parameterValue(Choice): 
			//			  6201                                parameterValue: 1 => smlValue (0x01)
			//			  4200                                smlValue: False
			//			01                                    child_List: not set
			//  633752                                          crc16: 14162
			//  00                                              endOfSmlMsg: 00 

			const std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
			const std::time_t now_tt = std::chrono::system_clock::to_time_t(now);
			const std::int32_t tz = 60;
			const bool sync_active = true;

			sml_gen_.append_msg(message(frame.at(1)	//	trx
				, 2 //, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(frame.at(3)	//	server id
					, OBIS_CODE_ROOT_DEVICE_TIME	//	path entry 81 81 C7 88 10 FF 

					//
					//	generate get process parameter response
					//
					, child_list_tree(OBIS_CODE_ROOT_DEVICE_TIME, {

						parameter_tree(OBIS_CURRENT_UTC, make_value(now)),	//	timestamp (01 00 00 09 0B 00 )
						parameter_tree(obis(0x00, 0x00, 0x60, 0x08, 0x00, 0xFF), make_sec_index_value(now)),
						parameter_tree(obis(0x81, 0x00, 0x00, 0x09, 0x0B, 0x01), make_value(tz)),
						parameter_tree(obis(0x81, 0x00, 0x00, 0x09, 0x0B, 0x02), make_value(sync_active))
					}))));
		}

		void kernel::sml_get_proc_active_devices(cyng::context& ctx)
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
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.get.proc.active.devices " << cyng::io::to_str(frame));

			//	last received data
			const std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
			const std::string device_class = "---";	//	2D 2D 2D

			sml_gen_.append_msg(message(frame.at(1)	//	trx
				, 2 //, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(frame.at(3)	//	server id
					, OBIS_CODE_ROOT_ACTIVE_DEVICES	//	path entry 81 81 11 06 FF FF

					//
					//	generate get process parameter response
					//
					, child_list_tree(OBIS_CODE_ROOT_ACTIVE_DEVICES, {
						child_list_tree(obis(0x81, 0x81, 0x11, 0x06, 0x01, 0xFF), {

						child_list_tree(obis(0x81, 0x81, 0x11, 0x06, 0x01, 0x01),{
							parameter_tree(obis(0x81, 0x81, 0xC7, 0x82, 0x04, 0xFF), make_value(server_id_)),
							parameter_tree(obis(0x81, 0x81, 0xC7, 0x82, 0x02, 0xFF), make_value(device_class)),
							parameter_tree(OBIS_CURRENT_UTC, make_value(now))	//	timestamp (01 00 00 09 0B 00 )
						}),
						child_list_tree(obis(0x81, 0x81, 0x11, 0x06, 0x01, 0x02),{
							parameter_tree(obis(0x81, 0x81, 0xC7, 0x82, 0x04, 0xFF), make_value(cyng::buffer_t{ 0x01, (char)0xA8, 0x15, 0x74, (char)0x31, 0x45, 0x05, (char)0x01, (char)0x02 })),
							parameter_tree(obis(0x81, 0x81, 0xC7, 0x82, 0x02, 0xFF), make_value(device_class)),
							parameter_tree(OBIS_CURRENT_UTC, make_value(now))	//	timestamp (01 00 00 09 0B 00 )
						}),
						child_list_tree(obis(0x81, 0x81, 0x11, 0x06, 0x01, 0x03),{
							parameter_tree(obis(0x81, 0x81, 0xC7, 0x82, 0x04, 0xFF), make_value(cyng::buffer_t{ 0x01, (char)0xA8, 0x15, 0x74, (char)0x31, 0x45, 0x04, (char)0x01, (char)0x02 })),
							parameter_tree(obis(0x81, 0x81, 0xC7, 0x82, 0x02, 0xFF), make_value(device_class)),
							parameter_tree(OBIS_CURRENT_UTC, make_value(now))	//	timestamp (01 00 00 09 0B 00 )
						})
					}) 
				}))));
		}

		void kernel::sml_get_proc_visible_devices(cyng::context& ctx)
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
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.get.proc.visible.devices " << cyng::io::to_str(frame));

			//	last received data
			const std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
			const std::string device_class = "---";	//	2D 2D 2D

			sml_gen_.append_msg(message(frame.at(1)	//	trx
				, 2 //, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(frame.at(3)	//	server id
					, OBIS_CODE_ROOT_VISIBLE_DEVICES	//	path entry 81 81 10 06 FF FF

					//
					//	generate get process parameter response
					//
					, child_list_tree(OBIS_CODE_ROOT_VISIBLE_DEVICES, {
						child_list_tree(obis(0x81, 0x81, 0x10, 0x06, 0x01, 0xFF), {

						child_list_tree(obis(0x81, 0x81, 0x10, 0x06, 0x01, 0x01),{
							parameter_tree(obis(0x81, 0x81, 0xC7, 0x82, 0x04, 0xFF), make_value(server_id_)),
							parameter_tree(obis(0x81, 0x81, 0xC7, 0x82, 0x02, 0xFF), make_value(device_class)),
							parameter_tree(OBIS_CURRENT_UTC, make_value(now))	//	timestamp (01 00 00 09 0B 00 )
						}),
						child_list_tree(obis(0x81, 0x81, 0x10, 0x06, 0x01, 0x02),{
							parameter_tree(obis(0x81, 0x81, 0xC7, 0x82, 0x04, 0xFF), make_value(cyng::buffer_t{ 0x24, 0x23, 0x02, 0x12, (char)0x90, 0x20, 0x07, (char)0x86 })),
							parameter_tree(obis(0x81, 0x81, 0xC7, 0x82, 0x02, 0xFF), make_value(device_class)),
							parameter_tree(OBIS_CURRENT_UTC, make_value(now))	//	timestamp (01 00 00 09 0B 00 )
						}),
						child_list_tree(obis(0x81, 0x81, 0x10, 0x06, 0x01, 0x02),{
							parameter_tree(obis(0x81, 0x81, 0xC7, 0x82, 0x04, 0xFF), make_value(cyng::buffer_t{ 0x24, 0x23, 0x02, 0x15, (char)0x90, 0x20, 0x07, (char)0x86 })),
							parameter_tree(obis(0x81, 0x81, 0xC7, 0x82, 0x02, 0xFF), make_value(device_class)),
							parameter_tree(OBIS_CURRENT_UTC, make_value(now))	//	timestamp (01 00 00 09 0B 00 )
						})
					})
				}))));
		}

		void kernel::sml_get_proc_device_info(cyng::context& ctx)
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
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.get.proc.device.info " << cyng::io::to_str(frame));

			//
			//	linearize and set CRC16
			//	append to current SML message
			//
			sml_gen_.append_msg(message(frame.at(1)	//	trx
				, 2 //, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(frame.at(3)	//	server id
					, OBIS_CODE_ROOT_DEVICE_INFO	//	path entry - 81 81 12 06 FF FF
					, empty_tree(OBIS_CODE_ROOT_DEVICE_INFO))));

		}

		void kernel::sml_get_proc_ipt_state(cyng::context& ctx)
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
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.get.proc.ipt.state " << cyng::io::to_str(frame));

			//76                                                SML_Message(Sequence): 
			//  81063137303531313136303831363537393537312D33    transactionId: 170511160816579571-3
			//  6202                                            groupNo: 2
			//  6200                                            abortOnError: 0
			//  72                                              messageBody(Choice): 
			//	630501                                        messageBody: 1281 => SML_GetProcParameter_Res (0x00000501)
			//	73                                            SML_GetProcParameter_Res(Sequence): 
			//	  080500153B01EC46                            serverId: 05 00 15 3B 01 EC 46 
			//	  71                                          parameterTreePath(SequenceOf): 
			//		0781490D0600FF                            path_Entry: 81 49 0D 06 00 FF 
			//	  73                                          parameterTree(Sequence): 
			//		0781490D0600FF                            parameterName: 81 49 0D 06 00 FF 
			//		01                                        parameterValue: not set
			//		73                                        child_List(SequenceOf): 
			//		  73                                      tree_Entry(Sequence): 
			//			07814917070000                        parameterName: 81 49 17 07 00 00 
			//			72                                    parameterValue(Choice): 
			//			  6201                                parameterValue: 1 => smlValue (0x01)
			//			  652A96A8C0                          smlValue: 714516672
			//			01                                    child_List: not set
			//		  73                                      tree_Entry(Sequence): 
			//			0781491A070000                        parameterName: 81 49 1A 07 00 00 
			//			72                                    parameterValue(Choice): 
			//			  6201                                parameterValue: 1 => smlValue (0x01)
			//			  6368EF                              smlValue: 26863
			//			01                                    child_List: not set
			//		  73                                      tree_Entry(Sequence): 
			//			07814919070000                        parameterName: 81 49 19 07 00 00 
			//			72                                    parameterValue(Choice): 
			//			  6201                                parameterValue: 1 => smlValue (0x01)
			//			  631019                              smlValue: 4121
			//			01                                    child_List: not set
			//  639CB8                                          crc16: 40120
			//  00                                              endOfSmlMsg: 00 

			//const std::uint32_t ip_address = bus_->remote_endpoint().address().to_v4().to_ulong();
			const std::uint32_t ip_address = 0x11223344;
			//const std::uint16_t target_port = bus_->local_endpoint().port()
			//	, source_port = bus_->remote_endpoint().port();
			//	ToDo: use "ip.tcp.socket.ep.local.port"
			//	ToDo: use "ip.tcp.socket.ep.remote.port"
			const std::uint16_t target_port = 1000, source_port = 2000;

			//
			//	linearize and set CRC16
			//	append to current SML message
			//
			sml_gen_.append_msg(message(frame.at(1)	//	trx
				, 2 //, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(frame.at(3)	//	server id  
					, OBIS_CODE_ROOT_IPT_STATE	//	path entry - 81 49 0D 06 00 FF 
					, child_list_tree(OBIS_CODE_ROOT_IPT_STATE, {

						parameter_tree(obis(0x81, 0x49, 0x17, 0x07, 0x00, 0x00), make_value(ip_address)),
						parameter_tree(obis(0x81, 0x49, 0x1A, 0x07, 0x00, 0x00), make_value(target_port)),
						parameter_tree(obis(0x81, 0x49, 0x19, 0x07, 0x00, 0x00), make_value(source_port))
			}))));
		}

		void kernel::sml_get_proc_ipt_param(cyng::context& ctx)
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
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.get.proc.ipt.param " << cyng::io::to_str(frame));

			//76                                                SML_Message(Sequence): 
			//  81063137303531313136303831363537393537312D32    transactionId: 170511160816579571-2
			//  6201                                            groupNo: 1
			//  6200                                            abortOnError: 0
			//  72                                              messageBody(Choice): 
			//	630501                                        messageBody: 1281 => SML_GetProcParameter_Res (0x00000501)
			//	73                                            SML_GetProcParameter_Res(Sequence): 
			//	  080500153B01EC46                            serverId: 05 00 15 3B 01 EC 46 
			//	  71                                          parameterTreePath(SequenceOf): 
			//		0781490D0700FF                            path_Entry: 81 49 0D 07 00 FF 
			//	  73                                          parameterTree(Sequence): 
			//		0781490D0700FF                            parameterName: 81 49 0D 07 00 FF 
			//		01                                        parameterValue: not set
			//		76                                        child_List(SequenceOf): 
			//		  73                                      tree_Entry(Sequence): 
			//			0781490D070001                        parameterName: 81 49 0D 07 00 01 
			//			01                                    parameterValue: not set
			//			75                                    child_List(SequenceOf): 
			//			  73                                  tree_Entry(Sequence): 
			//				07814917070001                    parameterName: 81 49 17 07 00 01 
			//				72                                parameterValue(Choice): 
			//				  6201                            parameterValue: 1 => smlValue (0x01)
			//				  652A96A8C0                      smlValue: 714516672
			//				01                                child_List: not set
			//			  73                                  tree_Entry(Sequence): 
			//				0781491A070001                    parameterName: 81 49 1A 07 00 01 
			//				72                                parameterValue(Choice): 
			//				  6201                            parameterValue: 1 => smlValue (0x01)
			//				  6368EF                          smlValue: 26863
			//				01                                child_List: not set
			//			  73                                  tree_Entry(Sequence): 
			//				07814919070001                    parameterName: 81 49 19 07 00 01 
			//				72                                parameterValue(Choice): 
			//				  6201                            parameterValue: 1 => smlValue (0x01)
			//				  6200                            smlValue: 0
			//				01                                child_List: not set
			//			  73                                  tree_Entry(Sequence): 
			//				078149633C0101                    parameterName: 81 49 63 3C 01 01 
			//				72                                parameterValue(Choice): 
			//				  6201                            parameterValue: 1 => smlValue (0x01)
			//				  094C534D5465737432              smlValue: LSMTest2
			//				01                                child_List: not set
			//			  73                                  tree_Entry(Sequence): 
			//				078149633C0201                    parameterName: 81 49 63 3C 02 01 
			//				72                                parameterValue(Choice): 
			//				  6201                            parameterValue: 1 => smlValue (0x01)
			//				  094C534D5465737432              smlValue: LSMTest2
			//				01                                child_List: not set
			//		  73                                      tree_Entry(Sequence): 
			//			0781490D070002                        parameterName: 81 49 0D 07 00 02 
			//			01                                    parameterValue: not set
			//			75                                    child_List(SequenceOf): 
			//			  73                                  tree_Entry(Sequence): 
			//				07814917070002                    parameterName: 81 49 17 07 00 02 
			//				72                                parameterValue(Choice): 
			//				  6201                            parameterValue: 1 => smlValue (0x01)
			//				  65B596A8C0                      smlValue: 3046549696
			//				01                                child_List: not set
			//			  73                                  tree_Entry(Sequence): 
			//				0781491A070002                    parameterName: 81 49 1A 07 00 02 
			//				72                                parameterValue(Choice): 
			//				  6201                            parameterValue: 1 => smlValue (0x01)
			//				  6368F0                          smlValue: 26864
			//				01                                child_List: not set
			//			  73                                  tree_Entry(Sequence): 
			//				07814919070002                    parameterName: 81 49 19 07 00 02 
			//				72                                parameterValue(Choice): 
			//				  6201                            parameterValue: 1 => smlValue (0x01)
			//				  6200                            smlValue: 0
			//				01                                child_List: not set
			//			  73                                  tree_Entry(Sequence): 
			//				078149633C0102                    parameterName: 81 49 63 3C 01 02 
			//				72                                parameterValue(Choice): 
			//				  6201                            parameterValue: 1 => smlValue (0x01)
			//				  094C534D5465737432              smlValue: LSMTest2
			//				01                                child_List: not set
			//			  73                                  tree_Entry(Sequence): 
			//				078149633C0202                    parameterName: 81 49 63 3C 02 02 
			//				72                                parameterValue(Choice): 
			//				  6201                            parameterValue: 1 => smlValue (0x01)
			//				  094C534D5465737432              smlValue: LSMTest2
			//				01                                child_List: not set
			//		  73                                      tree_Entry(Sequence): 
			//			07814827320601                        parameterName: 81 48 27 32 06 01 
			//			72                                    parameterValue(Choice): 
			//			  6201                                parameterValue: 1 => smlValue (0x01)
			//			  6201                                smlValue: 1
			//			01                                    child_List: not set
			//		  73                                      tree_Entry(Sequence): 
			//			07814831320201                        parameterName: 81 48 31 32 02 01 
			//			72                                    parameterValue(Choice): 
			//			  6201                                parameterValue: 1 => smlValue (0x01)
			//			  6278                                smlValue: 120
			//			01                                    child_List: not set
			//		  73                                      tree_Entry(Sequence): 
			//			070080800003FF                        parameterName: 00 80 80 00 03 FF 
			//			72                                    parameterValue(Choice): 
			//			  6201                                parameterValue: 1 => smlValue (0x01)
			//			  4200                                smlValue: False
			//			01                                    child_List: not set
			//		  73                                      tree_Entry(Sequence): 
			//			070080800004FF                        parameterName: 00 80 80 00 04 FF 
			//			01                                    parameterValue: not set
			//			01                                    child_List: not set
			//  63915D                                          crc16: 37213
			//  00                                              endOfSmlMsg: 00 

			const std::uint32_t ip_address_primary_master = 714516672;	//	192.168.150.42
			const std::uint16_t target_port_primary_master = 26862
				, source_port_primary_master = 0;
			//const std::string user_pm = config_[0].account_
			//	, pwd_pm = config_[0].pwd_;
			const std::string user_pm = "user-1"
				, pwd_pm = "pwd-1";

			const std::uint32_t ip_address_secondary_master = 3046549696;	//	192.168.150.181
			const std::uint16_t target_port_secondary_master = 26863
				, source_port_seccondary_master = 0;
			//const std::string user_sm = config_[1].account_
			//	, pwd_sm = config_[1].pwd_;
			const std::string user_sm = "user-2"
				, pwd_sm = "pwd-2";

			//const std::uint16_t wait_time = bus_->get_watchdog();	//	minutes
			const std::uint16_t wait_time = 12;	//	minutes
			const std::uint16_t repetitions = 120;	//	counter
			const bool ssl = false;

			//
			//	linearize and set CRC16
			//	append to current SML message
			//
			sml_gen_.append_msg(message(frame.at(1)	//	trx
				, 2 //, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(frame.at(3)	//	server id  
					, OBIS_CODE_ROOT_IPT_PARAM	//	path entry - 81 49 0D 07 00 FF 
					, child_list_tree(OBIS_CODE_ROOT_IPT_PARAM, {

						//	primary master
						child_list_tree(obis(0x81, 0x49, 0x0D, 0x07, 0x00, 0x01), {
							parameter_tree(obis(0x81, 0x49, 0x17, 0x07, 0x00, 0x01), make_value(ip_address_primary_master)),
							parameter_tree(obis(0x81, 0x49, 0x1A, 0x07, 0x00, 0x01), make_value(target_port_primary_master)),
							parameter_tree(obis(0x81, 0x49, 0x19, 0x07, 0x00, 0x01), make_value(source_port_primary_master)),
							parameter_tree(obis(0x81, 0x49, 0x63, 0x3C, 0x01, 0x01), make_value(user_pm)),
							parameter_tree(obis(0x81, 0x49, 0x63, 0x3C, 0x02, 0x01), make_value(pwd_pm))
						}),

						//	secondary master
						child_list_tree(obis(0x81, 0x49, 0x0D, 0x07, 0x00, 0x02), {
							parameter_tree(obis(0x81, 0x49, 0x17, 0x07, 0x00, 0x02), make_value(ip_address_secondary_master)),
							parameter_tree(obis(0x81, 0x49, 0x1A, 0x07, 0x00, 0x02), make_value(target_port_secondary_master)),
							parameter_tree(obis(0x81, 0x49, 0x19, 0x07, 0x00, 0x02), make_value(source_port_seccondary_master)),
							parameter_tree(obis(0x81, 0x49, 0x63, 0x3C, 0x01, 0x02), make_value(user_sm)),
							parameter_tree(obis(0x81, 0x49, 0x63, 0x3C, 0x02, 0x02), make_value(pwd_sm))
						}),

						//	waiting time (Wartezeit)
						parameter_tree(obis(0x81, 0x48, 0x27, 0x32, 0x06, 0x01), make_value(wait_time)),

						//	repetitions
						parameter_tree(obis(0x81, 0x48, 0x31, 0x32, 0x02, 0x01), make_value(repetitions)),

						//	SSL
						parameter_tree(obis(0x00, 0x80, 0x80, 0x00, 0x03, 0xFF), make_value(ssl)),

						//	certificates (none)
						empty_tree(obis(0x00, 0x80, 0x80, 0x00, 0x04, 0xFF))

			}))));

		}

		void kernel::sml_get_proc_sensor_property(cyng::context& ctx)
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
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.get.proc.sensor.property " << cyng::io::to_str(frame));

			//	ServerId: 05 00 15 3B 02 29 7E 
			//	ServerId: 01 E6 1E 13 09 00 16 3C 07 
			//	81 81 C7 86 00 FF                Not set
			//	   81 81 C7 82 04 FF             _______<_ (01 E6 1E 13 09 00 16 3C 07 )
			//	   81 81 C7 82 02 FF              ()
			//	   81 81 C7 82 03 FF             GWF (47 57 46 )
			//	   81 00 60 05 00 00             _ (00 )
			//	   81 81 C7 86 01 FF             0 (30 )
			//	   81 81 C7 86 02 FF             26000 (32 36 30 30 30 )
			//	   01 00 00 09 0B 00             1528100055 (timestamp)
			//	   81 81 C7 82 05 FF             ___________5}_w_ (18 01 16 05 E6 1E 0D 02 BF 0C FA 35 7D 9E 77 03 )
			//	   81 81 C7 86 03 FF              ()
			//	   81 81 61 3C 01 FF             Not set
			//	   81 81 61 3C 02 FF             Not set

			const std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
			const cyng::buffer_t	public_key{ 0x18, 0x01, 0x16, 0x05, (char)0xE6, 0x1E, 0x0D, 0x02, (char)0xBF, 0x0C, (char)0xFA, 0x35, 0x7D, (char)0x9E, 0x77, 0x03 };
			cyng::buffer_t server_id;
			server_id = cyng::value_cast(frame.at(3), server_id);
			const cyng::buffer_t status{ 0x00 };

			//
			//	linearize and set CRC16
			//	append to current SML message
			//
			sml_gen_.append_msg(message(frame.at(1)	//	trx
				, 2 //, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(frame.at(3)	//	server id  
					, OBIS_CODE_ROOT_SENSOR_PROPERTY	//	path entry - 81 81 C7 86 00 FF
					, child_list_tree(OBIS_CODE_ROOT_SENSOR_PROPERTY, {

						//	repeat server id
						parameter_tree(OBIS_CODE_SERVER_ID, make_value(server_id)),

						//	Geräteklasse
						parameter_tree(OBIS_CODE_DEVICE_CLASS, make_value()),

						//	Manufacturer
						parameter_tree(OBIS_DATA_MANUFACTURER, make_value("solosTec")),

						//	Statuswort [octet string]
						parameter_tree(OBIS_CLASS_OP_LOG_STATUS_WORD, make_value(status)),

						//	Bitmaske zur Definition von Bits, deren Änderung zu einem Eintrag im Betriebslogbuch zum Datenspiegel führt
						parameter_tree(OBIS_CLASS_OP_LOG_STATUS_WORD, make_value(static_cast<std::uint8_t>(0x0))),

						//	Durchschnittliche Zeit zwischen zwei empfangenen Datensätzen in Millisekunden
						parameter_tree(OBIS_CODE_AVERAGE_TIME_MS, make_value(static_cast<std::uint16_t>(1234))),

						//	aktuelle UTC-Zeit
						parameter_tree(OBIS_CURRENT_UTC, make_value(now)),

						//	public key
						parameter_tree(OBIS_DATA_PUBLIC_KEY, make_value(public_key)),

						//	AES Schlüssel für wireless M-Bus
						empty_tree(OBIS_DATA_AES_KEY),

						parameter_tree(OBIS_DATA_USER_NAME, make_value("user")),
						parameter_tree(OBIS_DATA_USER_PWD, make_value("pwd"))

			}))));
		}

	}	//	sml
}

