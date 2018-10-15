/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IP_MASTER_TASK_QUERY_GATEWAY_H
#define NODE_IP_MASTER_TASK_QUERY_GATEWAY_H

#include <smf/cluster/bus.h>
#include <smf/ipt/defs.h>
#include <smf/sml/protocol/parser.h>
#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/vm/controller.h>
#include <boost/predef.h>	//	requires Boost 1.55

namespace node
{

	class query_gateway
	{
	public:
		using msg_0 = std::tuple<boost::uuids::uuid>;
		using msg_1 = std::tuple<std::uint16_t, std::size_t>;	//	eom
		using msg_2 = std::tuple<cyng::buffer_t>;
		using msg_3 = std::tuple<cyng::buffer_t, std::uint8_t, std::uint8_t, cyng::tuple_t, std::uint16_t>;
		using msg_4 = std::tuple<boost::uuids::uuid, cyng::buffer_t, std::size_t>;
		using msg_5 = std::tuple<cyng::buffer_t, std::uint32_t>;
		using msg_6 = std::tuple<bool, cyng::buffer_t, std::uint16_t, cyng::buffer_t, cyng::buffer_t, std::chrono::system_clock::time_point>;
		using msg_7 = std::tuple<cyng::buffer_t, std::uint32_t, cyng::buffer_t, cyng::buffer_t, bool>;

		using signatures_t = std::tuple<msg_0, msg_1, msg_2, msg_3, msg_4, msg_5, msg_6, msg_7>;

	public:
		query_gateway(cyng::async::base_task* btp
			, cyng::logging::log_ptr
			, bus::shared_type bus
			, cyng::controller& vm
			, boost::uuids::uuid tag_remote
			, std::uint64_t seq_cluster		//	cluster seq
			, std::vector<std::string> params
			, boost::uuids::uuid tag_ws
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
		 * sucessful cluster login
		 */
		cyng::continuation process(boost::uuids::uuid);

		/**
		 * @brief slot [1]
		 *
		 * session closed - eom
		 */
		cyng::continuation process(std::uint16_t, std::size_t);

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
		cyng::continuation process(cyng::buffer_t trx, std::uint8_t, std::uint8_t, cyng::tuple_t msg, std::uint16_t crc);

		/**
		 * @brief slot [4]
		 *
		 * sml.public.close.response
		 */
		cyng::continuation process(boost::uuids::uuid pk, cyng::buffer_t trx, std::size_t);

		/**
		 * @brief slot [5]
		 *
		 * sml.get.proc.status.word
		 */
		cyng::continuation process(cyng::buffer_t const&
			, std::uint32_t);

		/**
		 * @brief slot [6]
		 *
		 * sml.get.proc.param.srv.visible
		 * sml.get.proc.param.srv.active
		 */
		cyng::continuation process(bool
			, cyng::buffer_t const&
			, std::uint16_t
			, cyng::buffer_t const&
			, cyng::buffer_t const&
			, std::chrono::system_clock::time_point);

		/**
		 * @brief slot [7]
		 *
		 * sml.get.proc.param.firmware
		 */
		cyng::continuation process(cyng::buffer_t const&
			, std::uint32_t
			, cyng::buffer_t const&
			, cyng::buffer_t const&
			, bool);

	private:
		void send_query_cmd();

	private:
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;
		bus::shared_type bus_;
		cyng::controller& vm_;
		const boost::uuids::uuid tag_remote_;
		const std::uint64_t seq_cluster_;
		const std::vector<std::string> params_;
		const boost::uuids::uuid tag_ws_;
		const cyng::buffer_t server_id_;
		const std::string user_, pwd_;
		const std::chrono::seconds timeout_;
		const boost::uuids::uuid tag_ctx_;
		const std::chrono::system_clock::time_point start_;
		sml::parser parser_;
		bool is_waiting_;
	};


}

#if BOOST_COMP_GNUC
namespace cyng {
	namespace async {

		//
		//	initialize static slot names
		//
		template <>
		std::map<std::string, std::size_t> cyng::async::task<node::query_srv_active>::slot_names_;
    }
}
#endif

#endif
