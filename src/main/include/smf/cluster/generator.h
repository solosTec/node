/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_CLUSTER_GENERATOR_H
#define NODE_CLUSTER_GENERATOR_H

#include <NODE_project_info.h>
#include <cyng/intrinsics/sets.h>
#include <cyng/vm/generator.h>

namespace node
{
	/**
	 * Send login request to cluster
	 */
	cyng::vector_t bus_req_login(std::string const& host
		, std::string const& service
		, std::string const& account
		, std::string const& pwd
		, bool auto_config
		, std::uint32_t group
		, std::string const& node_class);


	/**
	 * Subscribe specified table
	 * The task ID is optional.
	 */
	cyng::vector_t bus_req_subscribe(std::string const&, std::size_t tsk);
	cyng::vector_t bus_req_unsubscribe(std::string const&);

	/** 
	 * similiar to db.insert() but contains more information
	 * The task ID is optional.
	 */
	cyng::vector_t bus_res_subscribe(std::string const&
		, cyng::vector_t const&
		, cyng::vector_t const&
		, std::uint64_t generation
		, boost::uuids::uuid tag
		, std::size_t tsk);

	/**
	 * database operations
	 */
	cyng::vector_t bus_req_db_insert(std::string const&
		, cyng::vector_t const&
		, cyng::vector_t const&
		, std::uint64_t generation
		, boost::uuids::uuid source);

	cyng::vector_t bus_res_db_insert(std::string const&
		, cyng::vector_t const&
		, cyng::vector_t const&
		, std::uint64_t generation);

	cyng::vector_t bus_req_db_modify(std::string const&
		, cyng::vector_t const&
		, cyng::attr_t const&
		, boost::uuids::uuid source);

	cyng::vector_t bus_res_db_modify(std::string const&
		, cyng::vector_t const&
		, cyng::attr_t const&
		, std::uint64_t gen);

	cyng::vector_t bus_res_db_modify(std::string const&
		, cyng::vector_t const&
		, cyng::param_t&&
		, std::uint64_t gen);

	cyng::vector_t bus_req_db_modify(std::string const&
		, cyng::vector_t const&
		, cyng::param_t const&
		, std::uint64_t gen
		, boost::uuids::uuid source);

	cyng::vector_t bus_req_db_remove(std::string const&
		, cyng::vector_t const&
		, boost::uuids::uuid source);

	cyng::vector_t bus_res_db_remove(std::string const&
		, cyng::vector_t const&);

	cyng::vector_t bus_db_clear(std::string const&);

	cyng::vector_t bus_req_watchdog();
	cyng::vector_t bus_res_watchdog(std::uint64_t seq, std::chrono::system_clock::time_point);

	/**
	 * place a system message
	 */
	cyng::vector_t bus_insert_msg(cyng::logging::severity, std::string const&);

	/**
	 * place a LoRa uplink event
	 */
	cyng::vector_t bus_insert_LoRa_uplink(cyng::object tp
		, std::string const& devEUI
		, std::uint16_t FPort
		, std::uint32_t FCntUp
		, std::uint32_t ADRbit
		, std::uint32_t MType
		, std::uint32_t FCntDn
		, std::string const& customerID
		, std::string const& payload
		, boost::uuids::uuid tag);

	/**
	 * Send an arbitrary function call to receiver, which will send it back.
	 */
	template < typename ...Args >
	cyng::vector_t bus_reflect(std::string const& name, Args&&... args)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"

			//	reflect
			, cyng::generate_invoke_remote_unwinded("stream.serialize", cyng::generate_invoke_reflect_unwinded(name, std::forward<Args>(args)...))
			, cyng::generate_invoke_remote_unwinded("stream.flush")
			)

			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	/**
	 * Terminate a client
	 */
	cyng::vector_t bus_req_stop_client(cyng::vector_t const&
		, boost::uuids::uuid source);

	/**
	 * Reboot a client
	 */
	//cyng::vector_t bus_req_reboot_client(cyng::vector_t const&
	//	, boost::uuids::uuid source
	//	, boost::uuids::uuid tag_ws);

	/**
	 * Send a process parameter request to an gateway
	 * 
	 * @param key key for TGateway table
	 * @param source source tag
	 * @param vec vector of parameter requests
	 * @param tag_ws websocket session tag
	 */
	//cyng::vector_t bus_req_query_gateway(cyng::vector_t const&
	//	, boost::uuids::uuid source
	//	, cyng::vector_t vec	//	params
	//	, boost::uuids::uuid tag_ws);

	/**
	 * Send a process parameter request to an gateway
	 *
	 * @param key key for TGateway/TDevice table
	 * @param tag_ws websocket session tag
	 * @param channel channel name (get.proc.param, set.proc.param, ...)
	 * @param vec vector of parameter requests
	 */
	cyng::vector_t bus_req_gateway_proxy(cyng::vector_t const& key
		, boost::uuids::uuid tag_ws
		, std::string channel
		, cyng::vector_t sections
		, cyng::vector_t params);

	/**
	 * Response for bus_req_gateway_proxy()
	 *
	 * @param source source tag
	 * @param srv server id
	 */
	cyng::vector_t bus_res_gateway_proxy(boost::uuids::uuid ident
		, boost::uuids::uuid source
		, std::uint64_t seq
		, cyng::vector_t key
		, boost::uuids::uuid tag_ws
		, std::string channel
		, std::string srv
		, std::string code
		, cyng::param_map_t const&);

	/**
	 * Response for bus_req_query_gateway()
	 *
	 * @param source source tag
	 * @param srv server id
	 */
	//cyng::vector_t bus_res_query_gateway(boost::uuids::uuid source
	//	, std::uint64_t seq
	//	, boost::uuids::uuid tag_ws
	//	, std::string srv
	//	, std::string code
	//	, cyng::param_map_t const&);

	/**
	 * Send back attention codes
	 */
	cyng::vector_t bus_res_attention_code(boost::uuids::uuid source
		, std::uint64_t seq
		, boost::uuids::uuid tag_ws
		, std::string srv
		, cyng::buffer_t code
		, std::string msg);

	/**
	 * Send a process parameter change request to an gateway
	 *
	 * @param section section to modify
	 * @param key key for TGateway table
	 * @param source source tag
	 * @param params parameter list with named values
	 */
	cyng::vector_t bus_req_modify_gateway(std::string section
		, cyng::vector_t const& key
		, boost::uuids::uuid source
		, cyng::param_map_t	params);

	/**
	 * data bus
	 */
	cyng::vector_t bus_req_push_data(std::string const& class_name
		, std::string const& channel_name
		, bool distribution	//	single, all
		, cyng::vector_t const&
		, cyng::vector_t const&
		, boost::uuids::uuid source);

	/**
	 * deliver bus data
	 */
	cyng::vector_t bus_req_push_data(std::uint64_t seq
		, std::string const& channel_name
		, cyng::vector_t const&
		, cyng::vector_t const&
		, boost::uuids::uuid source);

	cyng::vector_t bus_res_push_data(std::uint64_t seq
		, std::string const& class_name
		, std::string const& channel_name
		, std::size_t);

	/**
	 * client management
	 */
	cyng::vector_t client_req_login(boost::uuids::uuid tag
		, std::string const& name
		, std::string const& pwd
		, std::string const& scheme //	login scheme
		, cyng::param_map_t const& bag);

	cyng::vector_t client_res_login(boost::uuids::uuid tag
		, std::uint64_t seq
		, bool success
		, std::string const& name
		, std::string const& msg
		, std::uint32_t query
		, cyng::param_map_t const& bag);

	cyng::vector_t client_req_close(boost::uuids::uuid tag, int);
	cyng::vector_t client_res_close(boost::uuids::uuid tag, std::uint64_t seq, bool success);

	/**
	 * Send reboot request
	 *
	 * @param tag target session
	 * @param source source session
	 * @param seq cluster sequence
	 * @param tag_ws ws tag
	 * @param server server id
	 * @param user login name / account
	 * @param pwd password
	 */
	//cyng::vector_t client_req_reboot(boost::uuids::uuid tag
	//	, boost::uuids::uuid source
	//	, std::uint64_t seq
	//	, boost::uuids::uuid tag_ws
	//	, cyng::buffer_t const& server
	//	, std::string const& user
	//	, std::string const& pwd);

	/**
	 * Send process parameter request
	 * 
	 * @param tag target session
	 * @param source source session
	 * @param seq cluster sequence
	 * @param vec parameters
	 * @param server server id
	 * @param user login name / account
	 * @param pwd password
	 */
	//cyng::vector_t client_req_query_gateway(boost::uuids::uuid tag
	//	, boost::uuids::uuid source
	//	, std::uint64_t seq
	//	, cyng::vector_t vec
	//	, boost::uuids::uuid tag_ws
	//	, cyng::buffer_t const& server
	//	, std::string const& user
	//	, std::string const& pwd);

	/**
	 * Send process parameter request
	 *
	 * @param tag target session
	 * @param source source session
	 * @param seq cluster sequence
	 * @param section request type
	 * @param params parameters
	 * @param server server id
	 * @param user login name / account
	 * @param pwd password
	 */
	//cyng::vector_t client_req_modify_gateway(boost::uuids::uuid tag
	//	, boost::uuids::uuid source
	//	, std::uint64_t seq
	//	, std::string const& section
	//	, cyng::param_map_t params
	//	, cyng::buffer_t const& server
	//	, std::string const& user
	//	, std::string const& pwd);

	/**
	 * Send a request to the IP-T proxy
	 *
	 * @param tag target (IP-T) session
	 * @param source source session tag
	 * @param seq cluster sequence
	 * @param section request type
	 * @param params parameters
	 * @param server server id
	 * @param user login name / account
	 * @param pwd password
	 */
	cyng::vector_t client_req_gateway_proxy(boost::uuids::uuid tag
		, boost::uuids::uuid source
		, std::uint64_t seq
		, cyng::vector_t key
		, boost::uuids::uuid ws
		, std::string const& channel
		, cyng::vector_t sections
		, cyng::vector_t params
		, cyng::buffer_t const& server
		, std::string const& user
		, std::string const& pwd);

	cyng::vector_t client_req_open_push_channel(boost::uuids::uuid tag
		, std::string const& target
		, std::string const& device
		, std::string const& number
		, std::string const& version
		, std::string const& id
		, std::uint16_t timeout
		, cyng::param_map_t const& bag);

	cyng::vector_t client_req_close_push_channel(boost::uuids::uuid tag
		, std::uint32_t channel
		, cyng::param_map_t const& bag);

	cyng::vector_t client_res_open_push_channel(boost::uuids::uuid tag
		, std::uint64_t seq
		, bool success
		, std::uint32_t channel
		, std::uint32_t source
		, std::uint32_t count
		, cyng::param_map_t const& options
		, cyng::param_map_t const& bag);

	cyng::vector_t client_res_close_push_channel(boost::uuids::uuid tag
		, std::uint64_t seq
		, bool success
		, std::uint32_t channel
		, cyng::param_map_t const& bag);

	cyng::vector_t client_req_register_push_target(boost::uuids::uuid tag
		, std::string const& target
		, cyng::param_map_t const& bag);

	cyng::vector_t client_req_deregister_push_target(boost::uuids::uuid tag
		, std::string const& target
		, cyng::param_map_t const& bag);

	cyng::vector_t client_res_register_push_target(boost::uuids::uuid tag
		, std::uint64_t seq
		, bool success
		, std::uint32_t channel
		, cyng::param_map_t const& options
		, cyng::param_map_t const& bag);

	cyng::vector_t client_res_deregister_push_target(boost::uuids::uuid tag
		, std::uint64_t seq
		, bool success
		, std::string const& target
		, cyng::param_map_t const& options
		, cyng::param_map_t const& bag);

	cyng::vector_t client_req_open_connection(boost::uuids::uuid tag
		, std::string const& number
		, cyng::param_map_t const& bag);

	cyng::vector_t client_req_open_connection_forward(boost::uuids::uuid tag
		, std::string const& number
		, cyng::param_map_t const& options
		, cyng::param_map_t const& bag);

	cyng::vector_t client_res_open_connection(boost::uuids::uuid tag
		, std::uint64_t seq
		, bool success
		, cyng::param_map_t const& options
		, cyng::param_map_t const& bag);

	cyng::vector_t client_res_open_connection_forward(boost::uuids::uuid tag
		, std::uint64_t seq
		, bool success
		, cyng::param_map_t const& options
		, cyng::param_map_t const& bag);

	cyng::vector_t client_req_close_connection(boost::uuids::uuid tag
		, bool shutdown
		, cyng::param_map_t&& bag);

	cyng::vector_t client_res_close_connection_forward(boost::uuids::uuid tag
		, std::uint64_t seq
		, bool success
		, cyng::param_map_t const& options
		, cyng::param_map_t const& bag);

	/**
	 * If shutdown is true, no response is required
	 */
	cyng::vector_t client_req_close_connection_forward(boost::uuids::uuid rtag
		, boost::uuids::uuid origin_tag
		, std::uint64_t seq
		, bool shutdown
		, cyng::param_map_t const& options
		, cyng::param_map_t const& bag);

	cyng::vector_t client_res_close_connection(boost::uuids::uuid tag
		, std::uint64_t seq
		, bool success
		, cyng::param_map_t const& options
		, cyng::param_map_t const& bag);

	cyng::vector_t client_req_transmit_data(boost::uuids::uuid tag
		, cyng::param_map_t bag
		, cyng::object const& data);

	cyng::vector_t client_req_transfer_data_forward(boost::uuids::uuid tag
		, std::uint64_t seq
		, cyng::param_map_t bag
		, cyng::object const& data);

	cyng::vector_t client_req_transfer_pushdata(boost::uuids::uuid tag
		, std::uint32_t
		, std::uint32_t
		, cyng::object 
		, cyng::param_map_t const&);

	cyng::vector_t client_req_transfer_pushdata_forward(boost::uuids::uuid tag
		, std::uint32_t
		, std::uint32_t
		, std::uint32_t
		, cyng::object
		, cyng::param_map_t const&);

	cyng::vector_t client_res_transfer_pushdata(boost::uuids::uuid tag
		, std::uint64_t seq
		, std::uint32_t source
		, std::uint32_t channel
		, std::size_t count		//	count
		, cyng::param_map_t const& bag);

	cyng::vector_t client_update_attr(boost::uuids::uuid tag
		, std::string attr
		, cyng::object value
		, cyng::param_map_t const& bag);

	cyng::vector_t client_inc_throughput(boost::uuids::uuid origin
		, boost::uuids::uuid target
		, std::uint64_t);

}

#endif
