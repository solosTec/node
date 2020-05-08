/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/sml/exporter/xml_sml_exporter.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/obis_io.h>
#include <smf/sml/intrinsics/obis_factory.hpp>
#include <smf/sml/srv_id_io.h>
#include <smf/sml/scaler.h>
#include <smf/mbus/defs.h>
#include <smf/mbus/units.h>

#include <cyng/io/io_buffer.h>
#include <cyng/io/io_chrono.hpp>
#include <cyng/value_cast.hpp>
#include <cyng/xml.h>
#include <cyng/factory.h>
#include <cyng/chrono.h>

#include <boost/core/ignore_unused.hpp>

namespace node
{
	namespace sml	
	{

		xml_exporter::xml_exporter()
		: doc_()
			, root_()
			, encoding_("UTF-8")
			, root_name_("SML")
			, source_(0)
			, channel_(0)
			, target_()
		{
			reset();
		}

		xml_exporter::xml_exporter(std::string const& encoding, std::string const& root)
		: doc_()
			, root_()
			, encoding_(encoding)
			, root_name_(root)
			, source_(0)
			, channel_(0)
			, target_()
		{
			reset();
		}

		xml_exporter::xml_exporter(std::string const& encoding
			, std::string const& root
			, std::uint32_t source
			, std::uint32_t channel
			, std::string const& target)
		: doc_()
			, root_()
			, encoding_(encoding)
			, root_name_(root)
			, source_(source)
			, channel_(channel)
			, target_(target)
		{
			reset();
		}

		boost::filesystem::path xml_exporter::get_filename() const
		{
			auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
			std::tm time = cyng::chrono::convert_utc(tt);

			std::stringstream ss;
			ss
				<< root_name_
				<< '-'
				<< std::setfill('0')
				<< cyng::chrono::year(time)
				<< std::setw(2)
				<< cyng::chrono::month(time)
				<< 'T'
				<< std::setw(2)
				<< cyng::chrono::day(time)
				<< std::setw(2)
				<< cyng::chrono::hour(time)
				<< std::setw(2)
				<< cyng::chrono::minute(time)
				<< '-'
				<< target_
				<< '-'
				<< std::hex
				<< std::setw(4)
				<< channel_
				<< '-'
				<< std::setw(4)
				<< source_
				<< ".xml"
				;
			return ss.str();
		}

		void xml_exporter::reset()
		{
            doc_.reset();

			// Generate XML declaration
			auto declarationNode = doc_.append_child(pugi::node_declaration);
			declarationNode.append_attribute("version") = "1.0";
			declarationNode.append_attribute("encoding") = encoding_.c_str();
			declarationNode.append_attribute("standalone") = "yes";

			root_ = doc_.append_child(root_name_.c_str());
			root_.append_attribute("xmlns:xsi") = "w3.org/2001/XMLSchema-instance";

			auto meta = root_.append_child("meta");
			meta.append_attribute("source").set_value(source_);
			meta.append_attribute("channel").set_value(channel_);
			meta.append_child(pugi::node_pcdata).set_value(target_.c_str());
		}

		bool xml_exporter::write(boost::filesystem::path const& p)
		{
			//BOOST_ASSERT_MSG(!boost::filesystem::exists(p), "file already exists");
			// Save XML tree to file.
			// Remark: second optional param is indent string to be used;
			// default indentation is tab character.
			return (!boost::filesystem::exists(p))
				? doc_.save_file(p.c_str(), PUGIXML_TEXT("  "))
				: false
				;
		}

		void xml_exporter::read(cyng::tuple_t const& msg)
		{
			read_msg(msg.begin(), msg.end());
		}

		void xml_exporter::read_msg(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 5, "SML message");
			boost::ignore_unused(count);	//	release version
			
			auto msg = root_.append_child("msg");

			//
			//	(1) - transaction id
			//	This is typically a printable sequence of 6 up to 9 ASCII values
			//
			read_string(msg.append_child("trx"), *pos++);

			//
			//	(2) groupNo
			//
			cyng::xml::write(msg.append_child("groupNo"), *pos++);

			//
			//	(3) abortOnError
			//
			cyng::xml::write(msg.append_child("abortOnError"), *pos++);

			//
			//	(4/5) CHOICE - msg type
			//
//             std::pair<message_e, cyng::tuple_t> readout::read_choice(*pos++);
			cyng::tuple_t choice;
			choice = cyng::value_cast(*pos++, choice);
			BOOST_ASSERT_MSG(choice.size() == 2, "CHOICE");
			if (choice.size() == 2)
			{
				cyng::xml::write(msg.append_child("code"), choice.front());
				read_body(msg.append_child("body"), choice.front(), choice.back());
			}

			//
			//	(6) CRC16
			//
			cyng::xml::write(msg.append_child("crc16"), *pos);
		}

		void xml_exporter::read_body(pugi::xml_node node, cyng::object type, cyng::object body)
		{
			auto code = cyng::value_cast<std::uint16_t>(type, 0);
			node.append_attribute("type").set_value(messages::name_from_value(code));

			cyng::tuple_t tpl;
			tpl = cyng::value_cast(body, tpl);

			switch (static_cast<message_e>(code))
			{
			case message_e::OPEN_REQUEST:		
				read_public_open_request(node, tpl.begin(), tpl.end());
				break;
			case message_e::OPEN_RESPONSE:	
				read_public_open_response(node, tpl.begin(), tpl.end());
				break;
			case message_e::CLOSE_REQUEST:
				cyng::xml::write(node.append_child("data"), body);
				break;
			case message_e::CLOSE_RESPONSE:
				cyng::xml::write(node.append_child("data"), body);
				break;
			case message_e::GET_PROFILE_PACK_REQUEST:
				cyng::xml::write(node.append_child("data"), body);
				break;
			case message_e::GET_PROFILE_PACK_RESPONSE:
				cyng::xml::write(node.append_child("data"), body);
				break;
			case message_e::GET_PROFILE_LIST_REQUEST:
				cyng::xml::write(node.append_child("data"), body);
				break;
			case message_e::GET_PROFILE_LIST_RESPONSE:
				read_get_profile_list_response(node, tpl.begin(), tpl.end());
				break;
			case message_e::GET_PROC_PARAMETER_REQUEST:
				cyng::xml::write(node.append_child("data"), body);
				break;
			case message_e::GET_PROC_PARAMETER_RESPONSE:
				read_get_proc_parameter_response(node, tpl.begin(), tpl.end());
				break;
			case message_e::SET_PROC_PARAMETER_REQUEST:
				cyng::xml::write(node, body);
				break;
			case message_e::SET_PROC_PARAMETER_RESPONSE:
				cyng::xml::write(node.append_child("data"), body);
				break;
			case message_e::GET_LIST_REQUEST:
				cyng::xml::write(node.append_child("data"), body);
				break;
			case message_e::GET_LIST_RESPONSE:
				cyng::xml::write(node.append_child("data"), body);
				break;
			case message_e::ATTENTION_RESPONSE:
				read_attention_response(node, tpl.begin(), tpl.end());
				break;
			default:
				cyng::xml::write(node.append_child("data"), body);
				break;
			}
		}

		void xml_exporter::read_public_open_request(pugi::xml_node node, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 7, "Public Open Request");
			boost::ignore_unused(count);	//	release version
			
			//	codepage "ISO 8859-15"
			cyng::xml::write(node.append_child("codepage"), *pos++);

			//
			//	clientId (MAC)
			//	Typically 7 bytes to identify gateway/MUC
			read_client_id(node.append_child("clientId"), *pos++);

			//
			//	reqFileId
			//
			read_string(node.append_child("reqFileId"), *pos++);

			//
			//	serverId
			//
			read_server_id(node.append_child("serverId"), *pos++);
			
			//
			//	username
			//
			cyng::xml::write(node.append_child("userName"), *pos++);

			//
			//	password
			//
			cyng::xml::write(node.append_child("password"), *pos++);

			//
			//	sml-Version: default = 1
			//
			cyng::xml::write(node.append_child("SMLVersion"), *pos++);

		}

		void xml_exporter::read_public_open_response(pugi::xml_node node, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 6, "Public Open Response");
			boost::ignore_unused(count);	//	release version
			
			//	codepage "ISO 8859-15"
			cyng::xml::write(node.append_child("codepage"), *pos++);

			//
			//	clientId (MAC)
			//	Typically 7 bytes to identify gateway/MUC
			read_client_id(node.append_child("clientId"), *pos++);

			//
			//	reqFileId
			//
			read_string(node.append_child("reqFileId"), *pos++);

			//
			//	serverId
			//
			read_server_id(node.append_child("serverId"), *pos++);

			//
			//	refTime
			//
			cyng::xml::write(node.append_child("refTime"), *pos++);

			//	sml-Version: default = 1
			cyng::xml::write(node.append_child("SMLVersion"), *pos++);

		}

		void xml_exporter::read_get_profile_list_response(pugi::xml_node node, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 9, "Get Profile List Response");
			boost::ignore_unused(count);	//	release version
			
			//
			//	serverId
			//
			read_server_id(node.append_child("serverId"), *pos++);

			//
			//	actTime
			//
			read_time(node.append_child("actTime"), *pos++);

			//
			//	regPeriod
			//
			cyng::xml::write(node.append_child("regPeriod"), *pos++);

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
			cyng::xml::write(node.append_child("status"), *pos++);

			//	period-List (1)
			//	first we make the TSMLGetProfileListResponse complete, then 
			//	the we store the entries in TSMLPeriodEntry.
			cyng::tuple_t tpl;
			tpl = cyng::value_cast(*pos++, tpl);
			read_period_list(node.append_child("period"), path, tpl.begin(), tpl.end());

			//	rawdata
			cyng::xml::write(node.append_child("rawData"), *pos++);

			//	periodSignature
			cyng::xml::write(node.append_child("signature"), *pos++);

		}

		void xml_exporter::read_get_proc_parameter_response(pugi::xml_node node, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 3, "Get Proc Parameter Response");
			boost::ignore_unused(count);	//	release version
			
			//
			//	serverId
			//
			read_server_id(node.append_child("serverId"), *pos++);

			//
			//	parameterTreePath (OBIS)
			//
			auto const path = read_param_tree_path(node.append_child("profiles"), *pos++);

			//
			//	parameterTree
			//
			cyng::tuple_t tpl;
			tpl = cyng::value_cast(*pos++, tpl);
			read_param_tree(node.append_child("parameterTree"), 0, tpl.begin(), tpl.end());

		}

		void xml_exporter::read_attention_response(pugi::xml_node node, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 4, "Attention Response");
			boost::ignore_unused(count);	//	release version
			
			//
			//	serverId
			//
			read_server_id(node.append_child("serverId"), *pos++);

			//
			//	attentionNo (OBIS)
			//
			obis code = read_obis(node.append_child("msg"), *pos++);
			boost::ignore_unused(code);	//	release version
			
			//
			//	attentionMsg
			//
			cyng::xml::write(node.append_child("attentionMsg"), *pos++);

			//
			//	attentionDetails (SML_Tree)
			//
			cyng::tuple_t tpl;
			tpl = cyng::value_cast(*pos++, tpl);
			read_param_tree(node.append_child("parameterTree"), 0, tpl.begin(), tpl.end());

		}

		void xml_exporter::read_param_tree(pugi::xml_node node
			, std::size_t depth
			, cyng::tuple_t::const_iterator pos
			, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 3, "SML Tree");
 			boost::ignore_unused(count);	//	release version

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

		void xml_exporter::read_period_list(pugi::xml_node node
			, std::vector<obis> const& path
			, cyng::tuple_t::const_iterator pos
			, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			const std::string str_count = std::to_string(count);
			node.append_attribute("size").set_value(str_count.c_str());

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

		void xml_exporter::read_period_entry(pugi::xml_node node
			, std::vector<obis> const& path
			, std::size_t index
			, cyng::tuple_t::const_iterator pos
			, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 5, "Period Entry");
			boost::ignore_unused(count);	//	release version
			
			node.append_attribute("idx").set_value(index);

			//
			//	object name
			//
			auto const code = read_obis(node, *pos++);
			//if (code == OBIS_CODE(01, 00, 01, 08, 00, ff)) {
			//	std::string msg("things get interesting here");
			//	//	scaler = 0, value = 2746916 expected
			//}

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
			cyng::xml::write(node.append_child("signature"), *pos++);

		}


		void xml_exporter::read_time(pugi::xml_node node, cyng::object obj)
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
					cyng::xml::write(node, cyng::make_time_point(sec));
				}
				break;
				case TIME_SECINDEX:
					cyng::xml::write(node, choice.back());
					break;
				default:
					break;
				}
			}

		}

		std::vector<obis> xml_exporter::read_param_tree_path(pugi::xml_node node, cyng::object obj)
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

		obis xml_exporter::read_obis(pugi::xml_node node, cyng::object obj)
		{
			cyng::buffer_t tmp;
			tmp = cyng::value_cast(obj, tmp);

			const obis code = obis(tmp);
			const std::string name_dec = to_string(code);
			const std::string name_hex = to_hex(code);

			auto child = node.append_child("profile");
			if (OBIS_CODE(00, 80, 80, 11, 10, FF) == code)
			{
				child.append_attribute("gotIt").set_value("ActorID");
			}
			child.append_attribute("type").set_value(get_name(code).c_str());
			child.append_attribute("obis").set_value(name_hex.c_str());
			child.append_child(pugi::node_pcdata).set_value(name_dec.c_str());

			return code;
		}

		std::uint8_t xml_exporter::read_unit(pugi::xml_node node, cyng::object obj)
		{
			std::uint8_t unit = cyng::value_cast<std::uint8_t>(obj, 0);
			node.append_attribute("type").set_value(node::mbus::get_unit_name(unit));
			node.append_child(pugi::node_pcdata).set_value(std::to_string(+unit).c_str());
			return unit;
		}

		std::int8_t xml_exporter::read_scaler(pugi::xml_node node, cyng::object obj)
		{
			std::int8_t scaler = cyng::value_cast<std::int8_t>(obj, 0);
			node.append_child(pugi::node_pcdata).set_value(std::to_string(+scaler).c_str());
			return scaler;
		}

		std::string xml_exporter::read_string(pugi::xml_node node, cyng::object obj)
		{
			cyng::buffer_t buffer;
			buffer = cyng::value_cast(obj, buffer);
			const auto str = cyng::io::to_ascii(buffer);
			node.append_attribute("value").set_value(str.c_str());
			cyng::xml::write(node, obj);
			return str;

		}

		std::string xml_exporter::read_server_id(pugi::xml_node node, cyng::object obj)
		{
			cyng::buffer_t buffer;
			buffer = cyng::value_cast(obj, buffer);
			const auto str = from_server_id(buffer);
			node.append_attribute("value").set_value(str.c_str());
			if (is_mbus_wired(buffer) || is_mbus_radio(buffer))
			{
				//
				//	extract manufacturer name
				//	example: 01-2d2c-68866869-1c-04
				//
				const auto manufacturer = decode(buffer.at(1), buffer.at(2));
				node.append_attribute("manufacturer").set_value(manufacturer.c_str());

			}

			cyng::xml::write(node, obj);
			return str;
		}

		std::string xml_exporter::read_client_id(pugi::xml_node node, cyng::object obj)
		{
			cyng::buffer_t buffer;
			buffer = cyng::value_cast(obj, buffer);
			const auto str = from_server_id(buffer);
			node.append_attribute("value").set_value(str.c_str());
			cyng::xml::write(node, obj);
			return str;
		}

		void xml_exporter::read_value(pugi::xml_node node, obis code, std::int8_t scaler, std::uint8_t unit, cyng::object obj)
		{
			//
			//	write value
			//
			cyng::xml::write(node, obj);

			if (code == OBIS_DATA_MANUFACTURER)
			{
				cyng::buffer_t buffer;
				buffer = cyng::value_cast(obj, buffer);
				const auto manufacturer = cyng::io::to_ascii(buffer);
				node.append_attribute("name").set_value(manufacturer.c_str());
			}
			else if (code == OBIS_CURRENT_UTC)
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
					node.append_attribute("readoutTime").set_value(str.c_str());
				}
			}
			else if (code == OBIS_ACT_SENSOR_TIME)
			{
				const auto tm = cyng::value_cast<std::uint32_t>(obj, 0);
				const auto tp = std::chrono::system_clock::from_time_t(tm);
				const auto str = cyng::to_str(tp);
				node.append_attribute("sensorTime").set_value(str.c_str());
			}
			else if (code == OBIS_SERIAL_NR)
			{
				cyng::buffer_t buffer;
				buffer = cyng::value_cast(obj, buffer);
				const auto serial_nr = from_server_id(buffer);
				node.append_attribute("serial-nr-1").set_value(serial_nr.c_str());
				if (buffer.size() == 8)
				{
					//
					//	extract manufacturer name
					//	example: 01-2d2c-68866869-1c-04
					//
					const auto manufacturer = decode(buffer.at(0), buffer.at(1));
					node.append_attribute("manufacturer").set_value(manufacturer.c_str());
				}
			}
			else if (code == OBIS_SERIAL_NR_SECOND)
			{
				cyng::buffer_t buffer;
				buffer = cyng::value_cast(obj, buffer);
				const auto serial_nr = from_server_id(buffer);
				node.append_attribute("serial-nr-2").set_value(serial_nr.c_str());
				if (buffer.size() == 8)
				{
					//
					//	extract manufacturer name
					//	example: 01-2d2c-68866869-1c-04
					//
					const auto manufacturer = decode(buffer.at(0), buffer.at(1));
					node.append_attribute("manufacturer").set_value(manufacturer.c_str());
				}
			}
			else if (code == OBIS_CLASS_RADIO_KEY)
			{
				node.append_attribute("radio-key").set_value("M-bus");
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
						node.append_attribute("reading").set_value(str.c_str());
					}
						break;
					case cyng::TC_INT32:
					{
						const std::int32_t value = cyng::value_cast<std::int32_t>(obj, 0);
						const auto str = scale_value(value, scaler);
						node.append_attribute("reading").set_value(str.c_str());
					}
					break;
					default:
						break;
					}
				}
			}
		}

		void xml_exporter::read_parameter(pugi::xml_node node, obis code, cyng::object obj)
		{
			cyng::tuple_t tpl;
			tpl = cyng::value_cast(obj, tpl);
			if (tpl.size() == 2)
			{
				const auto type = cyng::value_cast<std::uint8_t>(tpl.front(), 0);
				switch (type)
				{
				case PROC_PAR_VALUE:
					node.append_attribute("parameter").set_value("value");
					cyng::xml::write(node, tpl.back());
					if ((code == OBIS_CLASS_OP_LSM_ACTIVE_RULESET) || (code == OBIS_CLASS_OP_LSM_PASSIVE_RULESET))
					{
						cyng::buffer_t buffer;
						buffer = cyng::value_cast(tpl.back(), buffer);
						const auto name = cyng::io::to_ascii(buffer);
						node.append_attribute("name").set_value(name.c_str());
					}
					else if (code == OBIS_CLASS_EVENT)
					{
						const auto evt = cyng::value_cast<std::uint32_t>(tpl.back(), 0);
						node.append_attribute("name").set_value(get_LSM_event_name(evt));						
					}
					break;
				case PROC_PAR_PERIODENTRY:
					node.append_attribute("parameter").set_value("period");
					cyng::xml::write(node, tpl.back());
					break;
				case PROC_PAR_TUPELENTRY:
					node.append_attribute("parameter").set_value("tuple");
					cyng::xml::write(node, tpl.back());
					break;
				case PROC_PAR_TIME:
					node.append_attribute("parameter").set_value("time");
					//cyng::xml::write(node, tpl.back());
					read_time(node, tpl.back());
					break;
				default:
					node.append_attribute("parameter").set_value("undefined");
					cyng::xml::write(node, obj);
					break;
				}
			}
			else
			{
				cyng::xml::write(node, obj);
			}
		}


	}	//	sml
}	

