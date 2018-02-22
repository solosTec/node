/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_STORE_TASK_WRITE_XML_H
#define NODE_IPT_STORE_TASK_WRITE_XML_H

#include <smf/sml/protocol/parser.h>
#include <smf/sml/exporter/xml_exporter.h>
#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/policy.h>
#include <cyng/intrinsics/buffer.h>
#include <cyng/vm/controller.h>

namespace node
{
	class storage_xml;
	class write_xml
	{
		friend storage_xml;

	public:
		using msg_0 = std::tuple<cyng::buffer_t>;
		using msg_1 = std::tuple<>;
		using signatures_t = std::tuple<msg_0, msg_1>;

	public:
		write_xml(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, boost::uuids::uuid
			, std::string root_dir
			, std::string root_name
			, std::string endocing
			, std::uint32_t channel
			, std::uint32_t source
			, std::string target);
		void run();
		void stop();

		/**
		 * @brief slot [0]
		 *
		 * push data
		 */
		cyng::continuation process(cyng::buffer_t const&);

		/**
		 * @brief slot [1]
		 *
		 * stop 
		 */
		cyng::continuation process();

	private:
		void init();
		void sml_msg(cyng::context& ctx);
		void sml_eom(cyng::context& ctx);

	private:
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;
		const boost::filesystem::path root_dir_;
		cyng::controller vm_;
		sml::parser parser_;
		sml::xml_exporter exporter_;	// ("UTF-8", "SML");
		bool initialized_;
	};
}

#endif