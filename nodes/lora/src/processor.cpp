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
#include <cyng/parser/buffer_parser.h>
#include <cyng/tuple_cast.hpp>
#include <cyng/parser/mac_parser.h>
#include <cyng/io/serializer.h>
#include <cyng/io/io_buffer.h>

namespace node
{
	processor::processor(cyng::logging::log_ptr logger
		, bool keep_xml_files
		, cyng::store::db& cache
		, cyng::io_service_t& ios
		, boost::uuids::uuid tag
		, bus::shared_type bus
		, std::ostream& ostream
		, std::ostream& estream)
	: logger_(logger)
		, keep_xml_files_(keep_xml_files)
		, cache_(cache)
		, vm_(ios, tag, ostream, estream)
		, bus_(bus)
		, uidgen_()
	{
		vm_.register_function("https.launch.session.plain", 0, std::bind(&processor::https_launch_session_plain, this, std::placeholders::_1));
		vm_.register_function("https.eof.session.plain", 0, std::bind(&processor::https_eof_session_plain, this, std::placeholders::_1));

		
		bus_->vm_.register_function("http.session.launch", 3, [&](cyng::context& ctx) {
			//	[849a5b98-429c-431e-911d-18a467a818ca,false,127.0.0.1:61383]
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "http.session.launch " << cyng::io::to_str(frame));
		});
		
		
		//
		//	data upload
		//
		bus_->vm_.register_function("http.upload.start", 2, std::bind(&processor::http_upload_start, this, std::placeholders::_1));
		bus_->vm_.register_function("http.upload.progress", 4, std::bind(&processor::http_upload_progress, this, std::placeholders::_1));
		bus_->vm_.register_function("http.upload.data", 5, std::bind(&processor::http_upload_data, this, std::placeholders::_1));
		bus_->vm_.register_function("http.upload.complete", 4, std::bind(&processor::http_upload_complete, this, std::placeholders::_1));
		bus_->vm_.register_function("http.post.xml", 3, std::bind(&processor::http_post_xml, this, std::placeholders::_1));

		bus_->vm_.register_function("bus.res.push.data", 0, std::bind(&processor::res_push_data, this, std::placeholders::_1));

		//
		//	report library size
		//
		vm_.async_run(cyng::generate_invoke("log.msg.info", cyng::invoke("lib.size"), "callbacks registered"));		
		bus_->vm_.async_run(cyng::generate_invoke("log.msg.info", cyng::invoke("lib.size"), "callbacks registered"));		
	}

	cyng::controller& processor::vm()
	{
		return vm_;
	}

	void processor::http_post_xml(cyng::context& ctx)
	{
		//	 [f24f628e-d799-454b-b036-b9e0c0bd26f2,0000000b,/LoRa,859,<?xml versio...]
		//
		//	* session tag
		//	* request version 
		//	* target path
		//	* payload size
		//	* content [string]
		const cyng::vector_t frame = ctx.get_frame();
		//CYNG_LOG_INFO(logger_, "https.post.xml " << cyng::io::to_str(frame));

		auto ptr = cyng::object_cast<std::string>(frame.at(4));
		if (ptr != nullptr)
		{
			parse_xml(ptr);
		}
	}

	void processor::parse_xml(std::string const* ptr)
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

	void processor::http_upload_start(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,	//	[0] session tag
			std::uint32_t,		//	[1] HTTP version
			std::string,		//	[2] target
			std::uint32_t		//	[3] payload size
		>(frame);

		CYNG_LOG_TRACE(logger_, "http.upload.start - "
			<< std::get<0>(tpl)
			<< ", "
			<< std::get<2>(tpl)
			<< ", payload size: "
			<< std::get<3>(tpl));

		//data_.erase(std::get<0>(tpl));
		//auto r = data_.emplace(std::get<0>(tpl), std::map<std::string, std::string>());
		//if (r.second) {
		//	r.first->second.emplace("target", std::get<2>(tpl));
		//}

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

	void processor::http_upload_data(cyng::context& ctx)
	{
		//	http.upload.data - [ecf139d4-184e-441d-a857-9d02eb58148b,devConf_0,device_localhost.xml,text/xml,3C3F786D6C2....]
		//
		//	* session tag
		//	* variable name
		//	* file name
		//	* mime type
		//	* content
		//	
		const cyng::vector_t frame = ctx.get_frame();
		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,	//	[0] session tag
			std::string,		//	[1] variable name
			std::string,		//	[2] file name
			std::string			//	[3] mime type
			//cyng::buffer_t	//	[4] data (skipped to save memory)
		>(frame);

		auto const mime_type = cyng::value_cast<std::string>(frame.at(3), "undefined");

		CYNG_LOG_TRACE(logger_, "http.upload.data - "
			<< cyng::value_cast<std::string>(frame.at(2), "no file name specified")
			<< " - mime type"
			<< mime_type);

		if (boost::algorithm::equals(mime_type, "application/xml")) {

			cyng::buffer_t tmp;
			tmp = cyng::value_cast<>(frame.at(4), tmp);
			std::string const s(tmp.begin(), tmp.end());
			parse_xml(&s);
		}
		else {
			CYNG_LOG_WARNING(logger_, "http.upload.data - "
				<< " no handler for mime type"
				<< mime_type);

		}

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

	void processor::process_uplink_msg(pugi::xml_document& doc, pugi::xml_node node)
	{
		std::string const dev_eui(node.child("DevEUI").child_value());
		std::pair<cyng::mac64, bool > const r = cyng::parse_mac64(dev_eui);

		if (r.second) {
			using cyng::io::operator<<;
			std::stringstream ss;
			ss << r.first;
			CYNG_LOG_TRACE(logger_, "DevEUI: " << ss.str());
		}
		else {
			CYNG_LOG_WARNING(logger_, "DevEUI: " << dev_eui);
		}

		auto tp = cyng::parse_rfc3339_obj(node.child("Time").child_value());
		CYNG_LOG_TRACE(logger_, "roTime: " 
			<< node.child("Time").child_value()
			<< " - "
			<< cyng::io::to_str(tp));

		auto const FPort = static_cast<std::uint16_t>(node.child("FPort").text().as_int());		//	FPort
		auto const FCntUp = static_cast<std::int32_t>(node.child("FCntUp").text().as_int());	//	FCntUp
		auto const ADRbit = static_cast<std::int32_t>(node.child("ADRbit").text().as_int());	//	ADRbit
		auto const MType = static_cast<std::int32_t>(node.child("MType").text().as_int());		//	MType
		auto const FCntDn = static_cast<std::int32_t>(node.child("FCntDn").text().as_int());	//	FCntDn

		std::string const customer_id = node.child("CustomerID").child_value();
		auto const tag = uidgen_();

		//
		//	push meta data
		//
		bus_->vm_.async_run(bus_req_push_data("setup"
			, "TLoraUplink"
			, false		//	single
			, cyng::table::key_generator(tag)
			, cyng::table::data_generator(dev_eui
				, tp //	Time
				, FPort
				, FCntUp
				, ADRbit
				, MType
				, FCntDn
				, node.child("payload_hex").child_value()	//	payload_hex
				, node.child("mic_hex").child_value()
				, static_cast<double>(node.child("LrrRSSI").text().as_double())	//	LrrRSSI
				, static_cast<double>(node.child("LrrSNR").text().as_double())	//	LrrSNR
				, node.child("SpFact").child_value()	//	SpFact
				, node.child("SubBand").child_value()	//	SubBand
				, node.child("Channel").child_value()	//	Channel
				, static_cast<std::int32_t>(node.child("DevLrrCnt").text().as_int())	//	DevLrrCnt
				, node.child("Lrrid").child_value()	//	Lrrid
				, customer_id
				, static_cast<double>(node.child("LrrLAT").text().as_double())	//	LrrLAT
				, static_cast<double>(node.child("LrrLON").text().as_double())	//	LrrLON
			)
			, bus_->vm_.tag()));

		//
		//	extract payload
		//
		std::string const raw{ node.child("payload_hex").child_value() };

		//
		//	lookup configured LoRa devices
		//
		if (r.second) {
//#if defined _WIN32 || (defined __GNUC__ && __GNUC_PREREQ(5,0))
#if defined __GNUC__ && __GNUC_PREREQ(5,0)
			auto const [aes_key, driver, found] = lookup(r.first);
#else
            std::string aes_key;
            std::string driver;
            bool found;
            std::tie(aes_key, driver, found) = lookup(r.first);
#endif
			if (!found) {
				CYNG_LOG_WARNING(logger_, "DevEUI " << dev_eui << " is not configured");
				if (is_autoconfig_on()) {

					//
					//	insert a LoRa device with default values
					//
					bus_->vm_.async_run(bus_req_db_insert("TLoRaDevice"
						, cyng::table::key_generator(uidgen_())
						, cyng::table::data_generator(r.first
							, "0000000000000000000000000000000000000000000000000000000000000000"
							, "raw"
							, true
							, 0x64
							, cyng::mac64(0, 0, 0, 0, 0, 0, 0, 0)
							, cyng::mac64(0, 0, 0, 0, 0, 0, 0, 0))
						, 0
						, vm_.tag()));
				}
			}
			else {
				CYNG_LOG_TRACE(logger_, "decode DevEUI " << dev_eui << " with driver " << driver);


				if (boost::algorithm::equals(driver, "water")) {

					//
					//	water
					//
					decode_water(doc, dev_eui, raw);
				}
				else if (boost::algorithm::equals(driver, "ascii")) {

					//
					//	ascii
					//
					decode_ascii(doc, dev_eui, raw);
				}
				else if (boost::algorithm::equals(driver, "mbus")) {

					//
					//	M-Bus
					//
					decode_mbus(doc, dev_eui, raw);
				}
				else {

					//
					//	binary
					//
					decode_raw(doc, dev_eui, raw);
				}
			}
		}

		//
		//	generate a LoRa uplink event
		//
		bus_->vm_.async_run(bus_insert_LoRa_uplink(tp
			, r.first
			, FPort
			, FCntUp
			, ADRbit
			, MType
			, FCntDn
			, customer_id
			, raw
			, tag));

	}

	void processor::decode_water(pugi::xml_document& doc, std::string const& dev_eui, std::string const& raw)
	{
		cyng::buffer_t payload = node::lora::parse_payload(raw);
		if (!payload.empty())
		{
			auto const meter = node::lora::meter_id(payload);
			auto const value = lora::value(lora::vif(payload), lora::volume(payload));
			CYNG_LOG_TRACE(logger_, "meter ID: " << meter);
			CYNG_LOG_TRACE(logger_, "value: " << value);
			CYNG_LOG_TRACE(logger_, "CRC: " << (node::lora::crc_ok(payload) ? "OK" : "ERROR"));
			if (keep_xml_files_)
			{
				pugi::xpath_node_set pos = doc.select_nodes("/DevEUI_uplink");
				if (pos.begin() != pos.end()) {
					pugi::xpath_node node = pos.first();
					auto vnode = node.node().append_child("value");
					vnode.append_child(pugi::node_pcdata).set_value(value.c_str());
					vnode.append_attribute("meter").set_value(meter.c_str());

					auto const manufacturer = node::lora::manufacturer(payload);
					vnode.append_attribute("manufacturer").set_value(manufacturer.c_str());

					vnode.append_attribute("medium").set_value(node::lora::medium(payload));
					vnode.append_attribute("state").set_value(node::lora::state(payload));
					vnode.append_attribute("actuality").set_value(node::lora::actuality(payload));
					vnode.append_attribute("semester").set_value(node::lora::semester(node::lora::lifetime(payload)));
					vnode.append_attribute("link_error").set_value(node::lora::link_error(payload));
					vnode.append_attribute("crc").set_value(node::lora::crc(payload));
					vnode.append_attribute("version").set_value(node::lora::version(payload));
				}
				
				std::string file_name_pattern = dev_eui + "--" + node::lora::meter_id(payload) + "--%%%%-%%%%-%%%%-%%%%.xml";
				const auto p = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path(file_name_pattern);
				CYNG_LOG_TRACE(logger_, "write " << p.string());
				//root.append_attribute("xmlns") = "http://uri.actility.com/lora";
				doc.save_file(p.c_str(), PUGIXML_TEXT("  "));
			}
		}
		else
		{
			std::string file_name_pattern = "LoRa--invalid.payload--%%%%-%%%%-%%%%-%%%%.xml";
			const auto p = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path(file_name_pattern);
			CYNG_LOG_TRACE(logger_, "write " << p.string());

			//
			//	save payload (raw)
			//
			if (keep_xml_files_)	doc.save_file(p.c_str(), PUGIXML_TEXT("  "));

		}
	}

	void processor::decode_ascii(pugi::xml_document& doc, std::string const& dev_eui, std::string const& raw)
	{
		if (keep_xml_files_) {
			const std::pair<cyng::buffer_t, bool > r = cyng::parse_hex_string(raw);
			if (r.second) {
				auto const ascii = cyng::io::to_ascii(r.first);

				pugi::xpath_node_set pos = doc.select_nodes("/DevEUI_uplink");
				if (pos.begin() != pos.end()) {
					pugi::xpath_node node = pos.first();
					auto vnode = node.node().append_child("value");
					vnode.append_child(pugi::node_pcdata).set_value(ascii.c_str());
					vnode.append_attribute("type").set_value("ascii");
				}
			}

			//
			//	write XML file to disk
			//
			std::string file_name_pattern = dev_eui + "--ASCII--%%%%-%%%%-%%%%-%%%%.xml";
			const auto p = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path(file_name_pattern);
			doc.save_file(p.c_str(), PUGIXML_TEXT("  "));
		}
	}

	void processor::decode_mbus(pugi::xml_document& doc, std::string const& dev_eui, std::string const& raw)
	{
		CYNG_LOG_ERROR(logger_, "decode DevEUI " << dev_eui << " with driver M-Bus not implemented yet");
	}

	void processor::decode_raw(pugi::xml_document& doc, std::string const& dev_eui, std::string const& raw)
	{
		if (keep_xml_files_) {

			//
			//	write XML file to disk
			//
			std::string file_name_pattern = dev_eui + "--RAW--%%%%-%%%%-%%%%-%%%%.xml";
			const auto p = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path(file_name_pattern);
			doc.save_file(p.c_str(), PUGIXML_TEXT("  "));
		}
	}

	void processor::process_localisation_msg(pugi::xml_document const& doc, pugi::xml_node node)
	{

	}

	std::tuple<std::string, std::string, bool> processor::lookup(cyng::mac64 eui)
	{
		std::string aes_key;
		std::string driver;
		bool found = false;
		cache_.access([&](cyng::store::table const* tbl)->void {

			tbl->loop([&](cyng::table::record const& rec) -> bool {

				cyng::mac64 tmp;
				BOOST_ASSERT_MSG(rec["DevEUI"].get_class().tag() == cyng::TC_MAC64, "DevEUI has wrong data type");
				tmp = cyng::value_cast(rec["DevEUI"], tmp);
				if (eui == tmp) {

					aes_key = cyng::value_cast<std::string>(rec["AESKey"], "");
					driver = cyng::value_cast<std::string>(rec["driver"], "raw");
					found = true;
					return false;	//	abort
				}
				return true;	//	continue
			});
		}, cyng::store::read_access("TLoRaDevice"));

		return std::make_tuple(aes_key, driver, found);
	}

	bool processor::is_autoconfig_on() const
	{
		bool on{ false };
		cache_.access([&](cyng::store::table const* tbl)->void {

			auto const rec = tbl->lookup(cyng::table::key_generator("catch-lora"));
			if (!rec.empty()) {
				on = cyng::value_cast(rec["value"], false);
			}
		}, cyng::store::read_access("_Config"));

		return on;
	}

	void processor::http_upload_progress(cyng::context& ctx)
	{
		//	[ecf139d4-184e-441d-a857-9d02eb58148b,0000a84c,0000a84c,00000064]
		//
		//	* session tag
		//	* upload size
		//	* content size
		//	* progress in %
		//	
#ifdef _DEBUG
		const cyng::vector_t frame = ctx.get_frame();
		auto const progress = cyng::value_cast<std::uint32_t>(frame.at(3), 0);
		if ((progress % 10) == 0)	CYNG_LOG_TRACE(logger_, "http.upload.progress - " << progress << "%");
#endif

	}
	
	void processor::http_upload_complete(cyng::context& ctx)
	{
		//	[300dc453-d52d-40c0-843c-e8fd7495ff0e,true,0000a84a,/upload/config/device/]
		//	[aa681c03-42a5-47a9-920c-9dd2b62b89d3,true,00000440,/LoRa]
		//
		//	* session tag
		//	* success flag
		//	* total size in bytes
		//	* target path
		//
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "http.upload.complete - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,	//	[0] session tag
			bool,				//	[1] success
			std::uint32_t,		//	[2] size in bytes
			std::string			//	[3] target (action: URL)
		>(frame);

		//
		//	post a system message
		//
		//auto pos = data_.find(std::get<0>(tpl));
		//auto count = (pos != data_.end())
		//	? pos->second.size()
		//	: 0
		//	;
		//std::stringstream ss;
		//ss
		//	<< "upload/download "
		//	<< std::get<2>(tpl)
		//	<< " bytes to "
		//	<< std::get<3>(tpl)
		//	<< " with "
		//	<< count
		//	<< " variable(s)"
		//	;
		//ctx.queue(bus_insert_msg((std::get<1>(tpl) ? cyng::logging::severity::LEVEL_TRACE : cyng::logging::severity::LEVEL_WARNING), ss.str()));

		////
		////	start procedure
		////
		//bool found = false;
		//if (pos != data_.end()) {
		//	auto idx = pos->second.find("smf-procedure");
		//	if (idx != pos->second.end()) {

		//		CYNG_LOG_INFO(logger_, "run proc "
		//			<< idx->second
		//			<< ":"
		//			<< std::get<0>(tpl));

		//		cyng::param_map_t params;
		//		for (auto const& v : pos->second) {
		//			params.insert(cyng::param_factory(v.first, v.second));
		//		}
		//		ctx.queue(cyng::generate_invoke(idx->second, std::get<0>(tpl), params));
		//		found = true;
		//	}
		//}

		////
		////	cleanup form data
		////
		//data_.erase(std::get<0>(tpl));

		////
		////	consider to send a 302 - Object moved response
		////
		//if (!found) {
		//	ctx.queue(cyng::generate_invoke("http.move", std::get<0>(tpl), std::get<3>(tpl)));
		//}
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
