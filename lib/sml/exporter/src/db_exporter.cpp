/*
* Copyright Sylko Olzscher 2016
*
* Use, modification, and distribution is subject to the Boost Software
* License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
* http://www.boost.org/LICENSE_1_0.txt)
*/

#include <smf/sml/exporter/db_exporter.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/obis_io.h>
#include <smf/sml/units.h>
#include <smf/sml/scaler.h>

#include <cyng/io/io_buffer.h>
#include <cyng/io/io_chrono.hpp>
#include <cyng/value_cast.hpp>
#include <cyng/xml.h>
#include <cyng/factory.h>

namespace node
{
	namespace sml
	{

		db_exporter::db_exporter(cyng::controller& vm)
			: vm_(vm)
			, source_(0)
			, channel_(0)
			, target_()
		{
			reset();
		}

		db_exporter::db_exporter(cyng::controller& vm
			, std::uint32_t source
			, std::uint32_t channel
			, std::string const& target)
		: vm_(vm)
			, source_(source)
			, channel_(channel)
			, target_(target)
		{
			reset();
		}


		void db_exporter::reset()
		{
		}

		void db_exporter::read(cyng::tuple_t const& msg, std::size_t idx)
		{
			//std::string s = std::to_string(idx);
			read_msg(msg.begin(), msg.end(), idx);
		}

		void db_exporter::read_msg(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end, std::size_t idx)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 5, "SML message");

			//auto msg = root_.append_child("msg");
			//msg.append_attribute("idx").set_value(idx);

			//
			//	(1) - transaction id
			//	This is typically a printable sequence of 6 up to 9 ASCII values
			//
			//std::cout << cyng::io::to_ascii(cyng::value_cast<cyng::buffer_t>(*pos, cyng::buffer_t())) << std::endl;
			const std::string trx = cyng::io::to_ascii(cyng::value_cast<cyng::buffer_t>(*pos, cyng::buffer_t()));
			//cyng::xml::write(msg.append_child("trx"), *pos++).append_attribute("ascii").set_value(trx.c_str());

			//
			//	(2) groupNo
			//
			//cyng::xml::write(msg.append_child("groupNo"), *pos++);

			//
			//	(3) abortOnError
			//
			//cyng::xml::write(msg.append_child("abortOnError"), *pos++);

			//
			//	(4/5) CHOICE - msg type
			//
			cyng::tuple_t choice;
			choice = cyng::value_cast(*pos++, choice);
			BOOST_ASSERT_MSG(choice.size() == 2, "CHOICE");
			if (choice.size() == 2)
			{
				//cyng::xml::write(msg.append_child("code"), choice.front());
				//read_body(msg.append_child("body"), choice.front(), choice.back());
			}

			//
			//	(6) CRC16
			//
			//cyng::xml::write(msg.append_child("crc16"), *pos);
		}

		void db_exporter::read_body(context node, cyng::object type, cyng::object body)
		{
			auto code = cyng::value_cast<std::uint16_t>(type, 0);
			//node.append_attribute("type").set_value(messages::name(code));

			cyng::tuple_t tpl;
			tpl = cyng::value_cast(body, tpl);

			switch (code)
			{
			case BODY_OPEN_REQUEST:
				read_public_open_request(node, tpl.begin(), tpl.end());
				break;
			case BODY_OPEN_RESPONSE:
				read_public_open_response(node, tpl.begin(), tpl.end());
				break;
			case BODY_CLOSE_REQUEST:
				//cyng::xml::write(node.append_child("data"), body);
				break;
			case BODY_CLOSE_RESPONSE:
				//cyng::xml::write(node.append_child("data"), body);
				break;
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
				read_get_profile_list_response(node, tpl.begin(), tpl.end());
				break;
			case BODY_GET_PROC_PARAMETER_REQUEST:
				//cyng::xml::write(node.append_child("data"), body);
				break;
			case BODY_GET_PROC_PARAMETER_RESPONSE:
				read_get_proc_parameter_response(node, tpl.begin(), tpl.end());
				break;
			case BODY_SET_PROC_PARAMETER_REQUEST:
				//cyng::xml::write(node, body);
				break;
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
				read_attention_response(node, tpl.begin(), tpl.end());
				break;
			default:
				//cyng::xml::write(node.append_child("data"), body);
				break;
			}
		}

		void db_exporter::read_public_open_request(context node, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 7, "Public Open Request");

			//	codepage "ISO 8859-15"
			//cyng::xml::write(node.append_child("codepage"), *pos++);

			//
			//	clientId (MAC)
			//	Typically 7 bytes to identify gateway/MUC
			//cyng::xml::write(node.append_child("clientId"), *pos++);

			//
			//	reqFileId
			//
			//cyng::xml::write(node.append_child("reqFileId"), *pos++);

			//
			//	serverId
			//
			//cyng::xml::write(node.append_child("serverId"), *pos++);

			//
			//	username
			//
			//cyng::xml::write(node.append_child("userName"), *pos++);

			//
			//	password
			//
			//cyng::xml::write(node.append_child("password"), *pos++);

			//
			//	sml-Version: default = 1
			//
			//cyng::xml::write(node.append_child("SMLVersion"), *pos++);

		}

		void db_exporter::read_public_open_response(context node, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 6, "Public Open Response");

			//	codepage "ISO 8859-15"
			//cyng::xml::write(node.append_child("codepage"), *pos++);

			//
			//	clientId (MAC)
			//	Typically 7 bytes to identify gateway/MUC
			//cyng::xml::write(node.append_child("clientId"), *pos++);

			//
			//	reqFileId
			//
			//cyng::xml::write(node.append_child("reqFileId"), *pos++);

			//
			//	serverId
			//
			//cyng::xml::write(node.append_child("serverId"), *pos++);

			//
			//	refTime
			//
			//cyng::xml::write(node.append_child("refTime"), *pos++);

			//	sml-Version: default = 1
			//cyng::xml::write(node.append_child("SMLVersion"), *pos++);

		}

		void db_exporter::read_get_profile_list_response(context node, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 9, "Get Profile List Response");

			//
			//	serverId
			//
			//cyng::xml::write(node.append_child("serverId"), *pos++);

			//
			//	actTime
			//
			read_time(node.append_child("actTime"), *pos++);

			//
			//	regPeriod
			//
			//cyng::xml::write(node.append_child("regPeriod"), *pos++);

			//
			//	parameterTreePath (OBIS)
			//
			std::vector<obis> path = read_param_tree_path(node.append_child("profiles"), *pos++);

			//
			//	valTime
			//
			read_time(node.append_child("valTime"), *pos++);

			//
			//	status
			//
			//cyng::xml::write(node.append_child("status"), *pos++);

			//	period-List (1)
			//	first we make the TSMLGetProfileListResponse complete, then 
			//	the we store the entries in TSMLPeriodEntry.
			cyng::tuple_t tpl;
			tpl = cyng::value_cast(*pos++, tpl);
			read_period_list(node.append_child("period"), path, tpl.begin(), tpl.end());

			//	rawdata
			//cyng::xml::write(node.append_child("rawData"), *pos++);

			//	periodSignature
			//cyng::xml::write(node.append_child("signature"), *pos++);

		}

		void db_exporter::read_get_proc_parameter_response(context node, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 3, "Get Proc Parameter Response");

			//
			//	serverId
			//
			//cyng::xml::write(node.append_child("serverId"), *pos++);

			//
			//	parameterTreePath (OBIS)
			//
			std::vector<obis> path = read_param_tree_path(node.append_child("profiles"), *pos++);

			//
			//	parameterTree
			//
			cyng::tuple_t tpl;
			tpl = cyng::value_cast(*pos++, tpl);
			read_param_tree(node.append_child("parameterTree"), 0, tpl.begin(), tpl.end());

		}

		void db_exporter::read_attention_response(context node, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 4, "Attention Response");

			//
			//	serverId
			//
			//cyng::xml::write(node.append_child("serverId"), *pos++);

			//
			//	attentionNo (OBIS)
			//
			obis code = read_obis(node.append_child("msg"), *pos++);

			//
			//	attentionMsg
			//
			//cyng::xml::write(node.append_child("attentionMsg"), *pos++);

			//
			//	attentionDetails (SML_Tree)
			//
			cyng::tuple_t tpl;
			tpl = cyng::value_cast(*pos++, tpl);
			read_param_tree(node.append_child("parameterTree"), 0, tpl.begin(), tpl.end());

		}

		void db_exporter::read_param_tree(context node
			, std::size_t depth
			, cyng::tuple_t::const_iterator pos
			, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 3, "SML Tree");

			//
			//	1. parameterName Octet String,
			//
			obis code = read_obis(node.append_child("parameterName"), *pos++);

			//
			//	2. parameterValue SML_ProcParValue OPTIONAL,
			//
			//cyng::xml::write(node.append_child("parameterValue"), *pos++);
			read_parameter(node.append_child("parameterValue"), code, *pos++);

			//
			//	3. child_List List_of_SML_Tree OPTIONAL
			//
			//cyng::xml::write(node.append_child("child_List"), *pos++);
			cyng::tuple_t tpl;
			tpl = cyng::value_cast(*pos++, tpl);
			for (auto const child : tpl)
			{
				cyng::tuple_t tmp;
				tmp = cyng::value_cast(child, tmp);
				read_param_tree(node.append_child("parameterTree"), ++depth, tmp.begin(), tmp.end());
			}
		}

		void db_exporter::read_period_list(context node
			, std::vector<obis> const& path
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
				read_period_entry(node.append_child("entry"), path, counter, tpl.begin(), tpl.end());
				++counter;

			}
		}

		void db_exporter::read_period_entry(context node
			, std::vector<obis> const& path
			, std::size_t index
			, cyng::tuple_t::const_iterator pos
			, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 5, "Period Entry");

			//
			//	object name
			//
			obis code = read_obis(node, *pos++);

			//
			//	unit (see sml_unit_enum)
			//
			const auto unit = read_unit(node.append_child("SMLUnit"), *pos++);

			//
			//	scaler
			//
			const auto scaler = read_scaler(node.append_child("scaler"), *pos++);

			//
			//	value
			//
			read_value(node.append_child("value"), code, scaler, unit, *pos++);

			//
			//	valueSignature
			//
			//cyng::xml::write(node.append_child("signature"), *pos++);

		}


		void db_exporter::read_time(context node, cyng::object obj)
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
					//cyng::xml::write(node, cyng::make_time_point(sec));
				}
				break;
				case TIME_SECINDEX:
					//cyng::xml::write(node, choice.back());
					break;
				default:
					break;
				}
			}

		}

		std::vector<obis> db_exporter::read_param_tree_path(context node, cyng::object obj)
		{
			std::vector<obis> result;

			cyng::tuple_t tpl;
			tpl = cyng::value_cast(obj, tpl);
			for (auto obj : tpl)
			{

				const obis object_type = read_obis(node, obj);
				result.push_back(object_type);

			}
			return result;
		}

		obis db_exporter::read_obis(context node, cyng::object obj)
		{
			cyng::buffer_t tmp;
			tmp = cyng::value_cast(obj, tmp);

			const obis code = obis(tmp);
			const std::string name_dec = to_string(code);
			const std::string name_hex = to_hex(code);

			auto child = node.append_child("profile");
			//child.append_attribute("type").set_value(get_name(code));
			//child.append_attribute("obis").set_value(name_hex.c_str());
			//child.append_child(pugi::node_pcdata).set_value(name_dec.c_str());

			return code;
		}

		std::uint8_t db_exporter::read_unit(context node, cyng::object obj)
		{
			std::uint8_t unit = cyng::value_cast<std::uint8_t>(obj, 0);
			//node.append_attribute("type").set_value(get_unit_name(unit));
			//node.append_child(pugi::node_pcdata).set_value(std::to_string(+unit).c_str());
			return unit;
		}

		std::int8_t db_exporter::read_scaler(context node, cyng::object obj)
		{
			std::int8_t scaler = cyng::value_cast<std::int8_t>(obj, 0);
			//node.append_child(pugi::node_pcdata).set_value(std::to_string(+scaler).c_str());
			return scaler;
		}

		void db_exporter::read_value(context node, obis code, std::int8_t scaler, std::uint8_t unit, cyng::object obj)
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
				//node.append_attribute("name").set_value(manufacturer.c_str());
			}
			else if (code == OBIS_READOUT_UTC)
			{
				if (obj.get_class().tag() == cyng::TC_TUPLE)
				{
					read_time(node, obj);
				}
				else
				{
					const auto tm = cyng::value_cast<std::uint32_t>(obj, 0);
					const auto tp = std::chrono::system_clock::from_time_t(tm);
					const auto str = cyng::to_str(tp);
					//node.append_attribute("readoutTime").set_value(str.c_str());
				}
			}
			else if (code == OBIS_ACT_SENSOR_TIME)
			{
				const auto tm = cyng::value_cast<std::uint32_t>(obj, 0);
				const auto tp = std::chrono::system_clock::from_time_t(tm);
				const auto str = cyng::to_str(tp);
				//node.append_attribute("sensorTime").set_value(str.c_str());
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
						//node.append_attribute("reading").set_value(str.c_str());
					}
					break;
					case cyng::TC_INT32:
					{
						const std::int32_t value = cyng::value_cast<std::int32_t>(obj, 0);
						const auto str = scale_value(value, scaler);
						//node.append_attribute("reading").set_value(str.c_str());
					}
					break;
					default:
						break;
					}
				}
			}
		}

		void db_exporter::read_parameter(context node, obis code, cyng::object obj)
		{
			cyng::tuple_t tpl;
			tpl = cyng::value_cast(obj, tpl);
			if (tpl.size() == 2)
			{

				//PROC_PAR_VALUE = 1,
				//PROC_PAR_PERIODENTRY = 2,
				//PROC_PAR_TUPELENTRY = 3,
				//PROC_PAR_TIME = 4,

				const auto type = cyng::value_cast<std::uint8_t>(tpl.front(), 0);
				switch (type)
				{
				case PROC_PAR_VALUE:
					//node.append_attribute("parameter").set_value("value");
					//cyng::xml::write(node, tpl.back());
					if ((code == OBIS_CLASS_OP_LSM_ACTIVE_RULESET) || (code == OBIS_CLASS_OP_LSM_PASSIVE_RULESET))
					{
						cyng::buffer_t buffer;
						buffer = cyng::value_cast(tpl.back(), buffer);
						const auto name = cyng::io::to_ascii(buffer);
						//node.append_attribute("name").set_value(name.c_str());
					}
					break;
				case PROC_PAR_PERIODENTRY:
					//node.append_attribute("parameter").set_value("period");
					//cyng::xml::write(node, tpl.back());
					break;
				case PROC_PAR_TUPELENTRY:
					//node.append_attribute("parameter").set_value("tuple");
					//cyng::xml::write(node, tpl.back());
					break;
				case PROC_PAR_TIME:
					//node.append_attribute("parameter").set_value("time");
					read_time(node, tpl.back());
					break;
				default:
					//node.append_attribute("parameter").set_value("undefined");
					//cyng::xml::write(node, obj);
					break;
				}
			}
			else
			{
				//cyng::xml::write(node, obj);
			}
		}

		db_exporter::context db_exporter::context::append_child(std::string const&)
		{
			return *this;
		}


	}	//	sml
}

