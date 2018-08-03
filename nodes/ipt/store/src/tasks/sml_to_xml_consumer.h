/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_STORE_TASK_SML_XML_CONSUMER_H
#define NODE_IPT_STORE_TASK_SML_XML_CONSUMER_H

#include <smf/sml/exporter/xml_exporter.h>
#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/policy.h>
#include <cyng/intrinsics/buffer.h>
#include <cyng/vm/context.h>
#include <unordered_map>

namespace node
{
	class sml_xml_consumer
	{
	public:
		using msg_0 = std::tuple<std::uint64_t, std::string>;
		using msg_1 = std::tuple<std::uint64_t, std::uint16_t, std::size_t, cyng::tuple_t>;
		using msg_2 = std::tuple<std::uint64_t, std::size_t, std::uint16_t>;
		using signatures_t = std::tuple<msg_0, msg_1, msg_2>;

	public:
		sml_xml_consumer(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, std::size_t ntid	//	network task id
			, boost::filesystem::path
			, std::string
			, std::string
			, std::chrono::seconds);
		cyng::continuation run();
		void stop();

		/**
		 * @brief slot [0]
		 *
		 * create a new line
		 */
		cyng::continuation process(std::uint64_t line, std::string);

		/**
		 * @brief slot [1]
		 *
		 * receive push data
		 */
		cyng::continuation process(std::uint64_t line
			, std::uint16_t code
			, std::size_t idx
			, cyng::tuple_t msg);

		/**
		 * @brief slot [2]
		 *
		 * close line
		 */
		cyng::continuation process(std::uint64_t line, std::size_t idx, std::uint16_t crc);

	private:
		void register_consumer();

	private:
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;
		const std::size_t ntid_;
		const boost::filesystem::path root_dir_;
		const std::string root_name_;
		const std::string endcoding_;
		const std::chrono::seconds period_;
		enum task_state {
			TASK_STATE_INITIAL,
			TASK_STATE_REGISTERED,
		} task_state_;
		std::unordered_map<std::uint64_t, sml::xml_exporter>	lines_;
	};
}

#endif