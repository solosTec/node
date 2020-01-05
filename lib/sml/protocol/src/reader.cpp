/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/sml/protocol/reader.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/obis_io.h>
#include <smf/sml/units.h>
#include <smf/sml/scaler.h>
#include <smf/mbus/defs.h>
#include <smf/sml/ip_io.h>
#include <smf/sml/srv_id_io.h>

#include <cyng/io/io_buffer.h>
#include <cyng/io/io_chrono.hpp>
#include <cyng/io/serializer.h>
#include <cyng/value_cast.hpp>
#include <cyng/numeric_cast.hpp>
#include <cyng/buffer_cast.h>
#include <cyng/set_cast.h>
#include <cyng/xml.h>
#include <cyng/factory.h>
#include <cyng/vm/generator.h>
#include <cyng/vm/manip.h>
#include <cyng/io/swap.h>
#include <cyng/crypto/aes.h>

#include <boost/uuid/nil_generator.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/algorithm/string.hpp>

namespace node
{
	namespace sml
	{
		namespace reader
		{
			//
			//	forward declarations
			//	private interface
			//
			cyng::vector_t read_msg(readout& ro, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end);


			cyng::vector_t read_body(readout& ro, cyng::object, cyng::object);

			/** @brief BODY_OPEN_REQUEST (256)
			 */
			cyng::vector_t read_public_open_request(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

			/** @brief BODY_OPEN_RESPONSE (257)
			 */
			cyng::vector_t read_public_open_response(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

			/** @brief BODY_CLOSE_REQUEST (512)
			 */
			cyng::vector_t read_public_close_request(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

			/** @brief BODY_CLOSE_RESPONSE (513)
			 */
			cyng::vector_t read_public_close_response(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

			/** @brief BODY_GET_PROFILE_LIST_REQUEST (1024)
			 */
			cyng::vector_t read_get_profile_list_request(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

			/** @brief BODY_GET_PROFILE_LIST_RESPONSE (1025)
			 */
			cyng::vector_t read_get_profile_list_response(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

			/** @brief BODY_GET_PROC_PARAMETER_RESPONSE (1281)
			 */
			cyng::vector_t read_get_proc_parameter_response(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

			/** @brief BODY_GET_PROC_PARAMETER_REQUEST (1280)
			 */
			cyng::vector_t read_get_proc_parameter_request(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

			/** @brief BODY_SET_PROC_PARAMETER_REQUEST (1536)
			 */
			cyng::vector_t read_set_proc_parameter_request(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

			/** @brief BODY_SET_PROC_PARAMETER_RESPONSE (1537)
			 *
			 * This message type is undefined
			 */
			cyng::vector_t read_set_proc_parameter_response(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

			/** @brief BODY_GET_LIST_REQUEST (1792)
			 */
			cyng::vector_t read_get_list_request(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

			/** @brief BODY_GET_LIST_RESPONSE (1793)
			 */
			cyng::vector_t read_get_list_response(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

			cyng::vector_t  read_get_cosem_request(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			cyng::vector_t  read_get_cosem_response(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			cyng::vector_t  read_set_cosem_request(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			cyng::vector_t  read_set_cosem_response(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			cyng::vector_t  read_action_cosem_request(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			cyng::vector_t  read_action_cosem_response(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

			/** @brief BODY_ATTENTION_RESPONSE (65281)
			 */
			cyng::vector_t read_attention_response(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

			/**
			 * Convert data types of some specific OBIS values
			 */
			cyng::param_t customize_value(obis code, cyng::object);

			/**
			 * build a vector of devices
			 */
			cyng::param_t collect_devices(obis code, cyng::tuple_t const& tpl);

			/**
			 * build a vector of devices
			 */
			cyng::param_t collect_iec_devices(cyng::tuple_t const& tpl);

			/**
			 * @param use_vector if false the value is noted as a single object otherwise
			 * a vector of obis code / value pairs is notes. This is usefufull to distinguish between several values in a list.
			 */
			cyng::object read_value(obis, std::int8_t, std::uint8_t, cyng::object);
			obis read_obis(cyng::object);

			void read_sml_list(readout& ro, obis, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			void read_list_entry(readout& ro, obis, std::size_t index, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end);

			/**
			 * read a listor period entries
			 */
			cyng::param_map_t read_period_list(cyng::tuple_t::const_iterator
				, cyng::tuple_t::const_iterator);

			/**
			 * read a SML_PeriodEntry record
			 */
			cyng::param_t read_period_entry(std::size_t
				, cyng::tuple_t::const_iterator
				, cyng::tuple_t::const_iterator);



			cyng::vector_t read(cyng::tuple_t const& msg)
			{
				return read_msg(msg.begin(), msg.end());
			}

			cyng::vector_t read(cyng::tuple_t const& msg, boost::uuids::uuid pk)
			{
				readout ro;
				ro.set_pk(pk);
				return read_msg(ro, msg.begin(), msg.end());
			}

			cyng::vector_t read_msg(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
			{
				//
				//	readout values
				//
				readout ro;
				ro.set_pk(boost::uuids::nil_uuid());
				return read_msg(ro, pos, end);
			}

			cyng::vector_t read_msg(readout& ro, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
			{
				std::size_t count = std::distance(pos, end);
				BOOST_ASSERT_MSG(count == 5, "SML message");
				if (count != 5)	return cyng::generate_invoke("log.msg.error", "SMLMsg", count);

				//
				//	(1) - transaction id
				//	This is typically a printable sequence of 6 up to 19 ASCII values
				//
				cyng::buffer_t const buffer = cyng::to_buffer(*pos++);
				ro.set_trx(buffer);

				//
				//	(2) groupNo
				//
				ro.set_value("groupNo", *pos++);

				//
				//	(3) abortOnError
				//
				ro.set_value("abortOnError", *pos++);

				//
				//	(4/5) CHOICE - msg type
				//
				auto const choice = cyng::to_tuple(*pos++);

				//
				//	(6) CRC16
				//
				ro.set_value("crc16", *pos);

				//
				//	Read the SML message.
				//	By delaying the read the CRC is part of the complete message
				//
				return read_choice(ro, choice);
			}


			cyng::vector_t read_choice(readout& ro, cyng::tuple_t const& choice)
			{
				BOOST_ASSERT_MSG(choice.size() == 2, "CHOICE");
				if (choice.size() == 2)
				{
					ro.set_value("code", choice.front());
					return read_body(ro, choice.front(), choice.back());
				}
				return cyng::generate_invoke("log.msg.error", "SMLMsg-choice", choice.size());
			}

			cyng::vector_t read_body(readout& ro, cyng::object type, cyng::object body)
			{
				auto code = cyng::value_cast<std::uint16_t>(type, 0);

				cyng::tuple_t tpl;
				tpl = cyng::value_cast(body, tpl);

				switch (code)
				{
				case BODY_OPEN_REQUEST:
					return read_public_open_request(ro, tpl.begin(), tpl.end());
				case BODY_OPEN_RESPONSE:
					return read_public_open_response(ro, tpl.begin(), tpl.end());
				case BODY_CLOSE_REQUEST:
					return read_public_close_request(ro, tpl.begin(), tpl.end());
				case BODY_CLOSE_RESPONSE:
					return read_public_close_response(ro, tpl.begin(), tpl.end());
				case BODY_GET_PROFILE_PACK_REQUEST:
					//cyng::xml::write(node.append_child("data"), body);
					break;
				case BODY_GET_PROFILE_PACK_RESPONSE:
					//cyng::xml::write(node.append_child("data"), body);
					break;
				case BODY_GET_PROFILE_LIST_REQUEST:
					return read_get_profile_list_request(ro, tpl.begin(), tpl.end());
				case BODY_GET_PROFILE_LIST_RESPONSE:
					return read_get_profile_list_response(ro, tpl.begin(), tpl.end());
				case BODY_GET_PROC_PARAMETER_REQUEST:
					return read_get_proc_parameter_request(ro, tpl.begin(), tpl.end());
				case BODY_GET_PROC_PARAMETER_RESPONSE:
					return read_get_proc_parameter_response(ro, tpl.begin(), tpl.end());
				case BODY_SET_PROC_PARAMETER_REQUEST:
					return read_set_proc_parameter_request(ro, tpl.begin(), tpl.end());
				case BODY_SET_PROC_PARAMETER_RESPONSE:
					return read_set_proc_parameter_response(ro, tpl.begin(), tpl.end());
				case BODY_GET_LIST_REQUEST:
					return read_get_list_request(ro, tpl.begin(), tpl.end());
				case BODY_GET_LIST_RESPONSE:
					return read_get_list_response(ro, tpl.begin(), tpl.end());

				case BODY_GET_COSEM_REQUEST:	
					return read_get_cosem_request(ro, tpl.begin(), tpl.end());
				case BODY_GET_COSEM_RESPONSE:
					return read_get_cosem_response(ro, tpl.begin(), tpl.end());
				case BODY_SET_COSEM_REQUEST:
					return read_set_cosem_request(ro, tpl.begin(), tpl.end());
				case BODY_SET_COSEM_RESPONSE:
					return read_set_cosem_response(ro, tpl.begin(), tpl.end());
				case BODY_ACTION_COSEM_REQUEST:
					return read_action_cosem_request(ro, tpl.begin(), tpl.end());
				case BODY_ACTION_COSEM_RESPONSE:
					return read_action_cosem_response(ro, tpl.begin(), tpl.end());

				case BODY_ATTENTION_RESPONSE:
					return read_attention_response(ro, tpl.begin(), tpl.end());
				default:
					break;
				}

				return cyng::generate_invoke("log.msg.fatal"
					, "unknown SML message code"
					, code);
			}

			cyng::vector_t read_public_open_request(readout& ro, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
			{
				std::size_t count = std::distance(pos, end);
				BOOST_ASSERT_MSG(count == 7, "Public Open Request");
				if (count != 7)	return cyng::generate_invoke("log.msg.error", "SML Public Open Request", count);

				//	codepage "ISO 8859-15"
				ro.set_value("codepage", *pos++);

				//
				//	clientId (MAC)
				//	Typically 7 bytes to identify gateway/MUC
				auto const client_id = ro.read_client_id(*pos++);
				boost::ignore_unused(client_id);

				//
				//	reqFileId
				//
				ro.set_value("reqFileId", read_string(*pos++));

				//
				//	serverId
				//
				auto const srv_id = ro.read_server_id(*pos++);
				boost::ignore_unused(srv_id);

				//
				//	username
				//
				ro.set_value("userName", read_string(*pos++));

				//
				//	password
				//
				ro.set_value("password", read_string(*pos++));

				//
				//	sml-Version: default = 1
				//
				ro.set_value("SMLVersion", *pos++);

				//
				//	instruction vector
				//
				return cyng::generate_invoke("sml.public.open.request"
					, ro.trx_
					, ro.client_id_
					, ro.server_id_
					, ro.get_value("reqFileId")
					, ro.get_value("userName")
					, ro.get_value("password"));

			}

			cyng::vector_t read_public_open_response(readout& ro, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
			{
				std::size_t count = std::distance(pos, end);
				BOOST_ASSERT_MSG(count == 6, "Public Open Response");
				if (count != 6)	return cyng::generate_invoke("log.msg.error", "SML Public Open Response", count);

				//	codepage "ISO 8859-15"
				ro.set_value("codepage", *pos++);

				//
				//	clientId (MAC)
				//	Typically 7 bytes to identify gateway/MUC
				auto const client_id = ro.read_client_id(*pos++);
				boost::ignore_unused(client_id);

				//
				//	reqFileId
				//
				ro.set_value("reqFileId", read_string(*pos++));

				//
				//	serverId
				//
				auto const srv_id = ro.read_server_id(*pos++);
				boost::ignore_unused(srv_id);

				//
				//	refTime
				//
				ro.set_value("refTime", *pos++);

				//	sml-Version: default = 1
				ro.set_value("SMLVersion", *pos++);

				return cyng::generate_invoke("sml.public.open.response"
					, ro.trx_
					, ro.client_id_
					, ro.server_id_
					, ro.get_value("reqFileId"));
			}

			cyng::vector_t read_public_close_request(readout& ro, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
			{
				std::size_t count = std::distance(pos, end);
				BOOST_ASSERT_MSG(count == 1, "Public Close Request");
				if (count != 1)	return cyng::generate_invoke("log.msg.error", "SML Public Close Request", count);

				ro.set_value("globalSignature", *pos++);

				return cyng::generate_invoke("sml.public.close.request"
					, ro.trx_);
			}

			cyng::vector_t read_public_close_response(readout& ro, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
			{
				std::size_t count = std::distance(pos, end);
				BOOST_ASSERT_MSG(count == 1, "Public Close Response");
				if (count != 1)	return cyng::generate_invoke("log.msg.error", "SML Public Close Response", count);

				ro.set_value("globalSignature", *pos++);

				return cyng::generate_invoke("sml.public.close.response", ro.trx_);
			}

			cyng::vector_t read_get_profile_list_response(readout& ro, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
			{
				std::size_t count = std::distance(pos, end);
				BOOST_ASSERT_MSG(count == 9, "Get Profile List Response");
				if (count != 9)	return cyng::generate_invoke("log.msg.error", "SML Get Profile List Response", count);

				//
				//	serverId
				//
				auto const srv_id = ro.read_server_id(*pos++);
				boost::ignore_unused(srv_id);

				//
				//	actTime
				//
				ro.set_value("actTime", read_time(*pos++));

				//
				//	regPeriod
				//
				ro.set_value("regPeriod", *pos++);

				//
				//	parameterTreePath (OBIS)
				//
				std::vector<obis> const path = read_param_tree_path(*pos++);

				//
				//	valTime
				//
				ro.set_value("valTime", read_time(*pos++));

				//
				//	M-bus status
				//
				ro.set_value("status", *pos++);

				//	period-List (1)
				//	first we make the TSMLGetProfileListResponse complete, then 
				//	the we store the entries in TSMLPeriodEntry.
				auto const tpl = cyng::to_tuple(*pos++);
				auto const params = read_period_list(tpl.begin(), tpl.end());

				//	rawdata
				ro.set_value("rawData", *pos++);

				//	periodSignature
				ro.set_value("signature", *pos++);

				return cyng::generate_invoke("sml.get.profile.list.response"
					, ro.trx_
					, ro.get_value("actTime")
					, ro.get_value("regPeriod")
					, ro.get_value("valTime")
					, (path.empty() ? obis().to_buffer() : path.at(0).to_buffer())
					, ro.server_id_
					, ro.get_value("status")
					, params);

			}

			cyng::vector_t read_get_profile_list_request(readout& ro, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
			{
				std::size_t count = std::distance(pos, end);
				BOOST_ASSERT_MSG(count == 9, "Get Profile List Request");
				if (count != 9)	return cyng::generate_invoke("log.msg.error", "SML Get Profile List Request", count);

				//
				//	serverId
				//	Typically 7 bytes to identify gateway/MUC
				//
				auto const srv_id = ro.read_server_id(*pos++);
				boost::ignore_unused(srv_id);

				//
				//	username
				//
				ro.set_value("userName", read_string(*pos++));

				//
				//	password
				//
				ro.set_value("password", read_string(*pos++));

				//
				//	rawdata (typically not set)
				//
				ro.set_value("rawData", *pos++);

				//
				//	start/end time
				//
				ro.set_value("beginTime", read_time(*pos++));
				ro.set_value("endTime", read_time(*pos++));

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
				if (!path.empty()) {
					return cyng::generate_invoke("sml.get.profile.list.request"
						, ro.trx_
						, ro.client_id_
						, ro.server_id_
						, ro.get_value("userName")
						, ro.get_value("password")
						, ro.get_value("beginTime")
						, ro.get_value("endTime")
						, path.front().to_buffer());
				}

				return cyng::vector_t{};
			}

			cyng::vector_t read_get_proc_parameter_response(readout& ro, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
			{
				auto const count = std::distance(pos, end);
				BOOST_ASSERT_MSG(count == 3, "Get Proc Parameter Response");
				if (count != 3)	return cyng::generate_invoke("log.msg.error", "SML Get Proc Parameter Response", count);

				//
				//	serverId
				//
				auto const srv_id = ro.read_server_id(*pos++);
				boost::ignore_unused(srv_id);

				//
				//	parameterTreePath (OBIS)
				//
				std::vector<obis> path = read_param_tree_path(*pos++);
				BOOST_ASSERT(path.size() == 1);
				if (path.size() != 1)	return cyng::generate_invoke("log.msg.error", "SML Get Proc Parameter Response - wrong path size: ", path.size());

				//
				//	parameterTree
				//
				auto const tpl = cyng::to_tuple(*pos++);
				if (tpl.size() != 3)	return cyng::generate_invoke("log.msg.error", "SML Tree", tpl.size());

				//
				//	this is a parameter tree
				//
				auto const param = read_param_tree(0u, tpl.begin(), tpl.end());
				return cyng::generate_invoke("sml.get.proc.param.response"
					, ro.trx_
					, ro.get_value("groupNo")
					, ro.server_id_	//	binary
					, path.front().to_buffer()
					, param);

			}

			cyng::vector_t read_get_proc_parameter_request(readout& ro, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
			{
				std::size_t count = std::distance(pos, end);
				BOOST_ASSERT_MSG(count == 5, "Get Profile List Request");
				if (count != 5)	return cyng::generate_invoke("log.msg.error", "SML Get Profile List Request", count);

				//
				//	serverId
				//
				auto const srv_id = ro.read_server_id(*pos++);
				boost::ignore_unused(srv_id);

				//
				//	username
				//
				ro.set_value("userName", read_string(*pos++));

				//
				//	password
				//
				ro.set_value("password", read_string(*pos++));

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
						, ro.trx_
						, ro.server_id_
						, ro.get_value("userName")
						, ro.get_value("password")
						, path.at(0).to_buffer()
						, *pos);

				}

				return cyng::generate_invoke("sml.get.proc.parameter.request"
					, ro.trx_
					, ro.server_id_
					, ro.get_value("userName")
					, ro.get_value("password")
					, to_hex(path.at(0))	//	ToDo: send complete path
					, *pos);	//	attribute
			}

			cyng::vector_t read_set_proc_parameter_request(readout& ro, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
			{
				std::size_t count = std::distance(pos, end);
				BOOST_ASSERT_MSG(count == 5, "Set Proc Parameter Request");
				if (count != 5) return cyng::generate_invoke("log.msg.error", "SML Set Proc Parameter Request", count);

				//
				//	serverId
				//
				auto const srv_id = ro.read_server_id(*pos++);
				boost::ignore_unused(srv_id);

				//
				//	username
				//
				ro.set_value("userName", read_string(*pos++));

				//
				//	password
				//
				ro.set_value("password", read_string(*pos++));

				//
				//	parameterTreePath == parameter address
				//
				std::vector<obis> const path = read_param_tree_path(*pos++);

				//
				//	each setProcParamReq has to send an attension message as response
				//
				auto prg = cyng::generate_invoke("sml.attention.init"
					, ro.trx_
					, ro.server_id_
					, OBIS_ATTENTION_OK.to_buffer()
					, "Set Proc Parameter Request");

				//
				//	recursiv call to an parameter tree - similiar to read_param_tree()
				//	ToDo: use a generic implementation like in SML_GetProcParameter.Req
				//
				//auto p = read_set_proc_parameter_request_tree(path, 0, *pos++);
				//prg.insert(prg.end(), p.begin(), p.end());
				//
				//	parameterTree
				//
				auto const tpl = cyng::to_tuple(*pos++);
				if (tpl.size() != 3)	return cyng::generate_invoke("log.msg.error", "SML Tree", tpl.size());

				auto const param = read_param_tree(0u, tpl.begin(), tpl.end());
				prg << cyng::unwinder(cyng::generate_invoke("sml.set.proc.parameter.request"
					, ro.trx_
					, to_hex(path, ' ')	//	string
					, ro.server_id_
					, ro.get_value("userName")
					, ro.get_value("password")
					, param));

				//
				//	send prepared attention message
				//
				prg << cyng::unwinder(cyng::generate_invoke("sml.attention.send", ro.trx_));

				return prg;
			}

			cyng::vector_t read_set_proc_parameter_response(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator)
			{
				return cyng::generate_invoke("log.msg.error", "SML_SetProcParameter.Res not implemented");
			}

			cyng::vector_t read_get_list_request(readout& ro, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
			{
				std::size_t count = std::distance(pos, end);
				BOOST_ASSERT_MSG(count == 5, "Get List Request");
				if (count != 5) return cyng::generate_invoke("log.msg.error", "SML Get List Request", count);

				//
				//	clientId - MAC address from caller
				//
				auto const client_id = ro.read_client_id(*pos++);
				boost::ignore_unused(client_id);

				//
				//	serverId - meter ID
				//
				auto const server_id = ro.read_server_id(*pos++);
				boost::ignore_unused(server_id);

				//
				//	username
				//
				ro.set_value("userName", read_string(*pos++));

				//
				//	password
				//
				ro.set_value("password", read_string(*pos++));

				//
				//	list name
				//
				auto const code = read_obis(*pos++);

				return cyng::generate_invoke("sml.get.list.request"
					, ro.trx_
					, ro.client_id_
					, ro.server_id_
					, ro.get_value("reqFileId")
					, ro.get_value("userName")
					, ro.get_value("password")
					, code.to_buffer());
			}

			cyng::vector_t read_get_list_response(readout& ro, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
			{
				std::size_t count = std::distance(pos, end);
				BOOST_ASSERT_MSG(count == 7, "Get List Response");
				if (count != 7) return cyng::generate_invoke("log.msg.error", "SML Get List Response", count);

				//
				//	clientId - optional
				//	mostly empty
				//
				auto const client_id = ro.read_client_id(*pos++);
				boost::ignore_unused(client_id);

				//
				//	serverId - meter ID
				//
				auto const server_id = ro.read_server_id(*pos++);
				boost::ignore_unused(server_id);

				//
				//	list name - optional
				//
				obis code = read_obis(*pos++);
				if (code.is_nil()) {
					code = sml::OBIS_LIST_CURRENT_DATA_RECORD;	//	last data set (99 00 00 00 00 03)
				}

				//
				//	read act. sensor time - optional
				//
				ro.set_value("actSensorTime", read_time(*pos++));

				//
				//	valList
				//
				auto const tpl = cyng::to_tuple(*pos++);
				read_sml_list(ro, code, tpl.begin(), tpl.end());

				//
				//	SML_Signature - OPTIONAL
				//
				ro.set_value("listSignature", *pos++);

				//
				//	SML_Time - OPTIONAL 
				//
				ro.set_value("actGatewayTime", read_time(*pos++));

				return cyng::generate_invoke("sml.get.list.response"
					, ro.trx_
					, ro.server_id_
					, code.to_buffer()
					, ro.values_
					, ro.get_value("pk"));
			}

			cyng::vector_t  read_get_cosem_request(readout& ro, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
			{
				std::size_t count = std::distance(pos, end);
				BOOST_ASSERT_MSG(count == 8, "Get COSEM Request");
				if (count != 8) return cyng::generate_invoke("log.msg.error", "Get COSEM Request", count);

				//
				//	clientId
				//
				auto const client_id = ro.read_client_id(*pos++);

				//
				//	serverId - meter ID (optional)
				//
				auto const server_id = ro.read_server_id(*pos++);

				//
				//	username
				//
				auto const user = read_string(*pos++);

				//
				//	password
				//
				auto const pwd = read_string(*pos++);

				//
				//	object name
				//
				obis const code = read_obis(*pos++);

				//
				//	classId
				//
				auto const class_id = cyng::numeric_cast<std::int16_t>(*pos++, 0);

				//
				//	classVersion
				//
				auto const class_version = cyng::numeric_cast<std::int16_t>(*pos++, 0);

				auto const index_list = *pos++;

				return cyng::generate_invoke("sml.get.cosem.request"
					, ro.trx_
					, ro.client_id_
					, ro.server_id_
					, user
					, pwd
					, code.to_buffer()
					, class_id
					, class_version
					, index_list);

			}

			cyng::vector_t  read_get_cosem_response(readout& ro, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
			{
				std::size_t count = std::distance(pos, end);
				BOOST_ASSERT_MSG(count == 6, "Get COSEM Response");
				if (count != 6) return cyng::generate_invoke("log.msg.error", "Get COSEM Response", count);

				//
				//	clientId
				//
				auto const client_id = ro.read_client_id(*pos++);

				//
				//	serverId - meter ID (optional)
				//
				auto const server_id = ro.read_server_id(*pos++);

				//
				//	object name
				//
				obis const code = read_obis(*pos++);

				//
				//	classId
				//
				auto const class_id = cyng::numeric_cast<std::int16_t>(*pos++, 0);

				//
				//	classVersion
				//
				auto const class_version = cyng::numeric_cast<std::int16_t>(*pos++, 0);

				auto const attr_list = *pos++;

				return cyng::generate_invoke("sml.get.cosem.response"
					, ro.trx_
					, ro.client_id_
					, ro.server_id_
					, code.to_buffer()
					, class_id
					, class_version
					, attr_list);

			}

			cyng::vector_t  read_set_cosem_request(readout& ro, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
			{
				std::size_t count = std::distance(pos, end);
				BOOST_ASSERT_MSG(count == 8, "Set COSEM Request");
				if (count != 8) return cyng::generate_invoke("log.msg.error", "Set COSEM Request", count);

				//
				//	clientId
				//
				auto const client_id = ro.read_client_id(*pos++);

				//
				//	serverId - meter ID (optional)
				//
				auto const server_id = ro.read_server_id(*pos++);

				//
				//	username
				//
				auto const user = read_string(*pos++);

				//
				//	password
				//
				auto const pwd = read_string(*pos++);

				//
				//	object name
				//
				obis const code = read_obis(*pos++);

				//
				//	classId
				//
				auto const class_id = cyng::numeric_cast<std::int16_t>(*pos++, 0);

				//
				//	classVersion
				//
				auto const class_version = cyng::numeric_cast<std::int16_t>(*pos++, 0);

				auto const attr_list = *pos++;

				return cyng::generate_invoke("sml.set.cosem.request"
					, ro.trx_
					, ro.client_id_
					, ro.server_id_
					, user
					, pwd
					, code.to_buffer()
					, class_id
					, class_version
					, attr_list);
			}

			cyng::vector_t  read_set_cosem_response(readout& ro, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
			{
				std::size_t count = std::distance(pos, end);
				BOOST_ASSERT_MSG(count == 6, "Set COSEM Response");
				if (count != 6) return cyng::generate_invoke("log.msg.error", "Set COSEM Response", count);

				//
				//	clientId
				//
				auto const client_id = ro.read_client_id(*pos++);

				//
				//	serverId - meter ID (optional)
				//
				auto const server_id = ro.read_server_id(*pos++);

				//
				//	object name
				//
				obis const code = read_obis(*pos++);

				//
				//	classId
				//
				auto const class_id = cyng::numeric_cast<std::int16_t>(*pos++, 0);

				//
				//	classVersion
				//
				auto const class_version = cyng::numeric_cast<std::int16_t>(*pos++, 0);

				auto const attr_list = *pos++;

				return cyng::generate_invoke("sml.set.cosem.response"
					, ro.trx_
					, ro.client_id_
					, ro.server_id_
					, code.to_buffer()
					, class_id
					, class_version
					, attr_list);
			}

			cyng::vector_t  read_action_cosem_request(readout& ro, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
			{
				std::size_t count = std::distance(pos, end);
				BOOST_ASSERT_MSG(count == 9, "Action COSEM Request");
				if (count != 9) return cyng::generate_invoke("log.msg.error", "Action COSEM Request", count);

				//
				//	clientId
				//
				auto const client_id = ro.read_client_id(*pos++);

				//
				//	serverId - meter ID (optional)
				//
				auto const server_id = ro.read_server_id(*pos++);

				//
				//	username
				//
				auto const user = read_string(*pos++);

				//
				//	password
				//
				auto const pwd = read_string(*pos++);

				//
				//	object name
				//
				obis const code = read_obis(*pos++);

				//
				//	classId
				//
				auto const class_id = cyng::numeric_cast<std::int16_t>(*pos++, 0);

				//
				//	classVersion
				//
				auto const class_version = cyng::numeric_cast<std::int16_t>(*pos++, 0);

				//
				//	serviceIndex
				//
				auto const service_index = cyng::numeric_cast<std::int8_t>(*pos++, 0);

				auto const service_parameter = *pos++;

				return cyng::generate_invoke("sml.action.cosem.request"
					, ro.trx_
					, ro.client_id_
					, ro.server_id_
					, user
					, pwd
					, code.to_buffer()
					, class_id
					, class_version
					, service_index
					, service_parameter);
			}

			cyng::vector_t  read_action_cosem_response(readout& ro, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
			{
				std::size_t count = std::distance(pos, end);
				BOOST_ASSERT_MSG(count == 6, "Action COSEM Response");
				if (count != 6) return cyng::generate_invoke("log.msg.error", "Action COSEM Response", count);

				//
				//	clientId
				//
				auto const client_id = ro.read_client_id(*pos++);

				//
				//	serverId - meter ID (optional)
				//
				auto const server_id = ro.read_server_id(*pos++);

				//
				//	object name
				//
				obis const code = read_obis(*pos++);

				//
				//	classId
				//
				auto const class_id = cyng::numeric_cast<std::int16_t>(*pos++, 0);

				//
				//	classVersion
				//
				auto const class_version = cyng::numeric_cast<std::int16_t>(*pos++, 0);

				auto const attr_list = *pos++;

				return cyng::generate_invoke("sml.action.cosem.response"
					, ro.trx_
					, ro.client_id_
					, ro.server_id_
					, code.to_buffer()
					, class_id
					, class_version
					, attr_list);
			}


			cyng::vector_t read_attention_response(readout& ro, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
			{
				std::size_t count = std::distance(pos, end);
				BOOST_ASSERT_MSG(count == 4, "Attention Response");
				if (count != 4) return cyng::generate_invoke("log.msg.error", "SML Attention Response", count);

				//
				//	serverId
				//
				auto const srv_id = ro.read_server_id(*pos++);
				boost::ignore_unused(srv_id);

				//
				//	attentionNo (OBIS)
				//
				obis const code = read_obis(*pos++);

				//
				//	attentionMsg
				//
				ro.set_value("attentionMsg", *pos++);

				//
				//	attentionDetails (SML_Tree)
				//
				auto const params = read_param_tree(0, *pos++);
				boost::ignore_unused(params);

				return cyng::generate_invoke("sml.attention.msg"
					, ro.trx_
					, ro.server_id_
					, code.to_buffer()
					, ro.get_value("attentionMsg"));
			}

			void read_sml_list(readout& ro
				, obis code
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
					read_list_entry(ro, code, counter, tpl.begin(), tpl.end());
					++counter;

				}
			}

			void read_list_entry(readout& ro
				, obis list_name
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
				obis const code = read_obis(*pos++);

				//
				//	status - optional
				//
				auto const status = *pos++;

				//
				//	valTime - optional
				//
				auto const val_time = read_time(*pos++);

				//
				//	unit (see sml_unit_enum) - optional
				//
				auto const unit = read_unit(*pos++);

				//
				//	scaler - optional
				//
				auto const scaler = read_scaler(*pos++);

				//
				//	value - note value with OBIS code
				//
				auto raw = *pos++;
				auto val = read_value(code, scaler, unit, raw);
				auto obj = cyng::param_map_factory("unit", unit)("scaler", scaler)("value", val)("raw", raw)("valTime", val_time)("status", status)();
				ro.set_value(code, "values", obj);

				//
				//	valueSignature
				//
				//ro.set_value("signature", *pos++);

			}

			cyng::param_map_t read_period_list(cyng::tuple_t::const_iterator pos
				, cyng::tuple_t::const_iterator end)
			{
				cyng::param_map_t params;

				//
				//	list of tuples (period entry)
				//
				std::size_t counter{ 0 };
				while (pos != end)
				{
					cyng::tuple_t tpl;
					tpl = cyng::value_cast(*pos++, tpl);
					auto const param = read_period_entry(counter, tpl.begin(), tpl.end());
					params.insert(param);
					++counter;
				}
				return params;
			}

			cyng::param_t read_period_entry(std::size_t index
				, cyng::tuple_t::const_iterator pos
				, cyng::tuple_t::const_iterator end)
			{
				std::size_t count = std::distance(pos, end);
				BOOST_ASSERT_MSG(count == 5, "Period Entry");
				if (count != 5) return cyng::param_factory("error", "read_period_entry");

				//
				//	object name
				//
				obis const code = read_obis(*pos++);

				//
				//	unit (see sml_unit_enum)
				//
				auto const unit = read_unit(*pos++);

				//
				//	scaler
				//
				const auto scaler = read_scaler(*pos++);

				//
				//	value
				//
				auto val = read_value(code, scaler, unit, *pos++);

				//
				//	valueSignature
				//
				//vec.push_back(*pos);

				//if (OBIS_CODE_PUSH_TARGET == code) {

				//}

				//
				//	make a parameter
				//
				return cyng::param_factory(code.to_str(), val);
			}

			cyng::object read_value(obis code, std::int8_t scaler, std::uint8_t unit, cyng::object obj)
			{
				//
				//	write value
				//
				if (OBIS_DATA_MANUFACTURER == code
					|| OBIS_CODE_PUSH_TARGET == code
					|| OBIS_DATA_PUSH_DETAILS == code)
				{
					return read_string(obj);
				}
				else if (OBIS_CURRENT_UTC == code)
				{
					if (obj.get_class().tag() == cyng::TC_TUPLE)
					{
						return read_time(obj);
					}
					else
					{
						auto const tm = cyng::value_cast<std::uint32_t>(obj, 0);
						auto const tp = std::chrono::system_clock::from_time_t(tm);
						return cyng::make_object(tp);
					}
				}
				else if (OBIS_ACT_SENSOR_TIME == code)
				{
					auto const tm = cyng::value_cast<std::uint32_t>(obj, 0);
					auto const tp = std::chrono::system_clock::from_time_t(tm);
					//ro.set_value(get_name(code), cyng::make_object(tp));
					return cyng::make_object(tp);
				}
				else if (OBIS_SERIAL_NR == code)
				{
					cyng::buffer_t const buffer = cyng::to_buffer(obj);
					auto const serial_nr = cyng::io::to_hex(buffer);
					return cyng::make_object(serial_nr);
				}
				else if (OBIS_SERIAL_NR_SECOND == code)
				{
					cyng::buffer_t const buffer = cyng::to_buffer(obj);
					auto const serial_nr = cyng::io::to_hex(buffer);
					return cyng::make_object(serial_nr);
				}
				else if (OBIS_MBUS_STATE == code) {
					auto const state = cyng::numeric_cast<std::int32_t>(obj, 0);
					return cyng::make_object(state);
				}
				else if (OBIS_CODE_PEER_ADDRESS == code) {
					//	OBIS-T-Kennzahl der Ereignisquelle
					cyng::buffer_t const buffer = cyng::to_buffer(obj);
					return (buffer.size() == 6)
						? cyng::make_object(obis(buffer).to_str())
						: cyng::make_object(buffer)
						;
				}
				else if (OBIS_CLASS_EVENT == code) {
					auto const event = cyng::numeric_cast<std::uint32_t>(obj, 0);
					return cyng::make_object(event);
				}

				if (scaler != 0)
				{
					switch (obj.get_class().tag())
					{
					case cyng::TC_INT64:
					{
						std::int64_t const value = cyng::value_cast<std::int64_t>(obj, 0);
						auto const str = scale_value(value, scaler);
						return cyng::make_object(str);
					}
					break;
					case cyng::TC_INT32:
					{
						std::int32_t const value = cyng::value_cast<std::int32_t>(obj, 0);
						auto const str = scale_value(value, scaler);
						return cyng::make_object(str);
					}
					break;
					case cyng::TC_UINT64:
					{
						std::uint64_t const value = cyng::value_cast<std::uint64_t>(obj, 0);
						auto const str = scale_value(value, scaler);
						return cyng::make_object(str);
					}
					break;
					case cyng::TC_UINT32:
					{
						std::uint32_t const value = cyng::value_cast<std::uint32_t>(obj, 0);
						auto const str = scale_value(value, scaler);
						return cyng::make_object(str);
					}
					break;
					default:
						break;
					}
				}
				return  obj;
			}

			cyng::param_t read_param_tree(std::size_t depth, cyng::object obj)
			{
				cyng::tuple_t const tpl = cyng::to_tuple(obj);

				return (tpl.size() == 3)
					? read_param_tree(depth, tpl.begin(), tpl.end())
					: cyng::param_t{}
				;
			}

			cyng::param_t read_param_tree(std::size_t depth
				, cyng::tuple_t::const_iterator pos
				, cyng::tuple_t::const_iterator end)
			{
				std::size_t count = std::distance(pos, end);
				BOOST_ASSERT_MSG(count == 3, "SML Tree");
				if (count != 3) return cyng::param_t{};

				//
				//	1. parameterName Octet String,
				//
				obis const code = read_obis(*pos++);

				//
				//	2. parameterValue SML_ProcParValue OPTIONAL,
				//
				auto const attr = read_parameter(*pos++);
				if (attr.first != PROC_PAR_UNDEF) {
					return customize_value(code, attr.second);
				}

				//
				//	3. child_List List_of_SML_Tree OPTIONAL
				//
				cyng::tuple_t tpl;
				tpl = cyng::value_cast(*pos, tpl);
				//BOOST_ASSERT(!tpl.empty()); -> empty trees are possible e.g. 00, 80, 80, 00, 04, FF (certificates)

				if ((OBIS_ROOT_VISIBLE_DEVICES == code) || (OBIS_ROOT_ACTIVE_DEVICES == code)) {

					//
					//	build a vector of devices
					//
					return collect_devices(code, tpl);
				}
				else if (OBIS_IF_1107_METER_LIST == code) {
					//
					//	build a vector of devices
					//	81 81 C7 93 09 FF
					//
					return collect_iec_devices(tpl);
				}

				//
				//	collect all optional values in a parameter map
				//
				cyng::param_map_t params;

				for (auto const child : tpl)
				{
					BOOST_ASSERT(child.get_class().tag() == cyng::TC_TUPLE);
					cyng::tuple_t const tree = cyng::to_tuple(child);
					BOOST_ASSERT_MSG(tree.size() == 3, "SML Tree");

					auto const param = read_param_tree(depth + 1, tree.begin(), tree.end());

					//
					//	collect all optional values in a parameter map.
					//	No duplicates allowed
					//
					BOOST_ASSERT(params.find(param.first) == params.end());
					params.emplace(param.first, param.second);

				}

				//
				//	detect OBIS codes with index
				//
				if (code.is_matching(0x81, 0x49, 0x0D, 0x07, 0x00).second) {
					//	IP-T parameters
					params.emplace(cyng::param_factory("idx", code.get_storage()));
				}
				else if (code.is_matching(0x00, 0x80, 0x80, 0x11, 0x01).second) {
					//	Actuators:
					params.emplace(cyng::param_factory("idx", code.get_storage()));
				}
				else if (code.is_matching(0x81, 0x81, 0xC7, 0x82, 0x07).second) {
					//	firmware
					params.emplace(cyng::param_factory("idx", code.get_storage()));
				}
				else if (OBIS_DATA_PUBLIC_KEY == code
					|| OBIS_ROOT_SENSOR_BITMASK == code
					|| OBIS_DATA_USER_NAME == code
					|| OBIS_DATA_USER_PWD == code) {
					return cyng::param_factory(code.to_str(), "");
				}

				return cyng::param_factory(code.to_str(), params);
			}

			cyng::param_t customize_value(obis code, cyng::object obj)
			{
				if (OBIS_CODE(81, 49, 17, 07, 00, 01) == code
					|| OBIS_CODE(81, 49, 17, 07, 00, 02) == code) {
					return cyng::param_factory(code.to_str(), ip_address_to_str(obj));
				}
				else if (code.is_matching(0x81, 0x49, 0x1A, 0x07, 0x00).second
					|| OBIS_W_MBUS_MODE_S == code
					|| OBIS_W_MBUS_MODE_T == code) {
					return cyng::param_factory(code.to_str(), cyng::numeric_cast<std::uint16_t>(obj, 0u));
				}
				else if (code.is_matching(0x81, 0x49, 0x19, 0x07, 0x00).second) {
					return cyng::param_factory(code.to_str(), cyng::numeric_cast<std::uint16_t>(obj, 0u));
				}
				else if (code.is_matching(0x81, 0x49, 0x63, 0x3C, 0x01).second
					|| code.is_matching(0x81, 0x49, 0x63, 0x3C, 0x02).second
					|| OBIS_W_MBUS_ADAPTER_MANUFACTURER == code
					|| OBIS_W_MBUS_FIRMWARE == code
					|| OBIS_W_MBUS_HARDWARE == code
					|| OBIS_DATA_MANUFACTURER == code
					|| OBIS_CODE_DEVICE_KERNEL == code
					|| OBIS_CODE_VERSION == code
					|| OBIS_CODE_FILE_NAME == code
					|| OBIS_IF_1107_METER_ID == code
					|| OBIS_IF_1107_ADDRESS == code
					|| OBIS_IF_1107_P1 == code
					|| OBIS_IF_1107_W5 == code
					|| OBIS_CODE_PUSH_TARGET == code
					|| code.is_matching(0x81, 0x81, 0xC7, 0x82, 0x0A).second) {
					//	buffer to string
					cyng::buffer_t const buffer = cyng::to_buffer(obj);
					return cyng::param_factory(code.to_str(), std::string(buffer.begin(), buffer.end()));
				}
				else if (OBIS_CODE(81, 49, 17, 07, 00, 00) == code) {
					return cyng::param_factory(code.to_str(), to_ip_address_v4(obj));
				}
				else if (OBIS_DEVICE_CLASS == code
					|| OBIS_DATA_AES_KEY == code) {

					if (obj.is_null()) {
						return cyng::param_factory(code.to_str(), "");
					}
				}
				else if (OBIS_CURRENT_UTC == code) {
					return cyng::param_factory(code.to_str(), std::chrono::system_clock::now());
				}
				return cyng::param_t(code.to_str(), obj);
			}

			cyng::param_t collect_devices(obis code, cyng::tuple_t const& tpl)
			{
				cyng::param_map_t data;
				for (auto const& outer_list : tpl) {

					cyng::tuple_t const devices = cyng::to_tuple(outer_list);
					BOOST_ASSERT(devices.size() == 3);	//	SML tree
					if (devices.size() == 3) {

						auto pos = devices.begin();

						//
						//	1. parameterName Octet String,
						//
						auto const sub_code = read_obis(*pos++);

						//
						//	2. parameterValue SML_ProcParValue OPTIONAL,
						//
						auto const attr = read_parameter(*pos++);
						boost::ignore_unused(attr);

						//
						//	3. child_List List_of_SML_Tree OPTIONAL
						//

						cyng::tuple_t const inner_list = cyng::to_tuple(*pos);

						for (auto const& device : inner_list) {
							auto const param = read_param_tree(0, device);

							//
							//	ToDo: add device type (node::sml::get_srv_type(meter))
							//	manufacturer
							//	if (type == node::sml::SRV_MBUS_WIRED || type == node::sml::SRV_MBUS_RADIO) {
							//		auto const code = node::sml::get_manufacturer_code(meter);
							//		maker = node::sml::decode(code);
							//	}
							//	and meter id (short)
							//

							cyng::param_map_t values = cyng::to_param_map(param.second);

							//
							//	get/set number from OBIS code
							//
							auto const dev_code = to_obis(param.first);
							values.emplace("nr", cyng::make_object(dev_code.get_number()));

							//
							//	* 81 81 C7 82 04 FF: server ID
							//	* 81 81 C7 82 02 FF: device class (mostly 2D 2D 2D)
							//	* 01 00 00 09 0B 00: timestamp
							//
							auto const pos_srv = values.find(OBIS_SERVER_ID.to_str());
							if (pos_srv != values.end()) {

								//
								//	get server ID
								//
								BOOST_ASSERT(pos_srv->second.get_class().tag() == cyng::TC_BUFFER);
								cyng::buffer_t const meter = cyng::to_buffer(pos_srv->second);

								//
								//	get server type
								//
								std::uint32_t const srv_type = get_srv_type(meter);
								values.emplace("type", cyng::make_object(srv_type));

								//
								//	get manufacturer
								//
								switch (srv_type) {
								case SRV_MBUS_WIRED:
								case SRV_MBUS_RADIO:
								case SRV_W_MBUS:
								{
									auto const code = get_manufacturer_code(meter);
									auto const maker = decode(code);
									values.emplace("maker", cyng::make_object(maker));
								}
								break;
								default:
									values.emplace("maker", cyng::make_object(""));
									break;
								}

								//
								//	get meter ID (4 bytes)
								//
								auto const serial = get_serial(meter);
								values.emplace("serial", cyng::make_object(serial));

								//
								//	update meter ident
								//
								std::string s = from_server_id(meter);
								pos_srv->second = cyng::make_object(s);

							}

							data.emplace(param.first, cyng::make_object(values));
						}
					}
				}
				return cyng::param_factory(code.to_str(), data);
			}

			cyng::param_t collect_iec_devices(cyng::tuple_t const& tpl)
			{
				cyng::vector_t vec;
				for (auto const& outer_list : tpl) {

					cyng::tuple_t const devices = cyng::to_tuple(outer_list);
					BOOST_ASSERT(devices.size() == 3);	//	SML tree
					if (devices.size() == 3) {

						auto pos = devices.begin();

						//
						//	1. parameterName Octet String,
						//
						auto const sub_code = read_obis(*pos++);

						//
						//	2. parameterValue SML_ProcParValue OPTIONAL,
						//
						auto const attr = read_parameter(*pos++);
						boost::ignore_unused(attr);

						//
						//	3. child_List List_of_SML_Tree OPTIONAL
						//	all IEC devices
						//

						cyng::tuple_t const device = cyng::to_tuple(*pos);

						cyng::param_map_t values;
						for (auto const& data : device) {

							//
							//	read device data
							//
							auto const param = read_param_tree(0, data);
							values.insert(param);

						}
						values.emplace("nr", cyng::make_object(sub_code.get_storage()));
						vec.push_back(cyng::make_object(values));
					}
				}
				return cyng::param_factory(OBIS_IF_1107_METER_LIST.to_str(), vec);
			}

			obis read_obis(cyng::object obj)
			{
				auto const tmp = cyng::to_buffer(obj);

				return (tmp.empty())
					? obis()
					: obis(tmp)
					;
			}

			cyng::attr_t read_parameter(cyng::object obj)
			{
				cyng::tuple_t const tpl = cyng::to_tuple(obj);
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
				cyng::tuple_t const choice = cyng::to_tuple(obj);
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
				cyng::buffer_t const buffer = cyng::to_buffer(obj);
				return cyng::make_object(cyng::io::to_ascii(buffer));
			}

			std::int8_t read_scaler(cyng::object obj)
			{
				return cyng::numeric_cast<std::int8_t>(obj, 0);
			}

			std::uint8_t read_unit(cyng::object obj)
			{
				return cyng::value_cast<std::uint8_t>(obj, 0);
			}

			std::vector<obis> read_param_tree_path(cyng::object obj)
			{
				std::vector<obis> result;

				cyng::tuple_t const tpl = cyng::to_tuple(obj);

				std::transform(tpl.begin(), tpl.end(), std::back_inserter(result), [](cyng::object obj) -> obis {
					return read_obis(obj);
					});

				//for (auto obj : tpl)
				//{

				//	const obis object_type = read_obis(obj);
				//	result.push_back(object_type);

				//}
				return result;
			}
		}	//	reader
	}	//	sml
}

