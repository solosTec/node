/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#include "processor.h"
#include <smf/cluster/generator.h>
#include <smf/lora/payload/parser.h>
#include <cyng/vm/generator.h>
#include <cyng/table/key.hpp>
#include <cyng/table/body.hpp>
#include <cyng/parser/chrono_parser.h>
#include <cyng/tuple_cast.hpp>

namespace node
{
	processor::processor(cyng::logging::log_ptr logger, cyng::io_service_t& ios, boost::uuids::uuid tag, bus::shared_type bus, std::ostream& ostream, std::ostream& estream)
		: logger_(logger)
		, vm_(ios, tag, ostream, estream)
		, bus_(bus)
		, rgn_()
	{
		vm_.register_function("https.post.xml", 3, std::bind(&processor::https_post_xml, this, std::placeholders::_1));
		vm_.register_function("https.launch.session.plain", 0, std::bind(&processor::https_launch_session_plain, this, std::placeholders::_1));
		vm_.register_function("https.eof.session.plain", 0, std::bind(&processor::https_eof_session_plain, this, std::placeholders::_1));

		bus_->vm_.register_function("bus.res.push.data", 0, std::bind(&processor::res_push_data, this, std::placeholders::_1));
	}

	cyng::controller& processor::vm()
	{
		return vm_;
	}

	void processor::https_post_xml(cyng::context& ctx)
	{
		//	[<!259:plain-session>,/LoRa,<?xml version=\"1.0\" ...]
		//
		//	* session object
		//	* target path
		//	* content [string]
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_INFO(logger_, "https.post.xml " << cyng::io::to_str(frame));

		auto ptr = cyng::object_cast<std::string>(frame.at(2));
		if (ptr != nullptr)
		{
			pugi::xml_document doc;
			const auto result = doc.load_buffer(ptr->data(), ptr->size(), pugi::parse_default, pugi::encoding_auto);
			if (result.status == pugi::status_ok)
			{ 
				//
				//	get root node
				//
				auto node = doc.child("DevEUI_uplink");
				if (!node.empty())
				{
					CYNG_LOG_TRACE(logger_, "XML POST parsed uplink message: "
						<< result.description());
					//
					//	DevEUI_uplink
					//
					process_uplink_msg(doc, node);
					return;
				}
				node = doc.child("DevEUI_location");
				if (!node.empty())
				{
					CYNG_LOG_TRACE(logger_, "XML POST parsed localisation message: "
						<< result.description());

					//
					//	DevEUI_location
					//	This message will be send as soon as the LRC has calculated the location.
					//
					process_localisation_msg(doc, node);
					return;
				}

				CYNG_LOG_WARNING(logger_, "unknown LoRa message type "
					<< result.description());

			}
			else
			{
				CYNG_LOG_ERROR(logger_, "paring XML POST failed: "
					<< result.description());

			}
		}
	}

	void processor::https_launch_session_plain(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_INFO(logger_, "https.launch.session.plain " << cyng::io::to_str(frame));

	}

	void processor::https_eof_session_plain(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_INFO(logger_, "https.eof.session.plain " << cyng::io::to_str(frame));

	}


	void processor::res_push_data(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		//	[2,setup,TLoraUplink,1]
		//
		//	* seq (cluster)
		//	* class name
		//	* channel
		//	* target counter
		//	
		auto tpl = cyng::tuple_cast<
			std::size_t,			//	[0] cluster seq
			std::string,			//	[1] class name
			std::string,			//	[2] channel name
			std::size_t				//	[3] target counter
		>(frame);

		if (std::get<3>(tpl) == 0)
		{
			CYNG_LOG_WARNING(logger_, "bus.res.push.data "
				<< std::get<1>(tpl)
				<< ':'
				<< std::get<3>(tpl)
				<< " reached no targets");

		}
		else
		{
			CYNG_LOG_INFO(logger_, "bus.res.push.data " 
				<< std::get<1>(tpl) 
				<< ':'
				<< std::get<2>(tpl) 
				<< " reached "
				<< std::get<3>(tpl)
				<< " targets" );
		}
	}

	void processor::process_uplink_msg(pugi::xml_document const& doc, pugi::xml_node node)
	{
		const std::string dev_eui(node.child("DevEUI").child_value());

		CYNG_LOG_TRACE(logger_, "DevEUI: " << dev_eui);

		auto tp = cyng::parse_rfc3339_obj(node.child("Time").child_value());
		CYNG_LOG_TRACE(logger_, "roTime: " 
			<< node.child("Time").child_value()
			<< " - "
			<< cyng::io::to_str(tp));

		//
		//	push meta data
		//
		bus_->vm_.async_run(bus_req_push_data("setup"
			, "TLoraUplink"
			, false		//	single
			, cyng::table::key_generator(rgn_())
			, cyng::table::data_generator(dev_eui
				, tp //	Time
				, static_cast<std::uint16_t>(node.child("FPort").text().as_int())	//	FPort
				, static_cast<std::int32_t>(node.child("FCntUp").text().as_int())	//	FCntUp
				, static_cast<std::int32_t>(node.child("ADRbit").text().as_int())	//	ADRbit
				, static_cast<std::int32_t>(node.child("MType").text().as_int())	//	MType
				, static_cast<std::int32_t>(node.child("FCntDn").text().as_int())	//	FCntDn
				, node.child("payload_hex").child_value()	//	payload_hex
				, node.child("mic_hex").child_value()
				, static_cast<double>(node.child("LrrRSSI").text().as_double())	//	LrrRSSI
				, static_cast<double>(node.child("LrrSNR").text().as_double())	//	LrrSNR
				, node.child("SpFact").child_value()	//	SpFact
				, node.child("SubBand").child_value()	//	SubBand
				, node.child("Channel").child_value()	//	Channel
				, static_cast<std::int32_t>(node.child("DevLrrCnt").text().as_int())	//	DevLrrCnt
				, node.child("Lrrid").child_value()	//	Lrrid
				, node.child("CustomerID").child_value()	//	CustomerID
				, static_cast<double>(node.child("LrrLAT").text().as_double())	//	LrrLAT
				, static_cast<double>(node.child("LrrLON").text().as_double())	//	LrrLON
			)
			, bus_->vm_.tag()));

		//
		//	extract payload
		//
		cyng::buffer_t payload = node::lora::parse_payload(node.child("payload_hex").child_value());
		if (!payload.empty())
		{
			CYNG_LOG_TRACE(logger_, "meter ID: " << node::lora::meter_id(payload));
			CYNG_LOG_TRACE(logger_, "value: " << lora::value(lora::vif(payload), lora::volume(payload)));
			CYNG_LOG_TRACE(logger_, "CRC: " << (node::lora::crc_ok(payload) ? "OK" : "ERROR"));
			if (true)
			{
				std::string file_name_pattern = dev_eui + "--" + node::lora::meter_id(payload) + "--%%%%-%%%%-%%%%-%%%%.xml";
				const auto p = boost::filesystem::unique_path(file_name_pattern);
				CYNG_LOG_TRACE(logger_, "write " << p.string());
				doc.save_file(p.c_str(), PUGIXML_TEXT("  "));
			}
		}
		else
		{
			std::string file_name_pattern = "LoRa--invalid.payload--%%%%-%%%%-%%%%-%%%%.xml";
			const auto p = boost::filesystem::unique_path(file_name_pattern);
			CYNG_LOG_TRACE(logger_, "write " << p.string());
			doc.save_file(p.c_str(), PUGIXML_TEXT("  "));
		}


	}

	void processor::process_localisation_msg(pugi::xml_document const& doc, pugi::xml_node node)
	{

	}

	void processor::write_db(pugi::xml_node node, cyng::buffer_t const& payload)
	{
		//auto s = pool_.get_session();

		//
		//
		//	generate Meta data
		//const TLoraMeta tbl_meta;
		//auto cmd_meta = cyng::sql::factory(tbl_meta, cyng::sql::custom_callback());
		//cmd_meta->insert(s.get_dialect());

		////	INSERT INTO TLoraMeta (ident, roTime, DevEUI, FPort, FCntUp, ADRbit, MType, FCntDn, payload, mic, LrrRSSI, LrrSNR, SpFact, SubBand, Channel, DevLrrCnt, Lrrid, CustomerID) VALUES (?, julianday(?), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
		//const std::string sql_meta = cmd_meta->to_str();
		//CYNG_LOG_TRACE(logger_, sql_meta);

		//auto stmt_meta = s.create_statement();
		//std::pair<int, bool> prep_meta = stmt_meta->prepare(sql_meta);
		//if (prep_meta.second)
		//{
		//	cyng::uuid_random_factory uuid_gen;
		//	const auto uuid = uuid_gen();
		//	stmt_meta->push(uuid, 0)	//	ident
		//		.push(cyng::io::parse_rfc3339_obj(node.child("Time").child_value()), 0)	//	Time
		//		.push(cyng::io::parse_mac_obj(node.child("DevEUI").child_value()), 0)	//	DevEUI
		//		.push(cyng::numeric_factory_cast<std::uint16_t>(node.child("FPort").text().as_int()), 0)	//	FPort
		//		.push(cyng::numeric_factory_cast<std::int32_t>(node.child("FCntUp").text().as_int()), 0)	//	FCntUp
		//		.push(cyng::numeric_factory_cast<std::int32_t>(node.child("ADRbit").text().as_int()), 0)	//	ADRbit
		//		.push(cyng::numeric_factory_cast<std::int32_t>(node.child("MType").text().as_int()), 0)	//	MType
		//		.push(cyng::numeric_factory_cast<std::int32_t>(node.child("FCntDn").text().as_int()), 0)	//	FCntDn
		//		.push(cyng::string_factory(node.child("payload_hex").child_value()), 20)	//	payload_hex
		//		.push(cyng::string_factory(node.child("mic_hex").child_value()), 8)	//	payload_hex
		//																			//	skip <Lrcid>
		//		.push(cyng::numeric_factory_cast<std::int32_t>(node.child("LrrRSSI").text().as_double()), 0)	//	LrrRSSI
		//		.push(cyng::numeric_factory_cast<std::int32_t>(node.child("LrrSNR").text().as_double()), 0)	//	LrrSNR
		//		.push(cyng::string_factory(node.child("SpFact").child_value()), 8)	//	SpFact
		//		.push(cyng::string_factory(node.child("SubBand").child_value()), 8)	//	SubBand
		//		.push(cyng::string_factory(node.child("Channel").child_value()), 8)	//	Channel
		//		.push(cyng::numeric_factory_cast<std::int32_t>(node.child("DevLrrCnt").text().as_int()), 0)	//	DevLrrCnt
		//		.push(cyng::string_factory(node.child("Lrrid").child_value()), 8)	//	Lrrid
		//		.push(cyng::string_factory(node.child("CustomerID").child_value()), 16)	//	CustomerID
		//		.push(cyng::numeric_factory_cast<std::int32_t>(node.child("LrrLAT").text().as_double()), 0)	//	LrrLAT
		//		.push(cyng::numeric_factory_cast<std::int32_t>(node.child("LrrLON").text().as_double()), 0)	//	LrrLON
		//		;

		//	stmt_meta->execute();
		//	stmt_meta->clear();

		//	//
		//	//
		//	//	generate Payload data
		//	const TLoraPayload tbl_payload;
		//	auto cmd_payload = cyng::sql::factory(tbl_payload, cyng::sql::custom_callback());
		//	cmd_payload->insert(s.get_dialect());

		//	//	INSERT INTO TLoraPayload (ident, type, manufacturer, id, medium, state, actd, vif, volume, afun, lifetime, semester, CRC) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
		//	const std::string sql_payload = cmd_payload->to_str();
		//	CYNG_LOG_TRACE(logger_, sql_payload);


		//	auto stmt_payload = s.create_statement();
		//	std::pair<int, bool> prep_payload = stmt_payload->prepare(sql_payload);
		//	if (prep_payload.second)
		//	{
		//		stmt_payload->push(uuid, 0)	//	ident
		//			.push(cyng::factory(noddy::lora::version(payload)), 0)	//	type
		//			.push(cyng::factory(noddy::lora::manufacturer(payload)), 3)	//	manufacturer
		//			.push(cyng::factory(noddy::lora::meter_id(payload)), 0)	//	id as string with a length of 8 bytes
		//			.push(cyng::factory(noddy::lora::medium(payload)), 0)	//	medium code
		//			.push(cyng::factory(noddy::lora::state(payload)), 0)	//	M-Bus state
		//			.push(cyng::factory(noddy::lora::actuality(payload)), 0)	//	actuality duration
		//			.push(cyng::factory(noddy::lora::vif(payload)), 0)	//	vif
		//			.push(cyng::factory(noddy::lora::volume(payload)), 0)	//	volume
		//			.push(cyng::factory(noddy::lora::value(noddy::lora::vif(payload), noddy::lora::volume(payload))), 11)	//	value
		//			.push(cyng::factory(noddy::lora::functions(payload)), 0)	//	afun
		//			.push(cyng::factory(noddy::lora::lifetime(payload)), 0)	//	lifetime
		//			.push(cyng::factory(noddy::lora::semester(noddy::lora::lifetime(payload))), 0)	//	semester
		//			.push(cyng::factory(noddy::lora::link_error(payload)), 0)	//	link error flag
		//			.push(cyng::factory(noddy::lora::crc_ok(payload)), 0)	//	CRC OK as boolean
		//			;

		//		stmt_payload->execute();
		//		stmt_payload->clear();
		//	}
		//	else
		//	{
		//		CYNG_LOG_ERROR(logger_, "prepare SQL statement failed: "
		//			<< sql_payload);
		//	}
		//}
		//else
		//{
		//	CYNG_LOG_ERROR(logger_, "prepare SQL statement failed: "
		//		<< sql_meta);

		//}
	}

}
