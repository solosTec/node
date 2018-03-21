/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_STORE_XML_PROCESSOR_H
#define NODE_IPT_STORE_XML_PROCESSOR_H

#include <smf/sml/protocol/parser.h>
#include <smf/sml/exporter/xml_exporter.h>
#include <cyng/log.h>
#include <cyng/intrinsics/buffer.h>
#include <cyng/vm/controller.h>

namespace node
{
	class storage_xml;
	class xml_processor
	{
		friend storage_xml;

	public:
		xml_processor(cyng::io_service_t& ios
			, cyng::logging::log_ptr
			, boost::uuids::uuid
			, std::string root_dir
			, std::string root_name
			, std::string endocing
			, std::uint32_t channel
			, std::uint32_t source
			, std::string target);
		virtual ~xml_processor();
		void stop();

		/**
		 * @brief slot [0]
		 *
		 * push data
		 */
		void parse(cyng::buffer_t const&);

	private:
		void init();
		void sml_parse(cyng::context& ctx);
		void sml_msg(cyng::context& ctx);
		void sml_eom(cyng::context& ctx);

	private:
		cyng::logging::log_ptr logger_;
		const boost::filesystem::path root_dir_;
		cyng::controller vm_;
		sml::parser parser_;
		sml::xml_exporter exporter_;	// ("UTF-8", "SML");
	};
}

#endif