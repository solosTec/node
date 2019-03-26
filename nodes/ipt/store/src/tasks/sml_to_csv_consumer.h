/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_STORE_TASK_SML_CSV_CONSUMER_H
#define NODE_IPT_STORE_TASK_SML_CSV_CONSUMER_H

#include <smf/sml/exporter/csv_sml_exporter.h>
#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/policy.h>
#include <cyng/intrinsics/buffer.h>
#include <unordered_map>

namespace node
{
	class sml_csv_consumer
	{
	public:
		using msg_0 = std::tuple<std::uint64_t, std::string>;
		using msg_1 = std::tuple<std::uint64_t, std::uint16_t, std::size_t, cyng::tuple_t>;
		using msg_2 = std::tuple<std::uint64_t>;
		using msg_3 = std::tuple<std::uint64_t, std::size_t, std::uint16_t>;
		using signatures_t = std::tuple<msg_0, msg_1, msg_2, msg_3>;

	public:
		sml_csv_consumer(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, std::size_t ntid	//	network task id
			, boost::filesystem::path root_dir
			, std::string prefix
			, std::string suffix
			, bool header
			, std::chrono::seconds period);
		cyng::continuation run();
		void stop(bool shutdown);

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
		 * @brief slot [2] - CONSUMER_REMOVE_LINE
		 *
		 * close line
		 */
		cyng::continuation process(std::uint64_t line);

		/**
		 * @brief slot [3] - CONSUMER_EOM
		 *
		 * received End Of Message
		 */
		cyng::continuation process(std::uint64_t line, std::size_t idx, std::uint16_t crc);

	private:
		void cb(std::uint32_t channel
			, std::uint32_t source
			, std::string const& target
			, std::string const& protocol
			, cyng::vector_t&& prg);

		void register_consumer();

	private:
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;
		const std::size_t ntid_;
		const boost::filesystem::path root_dir_;
		const std::string prefix_;
		const std::string suffix_;
		const bool header_;
		const std::chrono::seconds period_;
		enum task_state {
			TASK_STATE_INITIAL,
			TASK_STATE_REGISTERED,
		} task_state_;
		std::unordered_map<std::uint64_t, sml::csv_exporter>	lines_;
	};
}

#endif
