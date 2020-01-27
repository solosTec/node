/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_TASK_PUSH_H
#define NODE_SEGW_TASK_PUSH_H

#include <smf/sml/intrinsics/obis.h>

#include <cyng/log.h>
#include <cyng/async/mux.h>

namespace node
{
	/**
	 * write cyclic OBISLOG entries
	 */
	class cache;
	class storage;
	class push
	{
	public:
		//	[0] write entry
		using msg_0 = std::tuple<>;


		using signatures_t = std::tuple<msg_0>;

	public:
		push(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cache&
			, storage&
			, cyng::buffer_t srv_id
			, std::uint8_t nr
			, cyng::buffer_t profile
			, std::chrono::seconds interval
			, std::chrono::seconds delay
			, std::string target);

		cyng::continuation run();
		void stop(bool shutdown);

		/**
		 * @brief slot [0] - write an entry
		 *
		 */
		cyng::continuation process();

	private:
		void send_push_data();

	private:
		cyng::async::base_task& base_;

		/**
		 * global logger
		 */
		cyng::logging::log_ptr logger_;

		/**
		 * permanent storage
		 */
		cache& cache_;

		/**
		 * permanent storage
		 */
		storage& storage_;

		cyng::buffer_t const srv_id_;
		std::uint8_t const nr_;
		sml::obis const profile_;
		std::chrono::seconds interval_;
		std::chrono::seconds delay_;
		std::string target_;
	};

}

#endif