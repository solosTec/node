/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/sml/protocol/reader.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/obis_io.h>
#include <smf/sml/srv_id_io.h>
#include <smf/sml/units.h>
#include <smf/sml/scaler.h>
#include <smf/mbus/defs.h>

#include <cyng/io/io_buffer.h>
#include <cyng/io/io_chrono.hpp>
#include <cyng/io/serializer.h>
#include <cyng/value_cast.hpp>
#include <cyng/numeric_cast.hpp>
#include <cyng/xml.h>
#include <cyng/factory.h>
#include <cyng/vm/generator.h>
#include <cyng/vm/manip.h>
#include <cyng/io/swap.h>

#include <boost/uuid/nil_generator.hpp>
#include <boost/core/ignore_unused.hpp>

namespace node
{
	namespace sml
	{

		reader::reader()
			: rgn_()
			, ro_(rgn_())
		{
			reset();
		}

		void reader::reset()
		{
			ro_.reset(rgn_(), 0);
		}

		cyng::vector_t reader::read(cyng::tuple_t const& msg, std::size_t idx)
		{
			return read_msg(msg.begin(), msg.end(), idx);
		}

		cyng::vector_t reader::read_msg(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end, std::size_t idx)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 5, "SML message");
			if (count!= 5)	return cyng::vector_t{};

			//
			//	instruction vector
			//
			cyng::vector_t prg;

			//
			//	delayed output
			//
			prg << cyng::generate_invoke_unwinded("log.msg.debug"
				, "message #"
				, idx);

			//
			//	reset readout context
			//
			ro_.set_index(idx);

			//
			//	(1) - transaction id
			//	This is typically a printable sequence of 6 up to 9 ASCII values
			//
			cyng::buffer_t buffer;
			ro_.set_trx(cyng::value_cast(*pos++, buffer));

			//
			//	(2) groupNo
			//
			ro_.set_value("groupNo", *pos++);

			//
			//	(3) abortOnError
			//
			ro_.set_value("abortOnError", *pos++);

			//
			//	(4/5) CHOICE - msg type
			//
			cyng::tuple_t choice;
			choice = cyng::value_cast(*pos++, choice);
			prg << cyng::unwinder(read_choice(choice));
			//BOOST_ASSERT_MSG(choice.size() == 2, "CHOICE");
			//if (choice.size() == 2)
			//{
			//	ro_.set_value("code", choice.front());
			//	prg << cyng::unwinder(read_body(choice.front(), choice.back()));
			//}

			//
			//	(6) CRC16
			//
			ro_.set_value("crc16", *pos);

			return prg;
		}

		void reader::set_trx(cyng::buffer_t const& trx)
		{
			ro_.set_trx(trx);
		}

		cyng::vector_t reader::read_choice(cyng::tuple_t const& choice)
		{
			BOOST_ASSERT_MSG(choice.size() == 2, "CHOICE");
			if (choice.size() == 2)
			{
				ro_.set_value("code", choice.front());
				return read_body(choice.front(), choice.back());
			}
			return cyng::vector_t();
		}

		cyng::vector_t reader::read_body(cyng::object type, cyng::object body)
		{
			auto code = cyng::value_cast<std::uint16_t>(type, 0);

			cyng::tuple_t tpl;
			tpl = cyng::value_cast(body, tpl);

			switch (code)
			{
			case BODY_OPEN_REQUEST:
				return read_public_open_request(tpl.begin(), tpl.end());
			case BODY_OPEN_RESPONSE:
				return read_public_open_response(tpl.begin(), tpl.end());
			case BODY_CLOSE_REQUEST:
				return read_public_close_request(tpl.begin(), tpl.end());
			case BODY_CLOSE_RESPONSE:
				return read_public_close_response(tpl.begin(), tpl.end());
			case BODY_GET_PROFILE_PACK_REQUEST:
				//cyng::xml::write(node.append_child("data"), body);
				break;
			case BODY_GET_PROFILE_PACK_RESPONSE:
				//cyng::xml::write(node.append_child("data"), body);
				break;
			case BODY_GET_PROFILE_LIST_REQUEST:
				return read_get_profile_list_request(tpl.begin(), tpl.end());
			case BODY_GET_PROFILE_LIST_RESPONSE:
				return read_get_profile_list_response(tpl.begin(), tpl.end());
			case BODY_GET_PROC_PARAMETER_REQUEST:
				return read_get_proc_parameter_request(tpl.begin(), tpl.end());
			case BODY_GET_PROC_PARAMETER_RESPONSE:
				return read_get_proc_parameter_response(tpl.begin(), tpl.end());
			case BODY_SET_PROC_PARAMETER_REQUEST:
				return read_set_proc_parameter_request(tpl.begin(), tpl.end());
			case BODY_SET_PROC_PARAMETER_RESPONSE:
				//cyng::xml::write(node.append_child("data"), body);
				break;
			case BODY_GET_LIST_REQUEST:
				return read_get_list_request(tpl.begin(), tpl.end());
			case BODY_GET_LIST_RESPONSE:
				//	ToDo: read get list response (0701)
				return read_get_list_response(tpl.begin(), tpl.end());
			case BODY_ATTENTION_RESPONSE:
				return read_attention_response(tpl.begin(), tpl.end());
			default:
				//cyng::xml::write(node.append_child("data"), body);
				break;
			}

			return cyng::generate_invoke("log.msg.fatal"
				, "unknown SML message code"
				, code);
		}

		cyng::vector_t reader::read_public_open_request(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 7, "Public Open Request");
			if (count != 7)	return cyng::vector_t{};

			//	codepage "ISO 8859-15"
			ro_.set_value("codepage", *pos++);

			//
			//	clientId (MAC)
			//	Typically 7 bytes to identify gateway/MUC
			read_client_id(*pos++);

			//
			//	reqFileId
			//
			//ro_.set_value("reqFileId", *pos++);
			ro_.set_value("reqFileId", read_string(*pos++));

			//
			//	serverId
			//
			read_server_id(*pos++);

			//
			//	username
			//
			ro_.set_value("userName", read_string(*pos++));

			//
			//	password
			//
			ro_.set_value("password", read_string(*pos++));

			//
			//	sml-Version: default = 1
			//
			ro_.set_value("SMLVersion", *pos++);

			//
			//	instruction vector
			//
			return cyng::generate_invoke("sml.public.open.request"
				, ro_.pk_
				, ro_.trx_
				, ro_.idx_
				, ro_.client_id_
				, ro_.server_id_
				, ro_.get_value("reqFileId")
				, ro_.get_value("userName")
				, ro_.get_value("password"));

		}

		cyng::vector_t reader::read_public_open_response(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 6, "Public Open Response");
			if (count != 6)	return cyng::vector_t{};

			//	codepage "ISO 8859-15"
			ro_.set_value("codepage", *pos++);

			//
			//	clientId (MAC)
			//	Typically 7 bytes to identify gateway/MUC
			read_client_id(*pos++);

			//
			//	reqFileId
			//
			ro_.set_value("reqFileId", read_string(*pos++));

			//
			//	serverId
			//
			read_server_id(*pos++);

			//
			//	refTime
			//
			ro_.set_value("refTime", *pos++);

			//	sml-Version: default = 1
			ro_.set_value("SMLVersion", *pos++);

			return cyng::generate_invoke("sml.public.open.response"
				, ro_.pk_
				, ro_.trx_
				, ro_.idx_
				, ro_.client_id_
				, ro_.server_id_
				, ro_.get_value("reqFileId"));
		}

		cyng::vector_t reader::read_public_close_request(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 1, "Public Close Request");
			if (count != 1)	return cyng::vector_t{};
			
			ro_.set_value("globalSignature", *pos++);

			return cyng::generate_invoke("sml.public.close.request"
				, ro_.pk_
				, ro_.trx_
				, ro_.idx_);
		}

		cyng::vector_t reader::read_public_close_response(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 1, "Public Close Response");
			if (count != 1)	return cyng::vector_t{};
			
			ro_.set_value("globalSignature", *pos++);

			return cyng::generate_invoke("sml.public.close.response"
				, ro_.pk_
				, ro_.trx_
				, ro_.idx_);
		}

		cyng::vector_t reader::read_get_profile_list_response(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 9, "Get Profile List Response");
			if (count != 9)	return cyng::vector_t{};
			
			//
			//	serverId
			//
			read_server_id(*pos++);

			//
			//	actTime
			//
			ro_.set_value("actTime", read_time(*pos++));

			//
			//	regPeriod
			//
			ro_.set_value("regPeriod", *pos++);

			//
			//	parameterTreePath (OBIS)
			//
			std::vector<obis> path = read_param_tree_path(*pos++);

			//
			//	valTime
			//
			ro_.set_value("valTime", read_time(*pos++));

			//
			//	M-bus status
			//
			ro_.set_value("status", *pos++);

			//	period-List (1)
			//	first we make the TSMLGetProfileListResponse complete, then 
			//	the we store the entries in TSMLPeriodEntry.
			cyng::tuple_t tpl;
			tpl = cyng::value_cast(*pos++, tpl);
			read_period_list(path, tpl.begin(), tpl.end());

			//	rawdata
			ro_.set_value("rawData", *pos++);

			//	periodSignature
			ro_.set_value("signature", *pos++);

			return cyng::generate_invoke("db.insert.meta"
				, ro_.pk_
				, ro_.trx_
				, ro_.idx_
				, ro_.get_value("roTime")
				, ro_.get_value("actTime")
				, ro_.get_value("valTime")
				, ro_.client_id_
				, ro_.server_id_
				, ro_.get_value("status"));

		}

		cyng::vector_t reader::read_get_profile_list_request(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 9, "Get Profile List Request");
			if (count != 9)	return cyng::vector_t{};

			//
			//	serverId
			//	Typically 7 bytes to identify gateway/MUC
			//
			read_server_id(*pos++);

			//
			//	username
			//
			ro_.set_value("userName", read_string(*pos++));

			//
			//	password
			//
			ro_.set_value("password", read_string(*pos++));

			//
			//	rawdata (typically not set)
			//
			ro_.set_value("rawData", *pos++);

			//
			//	start/end time
			//
			ro_.set_value("beginTime", read_time(*pos++));
			ro_.set_value("endTime", read_time(*pos++));

			//	parameterTreePath (OBIS)
			//
			std::vector<obis> path = read_param_tree_path(*pos++);
			BOOST_ASSERT(path.size() == 1);

			//
			//	object list
			//
			auto obj_list = *pos++;
			boost::ignore_unused(obj_list);

			//
			//	dasDetails
			//
			auto const das_details = *pos++;
			boost::ignore_unused(das_details);

			//
			//	instruction vector
			//
			if (path.size()) {
				return cyng::generate_invoke("sml.get.profile.list.request"
					, ro_.pk_
					, ro_.trx_
					, ro_.idx_
					, ro_.client_id_
					, ro_.server_id_
					, ro_.get_value("userName")
					, ro_.get_value("password")
					, ro_.get_value("beginTime")
					, ro_.get_value("endTime")
					, path.front().to_buffer());
			}

			return cyng::vector_t{};
		}

		cyng::vector_t reader::read_get_proc_parameter_response(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 3, "Get Proc Parameter Response");
			if (count != 3)	return cyng::vector_t{};
			
			//
			//	serverId
			//
			read_server_id(*pos++);

			//
			//	parameterTreePath (OBIS)
			//
			std::vector<obis> path = read_param_tree_path(*pos++);
			BOOST_ASSERT(path.size() == 1);

			//
			//	parameterTree
			//
			cyng::tuple_t tpl;
			tpl = cyng::value_cast(*pos++, tpl);

			//
			//	this is a parameter tree
			//	read_param_tree(0, tpl.begin(), tpl.end());
			//
			return read_get_proc_parameter_response(path, 0, tpl.begin(), tpl.end());
		}

		cyng::vector_t reader::read_get_proc_parameter_response(std::vector<obis> path
			, std::size_t depth
			, cyng::tuple_t::const_iterator pos
			, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 3, "SML Tree");
			if (count != 3)	return cyng::vector_t{};
			
			//
			//	1. parameterName Octet String,
			//
			obis code = read_obis(*pos++);
			if (code == path.back()) {
				//	root
				//std::cout << "root: " << code << std::endl;
				BOOST_ASSERT(depth == 0);
			}
			else {
				BOOST_ASSERT(depth == path.size());
				path.push_back(code);
			}

			//
			//	2. parameterValue SML_ProcParValue OPTIONAL,
			//
			auto attr = read_parameter(*pos++);

			//
			//	program vector
			//
			cyng::vector_t prg;

			switch (depth) {
			case 0:
				prg << cyng::unwinder(read_get_proc_parameter_response_L0(attr, pos, path.front()));
				break;
			case 1:
				prg << cyng::unwinder(read_get_proc_parameter_response_L1(attr, pos, path.front(), path.back()));
				break;
			case 2:
				prg << cyng::unwinder(read_get_proc_parameter_response_L2(attr, pos, path.front(), path.back()));
				break;
			case 3:
				prg << cyng::unwinder(read_get_proc_parameter_response_L3(attr, pos, path.front(), path.back()));
				break;
			default:
				break;
			}


			//
			//	3. child_List List_of_SML_Tree OPTIONAL
			//
			return (prg.empty())
				? read_tree_list(path, *pos++, ++depth)
				: prg
				;

		}

		cyng::vector_t reader::read_get_proc_parameter_response_L0(cyng::attr_t attr, cyng::tuple_t::const_iterator pos, obis root)
		{
			if (root == OBIS_CODE_IF_1107) {
				//	get wired IEC (IEC 62506-21) configuartion
				read_get_proc_multiple_parameters(*pos++);
				return cyng::generate_invoke("sml.get.proc.param.iec.config"
					, ro_.pk_
					, ro_.trx_
					, ro_.idx_
					, from_server_id(ro_.server_id_)
					, OBIS_CODE_IF_1107.to_buffer()	//	same as path.front()
					, ro_.get_value(OBIS_CODE_IF_1107_ACTIVE)	 //	(bool) - if true 1107 interface active
					, ro_.get_value(OBIS_CODE_IF_1107_LOOP_TIME)	//	(u) - Loop timeout in seconds
					, ro_.get_value(OBIS_CODE_IF_1107_RETRIES)	//	(u) - Retry count
					, ro_.get_value(OBIS_CODE_IF_1107_MIN_TIMEOUT)	//	(u) - Minimal answer timeout(300)
					, ro_.get_value(OBIS_CODE_IF_1107_MAX_TIMEOUT)	//	(u) - Maximal answer timeout(5000)
					, ro_.get_value(OBIS_CODE_IF_1107_MAX_DATA_RATE)	//	(u) - Maximum data bytes(10240)
					, ro_.get_value(OBIS_CODE_IF_1107_RS485) //	(bool) - if true RS 485, otherwise RS 323
					, ro_.get_value(OBIS_CODE_IF_1107_PROTOCOL_MODE) //	(u) - Protocol mode(A ... D)
					, ro_.get_value(OBIS_CODE_IF_1107_METER_LIST) // Liste der abzufragenden 1107 Zähler
					, ro_.get_value(OBIS_CODE_IF_1107_AUTO_ACTIVATION) //(True)
					, ro_.get_value(OBIS_CODE_IF_1107_TIME_GRID) //	time grid of load profile readout in seconds
					, ro_.get_value(OBIS_CODE_IF_1107_TIME_SYNC) //	time sync in seconds
					, ro_.get_value(OBIS_CODE_IF_1107_MAX_VARIATION) //(seconds)
				);
			}
			else if (root == OBIS_CLASS_OP_LOG_STATUS_WORD) {

				//	generate status word
				return cyng::generate_invoke("sml.get.proc.status.word"
					, ro_.pk_
					, ro_.trx_
					, ro_.idx_
					, from_server_id(ro_.server_id_)
					, OBIS_CLASS_OP_LOG_STATUS_WORD.to_buffer()	//	same as path.front()
					, cyng::value_cast<std::uint32_t>(attr.second, 0u));	//	[u32] value
			}
			else if (root == OBIS_CODE_ROOT_MEMORY_USAGE) {

				//	get memory usage
				read_get_proc_multiple_parameters(*pos++);
				return cyng::generate_invoke("sml.get.proc.param.memory"
					, ro_.pk_
					, ro_.trx_
					, ro_.idx_
					, from_server_id(ro_.server_id_)
					, OBIS_CODE_ROOT_MEMORY_USAGE.to_buffer()	//	same as path.front()
					, ro_.get_value(OBIS_CODE_ROOT_MEMORY_MIRROR)	//	mirror
					, ro_.get_value(OBIS_CODE_ROOT_MEMORY_TMP));	//	tmp
			}
			else if (root == OBIS_CODE_ROOT_W_MBUS_STATUS) {

				//	get memory usage
				read_get_proc_multiple_parameters(*pos++);
				return cyng::generate_invoke("sml.get.proc.param.wmbus.status"
					, ro_.pk_
					, ro_.trx_
					, ro_.idx_
					, from_server_id(ro_.server_id_)
					, OBIS_CODE_ROOT_W_MBUS_STATUS.to_buffer()	//	same as path.front()
					, ro_.get_string(OBIS_W_MBUS_ADAPTER_MANUFACTURER)	//	Manufacturer (RC1180-MBUS3)
					, ro_.get_value(OBIS_W_MBUS_ADAPTER_ID)	//	Adapter ID (A8 15 34 83 40 04 01 31)
					, ro_.get_string(OBIS_W_MBUS_FIRMWARE)	//	firmware version (3.08)
					, ro_.get_string(OBIS_W_MBUS_HARDWARE)	//	hardware version (2.00)
				);
			}
			else if (root == OBIS_CODE_IF_wMBUS) {
				//	get wireless M-Bus configuration
				read_get_proc_multiple_parameters(*pos++);
				return cyng::generate_invoke("sml.get.proc.param.wmbus.config"
					, ro_.pk_
					, ro_.trx_
					, ro_.idx_
					, from_server_id(ro_.server_id_)
					, OBIS_CODE_IF_wMBUS.to_buffer()	//	same as path.front()
					, ro_.get_value(OBIS_W_MBUS_PROTOCOL)	//	protocol
					, ro_.get_value(OBIS_W_MBUS_MODE_S)	//	S2 mode in seconds (30)
					, ro_.get_value(OBIS_W_MBUS_MODE_T)	//	T2 mode in seconds (20)
					, ro_.get_value(OBIS_W_MBUS_REBOOT)	//	automatic reboot in seconds (86400)
					, ro_.get_value(OBIS_W_MBUS_POWER)	//	0
					, ro_.get_value(OBIS_W_MBUS_INSTALL_MODE)	//	installation mode (true)
				);
			}
			else if (root == OBIS_CODE_ROOT_IPT_STATE) {
				//	get IP-T status
				read_get_proc_multiple_parameters(*pos++);
				//	81 49 17 07 00 00 ip address
				//	81 49 1A 07 00 00 local port
				//	81 49 19 07 00 00 remote port
				return cyng::generate_invoke("sml.get.proc.param.ipt.state"
					, ro_.pk_
					, ro_.trx_
					, ro_.idx_
					, from_server_id(ro_.server_id_)
					, OBIS_CODE_ROOT_IPT_STATE.to_buffer()	//	same as path.front()
					, ro_.get_value(OBIS_CODE_ROOT_IPT_STATE_ADDRESS)	//	IP adress
					, ro_.get_value(OBIS_CODE_ROOT_IPT_STATE_PORT_LOCAL)	//	local port
					, ro_.get_value(OBIS_CODE_ROOT_IPT_STATE_PORT_REMOTE)	//	remote port
				);
			}

			return cyng::vector_t{};
		}

		cyng::vector_t reader::read_get_proc_parameter_response_L1(cyng::attr_t attr, cyng::tuple_t::const_iterator pos, obis root, obis code)
		{
			if (root == OBIS_CODE_ROOT_DEVICE_IDENT)
			{
				if (code == OBIS_CODE_DEVICE_CLASS) {
					//
					//	device class
					//	CODE_ROOT_DEVICE_IDENT (81, 81, C7, 82, 01, FF)
					//		CODE_DEVICE_CLASS (81, 81, C7, 82, 02, FF)
					//
#ifdef _DEBUG
					//std::cout << "OBIS_CODE_DEVICE_CLASS: " << cyng::io::to_str(attr.second) << std::endl;
#endif
					return cyng::generate_invoke("sml.get.proc.param.simple"
						, ro_.pk_
						, ro_.trx_
						, ro_.idx_
						, ro_.server_id_
						, code.to_buffer()
						, attr.second);	//	value

				}
				else if (code == OBIS_DATA_MANUFACTURER) {
					//
					//	device class
					//
#ifdef _DEBUG
					//std::cout << "OBIS_DATA_MANUFACTURER: " << cyng::io::to_str(attr.second) << std::endl;
#endif
					return cyng::generate_invoke("sml.get.proc.param.simple"
						, ro_.pk_
						, ro_.trx_
						, ro_.idx_
						, ro_.server_id_
						, code.to_buffer()
						, attr.second);	//	value
				}
				else if (code == OBIS_CODE_SERVER_ID) {
					//
					//	server ID (81 81 C7 82 04 FF)
					//
#ifdef _DEBUG
					//std::cout << "OBIS_CODE_SERVER_ID: " << cyng::io::to_str(attr.second) << std::endl;
#endif
					return cyng::generate_invoke("sml.get.proc.param.simple"
						, ro_.pk_
						, ro_.trx_
						, ro_.idx_
						, ro_.server_id_
						, code.to_buffer()
						, attr.second);	//	value
				}
			}
			else if (root == OBIS_CODE_ROOT_IPT_PARAM) {
				const auto r = code.is_matching(0x81, 0x49, 0x0D, 0x07, 0x00);
				if (r.second) {
#ifdef _DEBUG
					std::cout << "OBIS_CODE_ROOT_IPT_PARAM: " << +r.first << std::endl;
#endif
					//	get IP-T params
					read_get_proc_multiple_parameters(*pos++);
					//	81 49 17 07 00 NN ip address/hostname as string - optional
					//	81 49 1A 07 00 NN local port
					//	81 49 19 07 00 NN remote port
					return cyng::generate_invoke("sml.get.proc.param.ipt.param"
						, ro_.pk_
						, ro_.trx_
						, ro_.idx_
						, from_server_id(ro_.server_id_)
						, OBIS_CODE_ROOT_IPT_PARAM.to_buffer()	//	same as path.front()
						, r.first
						, ro_.get_value(make_obis(0x81, 0x49, 0x17, 0x07, 0x00, r.first))	//	IP adress / hostname
						, ro_.get_value(make_obis(0x81, 0x49, 0x1A, 0x07, 0x00, r.first))	//	local port
						, ro_.get_value(make_obis(0x81, 0x49, 0x19, 0x07, 0x00, r.first))	//	remote port
						, ro_.get_string(make_obis(0x81, 0x49, 0x63, 0x3C, 0x01, r.first))	//	device name
						, ro_.get_string(make_obis(0x81, 0x49, 0x63, 0x3C, 0x02, r.first))	//	password
					);
				}
			}
			return cyng::vector_t{};
		}

		cyng::vector_t reader::read_get_proc_parameter_response_L2(cyng::attr_t attr, cyng::tuple_t::const_iterator pos, obis root, obis code)
		{
			if (root == OBIS_CODE_ROOT_VISIBLE_DEVICES && code.is_matching(0x81, 0x81, 0x10, 0x06)) {

				//
				//	collect meter info
				//	* 81 81 C7 82 04 FF: server ID
				//	* 81 81 C7 82 02 FF: --- (device class)
				//	* 01 00 00 09 0B 00: timestamp
				//
				read_get_proc_multiple_parameters(*pos++);

				return cyng::generate_invoke("sml.get.proc.param.srv.visible"
					, ro_.pk_
					, ro_.trx_
					, ro_.idx_
					, from_server_id(ro_.server_id_)
					, OBIS_CODE_ROOT_VISIBLE_DEVICES.to_buffer()
					, code.get_number()	//	4/5 
					, ro_.get_value(OBIS_CODE_SERVER_ID)	//	meter ID
					, ro_.get_string(OBIS_CODE_DEVICE_CLASS)	//	device class
					, ro_.get_value(OBIS_CURRENT_UTC));	//	UTC
			}
			else if (root == OBIS_CODE_ROOT_ACTIVE_DEVICES && code.is_matching(0x81, 0x81, 0x11, 0x06)) {

				//
				//	collect meter info
				//	* 81 81 C7 82 04 FF: server ID
				//	* 81 81 C7 82 02 FF: --- (device class)
				//	* 01 00 00 09 0B 00: timestamp
				//
				read_get_proc_multiple_parameters(*pos++);

				return cyng::generate_invoke("sml.get.proc.param.srv.active"
					, ro_.pk_
					, ro_.trx_
					, ro_.idx_
					, from_server_id(ro_.server_id_)
					, OBIS_CODE_ROOT_ACTIVE_DEVICES.to_buffer()
					, code.get_number()	//	4/5 
					, ro_.get_value(OBIS_CODE_SERVER_ID)	//	meter ID
					, ro_.get_string(OBIS_CODE_DEVICE_CLASS)	//	device class
					, ro_.get_value(OBIS_CURRENT_UTC));	//	UTC
			}
			else if (root == OBIS_CODE_ROOT_DEVICE_IDENT && code.is_matching(0x81, 0x81, 0xc7, 0x82, 0x07).second) {

				//	* 81, 81, c7, 82, 08, ff:	CURRENT_VERSION/KERNEL
				//	* 81, 81, 00, 02, 00, 00:	VERSION
				//	* 81, 81, c7, 82, 0e, ff:	activated/deactivated
				read_get_proc_multiple_parameters(*pos++);

#ifdef _DEBUG
				//std::cout << "81 81 c7 82 08 ff: " << cyng::io::to_str(ro_.get_value("81 81 c7 82 08 ff")) << std::endl;
				//std::cout << "81 81 00 02 00 00: " << cyng::io::to_str(ro_.get_value("81 81 00 02 00 00")) << std::endl;
				//std::cout << "81 81 c7 82 0e ff: " << cyng::io::to_str(ro_.get_value("81 81 c7 82 0e ff")) << std::endl;
#endif

				return cyng::generate_invoke("sml.get.proc.param.firmware"
					, ro_.pk_
					, ro_.trx_
					, ro_.idx_
					, from_server_id(ro_.server_id_)
					, OBIS_CODE_ROOT_DEVICE_IDENT.to_buffer()
					, code.get_storage()	//	[5] as u32
					, ro_.get_string(OBIS_CODE_DEVICE_KERNEL)	//	root-device-id name/section
					, ro_.get_string(OBIS_CODE_VERSION)	//	version
					, ro_.get_value(OBIS_CODE_DEVICE_ACTIVATED));	//	active/inactive

			}
			return cyng::vector_t{};
		}

		cyng::vector_t reader::read_get_proc_parameter_response_L3(cyng::attr_t attr, cyng::tuple_t::const_iterator pos, obis root, obis code)
		{
			return cyng::vector_t{};
		}


		cyng::vector_t reader::read_tree_list(std::vector<obis> path, cyng::object obj, std::size_t depth)
		{
			cyng::vector_t prg;
			cyng::tuple_t tpl;
			tpl = cyng::value_cast(obj, tpl);
			for (auto const child : tpl)
			{
				cyng::tuple_t tmp;
				tmp = cyng::value_cast(child, tmp);

				prg << cyng::unwinder(read_get_proc_parameter_response(path, depth, tmp.begin(), tmp.end()));
			}
			return prg;
		}

		void reader::read_get_proc_multiple_parameters(cyng::object obj)
		{
			cyng::tuple_t tpl;
			tpl = cyng::value_cast(obj, tpl);
			for (auto const child : tpl)
			{
				ro_.set_value(read_get_proc_single_parameter(child));
			}

		}

		cyng::vector_t reader::read_get_proc_parameter_request(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 5, "Get Profile List Request");
			if (count != 5)	return cyng::vector_t{};
			
			//
			//	serverId
			//
			read_server_id(*pos++);

			//
			//	username
			//
			ro_.set_value("userName", read_string(*pos++));

			//
			//	password
			//
			ro_.set_value("password", read_string(*pos++));

			//
			//	parameterTreePath == parameter address
			//
			std::vector<obis> path = read_param_tree_path(*pos++);
			BOOST_ASSERT(!path.empty());

			//
			//	attribute/constraints
			//
			//	*pos

			if (path.size() == 1)
			{
				return cyng::generate_invoke("sml.get.proc.parameter.request"
					, ro_.pk_
					, ro_.trx_
					, ro_.idx_
					, ro_.server_id_
					, ro_.get_value("userName")
					, ro_.get_value("password")
					, path.at(0).to_buffer()
					, *pos);

			}

			return cyng::generate_invoke("sml.get.proc.parameter.request"
				, ro_.pk_
				, ro_.trx_
				, ro_.idx_
				, ro_.server_id_
				, ro_.get_value("userName")
				, ro_.get_value("password")
				, to_hex(path.at(0))	//	ToDo: send complete path
				, *pos);	//	attribute
		}

		cyng::vector_t reader::read_set_proc_parameter_request(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 5, "Set Proc Parameter Request");
            if (count != 5) return cyng::vector_t{};
            
            //
            //	serverId
            //
            read_server_id(*pos++);

            //
            //	username
            //
            ro_.set_value("userName", read_string(*pos++));

            //
            //	password
            //
			ro_.set_value("password", read_string(*pos++));

            //
            //	parameterTreePath == parameter address
            //
            std::vector<obis> path = read_param_tree_path(*pos++);

            //
            //	parameterTree
            //
            cyng::tuple_t tpl;
            tpl = cyng::value_cast(*pos++, tpl);

            //
            //	recursiv call to an parameter tree - similiar to read_param_tree()
            //
            return read_set_proc_parameter_request_tree(path, 0, tpl.begin(), tpl.end());
		}

		cyng::vector_t reader::read_set_proc_parameter_request_tree(std::vector<obis> path
			, std::size_t depth
			, cyng::tuple_t::const_iterator pos
			, cyng::tuple_t::const_iterator end)
		{
			cyng::vector_t prg;

			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 3, "SML Tree");
            if (count != 3) return cyng::vector_t{};

			//
			//	1. parameterName Octet String,
			//
			obis code = read_obis(*pos++);
			if (code == path.back()) {
				//	root
				//std::cout << "root: " << code << std::endl;
				BOOST_ASSERT(depth == 0);
			}
			else {
				path.push_back(code);
			}

			//
			//	2. parameterValue SML_ProcParValue OPTIONAL,
			//
			auto attr = read_parameter(*pos++);

			if (depth == 0) {

				if (OBIS_CODE_IF_wMBUS == path.front()) {

					//
					//	wireless M-Bus parameter
					//
					return set_param_if_mbus(code, attr, pos);
				}
				else if (OBIS_CODE_IF_1107 == path.front()) {

					//
					//	root: IEC 62056-21 device
					//
					return set_param_if_1107(code, attr, pos);

				}
				else if (OBIS_CODE_ROOT_IPT_PARAM == path.front()) {

					//
					//	set IP-T parameters
					//
					BOOST_ASSERT_MSG(path.size() == 3, "OBIS_CODE_ROOT_IPT_PARAM param tree too short");
					//prg << cyng::unwinder(set_proc_param_request_ipt_param(code, attr.second));
					return set_proc_param_request_ipt_param(code, attr.second);

				}
				else if (OBIS_CODE_ACTIVATE_DEVICE == path.front()) {

					//
					//	activate device
					//
					BOOST_ASSERT_MSG(path.size() == 3, "OBIS_CODE_ACTIVATE_DEVICE param tree too short");

					cyng::buffer_t tmp;
					tmp = cyng::value_cast(attr.second, tmp);

					auto r = path.at(1).is_matching(0x81, 0x81, 0x11, 0x06, 0xfb);
					if (r.second) {

						//prg << cyng::generate_invoke_unwinded("sml.set.proc.activate"
						//	, ro_.pk_
						//	, ro_.trx_
						//	, r.first	//	should be 1
						//	, ro_.server_id_
						//	, ro_.get_value("userName")
						//	, ro_.get_value("password")
						//	, tmp);
						return cyng::generate_invoke("sml.set.proc.activate"
							, ro_.pk_
							, ro_.trx_
							, r.first	//	should be 1
							, ro_.server_id_
							, ro_.get_value("userName")
							, ro_.get_value("password")
							, tmp);
					}

				}
				else if (OBIS_CODE_DEACTIVATE_DEVICE == path.front()) {

					//
					//	deactivate device
					//
					BOOST_ASSERT_MSG(path.size() == 3, "OBIS_CODE_DEACTIVATE_DEVICE parameter tree too short");

					cyng::buffer_t tmp;
					tmp = cyng::value_cast(attr.second, tmp);

					auto r = path.at(1).is_matching(0x81, 0x81, 0x11, 0x06, 0xfc);
					if (r.second) {

						//prg << cyng::generate_invoke_unwinded("sml.set.proc.deactivate"
						//	, ro_.pk_
						//	, ro_.trx_
						//	, r.first	//	should be 1
						//	, ro_.server_id_
						//	, ro_.get_value("userName")
						//	, ro_.get_value("password")
						//	, tmp);
						return cyng::generate_invoke("sml.set.proc.deactivate"
							, ro_.pk_
							, ro_.trx_
							, r.first	//	should be 1
							, ro_.server_id_
							, ro_.get_value("userName")
							, ro_.get_value("password")
							, tmp);
					}
				}
				else if (OBIS_CODE_DELETE_DEVICE == path.front()) {

					//
					//	delete device
					//
					BOOST_ASSERT_MSG(path.size() == 3, "OBIS_CODE_DELETE_DEVICE parameter tree too short");

					cyng::buffer_t tmp;
					tmp = cyng::value_cast(attr.second, tmp);

					auto r = path.at(1).is_matching(0x81, 0x81, 0x11, 0x06, 0xfd);
					if (r.second) {

						//prg << cyng::generate_invoke_unwinded("sml.set.proc.delete"
						//	, ro_.pk_
						//	, ro_.trx_
						//	, r.first	//	should be 1
						//	, ro_.server_id_
						//	, ro_.get_value("userName")
						//	, ro_.get_value("password")
						//	, tmp);
						return cyng::generate_invoke("sml.set.proc.delete"
							, ro_.pk_
							, ro_.trx_
							, r.first	//	should be 1
							, ro_.server_id_
							, ro_.get_value("userName")
							, ro_.get_value("password")
							, tmp);
					}
				}
				else if (OBIS_PUSH_OPERATIONS == path.front()) {

					//
					//	set push target
					//
					//	example: push delay
					//	81 81 c7 8a 01 ff => 81 81 c7 8a 01 [01] => 81 81 c7 8a 03 ff
					//	//	std::cout << to_hex(path) << std::endl;
					BOOST_ASSERT_MSG(path.size() == 2, "OBIS_PUSH_OPERATIONS param tree has wro");
					return set_proc_param_request_push_op(code, attr.second);
					//prg << cyng::unwinder(set_proc_param_request_push_op(code, attr.second));

				}
			}

			//
			//	3. child_List List_of_SML_Tree OPTIONAL
			//
			cyng::tuple_t tpl;
			tpl = cyng::value_cast(*pos++, tpl);
			for (auto const child : tpl)
			{
				cyng::tuple_t tmp;
				tmp = cyng::value_cast(child, tmp);

				//
				//	recursive call of read_set_proc_parameter_request_tree()
				//
				prg << cyng::unwinder(read_set_proc_parameter_request_tree(path, depth + 1, tmp.begin(), tmp.end()));
			}
			return prg;
		}

		cyng::vector_t reader::set_param_if_mbus(obis code, cyng::attr_t attr, cyng::tuple_t::const_iterator pos)
		{
			return cyng::generate_invoke("sml.set.proc.mbus.param"
				, ro_.pk_
				, ro_.trx_
				, ro_.server_id_
				, code.to_buffer()
				, ro_.get_value("userName")
				, ro_.get_value("password")
				, attr.second);
		}

		cyng::vector_t reader::set_param_if_1107(obis code, cyng::attr_t attr, cyng::tuple_t::const_iterator pos)
		{
			if (OBIS_CODE_IF_1107_METER_LIST == code) {

				//
				//	read device list
				//
				//auto r = code.is_matching(0x81, 0x81, 0xC7, 0x93, 0x09);
				//if (r.second) {

				//	//
				//	//	add/modify IEC 62056-21 device
				//	//	
				//	//BOOST_ASSERT(path.size() == 3);

				//	//
				//	//	collect if 1107 device data
				//	//
				//	cyng::tuple_t tpl;
				//	tpl = cyng::value_cast(*pos, tpl);
				//	BOOST_ASSERT(tpl.size() <= 5);
				//	cyng::param_map_t params;
				//	for (auto obj : tpl) {
				//		auto const m = read_param_tree(1, obj);
				//		params.insert(m.begin(), m.end());	//	preserve existing keys
				//	}

				//	return cyng::generate_invoke("sml.set.proc.if1107.device"
				//		, ro_.pk_
				//		, ro_.trx_
				//		, r.first
				//		, ro_.server_id_
				//		, ro_.get_value("userName")
				//		, ro_.get_value("password")
				//		, params);
				//}

			}

			//
			//	set IEC 62056-21 parameters
			//
			return cyng::generate_invoke("sml.set.proc.if1107.param"
				, ro_.pk_
				, ro_.trx_
				, ro_.server_id_
				, ro_.get_value("userName")
				, ro_.get_value("password")
				, code.to_buffer()
				, attr.second);

		}

		cyng::vector_t reader::set_proc_param_request_push_op(obis code, cyng::object obj)
		{
			//if (path.size() > 2) {
			//	auto r = path.at(1).is_matching(0x81, 0x81, 0xc7, 0x8a, 0x01);
			//	if (r.second) {
			//		if (path.at(2) == OBIS_CODE(81, 81, c7, 8a, 02, ff)) {


			//			return cyng::generate_invoke("sml.set.proc.push.interval"
			//				, ro_.pk_
			//				, ro_.trx_
			//				, r.first
			//				, ro_.server_id_
			//				, ro_.get_value("userName")
			//				, ro_.get_value("password")
			//				, std::chrono::seconds(cyng::numeric_cast<std::int64_t>(obj, 900)));

			//		}
			//		else if (path.at(2) == OBIS_CODE(81, 81, c7, 8a, 03, ff)) {

			//			return cyng::generate_invoke("sml.set.proc.push.delay"
			//				, ro_.pk_
			//				, ro_.trx_
			//				, r.first
			//				, ro_.server_id_
			//				, ro_.get_value("userName")
			//				, ro_.get_value("password")
			//				, std::chrono::seconds(cyng::numeric_cast<std::int64_t>(obj, 0)));

			//		}
			//		else if (path.at(2) == OBIS_CODE(81, 47, 17, 07, 00, FF)) {

			//			cyng::buffer_t tmp;
			//			tmp = cyng::value_cast(obj, tmp);

			//			return cyng::generate_invoke("sml.set.proc.push.name"
			//				, ro_.pk_
			//				, ro_.trx_
			//				, r.first
			//				, ro_.server_id_
			//				, ro_.get_value("userName")
			//				, ro_.get_value("password")
			//				, std::string(tmp.begin(), tmp.end()));

			//		}
			//	}
			//}

			//	param tree too short
			return cyng::generate_invoke("log.msg.warning", "param tree too short");

		}

		cyng::vector_t reader::set_proc_param_request_ipt_param(obis code, cyng::object obj)
		{
			//
			//	set IP-T host address
			//
			auto m = code.is_matching(0x81, 0x49, 0x17, 0x07, 0x00);
			if (m.second) {

				BOOST_ASSERT(m.first != 0);
				--m.first;

				//	remove host byte ordering
				//auto num = cyng::swap_num(cyng::numeric_cast<std::uint32_t>(attr.second, 0u));
				//boost::asio::ip::address address = boost::asio::ip::make_address_v4(num);
				return cyng::generate_invoke("sml.set.proc.ipt.param.address"
					, ro_.pk_
					, ro_.trx_
					, m.first
					, obj);

			}

			//
			//	set IP-T target port
			//
			m = code.is_matching(0x81, 0x49, 0x1A, 0x07, 0x00);
			if (m.second) {

				BOOST_ASSERT(m.first != 0);
				--m.first;

				//
				//	set IP-T target port
				//
				auto port = cyng::numeric_cast<std::uint16_t>(obj, 0u);
				return cyng::generate_invoke("sml.set.proc.ipt.param.port.target"
					, ro_.pk_
					, ro_.trx_
					, m.first
					, port);

			}

			//
			//	set IP-T user name
			//
			m = code.is_matching(0x81, 0x49, 0x63, 0x3C, 0x01);
			if (m.second) {

				BOOST_ASSERT(m.first != 0);
				--m.first;

				//
				//	set IP-T user name
				//
				cyng::buffer_t tmp;
				tmp = cyng::value_cast(obj, tmp);

				return cyng::generate_invoke("sml.set.proc.ipt.param.user"
					, ro_.pk_
					, ro_.trx_
					, m.first
					, std::string(tmp.begin(), tmp.end()));
			}

			//
			//	set IP-T password
			//
			m = code.is_matching(0x81, 0x49, 0x63, 0x3C, 0x02);
			if (m.second) {

				BOOST_ASSERT(m.first != 0);
				--m.first;

				//
				//	set IP-T password
				//
				cyng::buffer_t tmp;
				tmp = cyng::value_cast(obj, tmp);

				return cyng::generate_invoke("sml.set.proc.ipt.param.pwd"
					, ro_.pk_
					, ro_.trx_
					, m.first
					, std::string(tmp.begin(), tmp.end()));

			}

			//	param tree too short
			return cyng::generate_invoke("log.msg.warning", "unknown IP-T parameter", code.to_str());
		}


		cyng::vector_t reader::read_get_list_request(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 5, "Get List Request");
			if (count != 5) return cyng::vector_t{};

			//
			//	clientId - MAC address from caller
			//
			read_client_id(*pos++);

			//
			//	serverId - meter ID
			//
			read_server_id(*pos++);

			//
			//	username
			//
			ro_.set_value("userName", read_string(*pos++));

			//
			//	password
			//
			ro_.set_value("password", read_string(*pos++));

			//
			//	list name
			//
			obis code = read_obis(*pos++);

			return cyng::generate_invoke("sml.get.list.request"
				, ro_.pk_
				, ro_.trx_
				, ro_.idx_
				, ro_.client_id_
				, ro_.server_id_
				, ro_.get_value("reqFileId")
				, ro_.get_value("userName")
				, ro_.get_value("password")
				, code.to_buffer());
		}

		cyng::vector_t reader::read_get_list_response(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 7, "Get List Response");
			if (count != 7) return cyng::vector_t{};

			//
			//	clientId - optional
			//
			read_client_id(*pos++);

			//
			//	serverId - meter ID
			//
			read_server_id(*pos++);

			//
			//	list name - optional
			//
			obis code = read_obis(*pos++);
			if (code.is_nil()) {
				code = OBIS_CODE(99, 00, 00, 00, 00, 03);	//	last data set
			}

			//
			//	read act. sensor time - optional
			//
			ro_.set_value("actSensorTime", read_time(*pos++));

			//
			//	valList
			//
			cyng::tuple_t tpl;
			tpl = cyng::value_cast(*pos++, tpl);
			read_sml_list(code, tpl.begin(), tpl.end());

			//
			//	SML_Signature - OPTIONAL
			//
			ro_.set_value("listSignature", *pos++);

			//
			//	SML_Time - OPTIONAL 
			//
			ro_.set_value("actGatewayTime", read_time(*pos++));

			
			return cyng::generate_invoke("sml.get.list.response"
				, ro_.pk_
				, ro_.trx_
				, ro_.idx_
				, ro_.client_id_	//	mostly empty
				, ro_.server_id_
				, code.to_buffer()
				, ro_.get_value("values")
				, ro_.get_value("actSensorTime")
				, ro_.get_value("actGatewayTime"));
		}

		cyng::vector_t reader::read_attention_response(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 4, "Attention Response");
            if (count != 4) return cyng::vector_t{};

			//
			//	serverId
			//
			read_server_id(*pos++);

			//
			//	attentionNo (OBIS)
			//
			obis code = read_obis(*pos++);

			//
			//	attentionMsg
			//
			ro_.set_value("attentionMsg", *pos++);

			//
			//	attentionDetails (SML_Tree)
			//
			read_param_tree(0, *pos++);

			return cyng::generate_invoke("sml.attention.msg"
				, ro_.pk_
				, ro_.trx_
				, ro_.idx_
				, from_server_id(ro_.server_id_)
				, code.to_buffer()
				, ro_.get_value("attentionMsg"));
		}

		void reader::read_sml_list(obis code
			, cyng::tuple_t::const_iterator pos
			, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			boost::ignore_unused(count);	//	release version

			//
			//	list of tuples (period entry)
			//
			std::size_t counter{ 0 };
			while (pos != end)
			{
				cyng::tuple_t tpl;
				tpl = cyng::value_cast(*pos++, tpl);
				read_list_entry(code, counter, tpl.begin(), tpl.end());
				++counter;

			}
		}

		void reader::read_list_entry(obis list_name
			, std::size_t index
			, cyng::tuple_t::const_iterator pos
			, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 7, "List Entry");
			if (count != 7) return;

			//
			//	object name
			//
			obis code = read_obis(*pos++);

			//
			//	status - optional
			//
			ro_.set_value("status", *pos++);

			//
			//	valTime - optional
			//
			auto val_time = read_time(*pos++);
			ro_.set_value("valTime", val_time);

			//
			//	unit (see sml_unit_enum) - optional
			//
			ro_.set_value("SMLUnit", *pos);
			auto const unit = read_unit(*pos++);

			//
			//	scaler - optional
			//
			auto const scaler = read_scaler(*pos++);

			//
			//	value - note value with OBIS code
			//
			auto val = read_value(code, scaler, unit, *pos++);
			auto obj = cyng::param_map_factory("unit", unit)("scaler", scaler)("value", val)("valTime", val_time)();
			ro_.set_value(code, "values", obj);

			//
			//	valueSignature
			//
			ro_.set_value("signature", *pos++);

		}

		void reader::read_period_list(std::vector<obis> const& path
			, cyng::tuple_t::const_iterator pos
			, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			boost::ignore_unused(count);	//	release version

			//
			//	list of tuples (period entry)
			//
			std::size_t counter{ 0 };
			while (pos != end)
			{
				cyng::tuple_t tpl;
				tpl = cyng::value_cast(*pos++, tpl);
				read_period_entry(path, counter, tpl.begin(), tpl.end());
				++counter;

			}
		}

		void reader::read_period_entry(std::vector<obis> const& path
			, std::size_t index
			, cyng::tuple_t::const_iterator pos
			, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 5, "Period Entry");
            if (count != 5) return;

			//
			//	object name
			//
			obis code = read_obis(*pos++);

			//
			//	unit (see sml_unit_enum)
			//
			ro_.set_value("SMLUnit", *pos);
			auto const unit = read_unit(*pos++);

			//
			//	scaler
			//
			const auto scaler = read_scaler(*pos++);

			//
			//	value
			//
			auto val = read_value(code, scaler, unit, *pos++);
			ro_.set_value("value", val);

			//
			//	valueSignature
			//
			ro_.set_value("signature", *pos++);

		}

		std::vector<obis> reader::read_param_tree_path(cyng::object obj)
		{
			std::vector<obis> result;

			cyng::tuple_t tpl;
			tpl = cyng::value_cast(obj, tpl);
			for (auto obj : tpl)
			{

				const obis object_type = read_obis(obj);
				result.push_back(object_type);

			}
			return result;
		}

		std::string reader::read_server_id(cyng::object obj)
		{
			ro_.server_id_ = cyng::value_cast(obj, ro_.server_id_);
			return from_server_id(ro_.server_id_);
		}

		std::string reader::read_client_id(cyng::object obj)
		{
			ro_.client_id_ = cyng::value_cast(obj, ro_.client_id_);
			return from_server_id(ro_.client_id_);
		}

		cyng::object reader::read_value(obis code, std::int8_t scaler, std::uint8_t unit, cyng::object obj)
		{
			//
			//	write value
			//
			if (OBIS_DATA_MANUFACTURER == code)
			{
				cyng::buffer_t buffer;
				buffer = cyng::value_cast(obj, buffer);
				const auto manufacturer = cyng::io::to_ascii(buffer);
				return cyng::make_object(manufacturer);
			}
			else if (OBIS_CURRENT_UTC == code)
			{
				if (obj.get_class().tag() == cyng::TC_TUPLE)
				{
					ro_.set_value(code, read_time(obj));
				}
				else
				{
					const auto tm = cyng::value_cast<std::uint32_t>(obj, 0);
					const auto tp = std::chrono::system_clock::from_time_t(tm);
					return cyng::make_object(tp);
				}
			}
			else if (OBIS_ACT_SENSOR_TIME == code)
			{
				const auto tm = cyng::value_cast<std::uint32_t>(obj, 0);
				const auto tp = std::chrono::system_clock::from_time_t(tm);
				ro_.set_value(get_name(code), cyng::make_object(tp));
			}
			else if (OBIS_SERIAL_NR == code)
			{
				cyng::buffer_t buffer;
				buffer = cyng::value_cast(obj, buffer);
				const auto serial_nr = cyng::io::to_hex(buffer);
				return cyng::make_object(serial_nr);
			}
			else if (OBIS_SERIAL_NR_SECOND == code)
			{
				cyng::buffer_t buffer;
				buffer = cyng::value_cast(obj, buffer);
				//std::reverse(buffer.begin(), buffer.end());
				const auto serial_nr = cyng::io::to_hex(buffer);
				ro_.set_value(get_name(code), cyng::make_object(serial_nr));
				if (buffer.size() == 8)
				{
					//
					//	extract manufacturer name
					//	example: [2d2c]688668691c04
					//
					const auto manufacturer = decode(buffer.at(0), buffer.at(1));
					return cyng::make_object(manufacturer);
				}
			}
			else if (OBIS_MBUS_STATE == code) {
				auto const state = cyng::numeric_cast<std::int32_t>(obj, 0);
				return cyng::make_object(state);
			}

			if (scaler != 0)
			{
				switch (obj.get_class().tag())
				{
				case cyng::TC_INT64:
				{
					const std::int64_t value = cyng::value_cast<std::int64_t>(obj, 0);
					const auto str = scale_value(value, scaler);
					return cyng::make_object(str);
				}
				break;
				case cyng::TC_INT32:
				{
					const std::int32_t value = cyng::value_cast<std::int32_t>(obj, 0);
					const auto str = scale_value(value, scaler);
					return cyng::make_object(str);
				}
				break;
				case cyng::TC_UINT64:
				{
					const std::uint64_t value = cyng::value_cast<std::uint64_t>(obj, 0);
					const auto str = scale_value(value, scaler);
					return cyng::make_object(str);
				}
				break;
				case cyng::TC_UINT32:
				{
					const std::uint32_t value = cyng::value_cast<std::uint32_t>(obj, 0);
					const auto str = scale_value(value, scaler);
					return cyng::make_object(str);
				}
				break;
				default:
					break;
				}
			}
			return  obj;
		}

		cyng::param_map_t read_param_tree(std::size_t depth, cyng::object obj)
		{
			cyng::tuple_t tpl;
			tpl = cyng::value_cast(obj, tpl);

			return (tpl.size() == 3)
				? read_param_tree(depth, tpl.begin(), tpl.end())
				: cyng::param_map_t()
				;
		}

		cyng::param_map_t read_param_tree(std::size_t depth
			, cyng::tuple_t::const_iterator pos
			, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 3, "SML Tree");
			if (count != 3) return cyng::param_map_t{};

			cyng::param_map_t params;

			//
			//	1. parameterName Octet String,
			//
			obis code = read_obis(*pos++);
			boost::ignore_unused(code);

			//
			//	2. parameterValue SML_ProcParValue OPTIONAL,
			//
			auto attr = read_parameter(*pos++);
			if (attr.first != PROC_PAR_UNDEF) {
				params.emplace(code.to_str(), attr.second);
			}

			//
			//	3. child_List List_of_SML_Tree OPTIONAL
			//
			cyng::tuple_t tpl;
			tpl = cyng::value_cast(*pos++, tpl);
			for (auto const child : tpl)
			{
				cyng::tuple_t tmp;
				tmp = cyng::value_cast(child, tmp);
				auto const m = read_param_tree(depth + 1, tmp.begin(), tmp.end());
				params.insert(m.begin(), m.end());	//	preserve existing keys
			}

			return params;
		}

		obis read_obis(cyng::object obj)
		{
			cyng::buffer_t tmp;
			tmp = cyng::value_cast(obj, tmp);

			return (tmp.empty())
				? obis()
				: obis(tmp)
				;

		}

		cyng::attr_t read_parameter(cyng::object obj)
		{
			cyng::tuple_t tpl;
			tpl = cyng::value_cast(obj, tpl);
			if (tpl.size() == 2)
			{
				const auto type = cyng::value_cast<std::uint8_t>(tpl.front(), 0);
				switch (type)
				{
				case PROC_PAR_VALUE:		return cyng::attr_t(type, tpl.back());
				case PROC_PAR_PERIODENTRY:	return cyng::attr_t(type, tpl.back());
				case PROC_PAR_TUPELENTRY:	return cyng::attr_t(type, tpl.back());
				case PROC_PAR_TIME:			return cyng::attr_t(type, read_time(tpl.back()));
				default:
					break;
				}
			}
			return cyng::attr_t(PROC_PAR_UNDEF, cyng::make_object());
		}

		cyng::object read_time(cyng::object obj)
		{
			cyng::tuple_t choice;
			choice = cyng::value_cast(obj, choice);
			if (choice.empty())	return cyng::make_object();

			BOOST_ASSERT_MSG(choice.size() == 2, "TIME");
			if (choice.size() == 2)
			{
				auto code = cyng::value_cast<std::uint8_t>(choice.front(), 0);
				switch (code)
				{
				case TIME_TIMESTAMP:
				{
					const std::uint32_t sec = cyng::value_cast<std::uint32_t>(choice.back(), 0);
					return cyng::make_time_point(sec);
				}
				break;
				case TIME_SECINDEX:
					return choice.back();
				default:
					break;
				}
			}
			return cyng::make_object();
		}

		cyng::object read_string(cyng::object obj)
		{
			cyng::buffer_t buffer;
			buffer = cyng::value_cast(obj, buffer);
			return cyng::make_object(cyng::io::to_ascii(buffer));
		}

		std::int8_t read_scaler(cyng::object obj)
		{
			std::int8_t const scaler = cyng::numeric_cast<std::int8_t>(obj, 0);
			return scaler;
		}

		std::uint8_t read_unit(cyng::object obj)
		{
			std::uint8_t const unit = cyng::value_cast<std::uint8_t>(obj, 0);
			//ro_.set_value(name, obj);
			return unit;
		}

		cyng::param_t read_get_proc_single_parameter(cyng::tuple_t::const_iterator pos
			, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 3, "SML Parameter");
			if (count != 3)	return cyng::param_t{ "", cyng::make_object() };

			//
			//	1. parameterName Octet String,
			//
			obis const code = read_obis(*pos++);

			//
			//	2. parameterValue SML_ProcParValue OPTIONAL,
			//
			auto attr = read_parameter(*pos++);

			if (OBIS_CODE_IF_1107_METER_LIST == code) {
				return cyng::param_t(code.to_str(), *pos);
			}
			return cyng::param_t(code.to_str(), attr.second);

		}

		cyng::param_t read_get_proc_single_parameter(cyng::object obj)
		{
			cyng::tuple_t tpl;
			tpl = cyng::value_cast(obj, tpl);
			BOOST_ASSERT_MSG(tpl.size() == 3, "SML Parameter");
			return read_get_proc_single_parameter(tpl.begin(), tpl.end());
		}

	}	//	sml
}

