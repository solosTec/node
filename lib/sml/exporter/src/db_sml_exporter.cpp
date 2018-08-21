/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/sml/exporter/db_sml_exporter.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/obis_io.h>
#include <smf/sml/srv_id_io.h>
#include <smf/sml/units.h>
#include <smf/sml/scaler.h>

#include <cyng/io/io_buffer.h>
#include <cyng/io/io_chrono.hpp>
#include <cyng/io/serializer.h>
#include <cyng/numeric_cast.hpp>
#include <cyng/intrinsics/traits/tag_names.hpp>
#include <cyng/sql.h>
#include <cyng/db/interface_statement.h>
#include <cyng/factory.h>

#include <boost/uuid/nil_generator.hpp>

namespace node
{
	namespace sml
	{

		db_exporter::db_exporter(cyng::table::mt_table const& mt, std::string const& schema)
			: mt_(mt)
			, schema_(schema)
			, source_(0)
			, channel_(0)
			, target_()
			, rgn_()
			, ro_(rgn_())
		{
			reset();
		}

		db_exporter::db_exporter(cyng::table::mt_table const& mt
			, std::string const& schema
			, std::uint32_t source
			, std::uint32_t channel
			, std::string const& target)
		: mt_(mt)
			, schema_(schema)
			, source_(source)
			, channel_(channel)
			, target_(target)
			, rgn_()
			, ro_(rgn_())
		{
			reset();
		}


		void db_exporter::reset()
		{
			ro_.reset(rgn_(), 0);
		}

		void db_exporter::write(cyng::db::session sp, cyng::tuple_t const& msg, std::size_t idx)
		{
			read_msg(sp, msg.begin(), msg.end(), idx);
		}

		void db_exporter::read_msg(cyng::db::session sp, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end, std::size_t idx)
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
				read_body(sp, choice.front(), choice.back());
			}

			//
			//	(6) CRC16
			//
			ro_.set_value("crc16", *pos);
		}

		void db_exporter::read_body(cyng::db::session sp, cyng::object type, cyng::object body)
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
				read_get_profile_list_response(sp, tpl.begin(), tpl.end());
				break;
			case BODY_GET_PROC_PARAMETER_REQUEST:
				//cyng::xml::write(node.append_child("data"), body);
				break;
			case BODY_GET_PROC_PARAMETER_RESPONSE:
				read_get_proc_parameter_response(sp, tpl.begin(), tpl.end());
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

		void db_exporter::read_public_open_request(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 7, "Public Open Request");

			//
			//	reset context
			//
			reset();
			
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

		void db_exporter::read_public_open_response(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 6, "Public Open Response");

			//
			//	reset context
			//
			reset();
			
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

		}

		void db_exporter::read_get_profile_list_response(cyng::db::session sp, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
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
			read_period_list(sp, path, tpl.begin(), tpl.end());

			//	rawdata
			ro_.set_value("rawData", *pos++);

			//	periodSignature
			ro_.set_value("signature", *pos++);

			store_meta(sp
				, ro_.pk_
				, ro_.trx_
				, ro_.idx_
				, ro_.get_value("roTime")
				, ro_.get_value("actTime")
				, ro_.get_value("valTime")
				, ro_.get_value("client")	//	gateway
				, ro_.get_value("clientId")	//	gateway - formatted
				, ro_.get_value("server")	//	server
				, ro_.get_value("serverId")	//	server - formatted
				, ro_.get_value("status")
				, path.empty() ? obis() : path.front());
			//ctx.attach(cyng::generate_invoke("db.insert.meta"
			//	, ro_.pk_
			//	, ro_.trx_
			//	, ro_.idx_
			//	, ro_.get_value("roTime")
			//	, ro_.get_value("actTime")
			//	, ro_.get_value("valTime")
			//	, ro_.get_value("client")	//	gateway
			//	, ro_.get_value("clientId")	//	gateway - formatted
			//	, ro_.get_value("server")	//	server
			//	, ro_.get_value("serverId")	//	server - formatted
			//	, ro_.get_value("status")
			//	, source_
			//	, channel_
			//	, target_));

		}

		void db_exporter::read_get_proc_parameter_response(cyng::db::session sp, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
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

		void db_exporter::read_attention_response(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
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

		void db_exporter::read_param_tree(std::size_t depth
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

		void db_exporter::read_period_list(cyng::db::session sp
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
				read_period_entry(sp, path, counter, tpl.begin(), tpl.end());
				++counter;

			}
		}

		void db_exporter::read_period_entry(cyng::db::session sp
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
			const auto type_tag = read_value(code, scaler, unit, *pos++);

			//
			//	valueSignature
			//
			ro_.set_value("signature", *pos++);

			store_data(sp
				, ro_.pk_
				, ro_.trx_
				, ro_.idx_
				, code	//	.to_buffer()
				, unit
				, get_unit_name(unit)
				, cyng::traits::get_type_name(type_tag)	//	CYNG data type name
				, scaler	//	scaler
				, ro_.get_value("raw")	//	raw value
				, ro_.get_value("value"));	//	formatted value

			//ctx.attach(cyng::generate_invoke("db.insert.data"
			//	, ro_.pk_
			//	, ro_.trx_
			//	, ro_.idx_
			//	, code.to_buffer()
			//	, unit
			//	, get_unit_name(unit)
			//	, cyng::traits::get_type_name(type_tag)	//	CYNG data type name
			//	, scaler	//	scaler
			//	, ro_.get_value("raw")	//	raw value
			//	, ro_.get_value("value")));	//	formatted value
		}


		void db_exporter::read_time(std::string const& name, cyng::object obj)
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

		std::vector<obis> db_exporter::read_param_tree_path(cyng::object obj)
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

		obis db_exporter::read_obis(cyng::object obj)
		{
			cyng::buffer_t tmp;
			tmp = cyng::value_cast(obj, tmp);

			return obis(tmp);
		}

		std::uint8_t db_exporter::read_unit(std::string const& name, cyng::object obj)
		{
			std::uint8_t unit = cyng::value_cast<std::uint8_t>(obj, 0);
			//node.append_attribute("type").set_value(get_unit_name(unit));
			//node.append_child(pugi::node_pcdata).set_value(std::to_string(+unit).c_str());
			ro_.set_value(name, obj);
			return unit;
		}

		std::string db_exporter::read_string(std::string const& name, cyng::object obj)
		{
			cyng::buffer_t buffer;
			buffer = cyng::value_cast(obj, buffer);
			const auto str = cyng::io::to_ascii(buffer);
			ro_.set_value(name, cyng::make_object(str));
			return str;
		}

		std::string db_exporter::read_server_id(cyng::object obj)
		{
			cyng::buffer_t buffer;
			buffer = cyng::value_cast(obj, buffer);
			ro_.set_value("server", cyng::make_object(buffer));
			const auto str = from_server_id(buffer);
			ro_.set_value("serverId", cyng::make_object(str));
			return str;
		}

		std::string db_exporter::read_client_id(cyng::object obj)
		{
			cyng::buffer_t buffer;
			buffer = cyng::value_cast(obj, buffer);
			ro_.set_value("client", cyng::make_object(buffer));
			const auto str = from_server_id(buffer);
			ro_.set_value("clientId", cyng::make_object(str));
			return str;
		}

		std::int8_t db_exporter::read_scaler(cyng::object obj)
		{
// 			return cyng::value_cast<std::int8_t>(obj, 0);
 			return cyng::numeric_cast<std::int8_t>(obj, 0);
		}

		std::size_t db_exporter::read_value(obis code, std::int8_t scaler, std::uint8_t unit, cyng::object obj)
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

		void db_exporter::read_parameter(obis code, cyng::object obj)
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

		void db_exporter::store_meta(cyng::db::session sp
			, boost::uuids::uuid pk
			, std::string const& trx
			, std::size_t idx
			, cyng::object obj_ro_time
			, cyng::object obj_act_time
			, cyng::object obj_val_time
			, cyng::object obj_client		//	gateway
			, cyng::object obj_client_id	//	gateway - formatted
			, cyng::object obj_server		//	server
			, cyng::object obj_server_id	//	server - formatted
			, cyng::object obj_status
			, obis profile)
		{

			cyng::sql::command cmd(mt_.find("TSMLMeta")->second, sp.get_dialect());
			cmd.insert();
			auto sql = cmd.to_str();
			auto stmt = sp.create_statement();
			std::pair<int, bool> r = stmt->prepare(sql);
			BOOST_ASSERT(r.second);

			if (boost::algorithm::equals(schema_, "v4.0"))
			{
				//
				//	examples:
				//
				//	sqlite> select * from TSMLMeta where roTime > julianday('2018-07-01');
				//	ident|trxID|midx|roTime|actTime|valTime|gateway|server|meter|status|source|channel
				//	91e54897-68aa-477b-97ae-efe459c192be|618732131|1|2458300.51030093|2458300.51070602|63122537|0500153b025459|01-2d2c-40800078-1c-04|78008040|0|949333985|581869302
				//	fa6f80d6-2a18-4873-ae15-407629529757|535249935|1|2458300.51025463|2458300.51070602|81491646|0500153b02499e|01-a815-76007904-01-02|04790076|131712|545404204|581869302
				//	5e45f89d-9a36-4f9b-8e79-8fdd617ace8f|65331623|1|2458300.50975694|2458300.51070602|41419967|0500153b023b36|02-d81c-05252359-ff-02|59232505|0|-1579004998|581869302
				//	a21ee414-8442-4b5b-bab0-2a815a034e73|510800075|1|2458300.50994213|2458300.51070602|55253893|0500153b025182|01-a815-77007904-01-02|04790077|131712|-372047867|581869302
				//	1df825d4-93f7-4518-b493-030b150ea3fa|439163211|1|2458300.51001157|2458300.51070602|92472720|0500153b024974|01-a815-46007904-01-02|04790046|131712|-1946129057|581869302
				//	c664ca3b-4dbf-4473-a847-f404118ce3fc|612380399|1|2458300.51028935|2458300.51070602|89139036|0500153b024996|01-a815-73007904-01-02|04790073|131714|-133711905|581869302
				//	5d2f7607-21b4-4a0b-835e-055118d8feb9|523186555|3|2458300.51025463|2458300.51070602|81442784|0500153b024998|01-a815-75007904-01-02|04790075|131712|-30574576|581869302
				//	90202d0f-46df-4f4e-9327-cd77ef96d0bb|51786655|4|2458300.51037037|2458300.51070602|44248573|0500153b024952|01-a815-65957202-01-02|02729565|640|418932835|581869302
				//	f29ff3f0-f570-43a5-93f2-933838e42d1f|514196843|4|2458300.51011574|2458300.51070602|79963528|0500153b024972|01-a815-44007904-01-02|04790044|131778|537655879|581869302
				//	601a7c18-ee9a-4957-8d06-d255b6b42ca2|533395235|1|2458300.51017361|2458300.51070602|48450161|0500153b026d31|02-d81c-05252350-ff-02|50232505|0|-150802599|581869302
				//	b70815c1-74e9-4881-b9d8-00db2f9a7fe6|49606703|4|2458300.51018519|2458300.51070602|44489752|0500153b026d35|01-a815-78957202-01-02|02729578|640|1323567403|581869302

				cyng::buffer_t server, client;
				server = cyng::value_cast(obj_server, server);
				client = cyng::value_cast(obj_client, client);

				stmt->push(cyng::make_object(pk), 0)	//	ident
					.push(cyng::make_object(trx), 16)	//	trxID
					.push(cyng::make_object(idx), 0)	//	midx
					.push(obj_ro_time, 0)	//	roTime
					.push(obj_act_time, 0)	//	actTime
					.push(obj_val_time, 0)	//	valTime
					.push(cyng::make_object(cyng::io::to_hex(client)), 0)	//	gateway/client
																			//	clientId from_server_id
					.push(cyng::make_object(sml::from_server_id(server)), 0)	//	server
																				//	serverId
					.push(cyng::make_object(sml::get_serial(server)), 0)	//	meter
					.push(obj_status, 0)	//	status
					.push(cyng::make_object(source_), 0)	//	source
					.push(cyng::make_object(channel_), 0)	//	channel
					;
			}
			else
			{
				//
				//	examples:
				//
				//	pk|trxID|msgIdx|roTime|actTime|valTime|gateway|server|status|source|channel|target
				//	0bf4bfff-6a83-411d-ab60-1e98fc232fd2|51207|1|2458299.33280093|2458301.43333333|98573619|00:15:3b:02:29:7e|01-e61e-13090016-3c-07|0|-1324790809|-1649078317|water@solostec

				//	bind parameters
				stmt->push(cyng::make_object(pk), 0)	//	pk
					.push(cyng::make_object(trx), 16)	//	trxID
					.push(cyng::make_object(idx), 0)	//	msgIdx
					.push(obj_ro_time, 0)	//	roTime
					.push(obj_act_time, 0)	//	actTime
					.push(obj_val_time, 0)	//	valTime
											//	client
					.push(obj_client_id, 0)	//	gateway
											//	server
					.push(obj_server_id, 0)	//	server
					.push(obj_status, 0)	//	status
					.push(cyng::make_object(source_), 0)	//	source
					.push(cyng::make_object(channel_), 0)	//	channel
					.push(cyng::make_object(target_), 32)	//	target
					.push(cyng::make_object(cyng::io::to_hex(profile.to_buffer())), 24)	//	OBIS
					;
			}

			if (!stmt->execute())
			{
				//CYNG_LOG_ERROR(logger_, sql << " failed");

			}
			stmt->clear();
		}

		void db_exporter::store_data(cyng::db::session sp
				, boost::uuids::uuid pk
				, std::string const& trx
				, std::size_t idx
				, obis const& code
				, std::uint8_t unit
				, std::string unit_name
				, std::string type_name	//	CYNG data type name
				, std::int8_t scaler	//	scaler
				, cyng::object raw		//	raw value
				, cyng::object value)
		{
			cyng::sql::command cmd(mt_.find("TSMLData")->second, sp.get_dialect());
			cmd.insert();
			auto sql = cmd.to_str();
			//CYNG_LOG_INFO(logger_, "db.insert.data " << sql);
			auto stmt = sp.create_statement();
			std::pair<int, bool> r = stmt->prepare(sql);
			BOOST_ASSERT(r.first == 8);	//	11 parameters to bind
			BOOST_ASSERT(r.second);

			//cyng::buffer_t tmp = cyng::value_cast(frame.at(3), tmp);

			if (boost::algorithm::equals(schema_, "v4.0"))
			{
				//sml::obis code(cyng::value_cast(frame.at(3), tmp));
				stmt->push(cyng::make_object(pk), 36)	//	pk
					.push(cyng::make_object(sml::to_string(code)), 24)	//	OBIS
					.push(cyng::make_object(unit), 0)	//	unitCode
					.push(cyng::make_object(unit_name), 0)	//	unitName
					.push(cyng::make_object(type_name), 0)	//	SML data type
					.push(cyng::make_object(scaler), 0)	//	scaler
					.push(raw, 0)	//	value
					.push(value, 512)	//	result
					;
			}
			else
			{
				stmt->push(cyng::make_object(pk), 36)	//	pk
					.push(cyng::make_object(cyng::io::to_hex(code.to_buffer())), 24)	//	OBIS
					.push(cyng::make_object(unit), 0)	//	unitCode
					.push(cyng::make_object(unit_name), 0)	//	unitName
					.push(cyng::make_object(type_name), 0)	//	SML data type
					.push(cyng::make_object(scaler), 0)	//	scaler
					.push(raw, 0)	//	value
					.push(value, 512)	//	result
					;

			}

			if (!stmt->execute())
			{
				//CYNG_LOG_ERROR(logger_, sql << " failed with the following data set: " << cyng::io::to_str(frame));

			}
			stmt->clear();

		}


		db_exporter::readout::readout(boost::uuids::uuid pk)
			: pk_(pk)
			, idx_(0)
			, trx_()
			, values_()
		{}

		void db_exporter::readout::reset(boost::uuids::uuid pk, std::size_t idx)
		{
			pk_ = pk;
			idx_ = idx;
			trx_.clear();
			values_.clear();
		}

		db_exporter::readout& db_exporter::readout::set_trx(cyng::buffer_t const& buffer)
		{
			trx_ = cyng::io::to_ascii(buffer);
			return *this;
		}

		db_exporter::readout& db_exporter::readout::set_index(std::size_t idx)
		{
			idx_ = idx;
			return *this;
		}

		db_exporter::readout& db_exporter::readout::set_value(std::string const& name, cyng::object value)
		{
			values_[name] = value;
			return *this;
		}

		cyng::object db_exporter::readout::get_value(std::string const& name) const
		{
			auto pos = values_.find(name);
			return (pos != values_.end())
				? pos->second
				: cyng::make_object()
				;
		}


	}	//	sml
}

