/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IP_MASTER_TASK_CLOSE_CONNECTION_H
#define NODE_IP_MASTER_TASK_CLOSE_CONNECTION_H

#include <smf/cluster/bus.h>
#include <smf/ipt/defs.h>
#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/vm/controller.h>
#include <boost/predef.h>	//	requires Boost 1.55

namespace node
{

	class close_connection
	{
	public:
		using msg_0 = std::tuple<ipt::response_type>;
		using signatures_t = std::tuple<msg_0>;

	public:
		close_connection(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			//, bus::shared_type bus
			, cyng::controller& vm
			//, bool shutdown
			//, boost::uuids::uuid tag
			//, std::size_t seq
			//, cyng::param_map_t const& options
			//, cyng::param_map_t const& bag
			, std::chrono::seconds timeout);
		cyng::continuation run();
		void stop();

		/**
		 * @brief slot [0]
		 *
		 * sucessful cluster login
		 */
		cyng::continuation process(ipt::response_type);

	private:
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;
		//bus::shared_type bus_;	//!< cluster bus
		cyng::controller& vm_;	//!< ipt device
		//bool shutdown_;	//!< sender is in shutdown mode 
		/**
		 * Remote session tag to address response.
		 * Not to bw confused with *this* session tag (vm_.tag())
		 */
		//const boost::uuids::uuid tag_origin_;	
		//const std::size_t seq_;
		//const cyng::param_map_t options_;
		//const cyng::param_map_t bag_;
		const std::chrono::seconds timeout_;
		//const bool local_connect_;
		const std::chrono::system_clock::time_point start_;
		bool is_waiting_;
	};
	
}

#endif
