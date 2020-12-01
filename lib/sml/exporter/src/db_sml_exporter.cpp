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
#include <smf/sml/scaler.h>
#include <smf/mbus/units.h>

#include <cyng/io/io_buffer.h>
#include <cyng/io/io_chrono.hpp>
#include <cyng/io/serializer.h>
#include <cyng/numeric_cast.hpp>
#include <cyng/intrinsics/traits/tag_names.hpp>
#include <cyng/sql.h>
#include <cyng/db/interface_statement.h>
#include <cyng/factory.h>

#include <boost/uuid/nil_generator.hpp>
#include <boost/core/ignore_unused.hpp>

namespace node
{
	namespace sml
	{

		db_exporter::db_exporter(cyng::table::meta_map_t const& mt
			, std::string const& schema)
		: mt_(mt)
			, schema_(schema)
			, source_(0)
			, channel_(0)
			, target_()
			, rgn_()
			, ro_()
		{
			reset();
		}

		db_exporter::db_exporter(cyng::table::meta_map_t const& mt
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
			, ro_()
		{
			reset();
		}


		void db_exporter::reset()
		{
			ro_.reset();
		}

		void db_exporter::write(cyng::db::session sp, cyng::tuple_t const& msg)
		{
			//std::pair<std::uint16_t, cyng::tuple_t> 
			auto const choice = ro_.read_msg(msg.begin(), msg.end());
			read_body(sp, choice.first, choice.second);
		}

		void db_exporter::read_body(cyng::db::session sp, message_e code, cyng::tuple_t const& tpl)
		{
			switch (code)
			{
			case message_e::OPEN_REQUEST:
				//
				//	reset context
				//
				reset();
				ro_.read_public_open_request(tpl.begin(), tpl.end());
				break;
			case message_e::OPEN_RESPONSE:

				//
				//	set default readout time
				//
				ro_.set_value("roTime", cyng::make_now());
				ro_.read_public_open_response(tpl.begin(), tpl.end());
				break;
			case message_e::CLOSE_REQUEST:
				break;
			case message_e::CLOSE_RESPONSE:
				break;
			case message_e::GET_PROFILE_PACK_REQUEST:
				break;
			case message_e::GET_PROFILE_PACK_RESPONSE:
				break;
			case message_e::GET_PROFILE_LIST_REQUEST:
				break;
			case message_e::GET_PROFILE_LIST_RESPONSE:
				read_get_profile_list_response(sp, tpl.begin(), tpl.end());
				break;
			case message_e::GET_PROC_PARAMETER_REQUEST:
				break;
			case message_e::GET_PROC_PARAMETER_RESPONSE:
				read_get_proc_parameter_response(sp, tpl.begin(), tpl.end());
				break;
			case message_e::SET_PROC_PARAMETER_REQUEST:
				break;
			case message_e::SET_PROC_PARAMETER_RESPONSE:
				break;
			case message_e::GET_LIST_REQUEST:
				break;
			case message_e::GET_LIST_RESPONSE:
				break;
			case message_e::ATTENTION_RESPONSE:
				read_attention_response(tpl.begin(), tpl.end());
				break;
			default:
				break;
			}
		}

		bool db_exporter::read_get_profile_list_response(cyng::db::session sp, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			////	std::pair<obis_path_t, cyng::param_map_t>
			//auto const r = ro_.read_get_profile_list_response(pos, end);
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 9, "Get Profile List Response");
			if (count != 9)	return false;
			
			//
			//	serverId
			//
			auto const srv_id = read_server_id(*pos++);
			boost::ignore_unused(srv_id);

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

			return store_meta(sp
				, rgn_()
				, ro_.trx_
				, 0u	//	ToDo: change database scheme
				, ro_.get_value("roTime")
				, ro_.get_value("actTime")
				, ro_.get_value("valTime")
				, ro_.client_id_	//	gateway - formatted (00:15:3b:02:29:7e)
				, ro_.server_id_	//	server - formatted
				, ro_.get_value("status")
				, path.empty() ? obis() : path.front());
		}

		void db_exporter::read_get_proc_parameter_response(cyng::db::session sp, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 3, "Get Proc Parameter Response");
			if (count != 3)	return;
			
			//
			//	serverId
			//
			auto const srv_id = read_server_id(*pos++);
			boost::ignore_unused(srv_id);

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
			if (count != 4)	return;

			//
			//	serverId
			//
			auto const srv_id = read_server_id(*pos++);
			boost::ignore_unused(srv_id);

			//
			//	attentionNo (OBIS)
			//
			obis code = read_obis(*pos++);
			boost::ignore_unused(code);

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
			if (count != 3)	return;
			
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
			boost::ignore_unused(count);

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

		bool db_exporter::read_period_entry(cyng::db::session sp
			, std::vector<obis> const& path
			, std::size_t index
			, cyng::tuple_t::const_iterator pos
			, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 5, "Period Entry");
			if (count != 5)	return false;

			//
			//	object name
			//
			obis code = read_obis(*pos++);

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

			return store_data(sp
				, rgn_()
				, ro_.trx_
				,  0u // ToDo: change database scheme
				, code	//	.to_buffer()
				, unit
				, node::mbus::get_unit_name(unit)
				, cyng::traits::get_type_name(type_tag)	//	CYNG data type name
				, scaler	//	scaler
				, ro_.get_value("raw")	//	raw value
				, ro_.get_value("value"));	//	formatted value
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
			ro_.server_id_ = cyng::value_cast(obj, ro_.server_id_);
			return from_server_id(ro_.server_id_);
		}

		std::string db_exporter::read_client_id(cyng::object obj)
		{
			ro_.client_id_ = cyng::value_cast(obj, ro_.client_id_);
			return from_server_id(ro_.client_id_);
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
				//ro_.set_value("type", cyng::make_object("manufacturer"));
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
				//ro_.set_value("type", cyng::make_object("roTime"));
			}
			else if (code == OBIS_ACT_SENSOR_TIME)
			{
				const auto tm = cyng::value_cast<std::uint32_t>(obj, 0);
				const auto tp = std::chrono::system_clock::from_time_t(tm);
				ro_.set_value("value", cyng::make_object(tp));
				//ro_.set_value("type", cyng::make_object("actTime"));
			}
			else if (code == OBIS_SERIAL_NR)
			{ 
				cyng::buffer_t buffer;
				buffer = cyng::value_cast(obj, buffer);
				std::reverse(buffer.begin(), buffer.end());
				const auto serial_nr = cyng::io::to_hex(buffer);
				ro_.set_value("value", cyng::make_object(serial_nr));
				//ro_.set_value("type", cyng::make_object("serialNr"));
			}
			else if (code == OBIS_SERIAL_NR_SECOND)
			{
				cyng::buffer_t buffer;
				buffer = cyng::value_cast(obj, buffer);
				std::reverse(buffer.begin(), buffer.end());
				const auto serial_nr = cyng::io::to_hex(buffer);
				ro_.set_value("value", cyng::make_object(serial_nr));
				//ro_.set_value("type", cyng::make_object("serialNr2"));
			}
			else if (code == OBIS_MBUS_STATE)
			{
				//	see EN13757-3
				//std::uint8_t status = cyng::value_cast<std::uint8_t>(obj, 0u);
				//ro_.set_value("MBusPermanentError", cyng::make_object(((status & 0x08) == 0x08)));
				//ro_.set_value("MBusTemporaryError", cyng::make_object(((status & 0x10) == 0x10)));
				ro_.set_value("value", obj);
				//ro_.set_value("type", cyng::make_object("MBus"));
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
						//ro_.set_value("type", cyng::make_object("valueScaled64"));
					}
					break;
					case cyng::TC_INT32:
					{
						const std::int32_t value = cyng::value_cast<std::int32_t>(obj, 0);
						const auto str = scale_value(value, scaler);
						ro_.set_value("value", cyng::make_object(str));
						//ro_.set_value("type", cyng::make_object("valueScaled32"));
					}
					break;
					case cyng::TC_INT16:
					{
						const std::int16_t value = cyng::value_cast<std::int16_t>(obj, 0);
						const auto str = scale_value(value, scaler);
						ro_.set_value("value", cyng::make_object(str));
						//ro_.set_value("type", cyng::make_object("valueScaled16"));
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
					//node.append_attribute("parameter").set_value("interval");
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

		bool db_exporter::store_meta(cyng::db::session sp
			, boost::uuids::uuid pk
			, std::string const& trx
			, std::size_t idx
			, cyng::object obj_ro_time
			, cyng::object obj_act_time
			, cyng::object obj_val_time
			, cyng::buffer_t const& client_id	//	gateway - formatted
			, cyng::buffer_t const& server_id	//	server - formatted
			, cyng::object obj_status
			, obis profile)
		{

			cyng::sql::command cmd(mt_.find("TSMLMeta")->second, sp.get_dialect());
			auto const sql = cmd.insert()();
			auto stmt = sp.create_statement();
			std::pair<int, bool> r = stmt->prepare(sql);
			BOOST_ASSERT(r.second);
			boost::ignore_unused(r);	//	release version

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

				//cyng::buffer_t server, client;
				//server = cyng::value_cast(obj_server, server);
				//client = cyng::value_cast(obj_client, client);

				stmt->push(cyng::make_object(pk), 0)	//	ident
					.push(cyng::make_object(trx), 16)	//	trxID
					.push(cyng::make_object(idx), 0)	//	midx
					.push(obj_ro_time, 0)	//	roTime
					.push(obj_act_time, 0)	//	actTime
					.push(obj_val_time, 0)	//	valTime
					.push(cyng::make_object(cyng::io::to_hex(client_id)), 0)	//	gateway/client
																			//	clientId from_server_id
					.push(cyng::make_object(sml::from_server_id(server_id)), 0)	//	server
																				//	serverId
					.push(cyng::make_object(sml::get_serial(server_id)), 0)	//	meter
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

				auto obj_client = cyng::make_object(from_server_id(client_id));
				auto obj_server = cyng::make_object(from_server_id(server_id));

				//	bind parameters
				stmt->push(cyng::make_object(pk), 0)	//	pk
					.push(cyng::make_object(trx), 16)	//	trxID
					.push(cyng::make_object(idx), 0)	//	msgIdx
					.push(obj_ro_time, 0)	//	roTime
					.push(obj_act_time, 0)	//	actTime
					.push(obj_val_time, 0)	//	valTime
											//	client
					.push(obj_client, 0)	//	gateway
											//	server
					.push(obj_server, 0)	//	server
					.push(obj_status, 0)	//	status
					.push(cyng::make_object(source_), 0)	//	source
					.push(cyng::make_object(channel_), 0)	//	channel
					.push(cyng::make_object(target_), 32)	//	target
					.push(cyng::make_object(cyng::io::to_hex(profile.to_buffer())), 24)	//	OBIS
					;
			}

			const bool b = stmt->execute();
			stmt->clear();
			return b;
		}

		bool db_exporter::store_data(cyng::db::session sp
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
			auto const sql = cmd.insert()();
			//CYNG_LOG_INFO(logger_, "db.insert.data " << sql);
			auto stmt = sp.create_statement();
			std::pair<int, bool> r = stmt->prepare(sql);
			BOOST_ASSERT(r.first == 8);	//	11 parameters to bind
			BOOST_ASSERT(r.second);
			boost::ignore_unused(r);	//	release version
			

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

			const bool b = stmt->execute();
			stmt->clear();
			return b;

		}



	}	//	sml
}

