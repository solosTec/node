/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/sml/protocol/generator.h>
#include <smf/sml/protocol/message.h>
#include <smf/sml/protocol/serializer.h>
#include <smf/sml/protocol/value.hpp>
#include <smf/sml/crc16.h>
#include <smf/sml/obis_db.h>
#include <NODE_project_info.h>
#include <cyng/vm/generator.h>
#include <cyng/chrono.h>
#include <cyng/numeric_cast.hpp>
#include <cyng/io/swap.h>
#include <cyng/sys/info.h>
#include <sstream>
#include <iomanip>
#include <boost/core/ignore_unused.hpp>

namespace node
{
	namespace sml
	{
		generator::generator()
			: msg_()
			, group_no_(0)
		{}

		void generator::reset()
		{
			msg_.clear();
			group_no_ = 0;
		}

		cyng::buffer_t generator::boxing() const
		{
			return node::sml::boxing(msg_);
		}

		std::string generator::gen_file_id()
		{
			const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
			const std::tm utc = cyng::chrono::convert_utc(now);
			std::stringstream ss;
			ss
				<< std::setfill('0')
				<< cyng::chrono::year(utc)
				<< std::setw(2)
				<< cyng::chrono::month(utc)
				<< std::setw(2)
				<< cyng::chrono::day(utc)
				<< std::setw(2)
				<< cyng::chrono::hour(utc)
				<< std::setw(2)
				<< cyng::chrono::minute(utc)
				<< std::setw(2)
				<< cyng::chrono::second(utc)
				;
			return ss.str();
		}

		std::size_t generator::append_msg(cyng::tuple_t&& msg)
		{
#ifdef SMF_IO_DEBUG
			CYNG_LOG_TRACE(logger_, "msg: " << cyng::io::to_str(msg));
#endif

			//
			//	linearize and set CRC16
			//
			cyng::buffer_t b = linearize(msg);
			sml_set_crc16(b);

			//
			//	append to current SML message
			//
			msg_.push_back(b);
			return msg_.size();
		}


		req_generator::req_generator()
			: generator()
			, trx_()
		{}

		std::size_t req_generator::public_open(cyng::mac48 client_id
			, cyng::buffer_t const& server_id
			, std::string const& name
			, std::string const& pwd)
		{
			BOOST_ASSERT_MSG(msg_.empty(), "pending SML data");
			return append_msg(message(cyng::make_object(*trx_)	//	trx
				, group_no_++	//	group
				, 0 // abort code
				, BODY_OPEN_REQUEST

				//
				//	generate public open request
				//
				, open_request(cyng::make_object()	// codepage
					, cyng::make_object(client_id.to_buffer())	// clientId
					, cyng::make_object(gen_file_id())	//	req file id
					, cyng::make_object(server_id)	//	server id
					, cyng::make_object(name)	//	name
					, cyng::make_object(pwd)	//	pwd
					, cyng::make_object()	// sml-Version 
				)
			));
		}

		std::size_t req_generator::public_close()
		{
			++trx_;
			return append_msg(message(cyng::make_object(*trx_)
				, 0	//	group is 0 for CLOSE REQUEST
				, 0 //	abort code
				, BODY_CLOSE_REQUEST

				//
				//	generate public open response
				//
				, close_response(cyng::make_object()
				)
			));
		}

		std::size_t req_generator::set_proc_parameter_restart(cyng::buffer_t const& server_id
			, std::string const& username
			, std::string const& password)
		{
			++trx_;
			return append_msg(message(cyng::make_object(*trx_)
				, group_no_++	//	group
				, 0 //	abort code
				, BODY_SET_PROC_PARAMETER_REQUEST	//	0x600

				//
				//	generate public open response
				//
				, set_proc_parameter_request(cyng::make_object(server_id)
					, username
					, password
					, OBIS_CODE_REBOOT
					, empty_tree(OBIS_CODE_REBOOT))
				)
			);
		}

		std::size_t req_generator::set_proc_parameter_ipt_host(cyng::buffer_t const& server_id
			, std::string const& username
			, std::string const& password
			, std::uint8_t idx
			, std::string const& address)
		{
			const obis_path tree_path({OBIS_CODE_ROOT_IPT_PARAM, make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx), make_obis(0x81, 0x49, 0x17, 0x07, 0x00, idx) });

			++trx_;
			return append_msg(message(cyng::make_object(*trx_)
				, group_no_++	//	group
				, 0 //	abort code
				, BODY_SET_PROC_PARAMETER_REQUEST	//	0x600 (1536)

				//
				//	generate process parameter request
				//
				, set_proc_parameter_request(cyng::make_object(server_id)
					, username
					, password
					, tree_path
					, parameter_tree(make_obis(0x81, 0x49, 0x17, 0x07, 0x00, idx), make_value(address)))
				)
			);
		}

		std::size_t req_generator::set_proc_parameter_ipt_port_local(cyng::buffer_t const& server_id
			, std::string const& username
			, std::string const& password
			, std::uint8_t idx
			, std::uint16_t port)
		{
			const obis_path tree_path({OBIS_CODE_ROOT_IPT_PARAM, make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx), make_obis(0x81, 0x49, 0x1A, 0x07, 0x00, idx) });

			++trx_;
			return append_msg(message(cyng::make_object(*trx_)
				, group_no_++	//	group
				, 0 //	abort code
				, BODY_SET_PROC_PARAMETER_REQUEST	//	0x600 (1536)

				//
				//	generate process parameter request
				//
				, set_proc_parameter_request(cyng::make_object(server_id)
					, username
					, password
					, tree_path
					, parameter_tree(make_obis(0x81, 0x49, 0x1A, 0x07, 0x00, idx), make_value(port)))
				)
			);
		}

		std::size_t req_generator::set_proc_parameter_ipt_port_remote(cyng::buffer_t const& server_id
			, std::string const& username
			, std::string const& password
			, std::uint8_t idx
			, std::uint16_t port)
		{
			const obis_path tree_path({OBIS_CODE_ROOT_IPT_PARAM, make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx), make_obis(0x81, 0x49, 0x19, 0x07, 0x00, idx) });

			++trx_;
			return append_msg(message(cyng::make_object(*trx_)
				, group_no_++	//	group
				, 0 //	abort code
				, BODY_SET_PROC_PARAMETER_REQUEST	//	0x600 (1536)

				//
				//	generate process parameter request
				//
				, set_proc_parameter_request(cyng::make_object(server_id)
					, username
					, password
					, tree_path
					, parameter_tree(make_obis(0x81, 0x49, 0x19, 0x07, 0x00, idx), make_value(port)))
				)
			);
		}

		std::size_t req_generator::set_proc_parameter_ipt_user(cyng::buffer_t const& server_id
			, std::string const& username
			, std::string const& password
			, std::uint8_t idx
			, std::string const& str)
		{
			const obis_path tree_path({OBIS_CODE_ROOT_IPT_PARAM, make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx), make_obis(0x81, 0x49, 0x63, 0x3C, 0x01, idx) });

			++trx_;
			return append_msg(message(cyng::make_object(*trx_)
				, group_no_++	//	group
				, 0 //	abort code
				, BODY_SET_PROC_PARAMETER_REQUEST	//	0x600 (1536)

				//
				//	generate process parameter request
				//
				, set_proc_parameter_request(cyng::make_object(server_id)
					, username
					, password
					, tree_path
					, parameter_tree(make_obis(0x81, 0x49, 0x63, 0x3C, 0x01, idx), make_value(str)))
				)
			);
		}

		std::size_t req_generator::set_proc_parameter_ipt_pwd(cyng::buffer_t const& server_id
			, std::string const& username
			, std::string const& password
			, std::uint8_t idx
			, std::string const& str)
		{
			const obis_path tree_path({OBIS_CODE_ROOT_IPT_PARAM, make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx), make_obis(0x81, 0x49, 0x63, 0x3C, 0x02, idx) });

			++trx_;
			return append_msg(message(cyng::make_object(*trx_)
				, group_no_++	//	group
				, 0 //	abort code
				, BODY_SET_PROC_PARAMETER_REQUEST	//	0x600 (1536)

				//
				//	generate process parameter request
				//
				, set_proc_parameter_request(cyng::make_object(server_id)
					, username
					, password
					, tree_path
					, parameter_tree(make_obis(0x81, 0x49, 0x63, 0x3C, 0x02, idx), make_value(str)))
				)
			);
		}


		std::size_t req_generator::set_proc_parameter_wmbus_install(cyng::buffer_t const& server_id
			, std::string const& username
			, std::string const& password
			, bool b)
		{
			const obis_path tree_path({ OBIS_CODE_IF_wMBUS, OBIS_W_MBUS_INSTALL_MODE });

			++trx_;
			return append_msg(message(cyng::make_object(*trx_)
				, group_no_++	//	group
				, 0 //	abort code
				, BODY_SET_PROC_PARAMETER_REQUEST	//	0x600 (1536)

				//
				//	generate process parameter request
				//
				, set_proc_parameter_request(cyng::make_object(server_id)
					, username
					, password
					, tree_path
					, parameter_tree(OBIS_W_MBUS_INSTALL_MODE, make_value(b)))
				)
			);
		}

		std::size_t req_generator::set_proc_parameter_wmbus_power(cyng::buffer_t const& server_id
			, std::string const& username
			, std::string const& password
			, std::uint8_t val)
		{
			const obis_path tree_path({ OBIS_CODE_IF_wMBUS, OBIS_W_MBUS_POWER });

			++trx_;
			return append_msg(message(cyng::make_object(*trx_)
				, group_no_++	//	group
				, 0 //	abort code
				, BODY_SET_PROC_PARAMETER_REQUEST	//	0x600 (1536)

				//
				//	generate process parameter request
				//
				, set_proc_parameter_request(cyng::make_object(server_id)
					, username
					, password
					, tree_path
					, parameter_tree(OBIS_W_MBUS_POWER, make_value(val)))
				)
			);
		}

		std::size_t req_generator::set_proc_parameter_wmbus_protocol(cyng::buffer_t const& server_id
			, std::string const& username
			, std::string const& password
			, std::uint8_t val)
		{
			const obis_path tree_path({ OBIS_CODE_IF_wMBUS, OBIS_W_MBUS_PROTOCOL });

			++trx_;
			return append_msg(message(cyng::make_object(*trx_)
				, group_no_++	//	group
				, 0 //	abort code
				, BODY_SET_PROC_PARAMETER_REQUEST	//	0x600 (1536)

				//
				//	generate process parameter request
				//
				, set_proc_parameter_request(cyng::make_object(server_id)
					, username
					, password
					, tree_path
					, parameter_tree(OBIS_W_MBUS_PROTOCOL, make_value(val)))
				)
			);
		}

		std::size_t req_generator::set_proc_parameter_wmbus_reboot(cyng::buffer_t const& server_id
			, std::string const& username
			, std::string const& password
			, std::uint64_t val)
		{
			const obis_path tree_path({ OBIS_CODE_IF_wMBUS, OBIS_W_MBUS_REBOOT });

			++trx_;
			return append_msg(message(cyng::make_object(*trx_)
				, group_no_++	//	group
				, 0 //	abort code
				, BODY_SET_PROC_PARAMETER_REQUEST	//	0x600 (1536)

				//
				//	generate process parameter request
				//
				, set_proc_parameter_request(cyng::make_object(server_id)
					, username
					, password
					, tree_path
					, parameter_tree(OBIS_W_MBUS_REBOOT, make_value(val)))
				)
			);
		}

		std::size_t req_generator::get_proc_parameter(cyng::buffer_t const& server_id
			, obis code
			, std::string const& username
			, std::string const& password)
		{
			++trx_;
			return append_msg(message(cyng::make_object(*trx_)
				, group_no_++	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_REQUEST	//	0x500

				//
				//	generate get process parameter request
				//
				, get_proc_parameter_request(cyng::make_object(server_id)
					, username
					, password
					, code)
				)
			);
		}

		std::size_t req_generator::get_list(cyng::buffer_t const& client_id
			, cyng::buffer_t const& server_id
			, std::string const& username
			, std::string const& password
			, obis code)
		{
			++trx_;
			return append_msg(message(cyng::make_object(*trx_)
				, group_no_++	//	group
				, 0 //	abort code
				, BODY_GET_LIST_REQUEST	//	0x700

				//
				//	generate get list request
				//
				, get_list_request(cyng::make_object(client_id)
					, cyng::make_object(server_id)
					, username
					, password
					, code)
				)
			);

		}

		std::size_t req_generator::get_proc_parameter_srv_visible(cyng::buffer_t const& srv
			, std::string const& username
			, std::string const& password)
		{
			return get_proc_parameter(srv, OBIS_CODE_ROOT_VISIBLE_DEVICES, username, password);
		}

		std::size_t req_generator::get_proc_parameter_srv_active(cyng::buffer_t const& srv
			, std::string const& username
			, std::string const& password)
		{
			return get_proc_parameter(srv, OBIS_CODE_ROOT_ACTIVE_DEVICES, username, password);
		}

		std::size_t req_generator::get_proc_parameter_firmware(cyng::buffer_t const& srv
			, std::string const& username
			, std::string const& password)
		{
			return get_proc_parameter(srv, OBIS_CODE_ROOT_DEVICE_IDENT, username, password);
		}

		std::size_t req_generator::get_proc_status_word(cyng::buffer_t const& srv
			, std::string const& username
			, std::string const& password)
		{
			return get_proc_parameter(srv, OBIS_CLASS_OP_LOG_STATUS_WORD, username, password);
		}

		std::size_t req_generator::get_proc_parameter_memory(cyng::buffer_t const& srv
			, std::string const& username
			, std::string const& password)
		{
			return get_proc_parameter(srv, OBIS_CODE_ROOT_MEMORY_USAGE, username, password);
		}

		std::size_t req_generator::get_proc_parameter_wireless_mbus_status(cyng::buffer_t const& srv
			, std::string const& username
			, std::string const& password)
		{
			return get_proc_parameter(srv, OBIS_CODE_ROOT_W_MBUS_STATUS, username, password);
		}

		std::size_t req_generator::get_proc_parameter_wireless_mbus_config(cyng::buffer_t const& srv
			, std::string const& username
			, std::string const& password)
		{
			return get_proc_parameter(srv, OBIS_CODE_IF_wMBUS, username, password);
		}

		std::size_t req_generator::get_proc_parameter_ipt_status(cyng::buffer_t const& srv
			, std::string const& username
			, std::string const& password)
		{
			return get_proc_parameter(srv, OBIS_CODE_ROOT_IPT_STATE, username, password);
		}

		std::size_t req_generator::get_proc_parameter_ipt_config(cyng::buffer_t const& srv
			, std::string const& username
			, std::string const& password)
		{
			return get_proc_parameter(srv, OBIS_CODE_ROOT_IPT_PARAM, username, password);
		}

		std::size_t req_generator::get_list_last_data_record(cyng::buffer_t const& client_id
			, cyng::buffer_t const& server_id
			, std::string const& username
			, std::string const& password)
		{
			return get_list(client_id, server_id, username, password, OBIS_CODE(99, 00, 00, 00, 00, 03));
		}


		res_generator::res_generator()
			: generator()
		{}

		std::size_t res_generator::public_open(cyng::object trx
			, cyng::object client_id
			, cyng::object req
			, cyng::object server_id)	//	server id
		{
			BOOST_ASSERT_MSG(msg_.empty(), "pending SML data");

			return append_msg(message(trx	//	trx
				, group_no_++	//	group
				, 0 // abort code
				, BODY_OPEN_RESPONSE

				//
				//	generate public open response
				//
				, open_response(cyng::make_object()	// codepage
					, client_id
					, req	//	req file id
					, server_id	//	server id
					, cyng::make_object()	//	 ref time
					, cyng::make_object()
				)
			));
		}

		std::size_t res_generator::public_close(cyng::object trx)
		{
			return append_msg(message(trx
				, 0	//	group is 0 for CLOSE RESPONSE
				, 0 //	abort code
				, BODY_CLOSE_RESPONSE

				//
				//	generate public open response
				//
				, close_response(cyng::make_object()
				)
			));
		}

		std::size_t res_generator::empty(cyng::object trx, cyng::object server_id, obis path)
		{
			return append_msg(message(trx	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(server_id
					, path	//	path entry
					, parameter_tree(path, empty_tree(path)))));
		}

		std::size_t res_generator::get_proc_parameter_status_word(cyng::object trx
			, cyng::object server_id
			, std::uint32_t status)
		{
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

			//	0x000070202 - ‭0111 0000 0010 0000 0010‬
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
			//	12	always 0 (authorized on WAN)
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

			//auto stat = cyng::swap_num(459266);
			//std::uint32_t stat = 459266;


			return append_msg(message(trx	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(server_id
					, OBIS_CLASS_OP_LOG_STATUS_WORD	//	path entry
					, parameter_tree(OBIS_CLASS_OP_LOG_STATUS_WORD, make_value(status)))));

		}

		std::size_t res_generator::get_proc_parameter_device_id(cyng::object trx
			, cyng::object server_id
			, std::string const& manufacturer
			, cyng::buffer_t const& server_id2
			, std::string const& model_code
			, std::uint32_t serial)
		{
			//
			//	reference implementation used a octet string as serial number - so we do
			//
			std::stringstream ss;
			ss
				<< std::setw(8)
				<< std::setfill('0')
				<< serial
				;
			const std::string serial_str = ss.str();
			
			const auto os_name = cyng::sys::get_os_name();

			return append_msg(message(trx	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(server_id	//	server id
					, OBIS_CODE_ROOT_DEVICE_IDENT	//	path entry - 81 81 C7 82 01 FF

					//
					//	generate get process parameter response
					//	const static obis	DEFINE_OBIS_CODE(81, 81, C7, 82, 01, FF, CODE_ROOT_DEVICE_IDENT);	
					//	see: 7.3.2.9 Datenstruktur zur Abfrage der Geräte-Identifikation) 
					//
					, child_list_tree(OBIS_CODE_ROOT_DEVICE_IDENT, {

						//	device class (81 81 C7 82 53 FF == MUC-LAN/DSL)
						parameter_tree(OBIS_CODE_DEVICE_CLASS, make_value(OBIS_CODE(81, 81, C7, 82, 53, FF))),
						parameter_tree(OBIS_DATA_MANUFACTURER, make_value(manufacturer)),	// manufacturer
						parameter_tree(OBIS_CODE_SERVER_ID, make_value(server_id2)),	// server id

						//	firmware
						child_list_tree(OBIS_CODE_ROOT_FIRMWARE, {
							//	section 1
							child_list_tree(OBIS_CODE(81, 81, c7, 82, 07, 01), {
									parameter_tree(OBIS_CODE(81, 81, c7, 82, 08, ff), make_value("CURRENT_VERSION")),
									parameter_tree(OBIS_CODE(81, 81, 00, 02, 00, 00), make_value(NODE_VERSION)),
									parameter_tree(OBIS_CODE(81, 81, c7, 82, 0e, ff), make_value(true)) 	//	activated
								}),
							//	section 2
							child_list_tree(OBIS_CODE(81, 81, c7, 82, 07, 02),	{
									parameter_tree(OBIS_CODE(81, 81, c7, 82, 08, ff), make_value("KERNEL")),
									parameter_tree(OBIS_CODE(81, 81, 00, 02, 00, 00), make_value(os_name)),
// 									parameter_tree(OBIS_CODE(81, 81, 00, 02, 00, 00), make_value(NODE_PLATFORM)),
									parameter_tree(OBIS_CODE(81, 81, c7, 82, 0e, ff), make_value(true)) 	//	activated
								})

							}),

						//	hardware
						child_list_tree(OBIS_CODE(81, 81, c7, 82, 09, ff), {
							//	Typenschlüssel
							parameter_tree(OBIS_CODE(81, 81, c7, 82, 0a, 01), make_value(model_code)),
							parameter_tree(OBIS_CODE(81, 81, c7, 82, 0a, 02), make_value(serial_str))
							//parameter_tree(OBIS_CODE(81, 81, c7, 82, 0a, 02), make_value(serial))	// works too
						})
					}))));

		}

		std::size_t res_generator::get_proc_mem_usage(cyng::object trx
			, cyng::object server_id
			, std::uint8_t mirror
			, std::uint8_t tmp)
		{
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

			return append_msg(message(trx	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(server_id	//	server id
					, OBIS_CODE_ROOT_MEMORY_USAGE		//	path entry

					//
					//	generate get process parameter response
					//	2 uint8 values 
					//
					, child_list_tree(OBIS_CODE_ROOT_MEMORY_USAGE, {

						parameter_tree(OBIS_CODE(00, 80, 80, 00, 11, FF), make_value(mirror)),	//	mirror
						parameter_tree(OBIS_CODE(00, 80, 80, 00, 12, FF), make_value(tmp))	// tmp
			}))));

		}

		std::size_t res_generator::get_proc_device_time(cyng::object trx
			, cyng::object server_id
			, std::chrono::system_clock::time_point now
			, std::int32_t tz
			, bool sync_active)
		{

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

			return append_msg(message(trx	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(server_id	//	server id
					, OBIS_CODE_ROOT_DEVICE_TIME		//	path entry

					//
					//	generate get process parameter response
					//	2 uint8 values 
					//
					, child_list_tree(OBIS_CODE_ROOT_DEVICE_TIME, {

						parameter_tree(OBIS_CURRENT_UTC, make_value(now)),	//	timestamp (01 00 00 09 0B 00 )
						parameter_tree(OBIS_CODE(00, 00, 60, 08, 00, FF), make_sec_index_value(now)),
						parameter_tree(OBIS_CODE(81, 00, 00, 09, 0B, 01), make_value(tz)),
						parameter_tree(OBIS_CODE(81, 00, 00, 09, 0B, 02), make_value(sync_active))
			}))));
		}

		std::size_t res_generator::get_proc_active_devices(cyng::object trx
			, cyng::object server_id
			, const cyng::store::table* tbl)
		{
			//	1.list: 81, 81, 11, 06, 01, FF
			//	2.list: 81, 81, 11, 06, 02, FF
			//	3.list: 81, 81, 11, 06, 03, FF
			cyng::tuple_t list;	// list of lists
			cyng::tuple_t tpl;	// current list
			
			const cyng::buffer_t tmp;
			tbl->loop([&](cyng::table::record const& rec)->bool {

				if (cyng::value_cast(rec["active"], false)) {

					tpl.push_back(cyng::make_object(child_list_tree(make_obis(0x81, 0x81, 0x11, 0x06, 0x01, tpl.size() + 1), {
						parameter_tree(OBIS_CODE(81, 81, C7, 82, 04, FF), make_value(cyng::value_cast(rec["serverID"], tmp))),
						parameter_tree(OBIS_CODE(81, 81, C7, 82, 02, FF), make_value(cyng::value_cast<std::string>(rec["class"], ""))),
						//	timestamp (01 00 00 09 0B 00 )
						parameter_tree(OBIS_CURRENT_UTC, make_value(cyng::value_cast(rec["lastSeen"], std::chrono::system_clock::now())))
					})));

					//
					//	start new list
					//
					if (tpl.size() == 0xFE)
					{
						list.push_back(cyng::make_object(child_list_tree(make_obis(0x81, 0x81, 0x11, 0x06, list.size() + 1, 0xFF), tpl)));
						tpl.clear();
					}
					BOOST_ASSERT_MSG(list.size() < 0xFA, "active device list to large");
				}

				//continue
				return true;
			});

			//
			//	append last pending list
			//
			list.push_back(cyng::make_object(child_list_tree(make_obis(0x81, 0x81, 0x11, 0x06, list.size() + 1, 0xFF), tpl)));


			return append_msg(message(trx	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(server_id	//	server id
					, OBIS_CODE_ROOT_ACTIVE_DEVICES		//	path entry

					//
					//	generate get process parameter response
					//
					, child_list_tree(OBIS_CODE_ROOT_ACTIVE_DEVICES, list)
			)));
		}

		std::size_t res_generator::get_proc_visible_devices(cyng::object trx
			, cyng::object server_id
			, const cyng::store::table* tbl)
		{

			BOOST_ASSERT(tbl != nullptr);
			BOOST_ASSERT(tbl->meta().get_name() == "devices");

			//	1. list: 81, 81, 10, 06, 01, FF
			//	2. list: 81, 81, 10, 06, 02, FF
			//	3. list: 81, 81, 10, 06, 03, FF
			cyng::tuple_t list;	// list of lists
			cyng::tuple_t tpl;	// current list

			const cyng::buffer_t tmp;
			tbl->loop([&](cyng::table::record const& rec)->bool {

				if (cyng::value_cast(rec["visible"], false)) {

					tpl.push_back(cyng::make_object(child_list_tree(make_obis(0x81, 0x81, 0x10, 0x06, 0x01, tpl.size() + 1), {
						parameter_tree(OBIS_CODE(81, 81, C7, 82, 04, FF), make_value(cyng::value_cast(rec["serverID"], tmp))),
						parameter_tree(OBIS_CODE(81, 81, C7, 82, 02, FF), make_value(cyng::value_cast<std::string>(rec["class"], ""))),
						//	timestamp (01 00 00 09 0B 00 )
						parameter_tree(OBIS_CURRENT_UTC, make_value(cyng::value_cast(rec["lastSeen"], std::chrono::system_clock::now())))
						})));

					//
					//	start new list
					//
					if (tpl.size() == 0xFE)
					{
						list.push_back(cyng::make_object(child_list_tree(make_obis(0x81, 0x81, 0x10, 0x06, list.size() + 1, 0xFF), tpl)));
						tpl.clear();
					}
					BOOST_ASSERT_MSG(list.size() < 0xFA, "visible device list to large");
				}

				//continue
				return true;
			});

			//
			//	append last pending list
			//
			list.push_back(cyng::make_object(child_list_tree(make_obis(0x81, 0x81, 0x10, 0x06, list.size() + 1, 0xFF), tpl)));


			return append_msg(message(trx	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(server_id	//	server id
					, OBIS_CODE_ROOT_VISIBLE_DEVICES		//	path entry

					//
					//	generate get process parameter response
					//
					, child_list_tree(OBIS_CODE_ROOT_VISIBLE_DEVICES, list)
				)));
		}

		std::size_t res_generator::get_proc_sensor_property(cyng::object trx
			, cyng::object server_id
			, const cyng::table::record& rec)
		{
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
			cyng::buffer_t tmp;
			const cyng::buffer_t status{ 0x00 };

			return append_msg(message(trx	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(server_id	//	server id  
					, OBIS_CODE_ROOT_SENSOR_PROPERTY	//	path entry - 81 81 C7 86 00 FF
					, child_list_tree(OBIS_CODE_ROOT_SENSOR_PROPERTY, {

						//	repeat server id
						parameter_tree(OBIS_CODE_SERVER_ID, make_value(cyng::value_cast(server_id, tmp))),

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

		std::size_t res_generator::get_proc_push_ops(cyng::object trx
			, cyng::object server_id
			, const cyng::store::table* tbl)
		{

			BOOST_ASSERT(tbl != nullptr);
			BOOST_ASSERT(tbl->meta().get_name() == "push.ops");

			//	example:
			//	ServerId: 05 00 15 3B 02 17 74 
			//	ServerId: 01 A8 15 70 94 48 03 01 02 
			//	81 81 C7 8A 01 FF                Not set
			//	   81 81 C7 8A 01 [nn]           Not set
			//		  81 81 C7 8A 02 FF          900 (39 30 30 )
			//		  81 81 C7 8A 03 FF          12 (31 32 )
			//		  81 81 C7 8A 04 FF          ____B_ (81 81 C7 8A 42 FF )
			//			 81 81 C7 8A 81 FF       ___p_H___ (01 A8 15 70 94 48 03 01 02 )
			//			 81 81 C7 8A 83 FF       ______ (81 81 C7 86 11 FF )
			//			 81 81 C7 8A 82 FF       Not set
			//		  81 47 17 07 00 FF          DataSink (44 61 74 61 53 69 6E 6B )
			//		  81 49 00 00 10 FF          ____!_ (81 81 C7 8A 21 FF )

			//	1. list: 81 81 C7 8A 01 01
			//	2. list: 81 81 C7 8A 01 02
			//	3. list: 81 81 C7 8A 01 03
			cyng::tuple_t tpl;	// list 

			//
			//	generate list of push targets
			//
			tbl->loop([&](cyng::table::record const& rec)->bool {

				auto idx = cyng::numeric_cast<std::uint8_t>(rec["idx"], 1);
				tpl.push_back(cyng::make_object(child_list_tree(obis(0x81, 0x81, 0xC7, 0x8A, 0x01, idx), {

					parameter_tree(OBIS_CODE(81, 81, C7, 8A, 02, FF), make_value(rec["interval"])),		//	intervall (sec.) [uint16]
					parameter_tree(OBIS_CODE(81, 81, C7, 8A, 03, FF), make_value(rec["delay"])),		//	intervall (sec.) [uint8]
					//	7.3.1.25 Liste möglicher Push-Quellen 
					//	push source: 
					//	* 81 81 C7 8A 42 FF == profile
					//	* 81 81 C7 8A 43 FF == Installationsparameter
					//	* 81 81 C7 8A 44 FF == list of visible sensors/actors
					tree(OBIS_CODE(81, 81, C7, 8A, 04, FF)
					, make_value(OBIS_CODE(81, 81, C7, 8A, 42, FF))
					, {
						parameter_tree(OBIS_CODE(81, 81, C7, 8A, 81, FF), make_value(rec["serverID"])),
						//	15 min period (load profile)
						parameter_tree(OBIS_CODE(81, 81, C7, 8A, 83, FF), make_value(OBIS_PROFILE_15_MINUTE)),
						parameter_tree(OBIS_CODE(81, 81, C7, 8A, 82, FF), make_value())
					}),
					parameter_tree(OBIS_CODE(81, 47, 17, 07, 00, FF), make_value(rec["target"])),		//	Targetname
					//	push service: 
					//	* 81 81 C7 8A 21 FF == IP-T
					//	* 81 81 C7 8A 22 FF == SML client address
					//	* 81 81 C7 8A 23 FF == KNX ID 
					parameter_tree(OBIS_CODE(81, 49, 00, 00, 10, FF), make_value(OBIS_CODE(81, 81, C7, 8A, 21, FF)))

					})));

				return true;	//	continue
			});

			return append_msg(message(trx	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(server_id	//	server id
					, OBIS_PUSH_OPERATIONS		//	path entry

					//
					//	generate get process parameter response
					//
					, child_list_tree(OBIS_PUSH_OPERATIONS, tpl)
				)));

		}

		std::size_t res_generator::get_proc_ipt_params(cyng::object trx
			, cyng::object server_id
			, node::ipt::redundancy const& cfg)
		{
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
			//				  652A96A8C0                      smlValue: 714516672	;;; IP Address as integer: 192.168.150.101
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


			cyng::tuple_t tpl;	// list 

			//
			//	generate list of IP-T parameters (2x)
			//
			std::uint8_t idx{ 1 };
			for (auto const& rec : cfg.config_) {

				try {
					boost::system::error_code ec;
					//auto address = boost::asio::ip::make_address(rec.host_, ec);
					//std::uint32_t numeric_address = cyng::swap_num(address.to_v4().to_uint());	//	network ordering

					std::uint16_t port = std::stoul(rec.service_);

					tpl.push_back(cyng::make_object(child_list_tree(obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx), {


						parameter_tree(obis(0x81, 0x49, 0x17, 0x07, 0x00, idx), make_value(rec.host_)),
						//parameter_tree(obis(0x81, 0x49, 0x17, 0x07, 0x00, idx), make_value(numeric_address)),
						parameter_tree(obis(0x81, 0x49, 0x1A, 0x07, 0x00, idx), make_value(port)),
						parameter_tree(obis(0x81, 0x49, 0x19, 0x07, 0x00, idx), make_value(0u)),
						parameter_tree(obis(0x81, 0x49, 0x63, 0x3C, 0x01, idx), make_value(rec.account_)),
						parameter_tree(obis(0x81, 0x49, 0x63, 0x3C, 0x02, idx), make_value(rec.pwd_))

					})));

					++idx;
				}
				catch (std::exception const& ex) {
#ifdef _DEBUG
					std::cerr
						<< "***error: "
						<< ex.what()
						<< std::endl
						;
#else 
					boost::ignore_unused(ex);
#endif
				}
			}

			const std::uint16_t wait_time = 12;	//	minutes
			const std::uint16_t repetitions = 120;	//	counter
			const bool ssl = false;


			//	waiting time (Wartezeit)
			tpl.push_back(cyng::make_object(parameter_tree(OBIS_CODE(81, 48, 27, 32, 06, 01), make_value(wait_time))));

			//	repetitions
			tpl.push_back(cyng::make_object(parameter_tree(OBIS_CODE(81, 48, 31, 32, 02, 01), make_value(repetitions))));

			//	SSL
			tpl.push_back(cyng::make_object(parameter_tree(OBIS_CODE(00, 80, 80, 00, 03, FF), make_value(ssl))));

			//	certificates (none)
			tpl.push_back(cyng::make_object(empty_tree(OBIS_CODE(00, 80, 80, 00, 04, FF))));

			return append_msg(message(trx	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(server_id	//	server id
					, OBIS_CODE_ROOT_IPT_PARAM		//	path entry

					//
					//	generate get process parameter response
					//
					, child_list_tree(OBIS_CODE_ROOT_IPT_PARAM, tpl)
				)));

		}

		std::size_t res_generator::get_proc_0080800000FF(cyng::object trx
			, cyng::object server_id
			, std::uint32_t value)
		{
			return append_msg(message(trx	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(server_id	
					, OBIS_CODE(00, 80, 80, 00, 00, FF)	//	path entry - 00 80 80 00 00 FF
					, child_list_tree(OBIS_CODE(00, 80, 80, 00, 00, FF), {

						parameter_tree(OBIS_CODE(00, 80, 80, 00, 01, FF), make_value(value))

				}))));
		}


		std::size_t res_generator::get_proc_990000000004(cyng::object trx
			, cyng::object server_id
			, std::string const& value)
		{
			return append_msg(message(trx	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(server_id	
					, OBIS_CODE_ROOT_DEVICE_IDENT	//	path entry - 81 81 C7 82 01 FF (CODE_ROOT_DEVICE_IDENT)
					, child_list_tree(OBIS_CODE(99, 00, 00, 00, 00, 04), {

						parameter_tree(OBIS_CODE(99, 00, 00, 00, 00, 04), make_value(value))

				}))));
		}

		std::size_t res_generator::get_proc_actuators(cyng::object trx
			, cyng::object server_id)
		{
			//00 80 80 11 00 FF                Not set
			//   00 80 80 11 01 FF             Not set
			//	  00 80 80 11 01 01          Not set
			//		 00 80 80 11 10 FF       ____ (0A E0 00 01 )
			//		 00 80 80 11 12 FF       11 (31 31 )
			//		 00 80 80 11 13 FF       True (54 72 75 65 )
			//		 00 80 80 11 14 FF       0 (30 )
			//		 00 80 80 11 15 FF       0 (30 )
			//		 00 80 80 11 16 FF       Einschalten Manuell 0-50% (45 69 6E 73 63 68 61 6C 74 65 6E 20 4D 61 6E 75 65 6C 6C 20 30 2D 35 30 25 )
			//	  00 80 80 11 01 02          Not set
			//		 00 80 80 11 10 FF       ____ (0A E0 00 02 )
			//		 00 80 80 11 12 FF       12 (31 32 )
			//		 00 80 80 11 13 FF       True (54 72 75 65 )
			//		 00 80 80 11 14 FF       0 (30 )
			//		 00 80 80 11 15 FF       0 (30 )
			//		 00 80 80 11 16 FF       Einschalten_Manuell 51-100% (45 69 6E 73 63 68 61 6C 74 65 6E 5F 4D 61 6E 75 65 6C 6C 20 35 31 2D 31 30 30 25 )

			cyng::buffer_t actor_1{ 0x0A, (char)0xE0, 0x00, 0x01 }, actor_2{ 0x0A, (char)0xE0, 0x00, 0x02 };

			return append_msg(message(trx	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(server_id
					, OBIS_ACTUATORS	//	path entry: 00 80 80 11 00 FF
					, child_list_tree(OBIS_CODE(00, 80, 80, 11, 01, FF), {

						child_list_tree(OBIS_CODE(00, 80, 80, 11, 01, 01), {
							parameter_tree(OBIS_CODE(00, 80, 80, 11, 10, FF), make_value(actor_1)),
							parameter_tree(OBIS_CODE(00, 80, 80, 11, 12, FF), make_value(11)),
							parameter_tree(OBIS_CODE(00, 80, 80, 11, 13, FF), make_value(true)),
							parameter_tree(OBIS_CODE(00, 80, 80, 11, 14, FF), make_value(30)),
							parameter_tree(OBIS_CODE(00, 80, 80, 11, 15, FF), make_value(30)),
							parameter_tree(OBIS_CODE(00, 80, 80, 11, 16, FF), make_value("Einschalten Manuell 0-50%"))
						}),
						child_list_tree(OBIS_CODE(00, 80, 80, 11, 01, 02),{
							parameter_tree(OBIS_CODE(00, 80, 80, 11, 10, FF), make_value(actor_2)),
							parameter_tree(OBIS_CODE(00, 80, 80, 11, 12, FF), make_value(11)),
							parameter_tree(OBIS_CODE(00, 80, 80, 11, 13, FF), make_value(true)),
							parameter_tree(OBIS_CODE(00, 80, 80, 11, 14, FF), make_value(30)),
							parameter_tree(OBIS_CODE(00, 80, 80, 11, 15, FF), make_value(30)),
							parameter_tree(OBIS_CODE(00, 80, 80, 11, 16, FF), make_value("Einschalten_Manuell 51-100%"))
						})

					}))));

		}

		std::size_t res_generator::get_profile_op_log(cyng::object trx
			, cyng::object client_id
			, std::chrono::system_clock::time_point act_time
			, std::uint32_t reg_period
			, std::chrono::system_clock::time_point val_time
			, std::uint64_t status
			, std::uint32_t evt
			, obis peer_address
			, std::chrono::system_clock::time_point tp
			, cyng::buffer_t const& server_id
			, std::string const& target
			, std::uint8_t push_nr)
		{
			//	seconds since epoch
			std::uint64_t sec = std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count();

			return append_msg(message(trx	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROFILE_LIST_RESPONSE	//	0x0401

				//
				//	generate get process parameter response
				//
				, get_profile_list_response(client_id
					, act_time
					, reg_period
					, OBIS_CLASS_OP_LOG	//	path entry: 81 81 C7 89 E1 FF
					, val_time
					, status
					, cyng::tuple_t{

						//	81 81 C7 89 E2 FF - OBIS_CLASS_EVENT
						//  62FF                                    unit: 255
						//  5200                                    scaler: 0
						//  64800008                                value: 8388616
						period_entry(OBIS_CLASS_EVENT, 0xFF, 0, cyng::make_object(evt)),

						//	81 81 00 00 00 FF - CODE_PEER_ADDRESS
						//  62FF                                    unit: 255
						//  5200                                    scaler: 0
						//  078146000002FF                          value: 81 46 00 00 02 FF 
						period_entry(OBIS_CODE_PEER_ADDRESS, 0xFF, 0, cyng::make_object(peer_address.to_buffer())),

						//  0781042B070000                          objName: 81 04 2B 07 00 00 
						//  62FE                                    unit: 254
						//  5200                                    scaler: 0
						//  5500000000                              value: 0
						period_entry(OBIS_CLASS_OP_LOG_FIELD_STRENGTH, 0xFE, 0, cyng::make_object(0u)),

						//  81 04 1A 07 00 00                       objName: CLASS_OP_LOG_CELL
						//  62FF                                    unit: 255
						//  5200                                    scaler: 0
						//  6200                                    value: 0
						period_entry(OBIS_CLASS_OP_LOG_CELL, 0xFF, 0, cyng::make_object(0u)),

						//  81 04 17 07 00 00                       objName:  CLASS_OP_LOG_AREA_CODE
						//  62FF                                    unit: 255
						//  5200                                    scaler: 0
						//  6200                                    value: 0
						period_entry(OBIS_CLASS_OP_LOG_AREA_CODE, 0xFF, 0, cyng::make_object(0u)),

						//  81 04 0D 06 00 00                       objName: CLASS_OP_LOG_PROVIDER
						//  62FF                                    unit: 255
						//  5200                                    scaler: 0
						//  6200                                    value: 0
						period_entry(OBIS_CLASS_OP_LOG_PROVIDER, 0xFF, 0, cyng::make_object(0u)),

						//  01 00 00 09 0B 00                       objName:  CURRENT_UTC
						//  6207                                    unit: 7
						//  5200                                    scaler: 0
						//  655B8E3F04                              value: 1536048900
						period_entry(OBIS_CURRENT_UTC, 0x07, 0, cyng::make_object(sec)),
						//period_entry(OBIS_CURRENT_UTC, 0x07, 0, cyng::make_object(1536048900ULL)),

						//  81 81 C7 82 04 FF                       objName:  CODE_SERVER_ID
						//  62FF                                    unit: 255
						//  5200                                    scaler: 0
						//  0A01A815743145040102                    value: 01 A8 15 74 31 45 04 01 02 
						period_entry(OBIS_CODE_SERVER_ID, 0xFF, 0, cyng::make_object(server_id)),

						//  81 47 17 07 00 FF                       objName:  CODE_PUSH_TARGET
						//  62FF                                    unit: 255
						//  5200                                    scaler: 0
						//  0F706F77657240736F6C6F73746563          value: power@solostec
						period_entry(OBIS_CODE_PUSH_TARGET, 0xFF, 0, cyng::make_object(target)),

						//  81 81 C7 8A 01 FF                       objName:  PUSH_OPERATIONS
						//  62FF                                    unit: 255
						//  5200                                    scaler: 0
						//  6201                                    value: 1
						period_entry(OBIS_PUSH_OPERATIONS, 0xFF, 0, cyng::make_object(push_nr))

				})));

     
		}

		std::size_t res_generator::get_profile_op_logs(cyng::object trx
			, cyng::object client_id
			, std::chrono::system_clock::time_point start_time
			, std::chrono::system_clock::time_point end_time
			, const cyng::store::table* tbl)
		{
			BOOST_ASSERT(tbl != nullptr);
			BOOST_ASSERT(tbl->meta().get_name() == "op.log");

			tbl->loop([&](cyng::table::record const& rec) {

				//	[2018-09-05 14:00:34.04194950,00000384,2018-09-05 14:00:34.04194950,00062602,00800008,8146000002FF,2018-09-05 14:00:34.04194580,01A815743145040102,power@solostec,1]

				cyng::buffer_t peer;
				peer = cyng::value_cast(rec["peer"], peer);

				cyng::buffer_t server;
				server = cyng::value_cast(rec["serverId"], server);

				get_profile_op_log(trx
					, client_id
					, cyng::value_cast(rec["actTime"], std::chrono::system_clock::now()) //	act_time
					, cyng::value_cast<std::uint32_t>(rec["regPeriod"], 900u) //	reg_period
					, cyng::value_cast(rec["valTime"], std::chrono::system_clock::now())
					, cyng::value_cast<std::uint64_t>(rec["status"], 0u) //	status
					, cyng::value_cast<std::uint32_t>(rec["event"], 0u) //	evt
					, obis(peer) //	peer_address
					, cyng::value_cast(rec["utc"], std::chrono::system_clock::now())
					, server
					, cyng::value_cast<std::string>(rec["target"], "")
					, cyng::value_cast<std::uint8_t>(rec["pushNr"], 1u));

				//
				//	continue
				//
				return true;
			});

			return 0;
		}

		std::size_t res_generator::get_proc_w_mbus_status(cyng::object trx
			, cyng::object server_id
			, std::string const& manufacturer	// manufacturer of w-mbus adapter
			, cyng::buffer_t const&	id	//	adapter id (EN 13757-3/4)
			, std::string const& firmware	//	firmware version of adapter
			, std::string const& hardware)	//	hardware version of adapter);
		{
			return append_msg(message(trx	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(server_id
					, OBIS_CODE_ROOT_W_MBUS_STATUS	//	path entry - 81 06 0F 06 00 FF
					, child_list_tree(OBIS_CODE_ROOT_W_MBUS_STATUS, {

						parameter_tree(OBIS_W_MBUS_ADAPTER_MANUFACTURER, make_value(manufacturer)),
						parameter_tree(OBIS_W_MBUS_ADAPTER_ID, make_value(id)),
						parameter_tree(OBIS_W_MBUS_FIRMWARE, make_value(firmware)),
						parameter_tree(OBIS_W_MBUS_HARDWARE, make_value(hardware))

			}))));
		}

		std::size_t res_generator::get_proc_w_mbus_if(cyng::object trx
			, cyng::object server_id
			, std::uint8_t protocol	// radio protocol
			, std::uint8_t s_mode	// duration in seconds
			, std::uint8_t t_mode	// duration in seconds
			, std::uint32_t reboot	//	duration in seconds
			, std::uint8_t power	//	transmision power (transmission_power)
			, bool install_mode)
		{
			return append_msg(message(trx	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(server_id
					, OBIS_CODE_IF_wMBUS	//	path entry - 81 06 19 07 00 FF
					, child_list_tree(OBIS_CODE_IF_wMBUS, {

						parameter_tree(OBIS_W_MBUS_PROTOCOL, make_value(protocol)),
						parameter_tree(OBIS_W_MBUS_S_MODE, make_value((s_mode == 0) ? 30 : s_mode)),
						parameter_tree(OBIS_W_MBUS_T_MODE, make_value((t_mode == 0) ? 20 : t_mode)),
						parameter_tree(OBIS_W_MBUS_REBOOT, make_value(reboot)),
						parameter_tree(OBIS_W_MBUS_POWER, make_value(power)),
						parameter_tree(OBIS_W_MBUS_INSTALL_MODE, make_value(install_mode))

			}))));
		}

		std::size_t res_generator::get_list(cyng::object trx
			, cyng::buffer_t const& client_id
			, cyng::buffer_t const& server_id
			, obis list_name
			, cyng::tuple_t act_sensor_time
			, cyng::tuple_t act_gateway_time
			, cyng::tuple_t val_list)
		{
			return append_msg(message(trx	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_LIST_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_list_response(client_id
					, server_id
					, list_name
					, act_sensor_time
					, act_gateway_time
					, val_list)));
		}


		trx::trx()
			: rng_()
			, gen_(rng_())
			, value_()
			, num_(1)
		{
			shuffle(7);
		}

		trx::trx(trx const& other)
			: rng_()
			, gen_(rng_())
			, value_(other.value_)
			, num_(other.num_)
		{}


		void trx::shuffle(std::size_t length)
		{
			std::uniform_int_distribution<int> dis('0', '9');
			value_.resize(length);
			std::generate(value_.begin(), value_.end(), [&]() {
				return dis(gen_);
			});
		}

		trx& trx::operator++()
		{
			++num_;
			return *this;
		}

		trx trx::operator++(int)
		{
			trx tmp(*this);
			++num_;
			return tmp;
		}

		std::string trx::operator*() const
		{
			std::stringstream ss;
			ss
				<< value_
				<< '-'
				<< num_
				;
			return ss.str();

		}

	}
}
