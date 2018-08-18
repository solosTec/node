/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_STORE_IEC_PROCESSOR_H
#define NODE_IPT_STORE_IEC_PROCESSOR_H

#include <smf/iec/parser.h>
#include <cyng/log.h>
#include <cyng/intrinsics/buffer.h>
#include <cyng/vm/controller.h>
#include <cyng/async/mux.h>

namespace node
{
	//class iec_xml_consumer;
	class iec_processor
	{
		//friend iec_xml_consumer;

	public:
		iec_processor(cyng::async::mux& m
			, cyng::logging::log_ptr
			, boost::uuids::uuid
			, std::uint32_t channel
			, std::uint32_t source
			, std::string target
			, std::size_t tid
			, std::vector<std::size_t> const& consumers);
		virtual ~iec_processor();
		void stop();

		/**
		 * @brief slot [0]
		 *
		 * push data
		 */
		void parse(cyng::buffer_t const&);

	private:
		void init(std::uint32_t channel
			, std::uint32_t source
			, std::string target);
		void sml_msg(cyng::context& ctx);
		void sml_eom(cyng::context& ctx);

	private:
		cyng::logging::log_ptr logger_;
		cyng::controller vm_;
		cyng::async::mux& mux_;
		const std::size_t tid_;
		const std::vector<std::size_t> consumers_;
		const std::uint64_t line_;
		iec::parser parser_;
		std::size_t total_bytes_;
	};
}

#endif