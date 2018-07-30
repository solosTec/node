/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_STORE_IEC_PROCESSOR_H
#define NODE_IPT_STORE_IEC_PROCESSOR_H

//#include <smf/sml/protocol/parser.h>
//#include <smf/sml/exporter/xml_exporter.h>
#include <cyng/log.h>
#include <cyng/intrinsics/buffer.h>
#include <cyng/vm/controller.h>

namespace node
{
	namespace iec
	{
		struct parser {};	//	dummy
	}

	class sml_xml_consumer;
	class iec_processor
	{
		friend sml_xml_consumer;

	public:
		iec_processor(cyng::io_service_t& ios
			, cyng::logging::log_ptr
			, boost::uuids::uuid
			, std::uint32_t channel
			, std::uint32_t source
			, std::string target
			, std::size_t tid);
		virtual ~iec_processor();
		void stop();

		/**
		 * @brief slot [0]
		 *
		 * push data
		 */
		void parse(cyng::buffer_t const&);

	private:
		void init();
		void sml_msg(cyng::context& ctx);
		void sml_eom(cyng::context& ctx);

	private:
		cyng::logging::log_ptr logger_;
		cyng::controller vm_;
		const std::size_t tid_;
		iec::parser parser_;
	};
}

#endif