/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_GATEWAY_TASK_VIRTUAL_METER_H
#define NODE_GATEWAY_TASK_VIRTUAL_METER_H

//#include <smf/ipt/bus.h>
#include <smf/sml/status.h>
#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/store/db.h>

namespace node
{
	class virtual_meter
	{
	public:
		//using msg_0 = std::tuple<>;
		using signatures_t = std::tuple<>;

	public:
		virtual_meter(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cyng::buffer_t server_id
			, cyng::store::db& config_db
			, std::chrono::seconds interval);

		cyng::continuation run();
		void stop(bool shutdown);

		/**
			* @brief slot [0]
			*
			* open push channel response
			*/
		//cyng::continuation process(bool success
		//	, std::uint32_t channel
		//	, std::uint32_t source
		//	, std::uint16_t status
		//	, std::size_t count
		//	, std::string target);


	private:

	private:
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;
			
		/**
 		 * server/meter ID
		 */
		cyng::buffer_t const server_id_;

		/**
		 * configuration db
		 */
		cyng::store::db& config_db_;

		/**
		 * interval
		 */
		std::chrono::seconds const interval_;

	};
}

#endif
