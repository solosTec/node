/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IP_MASTER_TASK_QUERY_SRV_VISIBLE_H
#define NODE_IP_MASTER_TASK_QUERY_SRV_VISIBLE_H

#include <smf/cluster/bus.h>
#include <smf/ipt/defs.h>
#include <smf/sml/protocol/parser.h>
#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/vm/controller.h>

namespace node
{

	class query_srv_visible
	{
	public:
		using msg_0 = std::tuple<boost::uuids::uuid>;
		using msg_1 = std::tuple<>;
		using msg_2 = std::tuple<cyng::buffer_t>;
		using msg_3 = std::tuple<cyng::buffer_t, std::uint8_t, std::uint8_t, cyng::tuple_t, std::uint16_t>;
		using signatures_t = std::tuple<msg_0, msg_1, msg_2, msg_3>;

	public:
		query_srv_visible(cyng::async::base_task* btp
			, cyng::logging::log_ptr
			, bus::shared_type bus
			, cyng::controller& vm
			, boost::uuids::uuid tag_remote
			, std::uint64_t seq_cluster		//	cluster seq
			, cyng::buffer_t const& server	//	server id
			, std::string user
			, std::string pwd
			, std::chrono::seconds timeout
			, boost::uuids::uuid tag_ctx);
		cyng::continuation run();
		void stop();

		/**
		 * @brief slot [0]
		 *
		 * acknowlegde
		 */
		cyng::continuation process(boost::uuids::uuid);

		/**
		 * @brief slot [1]
		 *
		 * session closed
		 */
		cyng::continuation process();

		/**
		 * @brief slot [2]
		 *
		 * get response (SML)
		 */
		cyng::continuation process(cyng::buffer_t const&);

		/**
		 * @brief slot [3]
		 *
		 * get SML tree
		 */
		cyng::continuation process(cyng::buffer_t, std::uint8_t, std::uint8_t, cyng::tuple_t, std::uint16_t);

	private:
		void send_query_cmd();

	private:
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;
		cyng::controller& vm_;
		const boost::uuids::uuid tag_remote_;
		const cyng::buffer_t server_id_;
		const std::string user_, pwd_;
		const std::chrono::seconds timeout_;
		const boost::uuids::uuid tag_ctx_;
		const std::chrono::system_clock::time_point start_;
		sml::parser parser_;
		bool is_waiting_;
	};
	
}

#endif