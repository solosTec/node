/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IP_MASTER_TASK_GATEWAY_PROXY_H
#define NODE_IP_MASTER_TASK_GATEWAY_PROXY_H

#include "../proxy_data.h"
#include <smf/cluster/bus.h>
#include <smf/ipt/defs.h>
#include <smf/sml/protocol/parser.h>
#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/vm/controller.h>
#include <queue>
#include <boost/predef.h>	//	requires Boost 1.55

namespace node
{
	namespace sml {
		class req_generator;
	}
	class gateway_proxy
	{
		using proxy_queue = std::queue< ipt::proxy_data >;

	public:
		using msg_0 = std::tuple<>;
		using msg_1 = std::tuple<std::uint16_t, std::size_t>;	//	eom
		using msg_2 = std::tuple<cyng::buffer_t>;
		using msg_3 = std::tuple<cyng::buffer_t, std::uint8_t, std::uint8_t, cyng::tuple_t, std::uint16_t>;
		using msg_4 = std::tuple<boost::uuids::uuid, cyng::buffer_t, std::size_t>;

		using msg_5 = std::tuple<
			std::string,	//	trx
			std::size_t,	//	idx
			std::string,	//	server ID
			cyng::buffer_t,	//	root (obis code)
			cyng::param_map_t	//	params
		>;

		using msg_6 = std::tuple<std::string, cyng::buffer_t>;

		using msg_7 = std::tuple <
			boost::uuids::uuid,		//	[0] ident tag
			boost::uuids::uuid,		//	[1] source tag
			std::uint64_t,			//	[2] cluster seq
			cyng::vector_t,			//	[3] TGateway key
			boost::uuids::uuid,		//	[4] ws tag
			std::string,			//	[5] channel
			cyng::vector_t,			//	[5] sections
			cyng::vector_t,			//	[6] parameters
			cyng::buffer_t,			//	[7] server id
			std::string,			//	[8] name
			std::string				//	[9] pwd
		>;

		using signatures_t = std::tuple<msg_0, msg_1, msg_2, msg_3, msg_4, msg_5, msg_6, msg_7>;

	public:
		gateway_proxy(cyng::async::base_task* btp
			, cyng::logging::log_ptr
			, bus::shared_type bus
			, cyng::controller& vm
			, std::chrono::seconds timeout);
		cyng::continuation run();
		void stop();

		/**
		 * @brief slot [0]
		 *
		 * ack - connected to gateway
		 */
		cyng::continuation process();

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
		 * bus_res_query_gateway()
		 */
		cyng::continuation process(std::string trx,
			std::size_t idx,
			std::string serverID,
			cyng::buffer_t code,
			cyng::param_map_t params);

		/**
		 * @brief slot [6]
		 *
		 * attention code
		 */
		cyng::continuation process(std::string
			, cyng::buffer_t const&);

		/**
		 * @brief slot [7]
		 *
		 * add new entry in work queue
		 */
		cyng::continuation process(boost::uuids::uuid,		//	[0] ident tag
			boost::uuids::uuid,		//	[1] source tag
			std::uint64_t,			//	[2] cluster seq
			cyng::vector_t,			//	[3] TGateway key
			boost::uuids::uuid,		//	[4] ws tag
			std::string channel,	//	[5] channel
			cyng::vector_t sections,	//	[6] sections
			cyng::vector_t params,		//	[7] parameters
			cyng::buffer_t srv,			//	[8] server id
			std::string name,			//	[9] name
			std::string	pwd				//	[10] pwd
		);
	private:
		void execute_cmd();
		void execute_cmd_get_proc_param();
		void execute_cmd_set_proc_param();
		void execute_cmd_set_proc_param_ipt(sml::req_generator& sml_gen);
		void execute_cmd_set_proc_param_wmbus(sml::req_generator& sml_gen);
		void execute_cmd_get_list_req_last_data_set(sml::req_generator& sml_gen);
		cyng::mac48 get_mac() const;

	private:
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;
		bus::shared_type bus_;
		cyng::controller& vm_;
		const std::chrono::seconds timeout_;
		const std::chrono::system_clock::time_point start_;
		sml::parser parser_;
		proxy_queue queue_;
	};


}

#if BOOST_COMP_GNUC
namespace cyng {
	namespace async {

		//
		//	initialize static slot names
		//
		template <>
		std::map<std::string, std::size_t> cyng::async::task<node::gateway_proxy>::slot_names_;
    }
}
#endif

#endif
