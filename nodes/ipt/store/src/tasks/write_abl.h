/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_STORE_TASK_WRITE_ABL_H
#define NODE_IPT_STORE_TASK_WRITE_ABL_H

#include <smf/sml/protocol/parser.h>
#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/intrinsics/buffer.h>

namespace node
{
	class write_abl
	{
	public:
		using msg_0 = std::tuple<std::uint32_t, std::uint32_t, cyng::buffer_t>;
		using signatures_t = std::tuple<msg_0>;

	public:
		write_abl(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, std::size_t ntid	//	network task id
			, std::string
			, std::string
			, std::string);
		cyng::continuation run();
		void stop();

		/**
			* @brief slot [0]
			*
			* push data
			*/
		cyng::continuation process(cyng::buffer_t const&);

	private:
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;
		const std::string root_dir_;
		const std::string root_name_;
		const std::string endocing_;
		sml::parser parser_;
	};
}

#endif