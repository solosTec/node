/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IP_MASTER_TASK_GATEWAY_PROXY_H
#define NODE_IP_MASTER_TASK_GATEWAY_PROXY_H

#include "../proxy_data.h"
#include "../config_cache.h"

#include <smf/cluster/bus.h>
#include <smf/ipt/defs.h>
#include <smf/sml/protocol/parser.h>

#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/vm/controller.h>
#include <cyng/dom/reader.h>

#include <queue>
#include <boost/predef.h>	//	requires Boost 1.55

namespace node
{
	namespace sml {
		class req_generator;
	}

	/**
	 * The gateway proxy controls the communication with gateways over the IP-T protocol.
	 */
	class gateway_proxy
	{
		using input_queue = std::queue< proxy_data >;
		using output_map = std::map<std::string, proxy_data>;

	public:
		using msg_0 = std::tuple<>;
		using msg_1 = std::tuple<std::uint16_t, std::size_t>;	//	eom
		using msg_2 = std::tuple<cyng::buffer_t>;

		using msg_3 = std::tuple<std::string
			, std::uint8_t
			, cyng::buffer_t
			, sml::obis_path
			, cyng::param_t>;

		using msg_4 = std::tuple<std::string>;

		using msg_5 = std::tuple<
			std::string,	//	trx
			cyng::buffer_t,	//	server ID
			sml::obis_path,	//	OBIS (path)
			cyng::param_map_t	//	data
		>;

		using msg_6 = std::tuple<std::string, std::string, cyng::buffer_t>;

        //  gcc 5.4.0 has a problem with this tuple - maybe to big
		using msg_7 = std::tuple <
			boost::uuids::uuid,		//	[0] ident tag
			boost::uuids::uuid,		//	[1] source tag
			std::uint64_t,			//	[2] cluster seq
			boost::uuids::uuid,		//	[3] ws tag (origin)
			std::string,			//	[4] channel (SML message type)
			cyng::buffer_t,			//	[5] OBIS root code
			cyng::vector_t,			//	[6] TGateway/TDevice key
			cyng::tuple_t,			//	[7] parameters
			cyng::buffer_t,			//	[8] server id
			std::string,			//	[9] name
			std::string				//	[10] pwd
		>;

		//	get profile list response
		using msg_8 = std::tuple <
			std::string,	//	trx
			std::uint32_t,	//	act_time
			std::uint32_t,	//	reg_period
			std::uint32_t,	//	val_time
			sml::obis_path,	//	OBIS (path)
			cyng::buffer_t,	//	server ID
			std::uint32_t,	//	status
			cyng::param_map_t	//	params
		>;

		//	communicate with this proxy tasks
		//	* [uuid] ident
		//	* [uuid] source
		//	* [u64] seq
		//	* [uuid] origin (web-socket)
		//	* [str] job
		//	* [vec] gateway pk
		//	* [vec] sections
		//	* [buffer] server
		//	* [string] user
		//	* [string] pwd

		using msg_9 = std::tuple <
			boost::uuids::uuid,		//	[0] ident tag
			boost::uuids::uuid,		//	[1] source tag
			std::uint64_t,			//	[2] cluster seq
			boost::uuids::uuid,		//	[3] ws tag (origin)
			std::string,			//	[4] job
			cyng::vector_t,			//	[5] TGateway/TDevice pk
			cyng::vector_t,			//	[6] sections
			cyng::buffer_t,			//	[7] server id
			std::string,			//	[8] name
			std::string				//	[9] pwd
		>;
		using signatures_t = std::tuple<msg_0, msg_1, msg_2, msg_3, msg_4, msg_5, msg_6, msg_7, msg_8, msg_9>;

	public:
		gateway_proxy(cyng::async::base_task* btp
			, cyng::logging::log_ptr
			, bus::shared_type bus
			, cyng::controller& vm
			, std::chrono::seconds timeout
			, bool sml_log);
		cyng::continuation run();
		void stop(bool shutdown);

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
		 *
		 * @param crc CRC16 of the whole SMS message
		 * @param midx message index (sequential counter)
		 */
		cyng::continuation process(std::uint16_t crc, std::size_t midx);

		/**
		 * @brief slot [2]
		 *
		 * Get an SML message. The SML parser will read this message
		 * and produce VM calls ("sml.msg" and "sml.eom") that will be executed 
		 * in the proxy communication class (proxy_comm).
		 *
		 * @see proxy_comm.h
		 */
		cyng::continuation process(cyng::buffer_t const&);

		/**
		 * @brief slot [3]
		 *
		 * get_proc_parameter() response from gateway. 
		 * If not in job-mode the response will be forwarded to the requester.
		 */
		cyng::continuation process(std::string trx
			, std::uint8_t group
			, cyng::buffer_t srv_id
			, sml::obis_path
			, cyng::param_t values);

		/**
		 * @brief slot [4]
		 *
		 * sml.public.close.response
		 *
		 * @param trx transaction id as string
		 */
		cyng::continuation process(std::string trx);

		/**
		 * @brief slot [5]
		 * Incoming message from gateway.
		 *
		 * GetList.Res
		 * @see session_state
		 */
		cyng::continuation process(std::string trx
			, cyng::buffer_t srv_id
			, sml::obis_path	//	OBIS (path)
			, cyng::param_map_t);

		/**
		 * @brief slot [6]
		 *
		 * attention code
		 */
		cyng::continuation process(std::string trx
			, std::string server
			, cyng::buffer_t const& code);

		/**
		 * @brief slot [7]
		 *
		 * Add new entry in work queue. 
		 * These requests are routed via the master node and originate, 
		 * for example, from the dash GUI.
		 */
		cyng::continuation process(boost::uuids::uuid tag,		//	[0] ident tag (target)
			boost::uuids::uuid source,	//	[1] source tag
			std::uint64_t seq,			//	[2] cluster seq
			boost::uuids::uuid ws_tag,	//	[3] ws tag (origin)
			std::string channel,		//	[4] channel
			cyng::buffer_t code,		//	[5] OBIS root code
			cyng::vector_t key,			//	[6] TGateway/TDevice PK
			cyng::tuple_t params,		//	[8] parameters
			cyng::buffer_t srv,			//	[9] server id
			std::string name,			//	[10] name
			std::string	pwd				//	[11] pwd
		);

		/**
		 * @brief slot [8]
		 *
		 * get profile list response
		 */
		cyng::continuation process(std::string trx
			, std::uint32_t act_time
			, std::uint32_t reg_period
			, std::uint32_t val_time
			, sml::obis_path
			, cyng::buffer_t srv_id
			, std::uint32_t	stat
			, cyng::param_map_t params);

		/**
		 * @brief slot [9]
		 *
		 * communication with this task
		 */
		cyng::continuation process(boost::uuids::uuid,		//	[0] ident tag
			boost::uuids::uuid,		//	[1] source tag
			std::uint64_t,			//	[2] cluster seq
			boost::uuids::uuid,		//	[3] ws tag (origin)
			std::string,			//	[4] job
			cyng::vector_t,			//	[5] TGateway/TDevice pk
			cyng::vector_t,			//	[6] sections
			cyng::buffer_t,			//	[7] server id
			std::string,			//	[8] name
			std::string);			//	[9] pwd

	private:
		void run_queue();
		void execute_cmd(sml::req_generator& sml_gen
			, proxy_data const&
			, cyng::tuple_reader const& params);
		void execute_cmd_get_profile_list(sml::req_generator& sml_gen
			, proxy_data const&
			, cyng::tuple_reader const& params);
		void execute_cmd_get_proc_param(sml::req_generator& sml_gen
			, proxy_data const&
			, cyng::tuple_reader const& params);
		void execute_cmd_set_proc_param(sml::req_generator& sml_gen
			, proxy_data const&
			, cyng::tuple_reader const& params);
		void execute_cmd_get_list_request(sml::req_generator& sml_gen
			, proxy_data const&
			, cyng::tuple_reader const& params);
		void execute_cmd_set_proc_param_ipt(sml::req_generator& sml_gen
			, proxy_data const&
			, cyng::vector_t vec);
		void execute_cmd_set_proc_param_wmbus(sml::req_generator& sml_gen
			, proxy_data const&
			, cyng::tuple_t const& params);
		void execute_cmd_set_proc_param_iec(sml::req_generator& sml_gen
			, proxy_data const&
			, cyng::tuple_t const& params);
		void execute_cmd_set_proc_param_activate(sml::req_generator& sml_gen
			, proxy_data const&
			, std::uint8_t nr
			, cyng::buffer_t const&);
		void execute_cmd_set_proc_param_deactivate(sml::req_generator& sml_gen
			, proxy_data const&
			, std::uint8_t nr
			, cyng::buffer_t const&);
		void execute_cmd_set_proc_param_delete(sml::req_generator& sml_gen
			, proxy_data const&
			, std::uint8_t nr
			, cyng::buffer_t const&);
		void execute_cmd_set_proc_param_meter(sml::req_generator& sml_gen
			, proxy_data const&
			, cyng::buffer_t const&
			, std::string 
			, std::string
			, std::string
			, std::string);

		/**
		 * Store proxy data in transaction/output map
		 */
		void push_trx(std::string, proxy_data const&);

		/**
		 * update configuration cache
		 */
		void update_cfg_cache(std::string srv
			, sml::obis_path root
			, proxy_data const& pd
			, cyng::param_map_t const& params);

		/**
		 * Make configuration cache ready to store the specfied paths.
		 */
		void adjust_cache(cyng::buffer_t srv_id, sml::obis_path&&);

		/**
		 * prepare cache to store store all active devices and 
		 * queue the required queries.
		 */
		void start_job_access_rights(boost::uuids::uuid tag,
			boost::uuids::uuid source,
			std::uint64_t seq,
			boost::uuids::uuid origin,
			cyng::vector_t pk,
			cyng::buffer_t srv_id,
			std::string name,
			std::string pwd);

		void send_access_rights_from_cache(boost::uuids::uuid tag,
			boost::uuids::uuid source,
			std::uint64_t seq,
			boost::uuids::uuid origin,
			cyng::vector_t pk,
			cyng::buffer_t srv_id,
			std::string name,
			std::string pwd);

		/**
		 * now the responses to the queries for access rights are coming in
		 * the next bunch of queries has to generated.
		 */
		void process_job_result(sml::obis_path path
			, proxy_data const&
			, cyng::param_map_t const& params);

		void queue_access_right_queries(proxy_data const& prx, cyng::param_map_t const& params);

		void place_access_right_request(boost::uuids::uuid tag
			, boost::uuids::uuid source
			, std::uint64_t seq
			, boost::uuids::uuid origin
			, std::uint8_t role
			, std::uint8_t	user
			, std::uint8_t meter
			, cyng::vector_t gw
			, cyng::buffer_t srv_id
			, std::string name
			, std::string pwd);

	private:
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;
		bus::shared_type bus_;
		cyng::controller& vm_;
		std::chrono::seconds const timeout_;
		std::chrono::system_clock::time_point const start_;
		sml::parser parser_;

		/**
		 * All incoming requests from the network are first
		 * queued in this input queue.
		 */
		input_queue		input_queue_;

		/**
		 * The output map stores the relation between a sent SML message
		 * and the original sender profile (proxy data) using the transaction ID.
		 * The transaction ID of the request is returned in the response.
		 * The output helps to redirect the responses from the gateway
		 * to the origin requester.
		 */
		output_map		output_map_;

		/**
		 * This is an counter for all pending requests sent
		 * to the gateway.
		 * If an element is removed from the input queue the counter
		 * will be incremented. After receiving an "sml.public.close.response"
		 * the counter will be decremented.
		 */
		std::size_t		open_requests_;

		/**
		 * gateway proxy state
		 */
		enum class GWPS {
			OFFLINE_,
			WAITING_,
			CONNECTED_,
		}	state_;

		/**
		 * Maintaining an configuration cache is optional
		 */
		std::unique_ptr<config_cache>	config_cache_;
	};

	cyng::mac48 get_mac();

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
