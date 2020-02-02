#ifndef NODE_LMN_H
#define NODE_LMN_H

#include "decoder.h"
#include <cyng/async/mux.h>
#include <cyng/vm/controller.h>

#include <boost/uuid/uuid.hpp>

namespace node
{
	class cache;
	class lmn
	{
	public:
		lmn(cyng::io_service_t& ios
			, cyng::logging::log_ptr logger
			, cache&
			, boost::uuids::uuid tag);

		/**
		 * Open serial and wireless communication ports
		 */
		void start(cyng::async::mux& mux);

	private:
		void start_lmn_wired(cyng::async::mux& mux);
		void start_lmn_wireless(cyng::async::mux& mux);

		std::pair<std::size_t, bool> start_lmn_port_wireless(cyng::async::mux& mux, std::size_t, std::size_t);
		std::pair<std::size_t, bool> start_lmn_port_wired(cyng::async::mux& mux, std::size_t);

		void wmbus_push_frame(cyng::context& ctx);
		void sml_msg(cyng::context& ctx);
		void sml_eom(cyng::context& ctx);
		void sml_get_list_response(cyng::context& ctx);

	private:
		cyng::logging::log_ptr logger_;
		cyng::controller vm_;
		cache& cache_;
		decoder_wireless_mbus decoder_wmbus_;
	};
}
#endif