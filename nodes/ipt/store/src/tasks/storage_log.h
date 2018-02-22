/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#ifndef NODE_IPT_STORE_TASK_STORAGE_LOG_H
#define NODE_IPT_STORE_TASK_STORAGE_LOG_H

#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/policy.h>
#include <cyng/intrinsics/buffer.h>
#include <map>

namespace node
{
	class storage_log
	{
	public:
		using msg_0 = std::tuple<std::uint32_t, std::uint32_t, std::string, cyng::buffer_t>;
		using signatures_t = std::tuple<msg_0>;

	public:
		storage_log(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cyng::param_map_t);
		void run();
		void stop();

		/**
		 * @brief slot [0]
		 *
		 * push data
		 */
		cyng::continuation process(std::uint32_t, std::uint32_t, std::string const&, cyng::buffer_t const&);

	private:
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;
		std::map<std::uint64_t, std::size_t>	lines_;
	};
}

#endif