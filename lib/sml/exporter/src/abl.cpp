/*
* Copyright Sylko Olzscher 2016
*
* Use, modification, and distribution is subject to the Boost Software
* License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
* http://www.boost.org/LICENSE_1_0.txt)
*/

#include <noddy/sml/exporter/sml_abl_exporter.h>
#include <noddy/m2m/obis_codes.h>
#include <noddy/m2m/unit_codes.h>
#include <noddy/m2m/numeric.h>
#include <noddy/m2m/intrinsics/io/formatter/obis_format.h>
#include <noddy/m2m/intrinsics/io/formatter/ctrl_address_format.h>
#include <cyy/xml/xml_io.h>
#include <cyy/xml/xml_manip.h>
#include <cyy/value_cast.hpp>
#include <cyy/numeric_cast.hpp>
#include <cyy/intrinsics/factory/chrono_factory.h>
#include <cyy/intrinsics/factory/null_factory.h>
#include <cyy/io/format/buffer_format.h>
#include <cyy/io/format/time_format.h>
#include <boost/core/ignore_unused.hpp>
#include <iostream>
#include <fstream> 


namespace noddy
{
	namespace sml	
	{

		sml_abl_exporter::sml_abl_exporter(std::string const& dir, std::string const& prefix)
			: out_dir_(dir)
			, prefix_(prefix)
			, trx_()
			, client_id_()
			, server_id_()
			, choice_(sml::BODY_UNKNOWN)
		{
			//	test
			// 		cyy::buffer_t buffer;
			// 		buffer.push_back(0x30);
			// 		buffer.push_back(0x32);
			// 		buffer.push_back(0x34);
			// 		buffer.push_back(0x36);
			// 		buffer.push_back(0x37);
			// 		buffer.push_back(0x34);
			// 		buffer.push_back(0x38);
			// 		buffer.push_back(0x36);
			// 		sml::ctrl_address test(buffer);
			// 		std::cout << test.get_address_type() << std::endl;
			// 		if (test.is_mbus_address())
			// 		{
			// 			sml::mbus_address ma(test);
			// 			std::cout << ma.get_serial_number() << ", " << ma.get_manufacturer_name() << std::endl; 
			// 		}

			reset();
		}

		std::string const& sml_abl_exporter::get_last_trx() const
		{
			return trx_;
		}

		cyy::buffer_t const& sml_abl_exporter::get_last_client_id() const
		{
			return client_id_;
		}

		cyy::buffer_t const& sml_abl_exporter::get_last_server_id_() const
		{
			return server_id_;
		}

		std::uint32_t sml_abl_exporter::get_last_choice() const
		{
			return choice_;
		}

		void sml_abl_exporter::extract(cyy::object const& obj, std::size_t msg_index)
		{
			BOOST_ASSERT_MSG(cyy::primary_type_code_test< cyy::types::CYY_TUPLE >(obj), "tuple type expected");
			const cyy::tuple_t def;
			extract(cyy::value_cast<cyy::tuple_t>(obj, def), msg_index);
		}

		void sml_abl_exporter::extract(cyy::tuple_t const& tpl, std::size_t msg_index)
		{
			traverse(std::begin(tpl), std::end(tpl), msg_index);
		}

		void sml_abl_exporter::reset()
		{
			trx_.clear();
			client_id_.clear();
			server_id_.clear();
			choice_ = sml::BODY_UNKNOWN;
		}

		//
		//	traverse()
		//
		void sml_abl_exporter::traverse(cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end
			, std::size_t msg_index)
		{
			std::size_t count = std::distance(begin, end);

			if (count < 5)
			{
				//	missing elements
				//	error message as comment
				//auto child = root_.append_child(pugi::node_comment);
				//child.set_value("***ERROR: SML_Message(5) - missing parameter");
				return;
			}

			//auto msg = root_.append_child("message");
			//msg.append_attribute("index") = (unsigned)msg_index;

			//
			//	(1) - transaction id
			//	This is typically a printable sequence of 6 up to 9 ASCII values
			//
			BOOST_ASSERT_MSG(cyy::type_code_test<cyy::types::CYY_BUFFER>(*begin), "wrong data type - expected CYY_BUFFER as transaction id");
//			serialize_xml(msg.append_child("trx"), *begin);
			
			//	store transaction name
			trx_ = cyy::buffer_format_ascii(cyy::value_cast<cyy::buffer_t>(*begin));


			//
			//	(2) groupNo
			//
			++begin;
			BOOST_ASSERT_MSG(cyy::type_code_test<cyy::types::CYY_UINT8>(*begin), "wrong data type - expected CYY_UINT8 as groupNo");
//			serialize_xml(msg.append_child("groupNo"), *begin);

			//
			//	(3) abortOnError
			//
			++begin;
			BOOST_ASSERT_MSG(cyy::type_code_test<cyy::types::CYY_UINT8>(*begin), "wrong data type - expected CYY_UINT8 as abortOnError");
			//{
			//	auto child = msg.append_child("abortOnError");
			//	serialize_xml(child, *begin);
			//}

			//
			//	choice
			//
			++begin;
			BOOST_ASSERT_MSG(cyy::primary_type_code_test<cyy::types::CYY_TUPLE>(*begin), "wrong data type - expected CYY_TUPLE as choice");
			cyy::tuple_t choice;
			choice = cyy::value_cast<cyy::tuple_t>(*begin, choice);
			BOOST_ASSERT_MSG(choice.size() == 2, "wrong tuple size");
			traverse_body(std::begin(choice), std::end(choice));

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
				boost::ignore_unused(crc16);
#endif

				//auto child = msg.append_child("crc16");
				//serialize_xml(child, *begin);
			}

			//	message complete
		}

		//
		//	traverse_body()
		//
		void sml_abl_exporter::traverse_body(cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(begin, end);

			if (count < 2)
			{
				//	missing elements
				//	error message as comment
				//auto child = node.append_child(pugi::node_comment);
				//child.set_value("***ERROR: SML_Choice(2) - missing parameter");
				return;
			}

			//
			//	detect message type
			//
			const cyy::object obj = *begin;
//			std::uint32_t choice = 0;
			choice_ = cyy::numeric_cast(*begin, choice_);

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
				//	node.append_child("bodyOpenRequest"), 
				traverse_open_request(std::begin(body), std::end(body));
				break;

			case sml::BODY_OPEN_RESPONSE:
				//	node.append_child("bodyOpenResponse"), 
				traverse_open_response(std::begin(body), std::end(body));
				break;

			case sml::BODY_CLOSE_REQUEST:
				//	node.append_child("bodyCloseRequest"), 
//				serialize_xml(*begin);
				break;

			case sml::BODY_CLOSE_RESPONSE:
				//	node.append_child("bodyCloseResponse"), 
				traverse_close_response(std::begin(body), std::end(body));
				break;

			case sml::BODY_GET_PROFILE_PACK_REQUEST:
				//	node.append_child("bodyGetProfilePackRequest"), 
				traverse_get_profile_pack_request(std::begin(body), std::end(body));
				break;

			case sml::BODY_GET_PROFILE_PACK_RESPONSE:
				//	node.append_child("bodyGetProfilePackResponse"), 
				//serialize_xml(*begin);
				break;

			case sml::BODY_GET_PROFILE_LIST_REQUEST:
				//	node.append_child("bodyGetProfileListRequest"), 
				traverse_get_profile_list_request(std::begin(body), std::end(body));
				break;

			case sml::BODY_GET_PROFILE_LIST_RESPONSE:
				//	node.append_child("bodyGetProfileListResponse"), 
				//	
				//	This is the only really supported SML message for ABL files
				//
				traverse_get_profile_list_response(std::begin(body), std::end(body));
				break;

			case sml::BODY_GET_PROC_PARAMETER_REQUEST:
				//	node.append_child("bodyGetProcParameterRequest"), 
				traverse_get_process_parameter_request(std::begin(body), std::end(body));
				break;

			case sml::BODY_GET_PROC_PARAMETER_RESPONSE:
				//	node.append_child("bodyGetProcParameterResponse"), 
				traverse_get_process_parameter_response(std::begin(body), std::end(body));
				break;

			case sml::BODY_SET_PROC_PARAMETER_REQUEST:
				//	node.append_child("bodySetProcParameterRequest"), 
				traverse_set_process_parameter_request(std::begin(body), std::end(body));
				break;

			case sml::BODY_SET_PROC_PARAMETER_RESPONSE:
				//	node.append_child("bodySetProcParameterResponse"), 
				//	serialize_xml(*begin);
				break;

			case sml::BODY_GET_LIST_REQUEST:
				//serialize_xml(node.append_child("bodyListRequest"), *begin);
				break;

			case sml::BODY_GET_LIST_RESPONSE:
				//serialize_xml(node.append_child("bodyListResponse"), *begin);
				break;

			case sml::BODY_ATTENTION_RESPONSE:
				//	node.append_child("bodyAttentionResponse"), 
				traverse_attention_response(std::begin(body), std::end(body));
				break;

			case sml::BODY_UNKNOWN:
			default:
//				serialize_xml(node.append_child("choice"), *begin);
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
		void sml_abl_exporter::traverse_open_response(cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(begin, end);
			if (count < 6)
			{
				//	missing elements
				//	error message as comment
				//auto child = node.append_child(pugi::node_comment);
				//child.set_value("***ERROR: SML_PublicOpen.Res(6) - missing parameter");
				return;
			}

			//	codepage "ISO 8859-15"
			//serialize_xml(node.append_child("codepage"), *begin);

			//
			//	clientId (MAC)
			//	Typically 7 bytes to identify gateway/MUC
			++begin;
			//serialize_xml(node.append_child("clientId"), *begin);

			//
			//	reqFileId
			//	This is typically a printable sequence of 6 up to 9 ASCII values
			//
			++begin;
			//serialize_xml(node.append_child("reqFileId"), *begin);

			//
			//	serverId
			//	Typically 9 bytes to identify meter/sensor. 
			//	Same as clientId when receiving a LSM-Status.
			//
			++begin;
			//serialize_xml(node.append_child("serverId"), *begin);

			//
			//	refTime
			//
			++begin;
			//serialize_xml(node.append_child("refTime"), *begin);

			//	sml-Version: default = 1
			++begin;
			//serialize_xml(node.append_child("SMLVersion"), *begin);

		}

		//
		//	traverse_open_request()
		//
		void sml_abl_exporter::traverse_open_request(cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(begin, end);
			if (count < 7)
			{
				//	missing elements
				//	error message as comment
				//auto child = node.append_child(pugi::node_comment);
				//child.set_value("***ERROR: SML_PublicOpen.Req(7) - missing parameter");
				return;
			}

			//	codepage "ISO 8859-15"
			//serialize_xml(node.append_child("codepage"), *begin);

			//
			//	clientId (MAC)
			//	typically 7 bytes to identify gateway/MUC
			++begin;
			//serialize_xml(node.append_child("clientId"), *begin);
			client_id_ = cyy::value_cast(*begin, client_id_);

			//
			//	reqFileId
			//
			++begin;
			//serialize_xml(node.append_child("reqFileId"), *begin);

			//
			//	serverId
			//
			++begin;
			//serialize_xml(node.append_child("serverId"), *begin);
			server_id_ = cyy::value_cast(*begin, server_id_);

			//
			//	username
			//
			++begin;
			//serialize_xml(node.append_child("username"), *begin);

			//
			//	password
			//
			++begin;
			//serialize_xml(node.append_child("password"), *begin);

			//
			//	sml-Version: default = 1
			//
			++begin;
			//serialize_xml(node.append_child("SMLVersion"), *begin);

		}

		//
		//	traverse_close_response()
		//
		void sml_abl_exporter::traverse_close_response(cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(begin, end);
			if (count < 1)
			{
				//	missing elements
				//	error message as comment
				//auto child = node.append_child(pugi::node_comment);
				//child.set_value("***ERROR: SML_PublicClose.Res(1) - missing parameter");
				return;
			}

			//	globalSignature
			//serialize_xml(node.append_child("globalSignature"), *begin);

		}

		//
		//	traverse_get_profile_list_response()
		//	This is the only really supported SML message for ABL files
		//
		void sml_abl_exporter::traverse_get_profile_list_response(cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(begin, end);

			if (count < 9)
			{
				//	missing elements
				//	error message as comment
				//auto child = node.append_child(pugi::node_comment);
				//child.set_value("***ERROR: SML_GetProfileList.Res(9) - missing parameter");
				return;
			}

			//
			//	serverId
			//
			noddy::m2m::ctrl_address address;
			if(cyy::type_code_test<cyy::types::CYY_BUFFER>(*begin))
			{
				cyy::buffer_t server_id;
				server_id = cyy::value_cast(*begin, server_id);

				//
				//	build a controller address
				//	<bodyGetProfileListResponse profile = "15min" medium = "water" addressType = "wireless M-Bus">
				//
				address = noddy::m2m::ctrl_address(server_id);
			}

			//
			//	actTime
			//	<actTime size="8" type="ts">2016.08.10 15:01:47.00000000</actTime>
			//
			++begin;
			// "actTime"
			const auto act_time = traverse_time(*begin);

			//
			//	regPeriod
			//	<regPeriod size="1" type="ui8">0</regPeriod>
			//
			++begin;
			//serialize_xml(node.append_child("regPeriod"), *begin);

			//
			//	parameterTreePath (OBIS)
			//
			++begin;
			const m2m::obis obj_code = traverse_parameter_tree_path(*begin);

			//
			//	valTime
			//	<valTime size="4" type="u32">51901679</valTime>
			//
			++begin;
			//	"valTime"
			const auto val_time = traverse_time(*begin);

			//
			//	status
			//	<status size="1" type="ui8">0</status>
			//
			++begin;

			//	period-List (1)
			//	first we make the TSMLGetProfileListResponse complete, then 
			//	the we store the entries in TSMLPeriodEntry.
			++begin;
			if(cyy::primary_type_code_test<cyy::types::CYY_TUPLE>(*begin))
			{
				const boost::filesystem::path p = unique_file_name(cyy::value_cast<std::chrono::system_clock::time_point>(act_time), address);

				//	remove file if already existing
				if (boost::filesystem::exists(p) && boost::filesystem::is_regular_file(p)) 
				{
					boost::filesystem::remove(p);
				}

				std::ofstream file(p.string(), std::ios::out | std::ios::trunc);
				if (file.is_open())
				{

					if (address.is_mbus_address())
					{
						const std::time_t act_tt = std::chrono::system_clock::to_time_t(cyy::value_cast<std::chrono::system_clock::time_point>(act_time));
						std::tm act_utc;
						cyy::chrono::convert(act_utc, act_tt);

						noddy::m2m::mbus_address ma(address);
						file
							<< "[HEADER]"
							<< "\nPROT = 0"
							<< "\nMAN1 = "
							<< ma.get_manufacturer_code()
							<< "\nZNR1 = "
							<< ma.get_serial_number()
							<< "\nDATE = "
							<< std::setfill('0')
							<< std::setw(2)
							<< cyy::chrono::day(act_utc)
							<< "."
							<< std::setw(2)
							<< cyy::chrono::month(act_utc)
							<< "."
							<< (cyy::chrono::year(act_utc) - 2000)
							<< " UTC"
							<< "\nTIME = "
							<< std::setw(2)
							<< cyy::chrono::hour(act_utc)
							<< "."
							<< std::setw(2)
							<< cyy::chrono::minute(act_utc)
							<< "."
							<< std::setw(2)
							<< cyy::chrono::second(act_utc)
							<< " UTC"
							<< "\n"
							<< "\n[DATA]"
							<< "\n"
							//	Repeat manufacturer and serial number.
							<< ma.get_manufacturer_code()
							<< "\n0-0:96.1.0*255("
							<< ma.get_serial_number()
							<< ")"
							;
					}
					else
					{
						file
							<< "[HEADER]"
							<< "\nERROR = not an M-BUS device"
							;

					}

					cyy::tuple_t periods;
					periods = cyy::value_cast<cyy::tuple_t>(*begin, periods);
					//	node.append_child("ListOfSMLPeriodEntries")
					traverse_list_of_period_entries(file
						, std::begin(periods)
						, std::end(periods)
						, obj_code);

					file.close();
				}
				else
				{
					std::cerr
						<< "***ERROR: unable to open file "
						<< p
						<< std::endl
						;
				}
			}

			++begin;	//	rawdata
			//serialize_xml(node.append_child("rawdata"), *begin);

			++begin;	//	periodSignature
			//serialize_xml(node.append_child("signature"), *begin);

		}

		//
		//	traverse_get_process_parameter_response()
		//
		void sml_abl_exporter::traverse_get_process_parameter_response(cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(begin, end);
			if (count < 3)
			{
				//	missing elements
				//	error message as comment
				//auto child = node.append_child(pugi::node_comment);
				//child.set_value("***ERROR: SML_GetProcParameter.Res(3) - missing parameter");
				return;
			}

			//
			//	serverId
			//
			//serialize_xml(node.append_child("serverId"), *begin);

			//
			//	parameterTreePath (OBIS)
			//
			++begin;
			const m2m::obis obj_code = traverse_parameter_tree_path(*begin);
			boost::ignore_unused(obj_code);

			//
			//	parameterTree
			//
			++begin;
			//	node.append_child("parameterTree"), 
			traverse_sml_tree(*begin, 0, 0);

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
		void sml_abl_exporter::traverse_get_profile_pack_request(cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(begin, end);
			if (count < 9)
			{
				//	missing elements
				//	error message as comment
				//auto child = node.append_child(pugi::node_comment);
				//child.set_value("***ERROR: SML_GetProfilePack.Req(9) - missing parameter");
				return;
			}

			//
			//	serverId
			//
			//serialize_xml(node.append_child("serverId"), *begin);

			//
			//	username
			//
			++begin;
			//serialize_xml(node.append_child("username"), *begin);

			//
			//	password
			//
			++begin;
			//serialize_xml(node.append_child("password"), *begin);

			//
			//	withRawData (boolean)
			//
			++begin;
			//serialize_xml(node.append_child("withRawData"), *begin);

			//
			//	beginTime
			//
			++begin;
			//	"beginTime
			traverse_time(*begin);

			//
			//	endTime
			//
			++begin;
			//	"endTime"
			traverse_time(*begin);

			//
			//	parameterTreePath
			//
			++begin;
			traverse_parameter_tree_path(*begin);

			//
			//	objectList (List_of_SML_ObjReqEntry)
			//
			++begin;
			//serialize_xml(node.append_child("objectList"), *begin);

			//
			//	dasDetails (SML_Tree)
			//
			++begin;
			traverse_sml_tree(*begin, 0, 0);

		}

		void sml_abl_exporter::traverse_get_profile_list_request(cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(begin, end);
			if (count < 9)
			{
				//	missing elements
				//	error message as comment
				//auto child = node.append_child(pugi::node_comment);
				//child.set_value("***ERROR: SML_GetProfileList.Req(9) - missing parameter");
				return;
			}

			//
			//	serverId
			//
			//serialize_xml(node.append_child("serverId"), *begin);

			//
			//	username
			//
			++begin;
			//serialize_xml(node.append_child("username"), *begin);

			//
			//	password
			//
			++begin;
			//serialize_xml(node.append_child("password"), *begin);

			//
			//	withRawData (boolean)
			//
			++begin;
			//serialize_xml(node.append_child("withRawData"), *begin);

			//
			//	beginTime
			//
			++begin;
			//	"beginTime"
			traverse_time(*begin);

			//
			//	endTime
			//
			++begin;
			//	"endTime"
			traverse_time(*begin);

			//
			//	parameterTreePath
			//
			++begin;
			traverse_parameter_tree_path(*begin);

			//
			//	objectList (List_of_SML_ObjReqEntry)
			//
			++begin;
			//serialize_xml(node.append_child("objectList"), *begin);

			//
			//	dasDetails (SML_Tree)
			//
			++begin;
			traverse_sml_tree(*begin, 0, 0);

		}

		void sml_abl_exporter::traverse_get_process_parameter_request(cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(begin, end);
			if (count < 5)
			{
				//	missing elements
				//	error message as comment
				//auto child = node.append_child(pugi::node_comment);
				//child.set_value("***ERROR: SSML_GetProcParameter.Req(5) - missing parameter");
				return;
			}

			//
			//	serverId
			//
			//serialize_xml(node.append_child("serverId"), *begin);

			//
			//	username
			//
			++begin;
			//serialize_xml(node.append_child("username"), *begin);

			//
			//	password
			//
			++begin;
			//serialize_xml(node.append_child("password"), *begin);

			//
			//	parameterTreePath
			//
			++begin;
			traverse_parameter_tree_path(*begin);

			//
			//	attribute
			//
			++begin;
			//serialize_xml(node.append_child("attribute"), *begin);
		}

		void sml_abl_exporter::traverse_set_process_parameter_request(cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(begin, end);
			if (count < 5)
			{
				//	missing elements
				//	error message as comment
				//auto child = node.append_child(pugi::node_comment);
				//child.set_value("***ERROR: SML_SetProcParameter.Req(5) - missing parameter");
				return;
			}

			//
			//	serverId
			//
			//serialize_xml(node.append_child("serverId"), *begin);

			//
			//	username
			//
			++begin;
			//serialize_xml(node.append_child("username"), *begin);

			//
			//	password
			//
			++begin;
			//serialize_xml(node.append_child("password"), *begin);

			//
			//	parameterTreePath
			//
			++begin;
			traverse_parameter_tree_path(*begin);

			//
			//	parameterTree (SML_Tree)
			//
			++begin;
			traverse_sml_tree(*begin, 0, 0);
		}

		void sml_abl_exporter::traverse_attention_response(cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(begin, end);
			if (count < 4)
			{
				//	missing elements
				//	error message as comment
				//auto child = node.append_child(pugi::node_comment);
				//child.set_value("***ERROR: SML_Attention.Res(4) - missing parameter");
				return;
			}

			//
			//	serverId
			//
			//serialize_xml(node.append_child("serverId"), *begin);

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
				//	node.append_child("objName"), 
				emit_object(object_name);

				const boost::system::error_code ec = m2m::get_attention_code(object_name);
                boost::ignore_unused(ec);     
				//node.append_attribute("msg") = ec.message().c_str();

			}

			//
			//	attentionMsg
			//
			++begin;
			//serialize_xml(node.append_child("attentionMsg"), *begin);

			//
			//	attentionDetails (SML_Tree)
			//
			++begin;
			if (cyy::primary_type_code_test<cyy::types::CYY_TUPLE>(*begin))
			{
				traverse_sml_tree(*begin, 0, 0);
			}
			else
			{
				//	else optional
				//serialize_xml(node.append_child("attentionDetails"), *begin);
			}
		}

		void sml_abl_exporter::traverse_sml_tree(cyy::object obj
			, std::size_t pos
			, std::size_t depth)
		{
			//auto child = node.append_child(pugi::node_comment);
			std::stringstream ss;
			if (depth == 0)
			{
				//child.set_value("root");
			}
			else
			{
				ss
					<< '['
					<< depth
					<< ']'
					<< '['
					<< (pos + 1)
					<< ']'
					;
				const std::string index = ss.str();
				//child.set_value(index.c_str());
			}

			if (cyy::primary_type_code_test<cyy::types::CYY_TUPLE>(obj))
			{
				cyy::tuple_t ptree;
				ptree = cyy::value_cast(obj, ptree);
				traverse_sml_tree(std::begin(ptree), std::end(ptree), pos, depth);
			}
			else
			{
				//serialize_xml(node.append_child("ptree"), obj);
			}
		}

		//
		//	traverse_sml_tree()
		//
		void sml_abl_exporter::traverse_sml_tree(cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end
			, std::size_t pos
			, std::size_t depth)
		{
			std::size_t count = std::distance(begin, end);
			if (count < 3)
			{
				//	missing elements
				//	error message as comment
				//auto child = node.append_child(pugi::node_comment);
				//child.set_value("***ERROR: SML_Tree(2) - missing parameter");
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
				//	node.append_child("objName"), 
				emit_object(object_name);

				const std::string class_name = m2m::get_lsm_class(object_name);
				if (!class_name.empty())
				{
					//node.append_attribute("class") = class_name.c_str();
				}
			}

			//
			//	parameterValue (SML_ProcParValue)
			//
			++begin;
			traverse_sml_proc_par_value(*begin);

			//
			//	child_List
			//
			++begin;
			if (cyy::primary_type_code_test<cyy::types::CYY_TUPLE>(*begin))
			{

				//	child list
				cyy::tuple_t ptree;
				ptree = cyy::value_cast(*begin, ptree);

				//node.append_attribute("type") = "node";
				//node.append_attribute("children") << ptree.size();

				//
				//	child list
				//
				traverse_child_list(std::begin(ptree), std::end(ptree), depth + 1);
			}
			else
			{
				//node.append_attribute("type") = "leaf";
				//serialize_xml(node.append_child("childList"), *begin);
			}
		}

		void sml_abl_exporter::traverse_child_list(cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end
			, std::size_t depth)
		{
			for (std::size_t idx = 0; begin != end; ++begin, ++idx)
			{
				//	node.append_child("treeEntry")
				traverse_sml_tree(*begin
					, idx
					, depth);
			}
		}


		//
		//	traverse_sml_proc_par_value()
		//
		void sml_abl_exporter::traverse_sml_proc_par_value(cyy::object obj)
		{
			if (cyy::primary_type_code_test<cyy::types::CYY_TUPLE>(obj))
			{
				cyy::tuple_t ptree;
				ptree = cyy::value_cast(obj, ptree);
				//	node.append_child("SMLProcParValue"), 
				traverse_sml_proc_par_value(std::begin(ptree), std::end(ptree));
			}
			else
			{
				//serialize_xml(node.append_child("SMLProcParValue"), obj);
			}
		}

		void sml_abl_exporter::traverse_sml_proc_par_value(cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(begin, end);
			if (count < 2)
			{
				//	missing elements
				//	error message as comment
				//auto child = node.append_child(pugi::node_comment);
				//child.set_value("***ERROR: SML_ProcParValue(2) - missing parameter");
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
			switch (choice) {
			case 1:
				//node.append_attribute("address") = "SMLValue";
				//serialize_xml(node, *begin);
				break;
			case 2:
				//node.append_attribute("address") = "SMLPeriodEntry";
				if (cyy::primary_type_code_test<cyy::types::CYY_TUPLE>(*begin))
				{
					cyy::tuple_t entry;
					entry = cyy::value_cast<cyy::tuple_t>(*begin, entry);

					m2m::obis dummy;
					//	ToDo: write to file
					//traverse_period_entry(0
					//	, dummy
					//	, std::begin(entry)
					//	, std::end(entry));
				}
				else
				{
					//serialize_xml(node, *begin);
				}
				break;
			case 3:
				//node.append_attribute("address") = "SMLTupleEntry";
				traverse_tuple_entry(*begin);
				break;
			case 4:
				//node.append_attribute("address") = "SMLTime";
				//	"valTime"
				traverse_time(*begin);
				break;
			default:
			{
				//auto child = node.append_child(pugi::node_comment);
				//child.set_value("***ERROR: traverse_sml_proc_par_value(invalid choice)");
				//serialize_xml(node.append_child("dump"), *begin);

			}
			break;
			}

		}

		void sml_abl_exporter::traverse_tuple_entry(cyy::object obj)
		{
			if (cyy::primary_type_code_test<cyy::types::CYY_TUPLE>(obj))
			{
				cyy::tuple_t ptree;
				ptree = cyy::value_cast(obj, ptree);
				//	node.append_child("SMLTupelEntry"), 
				traverse_tuple_entry(std::begin(ptree), std::end(ptree));
			}
			else
			{
				//serialize_xml(node.append_child("SMLTupelEntry"), obj);
			}
		}

		void sml_abl_exporter::traverse_tuple_entry(cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(begin, end);
			if (count < 23)
			{
				//	missing elements
				//	error message as comment
				//auto child = node.append_child(pugi::node_comment);
				//child.set_value("***ERROR: SML_TupelEntry(23) - missing parameter");
				return;
			}

			//
			//	serverId (Octet String)
			//
			//serialize_xml(node.append_child("serverId"), *begin);

			//
			//secIndex SML_Time,
			//	"secIndex"
			//
			traverse_time(*begin);

			//
			//status Unsigned64,
			//
			//serialize_xml(node.append_child("status"), *begin);

			//
			//unit_pA SML_Unit,
			//
			//serialize_xml(node.append_child("unit_pA"), *begin);

			//
			//scaler_pA Integer8,
			//
			//serialize_xml(node.append_child("scaler_pA"), *begin);

			//
			//value_pA Integer64,
			//
			//serialize_xml(node.append_child("value_pA"), *begin);

			//
			//unit_R1 SML_Unit,
			//
			//serialize_xml(node.append_child("unit_R1"), *begin);

			//
			//scaler_R1 Integer8,
			//
			//serialize_xml(node.append_child("scaler_R1"), *begin);

			//
			//value_R1 Integer64,
			//
			//serialize_xml(node.append_child("value_R1"), *begin);

			//
			//unit_R4 SML_Unit,
			//
			//serialize_xml(node.append_child("unit_R4"), *begin);

			//
			//scaler_R4 Integer8,
			//
			//serialize_xml(node.append_child("scaler_R4"), *begin);

			//
			//value_R4 Integer64,
			//
			//serialize_xml(node.append_child("value_R4"), *begin);

			//
			//signature_pA_R1_R4 Octet String,
			//
			//serialize_xml(node.append_child("signature_pA_R1_R4"), *begin);

			//
			//unit_mA SML_Unit,
			//
			//serialize_xml(node.append_child("unit_mA"), *begin);

			//
			//scaler_mA Integer8,
			//
			//serialize_xml(node.append_child("scaler_mA"), *begin);

			//
			//value_mA Integer64,
			//
			//serialize_xml(node.append_child("value_mA"), *begin);

			//
			//unit_R2 SML_Unit,
			//
			//serialize_xml(node.append_child("unit_R2"), *begin);

			//
			//scaler_R2 Integer8,
			//
			//serialize_xml(node.append_child("scaler_R2"), *begin);

			//
			//value_R2 Integer64,
			//
			//serialize_xml(node.append_child("value_R2"), *begin);

			//
			//unit_R3 SML_Unit,
			//
			//serialize_xml(node.append_child("unit_R3"), *begin);

			//
			//scaler_R3 Integer8,
			//
			//serialize_xml(node.append_child("scaler_R3"), *begin);

			//
			//value_R3 Integer64,
			//
			//serialize_xml(node.append_child("value_R3"), *begin);

			//
			//signature_mA_R2_R3 Octet String
			//
			//serialize_xml(node.append_child("signature_mA_R2_R3"), *begin);


		}


		//
		//	traverse_list_of_period_entries()
		//
		void sml_abl_exporter::traverse_list_of_period_entries(std::ofstream& file
			, cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end
			, m2m::obis const& object_code)
		{
			std::size_t count = std::distance(begin, end);
            boost::ignore_unused(count);
			//node.append_attribute("count") << count;

			for (std::uint32_t counter = 0; begin != end; ++begin, ++counter)
			{
				BOOST_ASSERT_MSG(cyy::primary_type_code_test<cyy::types::CYY_TUPLE>(*begin), "wrong data type - expected CYY_TUPLE in traverse_list_of_period_entries");
				cyy::tuple_t entry;
				entry = cyy::value_cast<cyy::tuple_t>(*begin, entry);

				//	element position as *comment*

				//auto comment = node.append_child(pugi::node_comment);
				//comment.set_value(std::to_string(counter + 1).c_str());

				//	node.append_child("PeriodEntry")
				traverse_period_entry(file
					, counter
					, object_code
					, std::begin(entry)
					, std::end(entry));
			}
		}

		void sml_abl_exporter::traverse_period_entry(std::ofstream& file
			, std::uint32_t counter
			, m2m::obis const& object_code
			, cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(begin, end);
			if (count < 5)
			{
				//	missing elements
				file
					<< "\nERROR(missing parameter in SML_PeriodEntry: "
					<< count
					<< "/5)"
					;
				return;
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
				//	node.append_child("objName"), 
				//emit_object(object_name);

				if (object_name == m2m::OBIS_CLASS_OP_LOG)
				{
					//	add property attribute
					//node.append_attribute("property") = m2m::get_property_name(object_name);
				}
				else if (object_name == m2m::OBIS_SERIAL_NR)
				{
					//	add property attribute
					//node.append_attribute("property") = "serial-number-1";
				}
				else if (object_name == m2m::OBIS_SERIAL_NR_SECOND)
				{
					//	add property attribute
					//node.append_attribute("property") = "serial-number-2";
				}
				else if (object_name == m2m::OBIS_MBUS_STATE)
				{
					//node.append_attribute("property") = "M-Bus-state";
				}

			}
			else
			{
				//serialize_xml(node.append_child("objName"), *begin);
			}

			//
			//	unit (see sml_unit_enum)
			//
			++begin;
			//
			//	set unit code as SML node value
			//
			//auto child = serialize_xml(node.append_child("SMLUnit"), *begin);

			//
			//	add unit name
			//
			std::uint8_t unit_code = 0;
			unit_code = cyy::numeric_cast(*begin, unit_code);
			//child.append_attribute("unit") = noddy::m2m::get_unit_name(unit_code);

			//
			//	scaler
			//
			++begin;
			const auto scaler = cyy::numeric_cast<std::int32_t>(*begin);

			//
			//	value
			//	Different datatypes are possible. Check it out.
			//	"valueOctet", "valueInt", "valueString", "value"
			++begin;
			if (object_name == m2m::OBIS_READOUT_UTC)
			{
				std::time_t tt = 0;
				tt = cyy::numeric_cast(*begin, tt);
				//serialize_xml(node.append_child("roTime"), cyy::time_point_factory(tt));

			}
			else if (object_name == m2m::OBIS_CLASS_EVENT)
			{
				//	see 2.2.1.12 Protokollierung im MUC Betriebslogbuch
				//	Ereignis (uint32)
				std::uint32_t event = 0;
				event = cyy::numeric_cast(*begin, event);

				//auto child = serialize_xml(node.append_child("event"), *begin);
				//child.append_attribute("type") = m2m::get_event_name(event);

			}
			else if (object_name == m2m::OBIS_CLASS_OP_LOG_PEER_ADDRESS)
			{
				//	peer address
				std::uint64_t source = 0;
				source = cyy::numeric_cast(*begin, source);

				//auto child = serialize_xml(node.append_child("peerAddress"), *begin);

				switch (source)
				{
				case 0x818100000020:
					//child.append_attribute("source") = "LSMC";
					break;
				case 0x818100000001:
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

				//auto child = serialize_xml(node.append_child("monitoringStatus"), *begin);

				switch (static_cast<std::uint16_t>(status) & 0xFFFF) {
				case 0xFFFC:
					//child.append_attribute("status") = "no answer";
					break;
				case 0xFFFD:
					//child.append_attribute("status") = "OK";
					break;
				case 0xFFFE:
					//child.append_attribute("status") = "Error";
					break;
				case 0xFFFF:
					//child.append_attribute("status") = "monitoring deactivated";
					break;
				default:
					//child.append_attribute("status") = "undefined status code";
					break;
				}

			}
			else
			{
				//
				//	Skip all "counter" data.
				//	Write physical data only.
				//
				if (object_name.is_physical_unit())
				{
					//
					//	normal value
					//
					file
						<< "\n"
						<< object_name
						<< '('
						<< noddy::m2m::scale_value(cyy::numeric_cast<std::int64_t>(*begin), scaler)
						<< '*'
						<< noddy::m2m::get_unit_name(unit_code)
						<< ')'
						;
				}
			}

			//
			//	valueSignature
			//
			++begin;
			//serialize_xml(node.append_child("signature"), *begin);

		}


		m2m::obis sml_abl_exporter::traverse_parameter_tree_path(cyy::object obj)
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

				//	Standard-Lastgänge
				//if (object_type == m2m::OBIS_CODE_1_MINUTE)
				//{
				//	node.append_attribute("profile") = "1min";
				//}
				//else if (object_type == m2m::OBIS_CODE_15_MINUTE)
				//{
				//	node.append_attribute("profile") = "15min";
				//}
				//else if (object_type == m2m::OBIS_CODE_60_MINUTE)
				//{
				//	node.append_attribute("profile") = "60min";
				//}
				//else if (object_type == m2m::OBIS_CODE_24_HOUR)
				//{
				//	node.append_attribute("profile") = "24h";
				//}
				//else if (object_type == m2m::OBIS_CODE_LAST_2_HOURS)
				//{
				//	node.append_attribute("profile") = "last2h";
				//}
				//else if (object_type == m2m::OBIS_CODE_LAST_WEEK)
				//{
				//	node.append_attribute("profile") = "lastWeek";
				//}
				//else if (object_type == m2m::OBIS_CODE_1_MONTH)
				//{
				//	node.append_attribute("profile") = "1month";
				//}
				//else if (object_type == m2m::OBIS_CODE_1_YEAR)
				//{
				//	node.append_attribute("profile") = "1year";
				//}
				//else if (object_type == m2m::OBIS_CODE_INITIAL)
				//{
				//	node.append_attribute("profile") = "initial";
				//}
				//else if (object_type == m2m::OBIS_CLASS_OP_LOG)
				//{
				//	node.append_attribute("root") = "operating log";
				//}
				//else if (object_type == m2m::OBIS_CLASS_EVENT)
				//{
				//	node.append_attribute("root") = "event";
				//}
				//else if (object_type == m2m::OBIS_CLASS_STATUS)
				//{
				//	node.append_attribute("root") = "LSM-status";
				//	//	see: 2.2.1.3 Status der Aktoren (Kontakte)
				//}
				//else if (object_type == m2m::OBIS_CODE_ROOT_NTP)
				//{
				//	node.append_attribute("root") = "NTP";
				//}
				//else {
				//	const std::string value = noddy::m2m::to_string(object_type);
				//	pugi::xml_node child = node.append_child("root");
				//	child.append_child(pugi::node_pcdata).set_value(value.c_str());
				//}

				return object_type;
			}

			//serialize_xml(node.append_child("parameterTreePath"), obj);
			return noddy::m2m::obis();
		}

		/*
		*	SML_Time parser
		*/
		cyy::object sml_abl_exporter::traverse_time(cyy::object obj)
		{

			if (cyy::primary_type_code_test<cyy::types::CYY_TUPLE>(obj))
			{
				cyy::tuple_t tpl;
				tpl = cyy::value_cast<cyy::tuple_t>(obj, tpl);
				return traverse_time(std::begin(tpl), std::end(tpl));
			}
			return obj;
		}

		/*
		 *	SML_Time parser
		 */
		cyy::object sml_abl_exporter::traverse_time(cyy::tuple_t::const_iterator begin
			, cyy::tuple_t::const_iterator end)
		{

			std::size_t count = std::distance(begin, end);

			if (count < 2)
			{
				//	missing elements
				//	error message as comment
				//auto child = node.append_child(pugi::node_comment);
				//child.set_value("***ERROR: SML_Time(2) - missing parameter");
				return cyy::factory();
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
				return cyy::time_point_factory(secindex);
			case sml::TIME_SECINDEX:
			default:
				break;
			}
			return *begin;
		}

		void sml_abl_exporter::emit_object(m2m::obis const& object_code)
		{
			//if (object_code.is_private())
			//{
			//	node.append_attribute("class") = "private";
			//}
			//else if (object_code.is_abstract())
			//{
			//	node.append_attribute("class") = "abstract";
			//}
			//else
			//{
			//	node.append_attribute("class") = object_code.get_medium_name();
			//}

			//const std::string s = m2m::to_hex(object_code);
			//node.append_attribute("value") << s;

			const std::string value = m2m::to_string(object_code);
//			node.append_child(pugi::node_pcdata).set_value(value.c_str());
		}

		boost::filesystem::path sml_abl_exporter::unique_file_name(std::chrono::system_clock::time_point const& act_time
			, noddy::m2m::ctrl_address const& address) const
		{

			std::stringstream ss;
			if (address.is_mbus_address())
			{
				noddy::m2m::mbus_address ma(address);

				ss
					<< prefix_
					<< cyy::short_format(act_time)
					<< '-'
					<< '-'
					<< ma.get_serial_number()
					<< '-'
					<< '-'
					<< ma.get_medium_name_long()
					<< ".abl"
					;


			}
			else
			{
				ss
					<< prefix_
					<< cyy::short_format(act_time)
					<< '-'
					<< '-'
					<< noddy::m2m::to_string(address)
					<< ".abl"
					;

			}
			return out_dir_ / ss.str();
		}

	}	//	sml
}	

