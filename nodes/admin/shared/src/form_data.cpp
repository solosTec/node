/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#include "form_data.h"
#include <smf/cluster/generator.h>

#include <cyng/io/serializer.h>
#include <cyng/io/io_bytes.hpp>
#include <cyng/tuple_cast.hpp>

#include <boost/uuid/uuid_io.hpp>
#include <boost/algorithm/string.hpp>

namespace node 
{
	form_data::form_data(cyng::logging::log_ptr logger)
		: logger_(logger)
		, data_()
	{}

	void form_data::register_this(cyng::controller& vm)
	{
		vm.register_function("http.upload.start", 2, std::bind(&form_data::http_upload_start, this, std::placeholders::_1));
		vm.register_function("http.upload.data", 5, std::bind(&form_data::http_upload_data, this, std::placeholders::_1));
		vm.register_function("http.upload.var", 3, std::bind(&form_data::http_upload_var, this, std::placeholders::_1));
		vm.register_function("http.upload.progress", 4, std::bind(&form_data::http_upload_progress, this, std::placeholders::_1));
		vm.register_function("http.upload.complete", 4, std::bind(&form_data::http_upload_complete, this, std::placeholders::_1));
	}

	void form_data::http_upload_start(cyng::context& ctx)
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

		data_.erase(std::get<0>(tpl));
		auto r = data_.emplace(std::get<0>(tpl), std::map<std::string, std::string>());
		if (r.second) {
			r.first->second.emplace("target", std::get<2>(tpl));
		}
	}

	void form_data::http_upload_data(cyng::context& ctx)
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

		CYNG_LOG_TRACE(logger_, "http.upload.data - "
			<< cyng::value_cast<std::string>(frame.at(2), "no file name specified")
			<< " - "
			<< cyng::value_cast<std::string>(frame.at(3), "no mime type specified"));

		auto pos = data_.find(std::get<0>(tpl));
		if (pos != data_.end()) {
			CYNG_LOG_TRACE(logger_, "http.upload.data - "
				<< std::get<0>(tpl)
				<< ", "
				<< std::get<1>(tpl)
				<< ", "
				<< std::get<2>(tpl)
				<< ", mime type: "
				<< std::get<3>(tpl));

			//
			//	write temporary file
			//
			auto p = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path("smf-dash-%%%%-%%%%-%%%%-%%%%.tmp");
			auto ptr = cyng::object_cast<cyng::buffer_t>(frame.at(4));
			std::ofstream of(p.string());
			if (of.is_open() && ptr != nullptr) {
				CYNG_LOG_DEBUG(logger_, "http.upload.data - write "
					<< ptr->size()
					<< " bytes into file "
					<< p);
				of.write(ptr->data(), ptr->size());
				of.close();
			}
			else {
				CYNG_LOG_FATAL(logger_, "http.upload.data - cannot open "
					<< p);
			}
			pos->second.emplace(std::get<1>(tpl), p.string());
			pos->second.emplace(std::get<2>(tpl), std::get<3>(tpl));
		}
		else {
			CYNG_LOG_WARNING(logger_, "http.upload.data - session "
				<< std::get<0>(tpl)
				<< " not found");
		}
	}

	void form_data::http_upload_var(cyng::context& ctx)
	{
		//	[5910f652-95ce-4d94-b60f-6ff4a26730ba,smf-upload-config-device-version,v5.0]
		const cyng::vector_t frame = ctx.get_frame();
		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,	//	[0] session tag
			std::string,		//	[1] variable name
			std::string			//	[2] value
		>(frame);


		auto pos = data_.find(std::get<0>(tpl));
		if (pos != data_.end()) {
			CYNG_LOG_TRACE(logger_, "http.upload.var - "
				<< std::get<0>(tpl)
				<< " - "
				<< std::get<1>(tpl)
				<< " = "
				<< std::get<2>(tpl));
			pos->second.emplace(std::get<1>(tpl), std::get<2>(tpl));
		}
		else {
			CYNG_LOG_WARNING(logger_, "http.upload.var - session "
				<< std::get<0>(tpl)
				<< " not found");
		}

	}

	void form_data::http_upload_progress(cyng::context& ctx)
	{
		//	[ecf139d4-184e-441d-a857-9d02eb58148b,0000a84c,0000a84c,00000064]
		//
		//	* session tag
		//	* upload size
		//	* content size
		//	* progress in %
		//	
#ifdef __DEBUG
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "http.upload.progress - " << cyng::value_cast<std::uint32_t>(frame.at(3), 0) << "%");
#endif

	}

	void form_data::http_upload_complete(cyng::context& ctx)
	{
		//	[300dc453-d52d-40c0-843c-e8fd7495ff0e,true,0000a84a,/upload/config/device/]
		//
		//	* session tag
		//	* success flag
		//	* total size in bytes
		//	* target path
		//
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,	//	[0] session tag
			bool,				//	[1] success
			std::uint32_t,		//	[2] size in bytes
			std::string			//	[3] target (action: URL)
		>(frame);

		//
		//	post a system message
		//
		auto const pos = data_.find(std::get<0>(tpl));
		auto count = (pos != data_.end())
			? pos->second.size()
			: 0
			;

		std::stringstream ss;
		ss
			<< "upload/download "
			<< cyng::bytes_to_str(std::get<2>(tpl))
			<< " to "
			<< std::get<3>(tpl)
			<< " with "
			<< count
			<< " variable(s)"
			;
		ctx.queue(bus_insert_msg((std::get<1>(tpl) ? cyng::logging::severity::LEVEL_TRACE : cyng::logging::severity::LEVEL_WARNING), ss.str()));

		//
		//	This has changed since version 0.8.
		//	Now we use XMLHttpRequest instead of enctype="multipart/form-data"
		//

		cyng::param_map_t params;
		if (pos != data_.end()) {
			for (auto const& v : pos->second) {
				params.insert(cyng::param_factory(v.first, v.second));
			}
		}

		if (boost::algorithm::equals(std::get<3>(tpl), "/upload/config/device/")) {
			ctx.queue(cyng::generate_invoke("cfg.upload.devices", std::get<0>(tpl), params));
		}
		else if (boost::algorithm::equals(std::get<3>(tpl), "/upload/config/gw/")) {
			ctx.queue(cyng::generate_invoke("cfg.upload.gateways", std::get<0>(tpl), params));
		}
		else if (boost::algorithm::equals(std::get<3>(tpl), "/upload/config/meter/")) {
			ctx.queue(cyng::generate_invoke("cfg.upload.meter", std::get<0>(tpl), params));
		}
		else if (boost::algorithm::equals(std::get<3>(tpl), "/config/upload.LoRa")) {
			ctx.queue(cyng::generate_invoke("cfg.upload.LoRa", std::get<0>(tpl), params));
		}
		else {
			CYNG_LOG_ERROR(logger_, ctx.get_name() << " - unknown upload path: " << std::get<3>(tpl));
			ctx.queue(bus_insert_msg(cyng::logging::severity::LEVEL_ERROR, "unknown upload path: " + std::get<3>(tpl)));
		}
		

		//
		//	cleanup form data
		//
		data_.erase(std::get<0>(tpl));


		////
		////	consider to send a 302 - Object moved response
		////
		//if (!found) {
		//	ctx.queue(cyng::generate_invoke("http.move", std::get<0>(tpl), std::get<3>(tpl)));
		//}
	}


}
