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

#include <cyng/io/io_buffer.h>
#include <cyng/io/io_chrono.hpp>
#include <cyng/value_cast.hpp>
#include <cyng/numeric_cast.hpp>
#include <cyng/xml.h>
#include <cyng/factory.h>
#include <cyng/vm/generator.h>
#include <cyng/vm/manip.h>
#include <cyng/io/swap.h>

#include <boost/uuid/nil_generator.hpp>

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
				//cyng::xml::write(node.append_child("data"), body);
				break;
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
				//cyng::xml::write(node.append_child("data"), body);
				break;
			case BODY_GET_LIST_RESPONSE:
				//cyng::xml::write(node.append_child("data"), body);
				break;
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
			read_string("reqFileId", *pos++);

			//
			//	serverId
			//
			read_server_id(*pos++);

			//
			//	username
			//
			read_string("userName", *pos++);

			//
			//	password
			//
			read_string("password", *pos++);

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
				, ro_.get_value("clientId")
				, ro_.get_value("serverId")
				, ro_.get_value("reqFileId")
				, ro_.get_value("userName")
				, ro_.get_value("password"));

		}

		cyng::vector_t reader::read_public_open_response(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 6, "Public Open Response");

			//	codepage "ISO 8859-15"
			ro_.set_value("codepage", *pos++);

			//
			//	clientId (MAC)
			//	Typically 7 bytes to identify gateway/MUC
			read_client_id(*pos++);

			//
			//	reqFileId
			//
			read_string("reqFileId", *pos++);

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
				, ro_.get_value("clientId")
				, ro_.get_value("serverId")
				, ro_.get_value("reqFileId"));
		}

		cyng::vector_t reader::read_public_close_request(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 1, "Public Close Request");

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

			//
			//	serverId
			//
			read_server_id(*pos++);

			//
			//	actTime
			//
			read_time("actTime", *pos++);

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
			read_time("valTime", *pos++);

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
				, ro_.get_value("clientId")
				, ro_.get_value("serverId")
				, ro_.get_value("status"));

		}

		cyng::vector_t reader::read_get_proc_parameter_response(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 3, "Get Proc Parameter Response");

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

			//
			//	1. parameterName Octet String,
			//
			obis code = read_obis(*pos++);
			if (code == path.back()) {
				//	root
				std::cout << "root: " << code << std::endl;
				BOOST_ASSERT(depth == 0);
			}
			else {
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

			if (path.size() == 3) {

				if (path.back().is_matching(0x81, 0x81, 0x10, 0x06)) {
					cyng::tuple_t tpl;
					tpl = cyng::value_cast(*pos++, tpl);

					//
					//	collect meter info
					//	* 81 81 C7 82 04 FF: server ID
					//	* 81 81 C7 82 02 FF: --- (device class)
					//	* 01 00 00 09 0B 00: timestamp
					//
					for (auto const child : tpl)
					{
						cyng::tuple_t tmp;
						tmp = cyng::value_cast(child, tmp);
						read_get_proc_single_parameter(tmp.begin(), tmp.end());

					}
					prg << cyng::generate_invoke_unwinded("sml.get.proc.param.srv.visible"
						, ro_.pk_
						, ro_.trx_
						, ro_.idx_
						, ro_.get_value("serverId")
						, path.back().get_number()	//	4/5 
						, ro_.get_value("81 81 c7 82 04 ff")	//	meter ID
						, ro_.get_value("81 81 c7 82 02 ff")	//	device class
						, ro_.get_value("01 00 00 09 0b 00"));	//	UTC
				}
				else if (path.back().is_matching(0x81, 0x81, 0x11, 0x06)) {
					cyng::tuple_t tpl;
					tpl = cyng::value_cast(*pos++, tpl);

					//
					//	collect meter info
					//	* 81 81 C7 82 04 FF: server ID
					//	* 81 81 C7 82 02 FF: --- (device class)
					//	* 01 00 00 09 0B 00: timestamp
					//
					for (auto const child : tpl)
					{
						cyng::tuple_t tmp;
						tmp = cyng::value_cast(child, tmp);
						read_get_proc_single_parameter(tmp.begin(), tmp.end());

					}
					prg << cyng::generate_invoke_unwinded("sml.get.proc.param.srv.active"
						, ro_.pk_
						, ro_.trx_
						, ro_.idx_
						, ro_.get_value("serverId")
						, path.back().get_number()	//	4/5 
						, ro_.get_value("81 81 c7 82 04 ff")	//	meter ID
						, ro_.get_value("81 81 c7 82 02 ff")	//	device class
						, ro_.get_value("01 00 00 09 0b 00"));	//	UTC
				}
			}
			else
			{
				//
				//	3. child_List List_of_SML_Tree OPTIONAL
				//
				cyng::tuple_t tpl;
				tpl = cyng::value_cast(*pos++, tpl);
				++depth;
				for (auto const child : tpl)
				{
					cyng::tuple_t tmp;
					tmp = cyng::value_cast(child, tmp);

					prg << cyng::unwinder(read_get_proc_parameter_response(path, depth, tmp.begin(), tmp.end()));
				}
			}
			return prg;
		}

		void reader::read_get_proc_single_parameter(cyng::tuple_t::const_iterator pos
			, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 3, "SML Tree");

			//
			//	1. parameterName Octet String,
			//
			obis code = read_obis(*pos++);

			//
			//	2. parameterValue SML_ProcParValue OPTIONAL,
			//
			auto attr = read_parameter(*pos++);
			ro_.set_value(code, attr.second);

			//std::cerr
			//	<< to_hex(code)
			//	<< " = "
			//	//<< cyng::io::to_str(attr.second)
			//	<< std::endl;

		}

		cyng::vector_t reader::read_get_proc_parameter_request(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 5, "Get Profile List Request");

			//
			//	serverId
			//
			read_server_id(*pos++);

			//
			//	username
			//
			read_string("userName", *pos++);

			//
			//	password
			//
			read_string("password", *pos++);

			//
			//	parameterTreePath == parameter address
			//
			std::vector<obis> path = read_param_tree_path(*pos++);

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
					, ro_.get_value("serverId")
					, ro_.get_value("userName")
					, ro_.get_value("password")
					, path.at(0).to_buffer()
					, *pos);

			}

			return cyng::generate_invoke("sml.get.proc.parameter.request"
				, ro_.pk_
				, ro_.trx_
				, ro_.idx_
				, ro_.get_value("serverId")
				, ro_.get_value("userName")
				, ro_.get_value("password")
				, to_hex(path.at(0))	//	ToDo: send complete path
				, *pos);	//	attribute
		}

		cyng::vector_t reader::read_set_proc_parameter_request(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 5, "Set Proc Parameter Request");

			//
			//	serverId
			//
			read_server_id(*pos++);

			//
			//	username
			//
			read_string("userName", *pos++);

			//
			//	password
			//
			read_string("password", *pos++);

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

			//
			//	1. parameterName Octet String,
			//
			obis code = read_obis(*pos++);
			if (code == path.back()) {
				//	root
				std::cout << "root: " << code << std::endl;
				BOOST_ASSERT(depth == 0);
			}
			else {
				path.push_back(code);
			}

			//
			//	2. parameterValue SML_ProcParValue OPTIONAL,
			//
			auto attr = read_parameter(*pos++);
			if (attr.first != PROC_PAR_UNDEF) {
				//	example: push delay
				//	81 81 c7 8a 01 ff => 81 81 c7 8a 01 [01] => 81 81 c7 8a 03 ff
				//	std::cout << to_hex(path) << std::endl;
				if (OBIS_PUSH_OPERATIONS == path.front()) {

					BOOST_ASSERT_MSG(path.size() == 3, "OBIS_PUSH_OPERATIONS param tree too short");

					if (path.size() > 2) {
						auto r = path.at(1).is_matching(0x81, 0x81, 0xc7, 0x8a, 0x01);
						if (r.second) {
							if (path.at(2) == OBIS_CODE(81, 81, c7, 8a, 02, ff)) {


								prg << cyng::generate_invoke_unwinded("sml.set.proc.push.interval"
									, ro_.pk_
									, ro_.trx_
									, r.first
									, ro_.get_value("serverId")
									, ro_.get_value("userName")
									, ro_.get_value("password")
									, std::chrono::seconds(cyng::numeric_cast<std::int64_t>(attr.second, 900)));

							}
							else if (path.at(2) == OBIS_CODE(81, 81, c7, 8a, 03, ff)) {

								prg << cyng::generate_invoke_unwinded("sml.set.proc.push.delay"
									, ro_.pk_
									, ro_.trx_
									, r.first
									, ro_.get_value("serverId")
									, ro_.get_value("userName")
									, ro_.get_value("password")
									, std::chrono::seconds(cyng::numeric_cast<std::int64_t>(attr.second, 0)));

							}
							else if (path.at(2) == OBIS_CODE(81, 47, 17, 07, 00, FF)) {

								cyng::buffer_t tmp;
								tmp = cyng::value_cast(attr.second, tmp);

								prg << cyng::generate_invoke_unwinded("sml.set.proc.push.name"
									, ro_.pk_
									, ro_.trx_
									, r.first
									, ro_.get_value("serverId")
									, ro_.get_value("userName")
									, ro_.get_value("password")
									, std::string(tmp.begin(), tmp.end()));

							}
						}
					}
					else {
						//	param tree too short
					}
				}
				else if (OBIS_CODE_ROOT_IPT_PARAM == path.front()) {
					BOOST_ASSERT_MSG(path.size() == 3, "OBIS_CODE_ROOT_IPT_PARAM param tree too short");
					if (path.size() > 2) {
						auto r = path.at(1).is_matching(0x81, 0x49, 0x0D, 0x07, 0x00);
						if (r.second) {
							BOOST_ASSERT(r.first != 0);

							//
							//	set IP-T host address
							//
							auto m = path.at(2).is_matching(0x81, 0x49, 0x17, 0x07, 0x00);
							if (m.second) {

								BOOST_ASSERT(r.first == m.first);
								BOOST_ASSERT(m.first != 0);
								--m.first;

								//	remove host byte ordering
								auto num = cyng::swap_num(cyng::numeric_cast<std::uint32_t>(attr.second, 0u));
								boost::asio::ip::address address = boost::asio::ip::make_address_v4(num);
								prg << cyng::generate_invoke_unwinded("sml.set.proc.ipt.param.address"
									, ro_.pk_
									, ro_.trx_
									, m.first
									, address);

							}

							//
							//	set IP-T target port
							//
							m = path.at(2).is_matching(0x81, 0x49, 0x1A, 0x07, 0x00);
							if (m.second) {

								BOOST_ASSERT(r.first == m.first);
								BOOST_ASSERT(m.first != 0);
								--m.first;

								//
								//	set IP-T target port
								//
								auto port = cyng::numeric_cast<std::uint16_t>(attr.second, 0u);
								prg << cyng::generate_invoke_unwinded("sml.set.proc.ipt.param.port.target"
									, ro_.pk_
									, ro_.trx_
									, m.first
									, port);

							}

							//
							//	set IP-T user name
							//
							m = path.at(2).is_matching(0x81, 0x49, 0x63, 0x3C, 0x01);
							if (m.second) {

								BOOST_ASSERT(r.first == m.first);
								BOOST_ASSERT(m.first != 0);
								--m.first;

								//
								//	set IP-T user name
								//
								cyng::buffer_t tmp;
								tmp = cyng::value_cast(attr.second, tmp);

								prg << cyng::generate_invoke_unwinded("sml.set.proc.ipt.param.user"
									, ro_.pk_
									, ro_.trx_
									, m.first
									, std::string(tmp.begin(), tmp.end()));

							}

							//
							//	set IP-T password
							//
							m = path.at(2).is_matching(0x81, 0x49, 0x63, 0x3C, 0x02);
							if (m.second) {

								BOOST_ASSERT(r.first == m.first);
								BOOST_ASSERT(m.first != 0);
								--m.first;

								//
								//	set IP-T password
								//
								cyng::buffer_t tmp;
								tmp = cyng::value_cast(attr.second, tmp);

								prg << cyng::generate_invoke_unwinded("sml.set.proc.ipt.param.pwd"
									, ro_.pk_
									, ro_.trx_
									, m.first
									, std::string(tmp.begin(), tmp.end()));

							}
						}
					}
				}
			}

			//
			//	3. child_List List_of_SML_Tree OPTIONAL
			//
			cyng::tuple_t tpl;
			tpl = cyng::value_cast(*pos++, tpl);
			++depth;
			for (auto const child : tpl)
			{
				cyng::tuple_t tmp;
				tmp = cyng::value_cast(child, tmp);
				//read_param_tree(depth, tmp.begin(), tmp.end());
				//
				//	recursive call
				//
				prg << cyng::unwinder(read_set_proc_parameter_request_tree(path, depth, tmp.begin(), tmp.end()));
			}

			return prg;
		}

		cyng::vector_t reader::read_attention_response(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 4, "Attention Response");

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
			cyng::tuple_t tpl;
			tpl = cyng::value_cast(*pos++, tpl);
			read_param_tree(0, tpl.begin(), tpl.end());

			return cyng::generate_invoke("sml.attention.msg"
				, ro_.pk_
				, ro_.trx_
				, ro_.idx_
				, ro_.get_value("serverId")
				, code.to_buffer()
				, ro_.get_value("attentionMsg"));
		}

		void reader::read_param_tree(std::size_t depth
			, cyng::tuple_t::const_iterator pos
			, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 3, "SML Tree");

			//
			//	1. parameterName Octet String,
			//
			obis code = read_obis(*pos++);

			//
			//	2. parameterValue SML_ProcParValue OPTIONAL,
			//
			auto attr = read_parameter(*pos++);

			//
			//	3. child_List List_of_SML_Tree OPTIONAL
			//
			//cyng::xml::write(node.append_child("child_List"), *pos++);
			cyng::tuple_t tpl;
			tpl = cyng::value_cast(*pos++, tpl);
			++depth;
			for (auto const child : tpl)
			{
				cyng::tuple_t tmp;
				tmp = cyng::value_cast(child, tmp);
				read_param_tree(depth, tmp.begin(), tmp.end());
			}
		}

		void reader::read_period_list(std::vector<obis> const& path
			, cyng::tuple_t::const_iterator pos
			, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			//const std::string str_count = std::to_string(count);
			//node.append_attribute("size").set_value(str_count.c_str());

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

			//
			//	object name
			//
			obis code = read_obis(*pos++);

			//
			//	unit (see sml_unit_enum)
			//
			const auto unit = read_unit("SMLUnit", *pos++);

			//
			//	scaler
			//
			const auto scaler = read_scaler(*pos++);

			//
			//	value
			//
			read_value(code, scaler, unit, *pos++);

			//
			//	valueSignature
			//
			ro_.set_value("signature", *pos++);

		}


		cyng::object reader::read_time(std::string const& name, cyng::object obj)
		{
			cyng::tuple_t choice;
			choice = cyng::value_cast(obj, choice);
			BOOST_ASSERT_MSG(choice.size() == 2, "TIME");
			if (choice.size() == 2)
			{
				auto code = cyng::value_cast<std::uint8_t>(choice.front(), 0);
				switch (code)
				{
				case TIME_TIMESTAMP:
				{
					const std::uint32_t sec = cyng::value_cast<std::uint32_t>(choice.back(), 0);
					ro_.set_value(name, cyng::make_time_point(sec));
					return cyng::make_time_point(sec);
				}
				break;
				case TIME_SECINDEX:
					ro_.set_value(name, choice.back());
					return choice.back();
				default:
					break;
				}
			}
			return cyng::make_object();
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

		obis reader::read_obis(cyng::object obj)
		{
			cyng::buffer_t tmp;
			tmp = cyng::value_cast(obj, tmp);

			const obis code = obis(tmp);
			//const std::string name_dec = to_string(code);
			//const std::string name_hex = to_hex(code);

			//auto child = node.append_child("profile");
			//child.append_attribute("type").set_value(get_name(code));
			//child.append_attribute("obis").set_value(name_hex.c_str());
			//child.append_child(pugi::node_pcdata).set_value(name_dec.c_str());

			return code;
		}

		std::uint8_t reader::read_unit(std::string const& name, cyng::object obj)
		{
			std::uint8_t unit = cyng::value_cast<std::uint8_t>(obj, 0);
			//node.append_attribute("type").set_value(get_unit_name(unit));
			//node.append_child(pugi::node_pcdata).set_value(std::to_string(+unit).c_str());
			ro_.set_value(name, obj);
			return unit;
		}

		std::string reader::read_string(std::string const& name, cyng::object obj)
		{
			cyng::buffer_t buffer;
			buffer = cyng::value_cast(obj, buffer);
			const auto str = cyng::io::to_ascii(buffer);
			ro_.set_value(name, cyng::make_object(str));
			return str;
		}

		std::string reader::read_server_id(cyng::object obj)
		{
			cyng::buffer_t buffer;
			buffer = cyng::value_cast(obj, buffer);
			//const auto str = from_server_id(buffer);
			ro_.set_value("serverId", cyng::make_object(buffer));
			return from_server_id(buffer);
		}

		std::string reader::read_client_id(cyng::object obj)
		{
			cyng::buffer_t buffer;
			buffer = cyng::value_cast(obj, buffer);
			//const auto str = from_server_id(buffer);
			//ro_.set_value("clientId", cyng::make_object(str));
			ro_.set_value("clientId", cyng::make_object(buffer));
			return from_server_id(buffer);
		}

		std::int8_t reader::read_scaler(cyng::object obj)
		{
			std::int8_t scaler = cyng::value_cast<std::int8_t>(obj, 0);
			//node.append_child(pugi::node_pcdata).set_value(std::to_string(+scaler).c_str());
			return scaler;
		}

		void reader::read_value(obis code, std::int8_t scaler, std::uint8_t unit, cyng::object obj)
		{
			//
			//	write value
			//
			//cyng::xml::write(node, obj);

			if (code == OBIS_DATA_MANUFACTURER)
			{
				cyng::buffer_t buffer;
				buffer = cyng::value_cast(obj, buffer);
				const auto manufacturer = cyng::io::to_ascii(buffer);
				ro_.set_value("manufacturer", cyng::make_object(manufacturer));
			}
			else if (code == OBIS_CURRENT_UTC)
			{
				if (obj.get_class().tag() == cyng::TC_TUPLE)
				{
					read_time("roTime", obj);
				}
				else
				{
					const auto tm = cyng::value_cast<std::uint32_t>(obj, 0);
					const auto tp = std::chrono::system_clock::from_time_t(tm);
					//const auto str = cyng::to_str(tp);
					//node.append_attribute("readoutTime").set_value(str.c_str());
					ro_.set_value("roTime", cyng::make_object(tp));
				}
			}
			else if (code == OBIS_ACT_SENSOR_TIME)
			{
				const auto tm = cyng::value_cast<std::uint32_t>(obj, 0);
				const auto tp = std::chrono::system_clock::from_time_t(tm);
				//const auto str = cyng::to_str(tp);
				//node.append_attribute("sensorTime").set_value(str.c_str());
				ro_.set_value("actTime", cyng::make_object(tp));
			}
			else if (code == OBIS_SERIAL_NR)
			{
				cyng::buffer_t buffer;
				buffer = cyng::value_cast(obj, buffer);
				std::reverse(buffer.begin(), buffer.end());
				const auto serial_nr = cyng::io::to_ascii(buffer);
				ro_.set_value("serialNr", cyng::make_object(serial_nr));
			}
			else if (code == OBIS_SERIAL_NR_SECOND)
			{
				cyng::buffer_t buffer;
				buffer = cyng::value_cast(obj, buffer);
				std::reverse(buffer.begin(), buffer.end());
				const auto serial_nr = cyng::io::to_ascii(buffer);
				ro_.set_value("serialNr", cyng::make_object(serial_nr));
			}
			else
			{
				if (scaler != 0)
				{
					switch (obj.get_class().tag())
					{
					case cyng::TC_INT64:
					{
						const std::int64_t value = cyng::value_cast<std::int64_t>(obj, 0);
						const auto str = scale_value(value, scaler);
						ro_.set_value("value", cyng::make_object(str));
					}
					break;
					case cyng::TC_INT32:
					{
						const std::int32_t value = cyng::value_cast<std::int32_t>(obj, 0);
						const auto str = scale_value(value, scaler);
						ro_.set_value("value", cyng::make_object(str));
					}
					break;
					default:
						break;
					}
				}
			}
		}

		cyng::attr_t reader::read_parameter(cyng::object obj)
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
				case PROC_PAR_TIME:			return cyng::attr_t(type, read_time("parTime", tpl.back()));
				default:
					break;
				}
			}
			return cyng::attr_t(PROC_PAR_UNDEF, cyng::make_object());
		}

		reader::readout::readout(boost::uuids::uuid pk)
			: pk_(pk)
			, idx_(0)
			, trx_()
			, values_()
		{}

		void reader::readout::reset(boost::uuids::uuid pk, std::size_t idx)
		{
			pk_ = pk;
			idx_ = idx;
			trx_.clear();
			values_.clear();
		}

		reader::readout& reader::readout::set_trx(cyng::buffer_t const& buffer)
		{
			trx_ = cyng::io::to_ascii(buffer);
			return *this;
		}

		reader::readout& reader::readout::set_index(std::size_t idx)
		{
			idx_ = idx;
			return *this;
		}

		reader::readout& reader::readout::set_value(std::string const& name, cyng::object obj)
		{
			values_[name] = obj;
			return *this;
		}

		reader::readout& reader::readout::set_value(obis code, cyng::object obj)
		{
			values_[to_hex(code)] = obj;
			return *this;
		}

		cyng::object reader::readout::get_value(std::string const& name) const
		{
			auto pos = values_.find(name);
			return (pos != values_.end())
				? pos->second
				: cyng::make_object()
				;
		}


	}	//	sml
}

