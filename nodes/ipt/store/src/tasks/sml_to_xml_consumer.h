/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_STORE_TASK_SML_XML_CONSUMER_H
#define NODE_IPT_STORE_TASK_SML_XML_CONSUMER_H

//#include "../processors/xml_processor.h"
#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/policy.h>
#include <cyng/intrinsics/buffer.h>
#include <cyng/vm/context.h>
#include <map>
#include <boost/uuid/random_generator.hpp>

namespace node
{
	class sml_xml_consumer
	{
	public:
		using msg_0 = std::tuple<std::uint32_t, std::uint32_t, std::string, std::string, cyng::buffer_t>;
		using signatures_t = std::tuple<msg_0>;

	public:
		sml_xml_consumer(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, std::size_t ntid	//	network task id
			, std::string
			, std::string
			, std::string
			, std::chrono::seconds);
		cyng::continuation run();
		void stop();

		/**
		 * @brief slot [0]
		 *
		 * push data
		 */
		cyng::continuation process(std::uint32_t channel
			, std::uint32_t source
			, std::string const& target
			, std::string const& protocol
			, cyng::buffer_t const& data);

	private:
		void stop_writer(cyng::context& ctx);
		void register_consumer();

	private:
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;
		const std::size_t ntid_;
		const std::string root_dir_;
		const std::string root_name_;
		const std::string endocing_;
		const std::chrono::seconds period_;
		boost::uuids::random_generator rng_;
		std::list<std::uint64_t>	hit_list_;
		enum task_state {
			TASK_STATE_INITIAL,
			TASK_STATE_REGISTERED,
		} task_state_;
	};
}

#endif