/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_STORE_SML_PROCESSOR_H
#define NODE_IPT_STORE_SML_PROCESSOR_H

#include <smf/sml/protocol/parser.h>
#include <cyng/log.h>
#include <cyng/intrinsics/buffer.h>
#include <cyng/vm/controller.h>
#include <cyng/async/mux.h>

namespace node
{
	class sml_xml_consumer;
	class sml_processor
	{
		friend sml_xml_consumer;

	public:
		sml_processor(cyng::async::mux& 
			, cyng::logging::log_ptr
			, boost::uuids::uuid
			, std::uint32_t channel
			, std::uint32_t source
			, std::string target
			, std::size_t tid
			, std::vector<std::size_t> const& consumers);
		virtual ~sml_processor();

		/**
		 * @brief slot [0]
		 *
		 * push data
		 */
		void parse(cyng::buffer_t const&);

	private:
		void stop();
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
		sml::parser parser_;
		bool shutdown_;
	};

	/**
	 * @return SML message type
	 */
	std::uint16_t get_msg_type(cyng::tuple_t const&);

}

#endif