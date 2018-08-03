/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#ifndef NODE_IPT_STORE_TASK_SML_JSON_CONSUMER_H
#define NODE_IPT_STORE_TASK_SML_JSON_CONSUMER_H

#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/policy.h>
#include <cyng/intrinsics/buffer.h>
#include <map>

namespace node
{
	class sml_json_consumer
	{
	public:
		using msg_0 = std::tuple<std::uint64_t, std::string>;
		using msg_1 = std::tuple<std::uint64_t, std::uint16_t, std::size_t, cyng::tuple_t>;
		using msg_2 = std::tuple<std::uint64_t, std::size_t, std::uint16_t>;
		using signatures_t = std::tuple<msg_0, msg_1, msg_2>;

	public:
		sml_json_consumer(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, std::size_t ntid	//	network task id
			, boost::filesystem::path root_dir
			, std::string prefix
			, std::string suffix);
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
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;
		const boost::filesystem::path root_dir_;
		const std::string prefix_;
		const std::string suffix_;
		std::map<std::uint64_t, std::size_t>	lines_;

	};
}

#endif