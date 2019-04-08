/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/sml/exporter/abl_sml_exporter.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/obis_io.h>
#include <smf/sml/srv_id_io.h>
#include <smf/sml/scaler.h>
#include <smf/mbus/units.h>

#include <cyng/io/io_buffer.h>
#include <cyng/io/io_chrono.hpp>
#include <cyng/io/serializer.h>
#include <cyng/numeric_cast.hpp>
#include <cyng/intrinsics/traits/tag_names.hpp>
#include <cyng/factory.h>

#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/algorithm/string.hpp>

namespace node
{
	namespace sml
	{

		abl_exporter::abl_exporter(boost::filesystem::path root_dir
			, std::string prefix
			, std::string suffix
			, bool eol)
		: root_dir_(root_dir)
			, prefix_(prefix)
			, suffix_(suffix)
			, eol_(eol ? "\r\n" : "\n")
			, source_(0)
			, channel_(0)
			, target_()
			, rgn_()
			, ro_(rgn_())
		{
			reset();
		}

		abl_exporter::abl_exporter(boost::filesystem::path root_dir
			, std::string prefix
			, std::string suffix
			, bool eol
			, std::uint32_t source
			, std::uint32_t channel
			, std::string const& target)
		: root_dir_(root_dir)
			, prefix_(prefix)
			, suffix_(suffix)
			, eol_(eol ? "\r\n" : "\n")
			, source_(source)
			, channel_(channel)
			, target_(target)
			, rgn_()
			, ro_(rgn_())
		{
			reset();
		}

		abl_exporter::~abl_exporter()
		{
		}

		void abl_exporter::reset()
		{
			ro_.reset(rgn_(), 0);
		}

		void abl_exporter::read(cyng::tuple_t const& msg, std::size_t idx)
		{
			read_msg(msg.begin(), msg.end(), idx);
		}

		void abl_exporter::read_msg(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end, std::size_t idx)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 5, "SML message");

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
			BOOST_ASSERT_MSG(choice.size() == 2, "CHOICE");
			if (choice.size() == 2)
			{
				ro_.set_value("code", choice.front());
				read_body(choice.front(), choice.back());
			}

			//
			//	(6) CRC16
			//
			ro_.set_value("crc16", *pos);
		}

		void abl_exporter::read_body(cyng::object type, cyng::object body)
		{
			auto code = cyng::value_cast<std::uint16_t>(type, 0);
			//node.append_attribute("type").set_value(messages::name(code));

			cyng::tuple_t tpl;
			tpl = cyng::value_cast(body, tpl);

			switch (code)
			{
			case BODY_OPEN_REQUEST:
				read_public_open_request(tpl.begin(), tpl.end());
				break;

			case BODY_OPEN_RESPONSE:

				//
				//	set default readout time
				//
				ro_.set_value("roTime", cyng::make_now());
				read_public_open_response(tpl.begin(), tpl.end());
				break;

			case BODY_CLOSE_REQUEST:
				//cyng::xml::write(node.append_child("data"), body);
				break;
			case BODY_CLOSE_RESPONSE:
				//cyng::xml::write(node.append_child("data"), body);
				//if (ofstream_.is_open())	ofstream_.close();
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
				read_get_profile_list_response(tpl.begin(), tpl.end());
				break;
			case BODY_GET_PROC_PARAMETER_REQUEST:
				//cyng::xml::write(node.append_child("data"), body);
				break;
			case BODY_GET_PROC_PARAMETER_RESPONSE:
				read_get_proc_parameter_response(tpl.begin(), tpl.end());
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
				read_attention_response(tpl.begin(), tpl.end());
				break;
			default:
				//cyng::xml::write(node.append_child("data"), body);
				break;
			}
		}

		void abl_exporter::read_public_open_request(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
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
			ro_.set_value("reqFileId", *pos++);

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

		}

		void abl_exporter::read_public_open_response(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 6, "Public Open Response");


			//	codepage "ISO 8859-15"
			ro_.set_value("codepage", *pos++);

			//
			//	clientId (MAC)
			//	Typically 7 bytes to identify gateway/MUC
			//	"client", "clientId"
			//
			read_client_id(*pos++);

			//
			//	reqFileId
			//
			read_string("reqFileId", *pos++);

			//
			//	serverId
			//	"server", "serverId"
			//
			read_server_id(*pos++);

			//
			//	refTime
			//
			ro_.set_value("refTime", *pos++);

			//	sml-Version: default = 1
			ro_.set_value("SMLVersion", *pos++);

		}

		void abl_exporter::read_get_profile_list_response(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 9, "Get Profile List Response");

			const std::string server_id = from_server_id(ro_.server_id_);
			const std::string gw_id = cyng::io::to_hex(ro_.client_id_);	//	gateway ID (e.g. 0500153b021774)
			

			//
			//	serverId
			//	ro_.server_id_
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

			std::ofstream of((root_dir_ / get_abl_filename(prefix_, suffix_, gw_id, server_id)).string(), std::ios::app | std::ios::binary);
			//ofstream_.open((root_dir_ / get_abl_filename(prefix_ + server_id, suffix_, source_, channel_, target_)).string());

			if (of.is_open()) {

				auto const obj = ro_.get_value("actTime");
				std::chrono::system_clock::time_point act_time = cyng::value_cast(obj, std::chrono::system_clock::now());
				std::tm const act_utc = cyng::chrono::make_utc_tm(act_time);

				of
					<< "[HEADER]"
					<< eol_	//	force windows format
					<< "PROT = 0"
					<< eol_	//	force windows format
					<< "MAN1 = "
					<< get_manufacturer(ro_.server_id_)
					<< eol_	//	force windows format
					<< "ZNR1 = "	//03685319"
					<< get_serial(ro_.server_id_)
					<< eol_	//	force windows format
					<< "DATE = "	//18.12.18 UTC"
					<< std::setfill('0')
					<< std::setw(2)
					<< cyng::chrono::day(act_utc)
					<< "."
					<< std::setw(2)
					<< cyng::chrono::month(act_utc)
					<< "."
					<< std::setw(2)
					<< (cyng::chrono::year(act_utc) - 2000)
					<< " UTC"
					<< eol_	//	force windows format
					<< "TIME = "	//11.13.38 UTC"
					<< std::setw(2)
					<< cyng::chrono::hour(act_utc)
					<< "."
					<< std::setw(2)
					<< cyng::chrono::minute(act_utc)
					<< "."
					<< std::setw(2)
					<< cyng::chrono::second(act_utc)
					<< " UTC"
					<< eol_	//	force windows format
					<< eol_	//	force windows format
					<< "[DATA]"
					<< eol_	//	force windows format
					<< get_manufacturer(ro_.server_id_)
					<< eol_	//	force windows format
					<< OBIS_SERIAL_NR
					<< '('
					<< get_serial(ro_.server_id_)
					<< ")"
					<< eol_	//	force windows format
					;
			}

			//	period-List (1)
			//	first we make the TSMLGetProfileListResponse complete, then 
			//	the we store the entries in TSMLPeriodEntry.
			cyng::tuple_t tpl;
			tpl = cyng::value_cast(*pos++, tpl);
			read_period_list(of, path, tpl.begin(), tpl.end());

			//	rawdata
			ro_.set_value("rawData", *pos++);

			if (of.is_open()) {
				of
					<< "Raw"
					<< '('
					;

				if (!pos->is_null()) {
					of << cyng::io::to_str(*pos);
				}
				else {
					//	random value
// 					of << 191677488;
					of << 0;
				}
				of
					<< ')'
					<< eol_	//	force windows format
					<< '!'
					<< eol_	//	force windows format
					;
			}

			//	periodSignature
			ro_.set_value("signature", *pos++);
		}

		void abl_exporter::read_get_proc_parameter_response(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
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

			//
			//	parameterTree
			//
			cyng::tuple_t tpl;
			tpl = cyng::value_cast(*pos++, tpl);
			read_param_tree(0, tpl.begin(), tpl.end());

		}

		void abl_exporter::read_attention_response(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
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

		}

		void abl_exporter::read_param_tree(std::size_t depth
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
			read_parameter(code, *pos++);

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
				read_param_tree(++depth, tmp.begin(), tmp.end());
			}
		}

		void abl_exporter::read_period_list(std::ofstream& of
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
				read_period_entry(of, path, counter, tpl.begin(), tpl.end());
				++counter;

			}
		}

		void abl_exporter::read_period_entry(std::ofstream& of
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
			obis const code = read_obis(*pos++);

			//
			//	unit (see sml_unit_enum)
			//
			auto const unit = read_unit("SMLUnit", *pos++);

			//
			//	scaler
			//
			auto const scaler = read_scaler(*pos++);

			//
			//	value
			//
			auto const type_tag = read_value(code, scaler, unit, *pos++);

			//
			//	valueSignature
			//
			ro_.set_value("signature", *pos++);

			//
			//	Skip all "counter" data.
			//	Write physical data only.
			//

			if (of.is_open() 
				&& (code.get_medium() < 129) 
				&& (code.get_storage() != 0)
				&& (code != OBIS_SERIAL_NR)
				&& (code != OBIS_SERIAL_NR_SECOND)) {

				//	filter 
				//	skip the following values
				//	0-0:96.1.0*255 - 00 00 60 01 00 FF - manufacturer ID (OBIS_SERIAL_NR) - is explicitely written at start of file
				//	0-0:96.1.1*255 - 00 00 60 01 01 FF - Seriennummer (OBIS_SERIAL_NR_SECOND)

				of
					<< to_string(code)
					<< '('
					<< cyng::value_cast<std::string>(ro_.get_value("value"), "0")
					;

				//	7-1:3.0.0*255(58634.69*m3)
				//	0-1:97.97.0*255(0)
				if (unit != node::mbus::COUNT)
				{
					of
						<< '*'
						<< node::mbus::get_unit_name(unit)
						;
				}

				of
					<< ')'
					<< eol_	//	force windows format
					<< std::flush
					;
			}
		}


		void abl_exporter::read_time(std::string const& name, cyng::object obj)
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
				}
				break;
				case TIME_SECINDEX:
					ro_.set_value(name, choice.back());
					break;
				default:
					break;
				}
			}

		}

		std::vector<obis> abl_exporter::read_param_tree_path(cyng::object obj)
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

		obis abl_exporter::read_obis(cyng::object obj)
		{
			cyng::buffer_t tmp;
			tmp = cyng::value_cast(obj, tmp);

			return obis(tmp);
		}

		std::uint8_t abl_exporter::read_unit(std::string const& name, cyng::object obj)
		{
			std::uint8_t unit = cyng::value_cast<std::uint8_t>(obj, 0);
			ro_.set_value(name, obj);
			return unit;
		}

		std::string abl_exporter::read_string(std::string const& name, cyng::object obj)
		{
			cyng::buffer_t buffer;
			buffer = cyng::value_cast(obj, buffer);
			const auto str = cyng::io::to_ascii(buffer);
			ro_.set_value(name, cyng::make_object(str));
			return str;
		}

		std::string abl_exporter::read_server_id(cyng::object obj)
		{
			ro_.server_id_ = cyng::value_cast(obj, ro_.server_id_);
			return from_server_id(ro_.server_id_);
		}

		std::string abl_exporter::read_client_id(cyng::object obj)
		{
			ro_.client_id_ = cyng::value_cast(obj, ro_.client_id_);
			return from_server_id(ro_.client_id_);
		}

		std::int8_t abl_exporter::read_scaler(cyng::object obj)
		{
			return cyng::numeric_cast<std::int8_t>(obj, 0);
		}

		std::size_t abl_exporter::read_value(obis code, std::int8_t scaler, std::uint8_t unit, cyng::object obj)
		{
			//
			//	write value
			//
			const auto type_tag = obj.get_class().tag();
			if (type_tag == cyng::TC_BUFFER) {
				//
				//	write binary data (octets) as hex string
				//
				cyng::buffer_t buffer;
				buffer = cyng::value_cast(obj, buffer);
				const auto raw = cyng::io::to_hex(buffer);
				ro_.set_value("raw", cyng::make_object(raw));
			}
			else {
				ro_.set_value("raw", obj);
			}

			if (code == OBIS_DATA_MANUFACTURER)
			{
				cyng::buffer_t buffer;
				buffer = cyng::value_cast(obj, buffer);
				const auto manufacturer = cyng::io::to_ascii(buffer);
				ro_.set_value("value", cyng::make_object(manufacturer));
				ro_.set_value("type", cyng::make_object("manufacturer"));
			}
			else if (code == OBIS_CURRENT_UTC)
			{
				if (type_tag == cyng::TC_TUPLE)
				{
					read_time("value", obj);
					read_time("roTime", obj);
				}
				else
				{
					const auto tm = cyng::value_cast<std::uint32_t>(obj, 0);
					const auto tp = std::chrono::system_clock::from_time_t(tm);
					ro_.set_value("value", cyng::make_object(tp));
					ro_.set_value("roTime", cyng::make_object(tp));
				}
				ro_.set_value("type", cyng::make_object("roTime"));
			}
			else if (code == OBIS_ACT_SENSOR_TIME)
			{
				const auto tm = cyng::value_cast<std::uint32_t>(obj, 0);
				const auto tp = std::chrono::system_clock::from_time_t(tm);
				ro_.set_value("value", cyng::make_object(tp));
				ro_.set_value("type", cyng::make_object("actTime"));
			}
			else if (code == OBIS_SERIAL_NR)
			{
				cyng::buffer_t buffer;
				buffer = cyng::value_cast(obj, buffer);
				std::reverse(buffer.begin(), buffer.end());
				const auto serial_nr = cyng::io::to_hex(buffer);
				ro_.set_value("value", cyng::make_object(serial_nr));
				ro_.set_value("type", cyng::make_object("serialNr"));
			}
			else if (code == OBIS_SERIAL_NR_SECOND)
			{
				cyng::buffer_t buffer;
				buffer = cyng::value_cast(obj, buffer);
				std::reverse(buffer.begin(), buffer.end());
				const auto serial_nr = cyng::io::to_hex(buffer);
				ro_.set_value("value", cyng::make_object(serial_nr));
				ro_.set_value("type", cyng::make_object("serialNr2"));
			}
			else if (code == OBIS_MBUS_STATE)
			{
				//	see EN13757-3
				//std::uint8_t status = cyng::value_cast<std::uint8_t>(obj, 0u);
				//ro_.set_value("MBusPermanentError", cyng::make_object(((status & 0x08) == 0x08)));
				//ro_.set_value("MBusTemporaryError", cyng::make_object(((status & 0x10) == 0x10)));
				ro_.set_value("value", obj);
				ro_.set_value("type", cyng::make_object("MBus"));
			}
			else
			{
				if (scaler != 0)
				{
					switch (type_tag)
					{
					case cyng::TC_INT64:
					{
						const std::int64_t value = cyng::value_cast<std::int64_t>(obj, 0);
						const auto str = scale_value(value, scaler);
						ro_.set_value("value", cyng::make_object(str));
						ro_.set_value("type", cyng::make_object("valueScaled64"));
					}
					break;
					case cyng::TC_INT32:
					{
						const std::int32_t value = cyng::value_cast<std::int32_t>(obj, 0);
						const auto str = scale_value(value, scaler);
						ro_.set_value("value", cyng::make_object(str));
						ro_.set_value("type", cyng::make_object("valueScaled32"));
					}
					break;
					default:
						ro_.set_value("value", obj);
						ro_.set_value("type", cyng::make_object("value"));
						break;
					}
				}
				else
				{
					ro_.set_value("value", obj);
					ro_.set_value("type", cyng::make_object("value"));
				}
			}

			return type_tag;
		}

		void abl_exporter::read_parameter(obis code, cyng::object obj)
		{
			cyng::tuple_t tpl;
			tpl = cyng::value_cast(obj, tpl);
			if (tpl.size() == 2)
			{
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
					read_time("parTime", tpl.back());
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

		std::string get_manufacturer(cyng::buffer_t const& buffer)
		{
			if (is_mbus(buffer)) {
				switch (buffer.back()) {
				case 3:		return "/GWF6\\M07Coder";
				case 7:		return "/GWF6\\M07Coder";
				case 8:		return "/GWF6\\M08Coder";
				default:	return "/EMH6\\M01GWF-ED300L";
				}
			}
			auto const srn = get_serial(buffer);


			if (boost::algorithm::equals(srn, "15036627"))	return "/GWF6\\M07Coder";
			if (boost::algorithm::equals(srn, "17001265"))	return "/GWF6\\M07Coder";

			if (boost::algorithm::equals(srn, "11522430"))	return "/GWF6\\M08Coder";
			if (boost::algorithm::equals(srn, "11549631"))	return "/GWF6\\M08Coder";

			return "/EMH6\\M01GWF-ED300L";
		}

		boost::filesystem::path get_abl_filename(std::string prefix
			, std::string suffix
			, std::string gw
			, std::string server_id)
		{
			//	example: push_20181218T191428.0--0500153b018ee9--01-a815-90870704-01-02.abl
			//	prefix datetime -- gateway ID -- server ID . suffix
			
			auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
			std::tm time = cyng::chrono::convert_utc(tt);

			std::stringstream ss;
			ss
				<< std::setfill('0')
				<< prefix
				<< std::dec
				<< cyng::chrono::year(time)
				<< std::setw(2)
				<< cyng::chrono::month(time)
				<< std::setw(2)
				<< cyng::chrono::day(time)
				<< 'T'
				<< std::setw(2)
				<< cyng::chrono::hour(time)
				<< std::setw(2)
				<< cyng::chrono::minute(time)
				<< std::setw(2)
				<< cyng::chrono::second(time)
				<< ".0"
				<< '-'
				<< '-'
				<< gw
				<< '-'
				<< '-'
				<< server_id
				<< '.'
				<< suffix
				;
			return ss.str();
		}


	}	//	sml
}

