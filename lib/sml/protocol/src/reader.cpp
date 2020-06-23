/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/sml/protocol/reader.h>
#include <smf/sml/defs.h>
#include <smf/sml/intrinsics/obis_factory.hpp>
#include <smf/sml/obis_db.h>
#include <smf/sml/obis_io.h>

#include <cyng/numeric_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/vm/generator.h>
#include <cyng/vm/manip.h>


#include <boost/uuid/nil_generator.hpp>
#include <boost/core/ignore_unused.hpp>


    
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

			/**
			 * Read the SML message body a selected by message type code
			 */
			cyng::vector_t read_body(readout& ro, message_e code, cyng::tuple_t const& tpl);

			/** @brief message_e::OPEN_REQUEST (256)
			 */
			cyng::vector_t read_public_open_request(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

			/** @brief message_e::OPEN_RESPONSE (257)
			 */
			cyng::vector_t read_public_open_response(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

			/** @brief message_e::CLOSE_REQUEST (512)
			 */
			cyng::vector_t read_public_close_request(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

			/** @brief message_e::CLOSE_RESPONSE (513)
			 */
			cyng::vector_t read_public_close_response(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

			/** @brief message_e::GET_PROFILE_LIST_REQUEST (1024)
			 */
			cyng::vector_t read_get_profile_list_request(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

			/** @brief message_e::GET_PROFILE_LIST_RESPONSE (1025)
			 */
			cyng::vector_t read_get_profile_list_response(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

			/** @brief message_e::GET_PROC_PARAMETER_RESPONSE (1281)
			 */
			cyng::vector_t read_get_proc_parameter_response(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

			/** @brief message_e::GET_PROC_PARAMETER_REQUEST (1280)
			 */
			cyng::vector_t read_get_proc_parameter_request(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

			/** @brief message_e::SET_PROC_PARAMETER_REQUEST (1536)
			 */
			cyng::vector_t read_set_proc_parameter_request(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

			/** @brief message_e::SET_PROC_PARAMETER_RESPONSE (1537)
			 *
			 * This message type is undefined
			 */
			cyng::vector_t read_set_proc_parameter_response(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

			/** @brief message_e::GET_LIST_REQUEST (1792)
			 */
			cyng::vector_t read_get_list_request(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

			/** @brief message_e::GET_LIST_RESPONSE (1793)
			 */
			cyng::vector_t read_get_list_response(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

			cyng::vector_t  read_get_cosem_request(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			cyng::vector_t  read_get_cosem_response(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			cyng::vector_t  read_set_cosem_request(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			cyng::vector_t  read_set_cosem_response(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			cyng::vector_t  read_action_cosem_request(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			cyng::vector_t  read_action_cosem_response(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

			/** @brief message_e::ATTENTION_RESPONSE (65281)
			 */
			cyng::vector_t read_attention_response(readout& ro, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);




			void read_sml_list(readout& ro, obis, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			void read_list_entry(readout& ro, obis, std::size_t index, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end);





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
				//	std::pair<message_e, cyng::tuple_t>
				auto const r = ro.read_msg(pos, end);

				//
				//	Read the SML message body
				//
				return read_body(ro, r.first, r.second);
			}

			cyng::vector_t read_body(readout& ro, message_e code, cyng::tuple_t const& tpl)
			{
				switch (code)
				{
				case message_e::OPEN_REQUEST:
					return read_public_open_request(ro, tpl.begin(), tpl.end());
				case message_e::OPEN_RESPONSE:
					return read_public_open_response(ro, tpl.begin(), tpl.end());
				case message_e::CLOSE_REQUEST:
					return read_public_close_request(ro, tpl.begin(), tpl.end());
				case message_e::CLOSE_RESPONSE:
					return read_public_close_response(ro, tpl.begin(), tpl.end());
				case message_e::GET_PROFILE_PACK_REQUEST:
					//cyng::xml::write(node.append_child("data"), body);
					break;
				case message_e::GET_PROFILE_PACK_RESPONSE:
					//cyng::xml::write(node.append_child("data"), body);
					break;
				case message_e::GET_PROFILE_LIST_REQUEST:
					return read_get_profile_list_request(ro, tpl.begin(), tpl.end());
				case message_e::GET_PROFILE_LIST_RESPONSE:
					return read_get_profile_list_response(ro, tpl.begin(), tpl.end());
				case message_e::GET_PROC_PARAMETER_REQUEST:
					return read_get_proc_parameter_request(ro, tpl.begin(), tpl.end());
				case message_e::GET_PROC_PARAMETER_RESPONSE:
					return read_get_proc_parameter_response(ro, tpl.begin(), tpl.end());
				case message_e::SET_PROC_PARAMETER_REQUEST:
					return read_set_proc_parameter_request(ro, tpl.begin(), tpl.end());
				case message_e::SET_PROC_PARAMETER_RESPONSE:
					return read_set_proc_parameter_response(ro, tpl.begin(), tpl.end());
				case message_e::GET_LIST_REQUEST:
					return read_get_list_request(ro, tpl.begin(), tpl.end());
				case message_e::GET_LIST_RESPONSE:
					return read_get_list_response(ro, tpl.begin(), tpl.end());

				case message_e::GET_COSEM_REQUEST:	
					return read_get_cosem_request(ro, tpl.begin(), tpl.end());
				case message_e::GET_COSEM_RESPONSE:
					return read_get_cosem_response(ro, tpl.begin(), tpl.end());
				case message_e::SET_COSEM_REQUEST:
					return read_set_cosem_request(ro, tpl.begin(), tpl.end());
				case message_e::SET_COSEM_RESPONSE:
					return read_set_cosem_response(ro, tpl.begin(), tpl.end());
				case message_e::ACTION_COSEM_REQUEST:
					return read_action_cosem_request(ro, tpl.begin(), tpl.end());
				case message_e::ACTION_COSEM_RESPONSE:
					return read_action_cosem_response(ro, tpl.begin(), tpl.end());

				case message_e::ATTENTION_RESPONSE:
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
				if (ro.read_public_open_request(pos, end)) {
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
				return cyng::generate_invoke("log.msg.error", "SML Public Open Request");
			}

			cyng::vector_t read_public_open_response(readout& ro, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
			{
				if (ro.read_public_open_response(pos, end)) {

					BOOST_ASSERT(ro.client_id_.size() == 6);
					BOOST_ASSERT(ro.server_id_.size() == 7);

					return cyng::generate_invoke("sml.public.open.response"
						, ro.trx_
						, ro.client_id_
						, ro.server_id_
						, ro.get_value("reqFileId"));
				}
				return cyng::generate_invoke("log.msg.error", "SML Public Open Response");
			}

			cyng::vector_t read_public_close_request(readout& ro, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
			{
				if (ro.read_public_close_request(pos, end)) {

					return cyng::generate_invoke("sml.public.close.request"
						, ro.trx_);
				}
				return cyng::generate_invoke("log.msg.error", "SML Public Close Request");
			}

			cyng::vector_t read_public_close_response(readout& ro, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
			{
				if (ro.read_public_close_response(pos, end)) {
					return cyng::generate_invoke("sml.public.close.response", ro.trx_);
				}
				return cyng::generate_invoke("log.msg.error", "SML Public Close Response");
			}

			cyng::vector_t read_get_profile_list_response(readout& ro, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
			{
				//std::pair<obis_path_t, cyng::param_map_t> readout::
				auto const r = ro.read_get_profile_list_response(pos, end);

				//
				//	slot [8] of gateway proxy
				//

				return cyng::generate_invoke("sml.get.profile.list.response"
					, ro.trx_
					, ro.get_value("actTime")
					, ro.get_value("regPeriod")
					, ro.get_value("valTime")
					//, (r.first.empty() ? obis().to_buffer() : r.first.at(0).to_buffer())
					, r.first
					, ro.server_id_
					, ro.get_value("status")
					, r.second);
			}

			cyng::vector_t read_get_profile_list_request(readout& ro, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
			{
				auto const path = ro.read_get_profile_list_request(pos, end);

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
				//	std::pair<obis_path_t, cyng::param_t> 
				auto const r = ro.read_get_proc_parameter_response(pos, end);

				//
				//	slot [3] of gateway proxy
				//

				return cyng::generate_invoke("sml.get.proc.parameter.response"
					, ro.trx_
					, ro.get_value("groupNo")
					, ro.server_id_	//	binary
					, r.first	//	send complete path
					, r.second);
			}

			cyng::vector_t read_get_proc_parameter_request(readout& ro, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
			{
				// std::pair<obis_path_t, cyng::object>
				auto const r = ro.read_get_proc_parameter_request(pos, end);

				return cyng::generate_invoke("sml.get.proc.parameter.request"
					, ro.trx_
					, ro.server_id_
					, ro.get_value("userName")
					, ro.get_value("password")
					, vector_from_path(r.first)
					, r.second);
			}

			cyng::vector_t read_set_proc_parameter_request(readout& ro, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
			{
				//std::pair<obis_path_t, cyng::param_t> 
				auto const r = ro.read_set_proc_parameter_request(pos, end);

				cyng::vector_t prg;
				prg << cyng::unwinder(cyng::generate_invoke("sml.attention.init"
						, ro.trx_
						, ro.server_id_
						, OBIS_ATTENTION_OK.to_buffer()
						, "Set Proc Parameter Request"))
					<< cyng::unwinder(cyng::generate_invoke("sml.set.proc.parameter.request"
						, ro.trx_
						, to_hex(r.first, ' ')	//	string
						, ro.server_id_
						, ro.get_value("userName")
						, ro.get_value("password")
						, r.second))
					<< cyng::unwinder(cyng::generate_invoke("sml.attention.send", ro.trx_))
					;

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
				auto code = read_obis(*pos++);
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
					, obis_path_t{ code }
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


		}	//	reader
	}	//	sml
}

