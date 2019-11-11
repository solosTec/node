#ifndef NODE_LMN_H
#define NODE_LMN_H

#include <cyng/async/mux.h>
#include <cyng/log.h>
#include <cyng/vm/controller.h>

#include <boost/uuid/uuid.hpp>

namespace node
{
	class bridge;
	class lmn
	{
	public:
		lmn(cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, boost::uuids::uuid tag
			, bridge&);

		/**
		 * Open serial and wireless communication ports
		 */
		void start();

	private:
		void start_lmn_wired();
		void start_lmn_wireless();
		//void wmbus_push_frame(cyng::context& ctx);

	private:
		cyng::async::mux& mux_;
		cyng::logging::log_ptr logger_;
		cyng::controller vm_;
		bridge& bridge_;
	};
}
#endif