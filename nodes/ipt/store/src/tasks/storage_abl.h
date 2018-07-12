/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_STORE_TASK_STORAGE_ABL_H
#define NODE_IPT_STORE_TASK_STORAGE_ABL_H

#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/policy.h>
#include <cyng/intrinsics/buffer.h>
#include <map>

namespace node
{
	class storage_abl
	{
	public:
		using msg_0 = std::tuple<std::uint32_t, std::uint32_t, std::string, std::string, cyng::buffer_t>;
		using signatures_t = std::tuple<msg_0>;

	public:
		storage_abl(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cyng::param_map_t);
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
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;
		std::map<std::uint64_t, std::size_t>	lines_;

	};
}

#endif