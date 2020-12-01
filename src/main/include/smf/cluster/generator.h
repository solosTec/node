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

	cyng::vector_t bus_req_db_merge(std::string const&
		, cyng::vector_t const&
		, cyng::vector_t const&
		, std::uint64_t generation
		, boost::uuids::uuid source);

	cyng::vector_t bus_req_db_update(std::string const&
		, cyng::vector_t const&
		, cyng::vector_t const&
		, std::uint64_t generation
		, boost::uuids::uuid source);

	cyng::vector_t bus_res_db_modify(std::string const&
		, cyng::vector_t const&
		, cyng::attr_t const&
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
	 * remove duplicates, orphaned entries, ...
	 */
	cyng::vector_t bus_db_cleanup(std::string const&
		, boost::uuids::uuid source);

	/**
	 * place a system message
	 */
	cyng::vector_t bus_insert_msg(cyng::logging::severity, std::string const&);

	/**
	 *  Update the "clients" counter in the "_Cluster" table
	 */
	cyng::vector_t bus_update_client_count(std::uint64_t count);

	/**
	 * place a LoRa uplink event
	 */
	cyng::vector_t bus_insert_LoRa_uplink(cyng::object tp
		, cyng::mac64 devEUI
		, std::uint16_t FPort
		, std::uint32_t FCntUp
		, std::uint32_t ADRbit
		, std::uint32_t MType
		, std::uint32_t FCntDn
		, std::string const& customerID
		, std::string const& payload
		, boost::uuids::uuid tag);

	/**
	 * place a wMBus uplink event
	 * 
	 * @param frame_type ci_field
	 */
	cyng::vector_t bus_insert_wMBus_uplink(std::chrono::system_clock::time_point
		, std::string const& srv_id
		, std::uint8_t medium
		, std::string  manufacturer
		, std::uint8_t frame_type
		, std::string const& payload
		, boost::uuids::uuid tag);

	/**
	 * place a IEC uplink event
	 *
	 * @param frame_type ci_field
	 */
	cyng::vector_t bus_insert_IEC_uplink(std::chrono::system_clock::time_point
		, std::string const& event
		, boost::asio::ip::tcp::endpoint
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
	 * Send a process parameter request to an gateway
	 *
	 * @param tag_ws websocket session tag (origin)
	 * @param key key for TGateway/TDevice table
	 * @param channel channel name (get.proc.param, set.proc.param, ...)
	 * @param params tuple of parameter requests
	 */
	cyng::vector_t bus_req_com_sml(boost::uuids::uuid tag_ws
		, std::string msg_type
		, cyng::buffer_t code
		, cyng::vector_t const& gw
		, cyng::tuple_t params);

	/**
	 * Response for bus_req_com_sml()
	 *
	 * @param source source tag
	 * @param srv server id
	 */
	cyng::vector_t bus_res_com_sml(boost::uuids::uuid ident
		, boost::uuids::uuid source
		, std::uint64_t seq
		, cyng::vector_t key
		, boost::uuids::uuid tag_ws
		, std::string channel
		, std::string srv
		, std::vector<std::string> const& path
		, cyng::param_map_t params);

	/**
	 * Send a process parameter request to an proxy
	 *
	 * @param tag_ws websocket session tag (origin)
	 * @param job job to execute
	 * @param key key for TGateway/TDevice table
	 * @param sections vector of strings that describe the data areas involved
	 */
	cyng::vector_t bus_req_com_proxy(boost::uuids::uuid tag_ws
		, std::string job
		, cyng::vector_t const& gw
		, cyng::vector_t sections);

	/**
	 * Response for bus_req_com_proxy()
	 *
	 * @param source source tag
	 * @param srv server id
	 * @param sections vector of OBIS paths
	 */
	cyng::vector_t bus_res_com_proxy(boost::uuids::uuid ident
		, boost::uuids::uuid source
		, std::uint64_t seq
		, cyng::vector_t key
		, boost::uuids::uuid tag_ws
		, std::string channel
		, std::string srv
		, cyng::vector_t sections	//	vector of paths
		, cyng::param_map_t params);

	/**
	 * Send back attention codes
	 */
	cyng::vector_t bus_res_attention_code(boost::uuids::uuid ident
		, boost::uuids::uuid source
		, std::uint64_t seq
		, boost::uuids::uuid tag_ws
		, std::string srv
		, cyng::buffer_t code
		, std::string msg);

	/**
	 * Send a request to a specific task
	 *
	 * @param key key for TGateway/TDevice table
	 * @param tag_ws websocket session tag
	 * @param channel channel name (get.proc.param, set.proc.param, ...)
	 * @param vec vector of parameter requests
	 */
	cyng::vector_t bus_req_com_task(boost::uuids::uuid key
		, boost::uuids::uuid tag_ws
		, std::string channel
		, cyng::vector_t sections
		, cyng::vector_t params);

	/**
	 * Send a request to a specific node
	 *
	 * @param key key for TGateway/TDevice table
	 * @param tag_ws websocket session tag
	 * @param channel channel name (get.proc.param, set.proc.param, ...)
	 * @param vec vector of parameter requests
	 */
	cyng::vector_t bus_req_com_node(boost::uuids::uuid key
		, boost::uuids::uuid tag_ws
		, std::string channel
		, cyng::vector_t sections
		, cyng::vector_t params);

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

	//
	// client management
	//

	 /**
	  * client login
	  */
	cyng::vector_t client_req_login(boost::uuids::uuid tag
		, std::string const& name
		, std::string const& pwd
		, std::string const& scheme //	login scheme
		, cyng::param_map_t const& bag);

	/**
	 * @param tag target session
	 * @param seq cluster sequence (from request)
	 * @param success true if successful
	 * @param name account/user name
	 * @param msg welcome message for logging/debugging purposes
	 * @param query bit field that defines which device parameter should be queried
	 * @param bag echo the data bag that came with the login request. Helps the client to identify the client that sent the request.
	 */
	cyng::vector_t client_res_login(boost::uuids::uuid tag
		, std::uint64_t seq
		, bool success
		, std::string const& name
		, std::string const& msg
		, std::uint32_t query
		, cyng::param_map_t const& bag);

	/**
	 * client is gone (logout)
	 */
	cyng::vector_t client_req_close(boost::uuids::uuid tag, int);
	cyng::vector_t client_res_close(boost::uuids::uuid tag, std::uint64_t seq, bool success);

	/**
	 * Send a request to an gateway over the IP-T proxy
	 *
	 * @param tag target (IP-T) session
	 * @param source source session tag
	 * @param seq cluster sequence
	 * @param origin web session (sender)
	 * @param channel SML message type
	 * @param code OBIS root code
	 * @param gw PK from table TGateway/TDevice
	 * @param server server id
	 * @param user login name / account
	 * @param pwd password
	 */
	cyng::vector_t client_req_gateway(boost::uuids::uuid tag
		, boost::uuids::uuid source
		, std::uint64_t seq
		, boost::uuids::uuid origin

		, std::string const& channel
		, cyng::buffer_t const& code
		, cyng::vector_t gw
		, cyng::tuple_t params

		, cyng::buffer_t const& server
		, std::string const& user
		, std::string const& pwd
	
		, bool cache_enabled);

	/**
	 * Send a request to an gateway itself
	 *
	 * @param tag target (IP-T) session
	 * @param source source session tag
	 * @param seq cluster sequence
	 * @param origin web session (sender)
	 * @param job job to execute
	 * @param gw PK from table TGateway/TDevice
	 * @param sections vector of strings that describe the data areas involved
	 * @param server server id
	 * @param user login name / account
	 * @param pwd password
	 */
	cyng::vector_t client_req_proxy(boost::uuids::uuid tag
		, boost::uuids::uuid source
		, std::uint64_t seq
		, boost::uuids::uuid origin

		, std::string const& job
		, cyng::vector_t gw
		, cyng::vector_t sections

		, cyng::buffer_t const& server
		, std::string const& user
		, std::string const& pwd);

	/**
	 * @param tag session tag
	 * @param target push target name
	 * @param device device name (typically an empty string)
	 * @param number device number (typically an empty string)
	 * @param version device version (typically an empty string)
	 * @param id device id (typically an empty string)
	 * @param timeout maximal timeout in seconds
	 * @param bag arbitrary data that will be included in response
	 */
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

	cyng::vector_t client_internal_connection(boost::uuids::uuid tag
		, bool state);

}

#endif
