/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_TASK_READOUT_H
#define NODE_SEGW_TASK_READOUT_H

#include <cyng/log.h>
#include <cyng/async/mux.h>

namespace node
{
	/**
	 * Check cache tables "_Readout" and "_ReadoutData" regularily
	 * to distribute readout data into SQL tables "TStorage_8181C78610FF", ...
	 */
	class storage;
	class cache;
	class readout
	{
	public:
		//	[0] static switch ON/OFF
		using msg_0 = std::tuple<cyng::buffer_t>;

		using signatures_t = std::tuple<msg_0>;

	public:
		readout(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cache&
			, storage&
			, std::chrono::seconds);

		cyng::continuation run();
		void stop(bool shutdown);

		/**
		 * @brief slot [0] - distribute readout data
		 * of specified meter.
		 *
		 */
		cyng::continuation process(cyng::buffer_t);

	private:
		void distribute();
		void distribute(cyng::buffer_t const& srv);

	private:
		cyng::async::base_task& base_;

		/**
		 * global logger
		 */
		cyng::logging::log_ptr logger_;

		/**
		 * global data cache
		 */
		cache& cache_;

		/**
		 * permanent storage
		 */
		storage& storage_;

		std::chrono::seconds const interval_;
	};

}

#endif