/*
 * Copyright Sylko Olzscher 2017
 *
 * Use, modification, and distribution is subject to the Boost Software
 * License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#include <noddy/sml/exporter/sml_json_exporter.h>
#include <noddy/m2m/intrinsics/io/json_io_callback.h>
#include <noddy/m2m/intrinsics/ctrl_address.h>
#include <noddy/m2m/obis_codes.h>
#include <noddy/m2m/unit_codes.h>
#include <noddy/m2m/numeric.h>
#include <noddy/m2m/intrinsics/io/formatter/obis_format.h>
#include <cyy/value_cast.hpp>
#include <cyy/numeric_cast.hpp>
#include <cyy/intrinsics/factory.h>
#include <cyy/io/format/buffer_format.h>
#include <cyy/io/format/time_format.h>
#include <cyy/io.h>
#include <cyy/io/io_set.h>
#include <cyy/io/classification.h>
#include <cyy/json/json_io.h>
#include <cyy/util/dom_manip.h>
#include <boost/core/ignore_unused.hpp>

namespace noddy
{
	namespace sml	
	{

		sml_json_exporter::sml_json_exporter(logger_type& logger)
			: logger_(logger)
			//, source_(0)
			//, channel_(0)
			, root_()
			, trx_()
			, client_id_()
			, server_id_()
			, choice_(sml::BODY_UNKNOWN)
		{
			reset();
		}

		std::string const& sml_json_exporter::get_last_trx() const
		{
			return trx_;
		}

		cyy::buffer_t const& sml_json_exporter::get_last_client_id() const
		{
			return client_id_;
		}

		cyy::buffer_t const& sml_json_exporter::get_last_server_id_() const
		{
			return server_id_;
		}

		std::uint32_t sml_json_exporter::get_last_choice() const
		{
			return choice_;
		}

		cyy::vector_t const& sml_json_exporter::get_root() const
		{
			return root_;
		}

		void sml_json_exporter::extract(cyy::object const& obj, std::size_t msg_index)
		{
			BOOST_ASSERT_MSG(cyy::primary_type_code_test< cyy::types::CYY_TUPLE >(obj), "tuple type expected");
			const cyy::tuple_t def;
			extract(cyy::value_cast<cyy::tuple_t>(obj, def), msg_index);
		}

		void sml_json_exporter::extract(cyy::tuple_t const& tpl, std::size_t msg_index)
		{
			traverse(std::begin(tpl), std::end(tpl), msg_index);
		}

		bool sml_json_exporter::save(boost::filesystem::path const& p)
		{
			BOOST_ASSERT_MSG(!boost::filesystem::exists(p), "file already exists");
			// Save object tree as file.
			std::ofstream fout(p.string(), std::ios::trunc | std::ios::out);
			if (fout.is_open())
			{
				//std::cout
				//	<< "***Info: output file is "
				//	<< p
				//	<< std::endl
				//	;

				//return print(fout);
				return cyy::serialize_json_pretty(fout, cyy::factory(root_), m2m::json_io_callback);
			}

			CYY_LOG_FATAL(logger_, "cannot open file " << p);
			return false;
		}

		bool sml_json_exporter::print(std::ostream& os)
		{
			os << cyy::io::to_literal(root_);
			return true;
			//return cyy::serialize_json_pretty(os, cyy::factory(root_), m2m::json_io_callback);
		}


		void sml_json_exporter::reset()
		{
			root_.clear();
			trx_.clear();
			client_id_.clear();
			server_id_.clear();
			choice_ = sml::BODY_UNKNOWN;
		}

		//
		//	traverse()
		//
		void sml_json_exporter::traverse(cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end
			, std::size_t msg_index)
		{
			std::size_t count = std::distance(begin, end);

			if (count < 5)
			{
				//	missing elements
				CYY_LOG_ERROR(logger_, "SML_Message("
					<< count
					<< "5) - missing parameter");

				return;
			}

			//
			//	create a new nessage (tuple)
			//
			cyy::tuple_t& msg = cyy::add_child(root_);

			//root_.push_back(cyy::set_factory("message", msg_index));

			//
			//	message counter
			//
			cyy::add_param(msg, "message", cyy::index_factory(msg_index));
#ifdef _DEBUG
			print(std::cerr);
#endif

			//auto msg = root_.append_child("message");
			//msg.append_attribute("index") = (unsigned)msg_index;

			//
			//	(1) - transaction id
			//	This is typically a printable sequence of 6 up to 9 ASCII values
			//
			BOOST_ASSERT_MSG(cyy::type_code_test<cyy::types::CYY_BUFFER>(*begin), "wrong data type - expected CYY_BUFFER as transaction id");
			//serialize_xml(msg, "trx", *begin);
			/*cyy::add_param(msg, "trx", *begin);*/

			//	store transaction name
			trx_ = cyy::buffer_format_ascii(cyy::value_cast<cyy::buffer_t>(*begin));
			cyy::add_param(msg, "trx", cyy::factory(trx_));

			//
			//	(2) groupNo
			//
			++begin;
			BOOST_ASSERT_MSG(cyy::type_code_test<cyy::types::CYY_UINT8>(*begin), "wrong data type - expected CYY_UINT8 as groupNo");
			cyy::add_param(msg, "groupNo", *begin);
			//serialize_xml(msg, "groupNo", *begin);

			//
			//	(3) abortOnError
			//
			++begin;
			BOOST_ASSERT_MSG(cyy::type_code_test<cyy::types::CYY_UINT8>(*begin), "wrong data type - expected CYY_UINT8 as abortOnError");
			{
				cyy::add_param(msg, "abortOnError", *begin);
				//auto child = msg.append_child("abortOnError");
				//serialize_xml(child, *begin);
			}

			//
			//	choice
			//
			++begin;
			BOOST_ASSERT_MSG(cyy::primary_type_code_test<cyy::types::CYY_TUPLE>(*begin), "wrong data type - expected CYY_TUPLE as choice");
			cyy::tuple_t choice;
			choice = cyy::value_cast<cyy::tuple_t>(*begin, choice);
			BOOST_ASSERT_MSG(choice.size() == 2, "wrong tuple size");
			traverse_body(msg, std::begin(choice), std::end(choice));

			//
			//	crc16
			//
			++begin;
			{
				//
				//	debugging
				//
#ifdef _DEBUG
				std::uint16_t crc16 = 0;
				crc16 = cyy::numeric_cast(*begin, crc16);
				boost::ignore_unused_variable_warning(crc16);
#endif

				cyy::add_param(msg, "crc16", *begin);
				//auto child = msg.append_child("crc16");
				//serialize_xml(child, *begin);
			}

			//	message complete
		}

		//
		//	traverse_body()
		//
		void sml_json_exporter::traverse_body(cyy::tuple_t& node
			, cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(begin, end);

			if (count < 2)
			{
				//	missing elements
				CYY_LOG_ERROR(logger_, "SML_Choice("
					<< count
					<< "2) - missing parameter");
				return;
			}

			//
			//	detect message type
			//
			const cyy::object obj = *begin;
			choice_ = cyy::numeric_cast(*begin, choice_);
			cyy::add_param(node, "choice", cyy::factory(choice_));

			//
			//	message body
			//
			++begin;
			BOOST_ASSERT_MSG(cyy::primary_type_code_test<cyy::types::CYY_TUPLE>(*begin), "wrong data type - expected CYY_TUPLE as message body");
			cyy::tuple_t body;
			body = cyy::value_cast<cyy::tuple_t>(*begin, body);


			switch (choice_)
			{
			case sml::BODY_OPEN_REQUEST:
			{
				auto& child = cyy::add_child("bodyOpenRequest", node);
				traverse_open_request(child, std::begin(body), std::end(body));

				//auto child = node.append_child("bodyOpenRequest");
				//child.append_attribute("code") = choice_;
				//traverse_open_request(child, std::begin(body), std::end(body));
			}
			break;

			case sml::BODY_OPEN_RESPONSE:
			{
				auto& child = cyy::add_child("bodyOpenResponse", node);
				traverse_open_response(child, std::begin(body), std::end(body));

				//auto child = node.append_child("bodyOpenResponse");
				//child.append_attribute("code") = choice_;
				//traverse_open_response(child, std::begin(body), std::end(body));
			}
			break;

			case sml::BODY_CLOSE_REQUEST:
			{
				auto& child = cyy::add_child("bodyCloseRequest", node);
				boost::ignore_unused(child);		
				//traverse_close_request(child, std::begin(body), std::end(body));

				//auto child = node.append_child("bodyCloseRequest");
				//child.append_attribute("code") = choice_;
				//serialize_xml(child, *begin);
			}
			break;

			case sml::BODY_CLOSE_RESPONSE:
			{
				auto& child = cyy::add_child("bodyCloseResponse", node);
				traverse_close_response(child, std::begin(body), std::end(body));

				//auto child = node.append_child("bodyCloseResponse");
				//child.append_attribute("code") = choice_;
				//traverse_close_response(child, std::begin(body), std::end(body));
			}
			break;

			case sml::BODY_GET_PROFILE_PACK_REQUEST:
			{
				auto& child = cyy::add_child("bodyGetProfilePackRequest", node);
				traverse_get_profile_pack_request(child, std::begin(body), std::end(body));

				//auto child = node.append_child("bodyGetProfilePackRequest");
				//child.append_attribute("code") = choice_;
				//traverse_get_profile_pack_request(child, std::begin(body), std::end(body));
			}
			break;

			case sml::BODY_GET_PROFILE_PACK_RESPONSE:
			{
				auto& child = cyy::add_child("bodyGetProfilePackResponse", node);
				boost::ignore_unused(child);		
				//serialize_json(child, std::begin(body), std::end(body));

				//auto child = node.append_child("bodyGetProfilePackResponse");
				//child.append_attribute("code") = choice_;
				//serialize_xml(child, *begin);
			}
			break;

			case sml::BODY_GET_PROFILE_LIST_REQUEST:
			{
				auto& child = cyy::add_child("bodyGetProfileListRequest", node);
				traverse_get_profile_list_request(child, std::begin(body), std::end(body));

				//auto child = node.append_child("bodyGetProfileListRequest");
				//child.append_attribute("code") = choice_;
				//traverse_get_profile_list_request(child, std::begin(body), std::end(body));
			}
				break;

			case sml::BODY_GET_PROFILE_LIST_RESPONSE:
			{
				auto& child = cyy::add_child("bodyGetProfileListResponse", node);
				traverse_get_profile_list_response(child, std::begin(body), std::end(body));

				//auto child = node.append_child("bodyGetProfileListResponse");
				//child.append_attribute("code") = choice_;
				//traverse_get_profile_list_response(child, std::begin(body), std::end(body));
			}
				break;

			case sml::BODY_GET_PROC_PARAMETER_REQUEST:
			{
				auto& child = cyy::add_child("bodyGetProcParameterRequest", node);
				traverse_get_process_parameter_request(child, std::begin(body), std::end(body));

				//auto child = node.append_child("bodyGetProcParameterRequest");
				//child.append_attribute("code") = choice_;
				//traverse_get_process_parameter_request(child, std::begin(body), std::end(body));
			}
				break;

			case sml::BODY_GET_PROC_PARAMETER_RESPONSE:
			{
				auto& child = cyy::add_child("bodyGetProcParameterResponse", node);
				traverse_get_process_parameter_response(child, std::begin(body), std::end(body));

				//auto child = node.append_child("bodyGetProcParameterResponse");
				//child.append_attribute("code") = choice_;
				//traverse_get_process_parameter_response(child, std::begin(body), std::end(body));
			}
				break;

			case sml::BODY_SET_PROC_PARAMETER_REQUEST:
			{
				auto& child = cyy::add_child("bodySetProcParameterRequest", node);
				traverse_set_process_parameter_request(child, std::begin(body), std::end(body));

				//auto child = node.append_child("bodySetProcParameterRequest");
				//child.append_attribute("code") = choice_;
				//traverse_set_process_parameter_request(child, std::begin(body), std::end(body));
			}
				break;

			case sml::BODY_SET_PROC_PARAMETER_RESPONSE:
			{
				auto& child = cyy::add_child("bodySetProcParameterResponse", node);
				boost::ignore_unused(child);		

				//auto child = node.append_child("bodySetProcParameterResponse");
				//child.append_attribute("code") = choice_;
				//serialize_xml(child, *begin);
			}
				break;

			case sml::BODY_GET_LIST_REQUEST:
			{
				auto& child = cyy::add_child("bodyListRequest", node);
				boost::ignore_unused(child);		
				//traverse_set_process_parameter_request(child, std::begin(body), std::end(body));

				//auto child = node.append_child("bodyListRequest");
				//child.append_attribute("code") = choice_;
				//serialize_xml(child, *begin);
			}
				break;

			case sml::BODY_GET_LIST_RESPONSE:
			{
				auto& child = cyy::add_child("bodyListResponse", node);
				boost::ignore_unused(child);		
				//auto child = node.append_child("bodyListResponse");
				//child.append_attribute("code") = choice_;
				//serialize_xml(child, *begin);
			}
				break;

			case sml::BODY_ATTENTION_RESPONSE:
			{
				auto& child = cyy::add_child("bodyAttentionResponse", node);
				traverse_attention_response(child, std::begin(body), std::end(body));

				//auto child = node.append_child("bodyAttentionResponse");
				//child.append_attribute("code") = choice_;
				//traverse_attention_response(child, std::begin(body), std::end(body));
			}
				break;

			case sml::BODY_UNKNOWN:
			default:
				//serialize_xml(node, "choice", *begin);
#ifdef _DEBUG
				std::cerr
					<< "***Fatal: unknown SML message type: "
					<< choice_
					<< std::endl
					;
#endif
				break;
			}
		}

		//
		//	traverse_open_response()
		//
		void sml_json_exporter::traverse_open_response(cyy::tuple_t& node
			, cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(begin, end);
			if (count < 6)
			{
				//	missing elements
				CYY_LOG_ERROR(logger_, "SML_PublicOpen.Res("
					<< count
					<< "6) - missing parameter");
				return;
			}

			//	codepage "ISO 8859-15"
			cyy::add_param(node, "codepage", *begin);

			//
			//	clientId (MAC)
			//	Typically 7 bytes to identify gateway/MUC
			//	example: 0500153b01ec46
			++begin;
			//cyy::add_param(node, "clientId", *begin);
			cyy::buffer_t tmp;
			emit_ctrl_address(cyy::add_child("clientId", node), cyy::value_cast(*begin, tmp));
			//serialize_xml(node, "clientId", *begin);

			//
			//	reqFileId
			//	This is typically a printable sequence of 6 up to 9 ASCII values
			//
			++begin;
			cyy::add_param(node, "reqFileId", *begin);
			//serialize_xml(node, "reqFileId", *begin);

			//
			//	serverId
			//	Typically 9 bytes to identify meter/sensor. 
			//	Same as clientId when receiving a LSM-Status.
			//
			++begin;
			emit_ctrl_address(cyy::add_child("serverId", node), cyy::value_cast(*begin, tmp));

			//
			//	refTime
			//
			++begin;
			cyy::add_param(node, "refTime", *begin);
			//serialize_xml(node, "refTime", *begin);

			//	sml-Version: default = 1
			++begin;
			cyy::add_param(node, "SMLVersion", *begin);
			//serialize_xml(node, "SMLVersion", *begin);

		}

		//
		//	traverse_open_request()
		//
		void sml_json_exporter::traverse_open_request(cyy::tuple_t& node
			, cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(begin, end);
			if (count < 7)
			{
				//	missing elements
				CYY_LOG_ERROR(logger_, "SML_PublicOpen.Req("
					<< count
					<< "7) - missing parameter");
				return;
			}

			//	codepage "ISO 8859-15"
			cyy::add_param(node, "codepage", *begin);
			//serialize_xml(node, "codepage", *begin);

			//
			//	clientId (MAC)
			//	typically 7 bytes to identify gateway/MUC
			++begin;
			//serialize_xml(node, "clientId", *begin);
			client_id_ = cyy::value_cast(*begin, client_id_);
			cyy::add_param(node, "clientId", cyy::factory(client_id_));

			//
			//	reqFileId
			//
			++begin;
			cyy::add_param(node, "reqFileId", *begin);
			//serialize_xml(node, "reqFileId", *begin);

			//
			//	serverId
			//
			++begin;
			server_id_ = cyy::value_cast(*begin, server_id_);
			emit_ctrl_address(cyy::add_child("serverId", node), server_id_);

			//
			//	username
			//
			++begin;
			cyy::add_param(node, "username", *begin);
			//serialize_xml(node, "username", *begin);

			//
			//	password
			//
			++begin;
			cyy::add_param(node, "password", *begin);
			//serialize_xml(node, "password", *begin);

			//
			//	sml-Version: default = 1
			//
			++begin;
			cyy::add_param(node, "SMLVersion", *begin);
			//serialize_xml(node, "SMLVersion", *begin);

		}

		//
		//	traverse_close_response()
		//
		void sml_json_exporter::traverse_close_response(cyy::tuple_t& node
			, cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(begin, end);
			if (count < 1)
			{
				//	missing elements
				CYY_LOG_ERROR(logger_, "SML_PublicClose.Res("
					<< count
					<< "1) - missing parameter");
				return;
			}

			//	globalSignature
			cyy::add_param(node, "globalSignature", *begin);
			//serialize_xml(node, "globalSignature", *begin);

		}

		//
		//	traverse_get_profile_list_response()
		//
		void sml_json_exporter::traverse_get_profile_list_response(cyy::tuple_t& node
			, cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(begin, end);

			if (count < 9)
			{
				//	missing elements
				CYY_LOG_ERROR(logger_, "SML_GetProfileList.Res("
					<< count
					<< "9) - missing parameter");
				return;
			}

			//
			//	serverId
			//
			if(cyy::type_code_test<cyy::types::CYY_BUFFER>(*begin))
			{
				cyy::buffer_t tmp;
				emit_ctrl_address(cyy::add_child("serverId", node), cyy::value_cast(*begin, tmp));
			}

			//
			//	actTime
			//
			++begin;
			traverse_time(node, *begin, "actTime");

			//
			//	regPeriod
			//
			++begin;
			cyy::add_param(node, "regPeriod", *begin);
			//serialize_xml(node, "regPeriod", *begin);

			//
			//	parameterTreePath (OBIS)
			//
			++begin;
			const m2m::obis obj_code = traverse_parameter_tree_path(node, *begin);

			//
			//	valTime
			//
			++begin;
			traverse_time(node, *begin, "valTime");

			//
			//	status
			//
			++begin;
			cyy::add_param(node, "status", *begin);
			//serialize_xml(node, "status", *begin);

			//	period-List (1)
			//	first we make the TSMLGetProfileListResponse complete, then 
			//	the we store the entries in TSMLPeriodEntry.
			++begin;
			if(cyy::primary_type_code_test<cyy::types::CYY_TUPLE>(*begin))
			{
				cyy::tuple_t periods;
				periods = cyy::value_cast<cyy::tuple_t>(*begin, periods);

				//cyy::add_child("ListOfSMLPeriodEntries", node)
				cyy::vector_t vec = traverse_list_of_period_entries(std::begin(periods)
					, std::end(periods)
					, obj_code);
				cyy::add_param(node, "ListOfSMLPeriodEntries", cyy::factory(vec));

				//traverse_list_of_period_entries(node.append_child("ListOfSMLPeriodEntries")
				//	, std::begin(periods)
				//	, std::end(periods)
				//	, obj_code);
			}

			++begin;	//	rawdata
			cyy::add_param(node, "rawdata", *begin);
			//serialize_xml(node, "rawdata", *begin);

			++begin;	//	periodSignature
			cyy::add_param(node, "signature", *begin);
			//serialize_xml(node, "signature", *begin);

		}

		//
		//	traverse_get_process_parameter_response()
		//
		void sml_json_exporter::traverse_get_process_parameter_response(cyy::tuple_t& node
			, cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(begin, end);
			if (count < 3)
			{
				//	missing elements
				CYY_LOG_ERROR(logger_, "SML_GetProcParameter.Res("
					<< count
					<< "3) - missing parameter");
				return;
			}

			//
			//	serverId
			//
			//cyy::add_param(node, "serverId", *begin);
			cyy::buffer_t tmp;
			emit_ctrl_address(cyy::add_child("serverId", node), cyy::value_cast(*begin, tmp));

			//
			//	parameterTreePath (OBIS)
			//
			++begin;
			const m2m::obis obj_code = traverse_parameter_tree_path(node, *begin);
			boost::ignore_unused_variable_warning(obj_code);

			//
			//	parameterTree
			//
			++begin;
			traverse_sml_tree(cyy::add_child("parameterTree", node), *begin, 0, 0);
			//traverse_sml_tree(node.append_child("parameterTree"), *begin, 0, 0);

		}

		/**
		*	traverse_get_profile_pack_request()
		*
		* @param serverId
		* @param actTime
		* @param regPeriod
		* @param parameterTreePath
		* @param headerList
		* @param periodList
		* @param rawdata
		*            OPTIONAL
		* @param periodSignature
		*            OPTIONAL
		*/
		void sml_json_exporter::traverse_get_profile_pack_request(cyy::tuple_t& node
			, cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(begin, end);
			if (count < 9)
			{
				//	missing elements
				CYY_LOG_ERROR(logger_, "SML_GetProfilePack.Req("
					<< count
					<< "9) - missing parameter");
				return;
			}

			//
			//	serverId
			//
			//cyy::add_param(node, "serverId", *begin);
			cyy::buffer_t tmp;
			emit_ctrl_address(cyy::add_child("serverId", node), cyy::value_cast(*begin, tmp));

			//
			//	username
			//
			++begin;
			cyy::add_param(node, "username", *begin);
			//serialize_xml(node, "username", *begin);

			//
			//	password
			//
			++begin;
			cyy::add_param(node, "password", *begin);
			//serialize_xml(node, "password", *begin);

			//
			//	withRawData (boolean)
			//
			++begin;
			cyy::add_param(node, "withRawData", *begin);
			//serialize_xml(node, "withRawData", *begin);

			//
			//	beginTime
			//
			++begin;
			traverse_time(node, *begin, "beginTime");

			//
			//	endTime
			//
			++begin;
			traverse_time(node, *begin, "endTime");

			//
			//	parameterTreePath
			//
			++begin;
			traverse_parameter_tree_path(node, *begin);

			//
			//	objectList (List_of_SML_ObjReqEntry)
			//
			++begin;
			cyy::add_param(node, "objectList", *begin);
			//serialize_xml(node, "objectList", *begin);

			//
			//	dasDetails (SML_Tree)
			//
			++begin;
			traverse_sml_tree(node, *begin, 0, 0);

		}

		void sml_json_exporter::traverse_get_profile_list_request(cyy::tuple_t& node
			, cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(begin, end);
			if (count < 9)
			{
				//	missing elements
				CYY_LOG_ERROR(logger_, "SML_GetProfileList.Req("
					<< count
					<< "9) - missing parameter");
				return;
			}

			//
			//	serverId
			//
			//cyy::add_param(node, "serverId", *begin);
			cyy::buffer_t tmp;
			emit_ctrl_address(cyy::add_child("serverId", node), cyy::value_cast(*begin, tmp));

			//
			//	username
			//
			++begin;
			cyy::add_param(node, "username", *begin);
			//serialize_xml(node, "username", *begin);

			//
			//	password
			//
			++begin;
			cyy::add_param(node, "password", *begin);
			//serialize_xml(node, "password", *begin);

			//
			//	withRawData (boolean)
			//
			++begin;
			cyy::add_param(node, "withRawData", *begin);
			//serialize_xml(node, "withRawData", *begin);

			//
			//	beginTime
			//
			++begin;
			traverse_time(node, *begin, "beginTime");

			//
			//	endTime
			//
			++begin;
			traverse_time(node, *begin, "endTime");

			//
			//	parameterTreePath
			//
			++begin;
			traverse_parameter_tree_path(node, *begin);

			//
			//	objectList (List_of_SML_ObjReqEntry)
			//
			++begin;
			cyy::add_param(node, "objectList", *begin);
			//serialize_xml(node, "objectList", *begin);

			//
			//	dasDetails (SML_Tree)
			//
			++begin;
			traverse_sml_tree(node, *begin, 0, 0);

		}

		void sml_json_exporter::traverse_get_process_parameter_request(cyy::tuple_t& node
			, cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(begin, end);
			if (count < 5)
			{
				//	missing elements
				CYY_LOG_ERROR(logger_, "SML_GetProcParameter.Req("
					<< count
					<< "5) - missing parameter");
				return;
			}

			//
			//	serverId
			//
			//cyy::add_param(node, "serverId", *begin);
			cyy::buffer_t tmp;
			emit_ctrl_address(cyy::add_child("serverId", node), cyy::value_cast(*begin, tmp));

			//
			//	username
			//
			++begin;
			cyy::add_param(node, "username", *begin);
			//serialize_xml(node, "username", *begin);

			//
			//	password
			//
			++begin;
			cyy::add_param(node, "password", *begin);
			//serialize_xml(node, "password", *begin);

			//
			//	parameterTreePath
			//
			++begin;
			traverse_parameter_tree_path(node, *begin);

			//
			//	attribute
			//
			++begin;
			cyy::add_param(node, "attribute", *begin);
			//serialize_xml(node, "attribute", *begin);
		}

		void sml_json_exporter::traverse_set_process_parameter_request(cyy::tuple_t& node
			, cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(begin, end);
			if (count < 5)
			{
				//	missing elements
				CYY_LOG_ERROR(logger_, "SML_SetProcParameter.Req("
					<< count
					<< "5) - missing parameter");
				return;
			}

			//
			//	serverId
			//
			//cyy::add_param(node, "serverId", *begin);
			cyy::buffer_t tmp;
			emit_ctrl_address(cyy::add_child("serverId", node), cyy::value_cast(*begin, tmp));

			//
			//	username
			//
			++begin;
			cyy::add_param(node, "username", *begin);
			//serialize_xml(node, "username", *begin);

			//
			//	password
			//
			++begin;
			cyy::add_param(node, "password", *begin);
			//serialize_xml(node, "password", *begin);

			//
			//	parameterTreePath
			//
			++begin;
			traverse_parameter_tree_path(node, *begin);

			//
			//	parameterTree (SML_Tree)
			//
			++begin;
			traverse_sml_tree(node, *begin, 0, 0);
		}

		void sml_json_exporter::traverse_attention_response(cyy::tuple_t& node
			, cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(begin, end);
			if (count < 4)
			{
				//	missing elements
				CYY_LOG_ERROR(logger_, "SML_Attention.Res("
					<< count
					<< "4) - missing parameter");
				return;
			}

			//
			//	serverId
			//
			//cyy::add_param(node, "serverId", *begin);
			cyy::buffer_t tmp;
			emit_ctrl_address(cyy::add_child("serverId", node), cyy::value_cast(*begin, tmp));

			//
			//	attentionNo (OBIS)
			//
			++begin;
			if (cyy::type_code_test<cyy::types::CYY_BUFFER>(*begin))
			{
				//	
				//	create OBIS code
				//
				cyy::buffer_t buffer;
				buffer = cyy::value_cast(*begin, buffer);
				BOOST_ASSERT_MSG(buffer.size() == m2m::obis::size(), "invalid buffer size");

				m2m::obis object_name = m2m::obis(buffer);
				emit_object(cyy::add_child("obis", node), object_name);
				//emit_object(node.append_child("obis"), object_name);

				const boost::system::error_code ec = m2m::get_attention_code(object_name);
				boost::ignore_unused(ec);
				//	ToDo:
				//cyy::add_param(node, "msg", std::string(ec.message()));
				//node.append_attribute("msg") = ec.message().c_str();

				//			node.append_attribute("msg") = m2m::get_attention_msg(object_name);
			}

			//
			//	attentionMsg
			//
			++begin;
			cyy::add_param(node, "attentionMsg", *begin);
			//serialize_xml(node, "attentionMsg", *begin);

			//
			//	attentionDetails (SML_Tree)
			//
			++begin;
			if (cyy::primary_type_code_test<cyy::types::CYY_TUPLE>(*begin))
			{
				traverse_sml_tree(node, *begin, 0, 0);
			}
			else
			{
				//	else optional
				cyy::add_param(node, "attentionDetails", *begin);
				//serialize_xml(node, "attentionDetails", *begin);
			}
		}

		void sml_json_exporter::traverse_sml_tree(cyy::tuple_t& node
			, cyy::object obj
			, std::size_t pos
			, std::size_t depth)
		{
			//auto child = node.append_child(pugi::node_comment);
			//std::stringstream ss;
			//if (depth == 0)
			//{
			//	child.set_value("root");
			//}
			//else
			//{
			//	ss
			//		<< '['
			//		<< depth
			//		<< ']'
			//		<< '['
			//		<< (pos + 1)
			//		<< ']'
			//		;
			//	const std::string index = ss.str();
			//	child.set_value(index.c_str());
			//}

			if (cyy::primary_type_code_test<cyy::types::CYY_TUPLE>(obj))
			{
				cyy::tuple_t ptree;
				ptree = cyy::value_cast(obj, ptree);
				traverse_sml_tree(node, std::begin(ptree), std::end(ptree), pos, depth);
			}
			else
			{
				//	ToDo:
				//cyy::add_param(node, "ptree", obj);
				//serialize_xml(node, "ptree", obj);
			}
		}

		//
		//	traverse_sml_tree()
		//
		void sml_json_exporter::traverse_sml_tree(cyy::tuple_t& node
			, cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end
			, std::size_t pos
			, std::size_t depth)
		{
			std::size_t count = std::distance(begin, end);
			if (count < 3)
			{
				//	missing elements
				CYY_LOG_ERROR(logger_, "SML_Tree("
					<< count
					<< "/2) - missing parameter");
				return;
			}

			//
			//	parameterName (OBIS)
			//
			m2m::obis object_name;
			if (cyy::type_code_test<cyy::types::CYY_BUFFER>(*begin))
			{
				//	
				//	create OBIS code
				//
				cyy::buffer_t buffer;
				buffer = cyy::value_cast(*begin, buffer);
				BOOST_ASSERT_MSG(buffer.size() == m2m::obis::size(), "invalid buffer size");

				object_name = m2m::obis(buffer);
				emit_object(cyy::add_child("obis", node), object_name);
				//emit_object(node.append_child("obis"), object_name);

				const std::string class_name = m2m::get_lsm_class(object_name);
				if (!class_name.empty())
				{
					cyy::add_param(node, "class", cyy::factory(class_name));
					//node.append_attribute("class") = class_name.c_str();
				}
			}

			//
			//	parameterValue (SML_ProcParValue)
			//
			++begin;
			traverse_sml_proc_par_value(node, *begin);

			//
			//	child_List
			//
			++begin;
			if (cyy::primary_type_code_test<cyy::types::CYY_TUPLE>(*begin))
			{

				//	child list
				cyy::tuple_t ptree;
				ptree = cyy::value_cast(*begin, ptree);

				//
				//	node
				//
				//cyy::add_param(node, "type", std::string("node"));
				cyy::add_param(node, "children",cyy::index(ptree.size()));
				//node.append_attribute("type") = "node";
				//node.append_attribute("children") << ptree.size();

				//
				//	child list
				//
				traverse_child_list(node, std::begin(ptree), std::end(ptree), depth + 1);
			}
			else
			{
				//
				//	leaf
				//
				emit_value(node, "childList", *begin);
				//node.append_attribute("type") = "leaf";
				//serialize_xml(node, "childList", *begin);
			}
		}

		void sml_json_exporter::traverse_child_list(cyy::tuple_t& node
			, cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end
			, std::size_t depth)
		{
			cyy::vector_t& list = cyy::add_list("treeEntry", node);

			for (std::size_t idx = 0; begin != end; ++begin, ++idx)
			{
				//
				//	"treeEntry" must be a vector
				//
				cyy::tuple_t tpl;
				traverse_sml_tree(tpl
					, *begin
					, idx
					, depth);

				list.push_back(cyy::factory(tpl));
			}
		}


		//
		//	traverse_sml_proc_par_value()
		//
		void sml_json_exporter::traverse_sml_proc_par_value(cyy::tuple_t& node
			, cyy::object obj)
		{
			if (cyy::primary_type_code_test<cyy::types::CYY_TUPLE>(obj))
			{
				cyy::tuple_t ptree;
				ptree = cyy::value_cast(obj, ptree);
				traverse_sml_proc_par_value(cyy::add_child("SMLProcParValue", node), std::begin(ptree), std::end(ptree));
				//traverse_sml_proc_par_value(node.append_child("SMLProcParValue"), std::begin(ptree), std::end(ptree));
			}
			else
			{
				emit_value(node, "SMLProcParValue", obj);
				//serialize_xml(node, "SMLProcParValue", obj);
			}
		}

		void sml_json_exporter::traverse_sml_proc_par_value(cyy::tuple_t& node
			, cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(begin, end);
			if (count < 2)
			{
				//	missing elements
				CYY_LOG_ERROR(logger_, "SML_ProcParValue("
					<< count
					<< "/2) - missing parameter");
				return;
			}

			//
			//	choice: 1 = SMLValue, 2 = SMLPeriodEntry, 3 = SMLTupleEntry, 4 = SMLTime
			//
			std::uint32_t choice = 0;
			choice = cyy::numeric_cast(*begin, choice);

			//
			//	SML value
			//
			begin++;
			switch (choice) 
			{
			case 1:
				emit_value(node, "SMLValue", *begin);
				//serialize_xml(node, *begin);
				break;
			case 2:
				cyy::add_param(node, "address", std::string("SMLPeriodEntry"));
				//node.append_attribute("address") = "SMLPeriodEntry";
				if (cyy::primary_type_code_test<cyy::types::CYY_TUPLE>(*begin))
				{
					cyy::tuple_t entry;
					entry = cyy::value_cast<cyy::tuple_t>(*begin, entry);

					m2m::obis dummy;
					const cyy::tuple_t result = traverse_period_entry(0
						, dummy
						, std::begin(entry)
						, std::end(entry));

				}
				else
				{
					emit_value(node, "SMLPeriodEntry", *begin);
					//serialize_xml(node, *begin);
				}
				break;
			case 3:
				cyy::add_param(node, "address", std::string("SMLTupleEntry"));
				//node.append_attribute("address") = "SMLTupleEntry";
				traverse_tuple_entry(node, *begin);
				break;
			case 4:
				cyy::add_param(node, "address", std::string("SMLTime"));
				traverse_time(node, *begin, "valTime");
				break;
			default:
				CYY_LOG_ERROR(logger_, "traverse_sml_proc_par_value(invalid choice)("
					<< choice
					<< ")");
				break;
			}

		}

		void sml_json_exporter::traverse_tuple_entry(cyy::tuple_t& node
			, cyy::object obj)
		{
			if (cyy::primary_type_code_test<cyy::types::CYY_TUPLE>(obj))
			{
				cyy::tuple_t ptree;
				ptree = cyy::value_cast(obj, ptree);
				traverse_tuple_entry(cyy::add_child("SMLTupelEntry", node), std::begin(ptree), std::end(ptree));
				//traverse_tuple_entry(node.append_child("SMLTupelEntry"), std::begin(ptree), std::end(ptree));
			}
			else
			{
				//	ToDo:
				emit_value(node, "SMLTupelEntry", obj);
				//serialize_xml(node, "SMLTupelEntry", obj);
			}
		}

		void sml_json_exporter::traverse_tuple_entry(cyy::tuple_t& node
			, cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(begin, end);
			if (count < 23)
			{
				//	missing elements
				CYY_LOG_ERROR(logger_, "SML_TupelEntry("
					<< count
					<< "23) - missing parameter");
				return;
			}

			//
			//	serverId (Octet String)
			//
			//cyy::add_param(node, "serverId", *begin);
			cyy::buffer_t tmp;
			emit_ctrl_address(cyy::add_child("serverId", node), cyy::value_cast(*begin, tmp));

			//
			//secIndex SML_Time,
			//
			++begin;
			traverse_time(node, *begin, "secIndex");

			//
			//	status Unsigned64,
			//
			++begin;
			emit_value(node, "status", *begin);
			//serialize_xml(node, "status", *begin);

			//
			//	unit_pA SML_Unit,
			//
			++begin;
			emit_value(node, "unit_pA", *begin);
			//serialize_xml(node, "unit_pA", *begin);

			//
			//	scaler_pA Integer8,
			//
			++begin;
			emit_value(node, "scaler_pA", *begin);
			//serialize_xml(node, "scaler_pA", *begin);

			//
			//	value_pA Integer64,
			//
			++begin;
			emit_value(node, "value_pA", *begin);
			//serialize_xml(node, "value_pA", *begin);

			//
			//	unit_R1 SML_Unit,
			//
			++begin;
			emit_value(node, "unit_R1", *begin);
			//serialize_xml(node, "unit_R1", *begin);

			//
			//	scaler_R1 Integer8,
			//
			++begin;
			emit_value(node, "scaler_R1", *begin);
			//serialize_xml(node, "scaler_R1", *begin);

			//
			//	value_R1 Integer64,
			//
			++begin;
			emit_value(node, "value_R1", *begin);
			//serialize_xml(node, "value_R1", *begin);

			//
			//	unit_R4 SML_Unit,
			//
			++begin;
			emit_value(node, "unit_R4", *begin);
			//serialize_xml(node, "unit_R4", *begin);

			//
			//	scaler_R4 Integer8,
			//
			++begin;
			emit_value(node, "scaler_R4", *begin);
			//serialize_xml(node, "scaler_R4", *begin);

			//
			//	value_R4 Integer64,
			//
			++begin;
			emit_value(node, "value_R4", *begin);
			//serialize_xml(node, "value_R4", *begin);

			//
			//	signature_pA_R1_R4 Octet String,
			//
			++begin;
			emit_value(node, "signature_pA_R1_R4", *begin);
			//serialize_xml(node, "signature_pA_R1_R4", *begin);

			//
			//	unit_mA SML_Unit,
			//
			++begin;
			emit_value(node, "unit_mA", *begin);
			//serialize_xml(node, "unit_mA", *begin);

			//
			//	scaler_mA Integer8,
			//
			++begin;
			emit_value(node, "scaler_mA", *begin);
			//serialize_xml(node, "scaler_mA", *begin);

			//
			//	value_mA Integer64,
			//
			++begin;
			emit_value(node, "value_mA", *begin);
			//serialize_xml(node, "value_mA", *begin);

			//
			//	unit_R2 SML_Unit,
			//
			++begin;
			emit_value(node, "unit_R2", *begin);
			//serialize_xml(node, "unit_R2", *begin);

			//
			//	scaler_R2 Integer8,
			//
			++begin;
			emit_value(node, "scaler_R2", *begin);
			//serialize_xml(node, "scaler_R2", *begin);

			//
			//	value_R2 Integer64,
			//
			++begin;
			emit_value(node, "value_R2", *begin);
			//serialize_xml(node, "value_R2", *begin);

			//
			//	unit_R3 SML_Unit,
			//
			++begin;
			emit_value(node, "unit_R3", *begin);
			//serialize_xml(node, "unit_R3", *begin);

			//
			//	scaler_R3 Integer8,
			//
			++begin;
			emit_value(node, "scaler_R3", *begin);
			//serialize_xml(node, "scaler_R3", *begin);

			//
			//value_R3 Integer64,
			//
			++begin;
			emit_value(node, "value_R3", *begin);
			//serialize_xml(node, "value_R3", *begin);

			//
			//signature_mA_R2_R3 Octet String
			//
			++begin;
			emit_value(node, "signature_mA_R2_R3", *begin);
			//serialize_xml(node, "signature_mA_R2_R3", *begin);

		}


		//
		//	traverse_list_of_period_entries()
		//
		cyy::vector_t sml_json_exporter::traverse_list_of_period_entries(cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end
			, m2m::obis const& object_code)
		{
			std::size_t count = std::distance(begin, end);
			//cyy::add_param(node, "size", cyy::index_factory(count));

			//
			//	vector
			//
			cyy::vector_t vec;
			vec.reserve(count);

			//
			//	read 'count' vector elements of type "PeriodEntry"
			//

			for (std::uint32_t counter = 0; begin != end; ++begin, ++counter)
			{
				BOOST_ASSERT_MSG(cyy::primary_type_code_test<cyy::types::CYY_TUPLE>(*begin), "wrong data type - expected CYY_TUPLE in traverse_list_of_period_entries");
				cyy::tuple_t entry;
				entry = cyy::value_cast<cyy::tuple_t>(*begin, entry);

				//	element position as *comment*

				//auto comment = node.append_child(pugi::node_comment);
				//comment.set_value(std::to_string(counter + 1).c_str());

				cyy::tuple_t result = traverse_period_entry(counter
					, object_code
					, std::begin(entry)
					, std::end(entry));

				//cyy::add_param(result, "index", counter);
				vec.push_back(cyy::factory(result));

				//traverse_period_entry(node.append_child("PeriodEntry")
				//	, counter
				//	, object_code
				//	, std::begin(entry)
				//	, std::end(entry));
			}
			//node.push_back(cyy::factory(vec));
			//cyy::add_param(node, "vector", cyy::factory(vec));
			return vec;

		}

		cyy::tuple_t sml_json_exporter::traverse_period_entry(std::uint32_t counter
			, m2m::obis const& object_code
			, cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end)
		{
			cyy::tuple_t result;

			std::size_t count = std::distance(begin, end);
			if (count < 5)
			{
				//	missing elements
				//	missing elements
				CYY_LOG_ERROR(logger_, "SML_PeriodEntry("
					<< count
					<< "5) - missing parameter");
				return result;
			}

			//
			//	object name
			//
			m2m::obis object_name;
			if (cyy::type_code_test<cyy::types::CYY_BUFFER>(*begin))
			{
				//	
				//	create OBIS code
				//
				cyy::buffer_t buffer;
				buffer = cyy::value_cast(*begin, buffer);
				BOOST_ASSERT_MSG(buffer.size() == m2m::obis::size(), "invalid buffer size");

				object_name = m2m::obis(buffer);
				emit_object(cyy::add_child("obis", result), object_name);
			}
			else
			{
				emit_value(result, "obis", *begin);
				//serialize_xml(node, "obis", *begin);
			}

			//
			//	unit (see sml_unit_enum)
			//
			++begin;
			{
				//
				//	set unit code as SML node value
				//
				auto& child = cyy::add_child("SMLUnit", result);
				//auto child = serialize_xml(node, "SMLUnit", *begin);

				//
				//	add unit name
				//
				std::uint8_t unit_code = 0;
				unit_code = cyy::numeric_cast(*begin, unit_code);
				cyy::add_param(child, "code", unit_code);
				cyy::add_param(child, "unit", noddy::m2m::get_unit_name(unit_code));
			}

			//
			//	scaler
			//
			++begin;
			const auto scaler = cyy::numeric_cast<std::int32_t>(*begin);
			cyy::add_param(result, "scaler", cyy::factory(scaler));

			//
			//	value
			//	Different datatypes are possible. Check it out.
			//	"valueOctet", "valueInt", "valueString", "value"
			++begin;
			if (object_name == m2m::OBIS_READOUT_UTC)
			{
				std::time_t seconds = 0;
				seconds = cyy::numeric_cast(*begin, seconds);
				cyy::add_param(result, "roTime", cyy::time_point_factory(seconds));
				//serialize_xml(node, "roTime", cyy::time_point_factory(seconds));

			}
			else if (object_name == m2m::OBIS_ACT_SENSOR_TIME)
			{
				//	u32 seconds
				std::time_t seconds = 0;
				seconds = cyy::numeric_cast(*begin, seconds);
				cyy::add_param(result, "actSensorTime", cyy::time_point_factory(seconds));
				//serialize_xml(node, "actSensorTime", cyy::time_point_factory(seconds));
				//node.append_attribute("actSensorTime") << cyy::sql_format(std::chrono::system_clock::from_time_t(seconds));
			}
			else if (object_name == m2m::OBIS_CLASS_EVENT)
			{
				//	see 2.2.1.12 Protokollierung im MUC Betriebslogbuch
				//	Ereignis (uint32)
				std::uint32_t event = 0;
				event = cyy::numeric_cast(*begin, event);

				auto& child = cyy::add_child("event", result);
				cyy::add_param(child, "type", std::string(m2m::get_event_name(event)));

				//auto child = serialize_xml(node, "event", *begin);
				//child.append_attribute("type") = m2m::get_event_name(event);
			}
			else if (object_name == m2m::OBIS_CLASS_OP_LOG_PEER_ADDRESS)
			{
				//	peer address
				std::uint64_t source = 0;
				source = cyy::numeric_cast(*begin, source);

				auto& child = cyy::add_child("peerAddress", result);
				//auto child = serialize_xml(node, "peerAddress", *begin);

				switch (source)
				{
				case 0x818100000020:
					cyy::add_param(child, "source", std::string("LSMC"));
					//child.append_attribute("source") = "LSMC";
					break;
				case 0x818100000001:
					cyy::add_param(child, "source", std::string("OBISLOG"));
					//child.append_attribute("source") = "OBISLOG";
					break;
				default:
					break;
				}
			}
			else if (object_name == m2m::OBIS_CLASS_OP_MONITORING_STATUS)
			{
				std::uint32_t status = 0;
				status = cyy::numeric_cast(*begin, status);

				auto& child = cyy::add_child("monitoringStatus", result);
				//auto child = serialize_xml(node, "monitoringStatus", *begin);

				switch (static_cast<std::uint16_t>(status) & 0xFFFF) {
				case 0xFFFC:
					cyy::add_param(child, "status", std::string("no answer"));
					//child.append_attribute("status") = "no answer";
					break;
				case 0xFFFD:
					cyy::add_param(child, "status", std::string("OK"));
					//child.append_attribute("status") = "OK";
					break;
				case 0xFFFE:
					cyy::add_param(child, "status", std::string("Error"));
					//child.append_attribute("status") = "Error";
					break;
				case 0xFFFF:
					cyy::add_param(child, "status", std::string("monitoring deactivated"));
					//child.append_attribute("status") = "monitoring deactivated";
					break;
				default:
					cyy::add_param(child, "status", std::string("undefined status code"));
					//child.append_attribute("status") = "undefined status code";
					break;
				}

			}
			else if (object_name == m2m::OBIS_CLASS_OP_LOG)
			{
				//	add property attribute
				cyy::add_param(result, "property", std::string(m2m::get_property_name(object_name)));
				//node.append_attribute("property") = m2m::get_property_name(object_name);
			}
			else if (object_name == m2m::OBIS_SERIAL_NR)
			{
				if (cyy::get_code(*begin) == cyy::types::CYY_BUFFER)
				{
					//	
					//	create OBIS code
					//
					cyy::buffer_t buffer;
					buffer = cyy::value_cast(*begin, buffer);
					std::reverse(buffer.begin(), buffer.end());

					cyy::add_param(result, "serial-number-1", cyy::factory(cyy::buffer_format_hex_spaces(buffer)));
					//node.append_attribute("serial-number-1") << cyy::buffer_format_hex_spaces(buffer);
				}
				else
				{
					//	add property attribute
					cyy::add_param(result, "property", std::string("serial-number-1"));
					//node.append_attribute("property") = "serial-number-1";
				}
			}
			else if (object_name == m2m::OBIS_SERIAL_NR_SECOND)
			{
				if (cyy::get_code(*begin) == cyy::types::CYY_BUFFER)
				{
					//	
					//	create OBIS code
					//
					cyy::buffer_t buffer;
					buffer = cyy::value_cast(*begin, buffer);
					std::reverse(buffer.begin(), buffer.end());

					cyy::add_param(result, "serial-number-2", cyy::factory(cyy::buffer_format_hex_spaces(buffer)));
					//node.append_attribute("serial-number-2") << cyy::buffer_format_hex_spaces(buffer);
				}
				else
				{
					//	add property attribute
					cyy::add_param(result, "property", std::string("serial-number-2"));
					//node.append_attribute("property") = "serial-number-2";
				}
			}
			else if (object_name == m2m::OBIS_MBUS_STATE)
			{
				cyy::add_param(result, "property", std::string("M-Bus-state"));
				//node.append_attribute("property") = "M-Bus-state";
			}
			else if (object_name == m2m::OBIS_SERVER_ID_1_1 || object_name == m2m::OBIS_SERVER_ID_1_2 || object_name == m2m::OBIS_SERVER_ID_1_3 || object_name == m2m::OBIS_SERVER_ID_1_4)
			{
				//	Identifikationsnummer 1.1 - 1.4
				std::stringstream ss;
				ss
					<< "id-1_"
					<< (int)object_name.get_quantities()
					;
				const std::string attr = ss.str();

				ss.str("");
				ss
					<< std::setfill('0')
					<< std::setw(8)
					<< cyy::numeric_cast<std::uint32_t>(*begin, 0)
					;
				const std::string val = ss.str();
				//node.append_attribute(attr.c_str()) = val.c_str();
				cyy::add_param(result, attr, cyy::factory(val));
			}
			else
			{
				//
				//	normal value
				//
				emit_value(result, "reading", *begin);
				//serialize_xml(node, "reading", *begin);

				//	calculate data
				const std::string data = noddy::m2m::scale_value(cyy::numeric_cast<std::int64_t>(*begin), scaler);
				cyy::add_param(result, "data", cyy::factory(data));
					
				//	add calculated data
				//auto& child = cyy::add_child("value", result);
				//cyy::add_param(result, "value", cyy::factory(data));
				//auto child = node.append_child("value");
				//child.append_child(pugi::node_pcdata).set_value(data.c_str());

			}

			//
			//	valueSignature
			//
			++begin;
			emit_value(result, "signature", *begin);
			//serialize_xml(node, "signature", *begin);
			return result;
		}


		m2m::obis sml_json_exporter::traverse_parameter_tree_path(cyy::tuple_t& node, cyy::object obj)
		{
			cyy::tuple_t ptree;
			ptree = cyy::value_cast(obj, ptree);
			if (ptree.size() == 1 && cyy::type_code_test<cyy::types::CYY_BUFFER>(ptree.front()))
			{
				//	
				//	create OBIS code
				//
				cyy::buffer_t buffer;
				buffer = cyy::value_cast(ptree.front(), buffer);
				BOOST_ASSERT_MSG(buffer.size() == noddy::m2m::obis::size(), "invalid buffer size");

				const m2m::obis object_type = m2m::obis(buffer);

				auto& child = cyy::add_child("obis", node);
				emit_object(child, object_type);

				//auto child = node.append_child("obis");
				//emit_object(child, object_type);

				//const std::string value = noddy::m2m::to_string(object_type);
				//node.append_attribute("obis") << value;
				//pugi::xml_node child = node.append_child("root");
				//child.append_child(pugi::node_pcdata).set_value(value.c_str());


				//	Standard-Lastgänge
				if (object_type == m2m::OBIS_CODE_1_MINUTE)
				{
					cyy::add_param(child, "profile", std::string("1min"));
					//child.append_attribute("profile") = "1min";
				}
				else if (object_type == m2m::OBIS_CODE_15_MINUTE)
				{
					cyy::add_param(child, "profile", std::string("15min"));
					//child.append_attribute("profile") = "15min";
				}
				else if (object_type == m2m::OBIS_CODE_60_MINUTE)
				{
					cyy::add_param(child, "profile", std::string("60min"));
					//child.append_attribute("profile") = "60min";
				}
				else if (object_type == m2m::OBIS_CODE_24_HOUR)
				{
					cyy::add_param(child, "profile", std::string("24h"));
					//child.append_attribute("profile") = "24h";
				}
				else if (object_type == m2m::OBIS_CODE_LAST_2_HOURS)
				{
					cyy::add_param(child, "profile", std::string("last2h"));
					//child.append_attribute("profile") = "last2h";
				}
				else if (object_type == m2m::OBIS_CODE_LAST_WEEK)
				{
					cyy::add_param(child, "profile", std::string("lastWeek"));
					//child.append_attribute("profile") = "lastWeek";
				}
				else if (object_type == m2m::OBIS_CODE_1_MONTH)
				{
					cyy::add_param(child, "profile", std::string("1month"));
					//child.append_attribute("profile") = "1month";
				}
				else if (object_type == m2m::OBIS_CODE_1_YEAR)
				{
					cyy::add_param(child, "profile", std::string("1year"));
					//child.append_attribute("profile") = "1year";
				}
				else if (object_type == m2m::OBIS_CODE_INITIAL)
				{
					cyy::add_param(child, "profile", std::string("initial"));
					//child.append_attribute("profile") = "initial";
				}
				else if (object_type == m2m::OBIS_CLASS_OP_LOG)
				{
					cyy::add_param(child, "profile", std::string("operating log"));
					//child.append_attribute("root") = "operating log";
				}
				else if (object_type == m2m::OBIS_CLASS_EVENT)
				{
					cyy::add_param(child, "root", std::string("event"));
					//child.append_attribute("root") = "event";
				}
				else if (object_type == m2m::OBIS_CLASS_STATUS)
				{
					cyy::add_param(child, "root", std::string("LSM-status"));
					//child.append_attribute("root") = "LSM-status";
					//	see: 2.2.1.3 Status der Aktoren (Kontakte)
				}
				else if (object_type == m2m::OBIS_CODE_ROOT_NTP)
				{
					cyy::add_param(child, "root", std::string("NTP"));
					//child.append_attribute("root") = "NTP";
				}
				//else 
				//{
				//	const std::string value = noddy::m2m::to_string(object_type);
				//	pugi::xml_node child = node.append_child("root");
				//	child.append_child(pugi::node_pcdata).set_value(value.c_str());
				//}


				return object_type;
			}

			cyy::add_param(node, "parameterTreePath", obj);
			//serialize_xml(node, "parameterTreePath", obj);
			return noddy::m2m::obis();
		}

		/*
		*	SML_Time parser
		*/
		void sml_json_exporter::traverse_time(cyy::tuple_t& node
			, cyy::object obj
			, std::string const& name)
		{

			if (cyy::primary_type_code_test<cyy::types::CYY_TUPLE>(obj))
			{
				cyy::tuple_t tpl;
				tpl = cyy::value_cast<cyy::tuple_t>(obj, tpl);
				traverse_time(cyy::add_child(name, node), std::begin(tpl), std::end(tpl));
				//traverse_time(node.append_child(name.c_str()), std::begin(tpl), std::end(tpl));
			}
			else
			{
				//	error handling
				CYY_LOG_ERROR(logger_, "SML-Time (CYY_TUPLE) expected: "
					<< cyy::to_literal(obj));
			}
		}

		/*
		*	SML_Time parser
		*/
		void sml_json_exporter::traverse_time(cyy::tuple_t& node
			, cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end)
		{

			std::size_t count = std::distance(begin, end);

			if (count < 2)
			{
				//	missing elements
				CYY_LOG_ERROR(logger_, "SML_Time("
					<< count
					<< "2) - missing parameter");
				return;
			}

			//	choice: 1 = secIndex, 2 = timestamp
			std::uint32_t choice = 0;
			choice = cyy::numeric_cast(*begin, choice);

			//
			//	timestamp value
			//
			++begin;

			//	second index
			const std::uint32_t secindex = cyy::numeric_cast<std::uint32_t>(*begin, 0);
			switch (choice)
			{
			case sml::TIME_TIMESTAMP:
				cyy::add_param(node, "timeStamp", cyy::time_point_factory(secindex));
				//serialize_xml(node, cyy::time_point_factory(secindex));
				break;
			case sml::TIME_SECINDEX:
			default:
				cyy::add_param(node, "secindex", secindex);
				//serialize_xml(node, *begin);
				break;
			}
		}

		void sml_json_exporter::emit_object(cyy::tuple_t& node
			, m2m::obis const& object_code)
		{
			if (object_code.is_private())
			{
				cyy::add_param(node, "class", std::string("private"));
				//node.append_attribute("class") = "private";
			}
			else if (object_code.is_abstract())
			{
				cyy::add_param(node, "class", std::string("abstract"));
				//node.append_attribute("class") = "abstract";
			}
			else
			{
				cyy::add_param(node, "class", object_code.get_medium_name());
				//node.append_attribute("class") = object_code.get_medium_name();
			}

			cyy::add_param(node, "hex", m2m::to_hex(object_code));
			cyy::add_param(node, "value", m2m::to_string(object_code));
		}

		void sml_json_exporter::emit_ctrl_address(cyy::tuple_t& node
			, cyy::buffer_t const& buffer)
		{
			cyy::add_param(node, "value", cyy::factory(buffer));

			if (cyy::io::is_printable(buffer))
			{
				cyy::add_param(node, "ascii", cyy::buffer_format_ascii(buffer));
			}
			else
			{
				cyy::add_param(node, "hex", cyy::buffer_format_hex(buffer));
			}

			//
			//	Build a controller address.
			//	If only a serial number is available an electric meter
			//	is assumed.
			//
			noddy::m2m::ctrl_address ca(buffer);
			cyy::add_param(node, "addressType", ca.get_address_type());
			//node.append_attribute("addressType") = ca.get_address_type();

			if (ca.is_mbus_address())
			{
				noddy::m2m::mbus_address ma(ca);
				cyy::add_param(node, "medium", ma.get_medium_name_long());
				//node.append_attribute("medium") << ma.get_medium_name_long();

				//	ToDo: add full address as string
			}
		}

		void sml_json_exporter::emit_value(cyy::tuple_t& node
			, std::string const& name
			, cyy::object const& obj)
		{
			//cyy::add_param(node, "address", name);
			const auto code = cyy::get_code(obj);
			cyy::tuple_t& child = cyy::add_child(name, node);
			cyy::add_param(child, "code", code);

			switch (code)
			{
			case cyy::types::CYY_NULL:		
				cyy::add_param(child, "type", std::string("null"));
				break;
			case cyy::types::CYY_BOOL:
				cyy::add_param(child, "type", std::string("bool"));
				cyy::add_param(child, "value", cyy::value_cast<bool>(obj));
				break;

			case cyy::types::CYY_INT8:
				cyy::add_param(child, "type", std::string("i8"));
				cyy::add_param(child, "value", cyy::value_cast<std::int8_t>(obj));
				break;
			case cyy::types::CYY_UINT8:
				cyy::add_param(child, "type", std::string("u8"));
				cyy::add_param(child, "value", cyy::value_cast<std::uint8_t>(obj));
				break;
			case cyy::types::CYY_INT16:
				cyy::add_param(child, "type", std::string("i16"));
				cyy::add_param(child, "value", cyy::value_cast<std::int16_t>(obj));
				break;
			case cyy::types::CYY_UINT16:
				cyy::add_param(child, "type", std::string("u16"));
				cyy::add_param(child, "value", cyy::value_cast<std::uint16_t>(obj));
				break;
			case cyy::types::CYY_INT32:
				cyy::add_param(child, "type", std::string("i32"));
				cyy::add_param(child, "value", cyy::value_cast<std::int32_t>(obj));
				break;
			case cyy::types::CYY_UINT32:
				cyy::add_param(child, "type", std::string("u32"));
				cyy::add_param(child, "value", cyy::value_cast<std::uint32_t>(obj));
				break;
			case cyy::types::CYY_INT64:
				cyy::add_param(child, "type", std::string("i64"));
				cyy::add_param(child, "value", cyy::value_cast<std::int64_t>(obj));
				break;
			case cyy::types::CYY_UINT64:
				cyy::add_param(child, "type", std::string("u64"));
				cyy::add_param(child, "value", cyy::value_cast<std::uint64_t>(obj));
				break;
			case cyy::types::CYY_STRING:
				cyy::add_param(child, "type", std::string("str"));
				cyy::add_param(child, "value", cyy::to_string(obj, m2m::json_io_callback));
				break;

			case cyy::types::CYY_TIME_STAMP:
				cyy::add_param(child, "type", std::string("timeStamp"));
				cyy::add_param(child, "value", cyy::to_string(obj, m2m::json_io_callback));
				break;
			case cyy::types::CYY_BUFFER:
				cyy::add_param(child, "type", std::string("binary"));
				{
					cyy::buffer_t tmp;
					const auto& buffer = cyy::value_cast(obj, tmp);
					if (cyy::io::is_hexdigit(buffer))
					{
						tmp = cyy::to_hex(buffer);
						cyy::add_param(child, "value", std::string(tmp.begin(), tmp.end()));
					}
					else
					{
						cyy::add_param(child, "value", cyy::to_string(obj, m2m::json_io_callback));
					}
				}
				break;
			default:
				cyy::add_param(child, "type", std::string("other"));
				cyy::add_param(child, "value", cyy::to_string(obj, m2m::json_io_callback));
				break;
			}
		}

	}	//	sml
}	

