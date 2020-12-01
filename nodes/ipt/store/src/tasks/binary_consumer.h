/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_STORE_TASK_BINARY_CONSUMER_H
#define NODE_IPT_STORE_TASK_BINARY_CONSUMER_H

#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/policy.h>
#include <cyng/intrinsics/buffer.h>

namespace node
{
	class binary_consumer
	{
	public:
		using msg_0 = std::tuple<std::uint32_t, std::uint32_t, std::string, std::string, cyng::buffer_t>;
		using signatures_t = std::tuple<msg_0>;

	public:
		binary_consumer(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, std::size_t ntid	//	network task id
			, std::string root_dir
			, std::string prefix
			, std::string suffix
			, std::chrono::seconds period);
		cyng::continuation run();
		void stop(bool shutdown);

		/**
		 * @brief slot [0]
		 *
		 * push data
		 */
		cyng::continuation process(std::uint32_t channel
			, std::uint32_t source
			, std::string protocol
			, std::string const& target
			, cyng::buffer_t const& data);

	private:
		void register_consumer();
		bool check_output_path() const;

	private:
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;
		const std::size_t ntid_;
		const std::string root_dir_;
		const std::string prefix_;
		const std::string suffix_;
		const std::chrono::seconds period_;
		enum task_state {
			TASK_STATE_INITIAL,
			TASK_STATE_REGISTERED,
		} task_state_;
		std::size_t total_bytes_;
	};
}

#endif