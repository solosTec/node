/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_STORE_TASK_STORAGE_XML_H
#define NODE_IPT_STORE_TASK_STORAGE_XML_H

#include "../processors/xml_processor.h"
#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/policy.h>
#include <cyng/intrinsics/buffer.h>
#include <cyng/vm/context.h>
#include <map>
#include <boost/uuid/random_generator.hpp>

namespace node
{
	class storage_xml
	{
	public:
		using msg_0 = std::tuple<std::uint32_t, std::uint32_t, std::string, cyng::buffer_t>;
		using signatures_t = std::tuple<msg_0>;

	public:
		storage_xml(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, std::string 
			, std::string
			, std::string);
		void run();
		void stop();

		/**
		 * @brief slot [0]
		 *
		 * push data
		 */
		cyng::continuation process(std::uint32_t, std::uint32_t, std::string const&, cyng::buffer_t const&);

	private:
		void stop_writer(cyng::context& ctx);

	private:
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;
		const std::string root_dir_;
		const std::string root_name_;
		const std::string endocing_;
		std::map<std::uint64_t, xml_processor>	lines_;
		boost::uuids::random_generator rng_;
		std::list<std::uint64_t>	hit_list_;
	};
}

#endif