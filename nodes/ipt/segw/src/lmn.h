#ifndef NODE_LMN_H
#define NODE_LMN_H

#include "decoder.h"
#include <cyng/async/mux.h>
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

		std::pair<std::size_t, bool> start_lmn_port_wireless(std::size_t);
		std::pair<std::size_t, bool> start_lmn_port_wired(std::size_t);

		void wmbus_push_frame(cyng::context& ctx);
		void sml_msg(cyng::context& ctx);
		void sml_eom(cyng::context& ctx);
		void sml_get_list_response(cyng::context& ctx);

	private:
		cyng::async::mux& mux_;
		cyng::logging::log_ptr logger_;
		cyng::controller vm_;
		bridge& bridge_;
		decoder_wireless_mbus decoder_wmbus_;
	};
}
#endif