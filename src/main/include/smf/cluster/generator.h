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
	 */
	cyng::vector_t bus_res_subscribe(std::string const&
		, cyng::vector_t const&
		, cyng::vector_t const&
		, std::uint64_t generation
		, boost::uuids::uuid tag
		, std::size_t tsk);

	/**
	 * database operations
	 * The task ID is optional.
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
	cyng::vector_t bus_req_reboot_client(cyng::vector_t const&
		, boost::uuids::uuid source
		, boost::uuids::uuid tag_ws);

	/**
	 * Send a process parameter request to an gateway
	 * 
	 * @param key key for TGateway table
	 * @param source source tag
	 * @param vec vector of parameter requests
	 * @param tag_ws websocket session tag
	 */
	cyng::vector_t bus_req_query_gateway(cyng::vector_t const&
		, boost::uuids::uuid source
		, cyng::vector_t vec	//	params
		, boost::uuids::uuid tag_ws);

	/**
	 * Response for bus.req.query.status.word
	 *
	 * @param source source tag
	 * @param srv server id
	 */
	cyng::vector_t bus_res_query_status_word(boost::uuids::uuid source
		, std::uint64_t seq
		, boost::uuids::uuid tag_ws
		, std::string srv
		, cyng::attr_map_t const&);

	/**
	 * Response for bus.req.query.srv.visible
	 *
	 * @param source source tag
	 * @param srv server id
	 * @param nr list number
	 * @param meter meter id
	 * @param st last status message
	 */
	cyng::vector_t bus_res_query_srv_visible(boost::uuids::uuid source
		, std::uint64_t seq
		, boost::uuids::uuid tag_ws
		, std::uint16_t nr
		, std::string srv
		, std::string meter
		, std::string dclass
		, std::chrono::system_clock::time_point st
		, std::uint32_t srv_type);


	/**
	 * Response for bus.req.query.srv.active
	 *
	 * @param source source tag
	 * @param srv server id
	 * @param nr list number
	 * @param meter meter id
	 * @param st last status message
	 */
	cyng::vector_t bus_res_query_srv_active(boost::uuids::uuid source
		, std::uint64_t seq
		, boost::uuids::uuid tag_ws
		, std::uint16_t nr
		, std::string srv
		, std::string meter
		, std::string dclass
		, std::chrono::system_clock::time_point st
		, std::uint32_t srv_type);

	/**
	 * Response for bus.req.query.firmware
	 *
	 * @param source source tag
	 * @param srv server id
	 * @param nr list number
	 * @param section firmware name/section
	 * @param version manufacturer-specific version
	 * @param active true/false
	 */
	cyng::vector_t bus_res_query_firmware(boost::uuids::uuid source
		, std::uint64_t seq
		, boost::uuids::uuid tag_ws
		, std::uint16_t nr
		, std::string srv
		, std::string section
		, std::string version
		, bool active);

	/**
	 * Response for bus.req.query.memory
	 *
	 * @param source source tag
	 * @param srv server id
	 * @param mirror usage of mirror memory in %
	 * @param mirror usage of temp memory in %
	 */
	cyng::vector_t bus_res_query_memory(boost::uuids::uuid source
		, std::uint64_t seq
		, boost::uuids::uuid tag_ws
		, std::string srv
		, std::uint8_t mirror
		, std::uint8_t tmp);

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
	cyng::vector_t client_req_reboot(boost::uuids::uuid tag
		, boost::uuids::uuid source
		, std::uint64_t seq
		, boost::uuids::uuid tag_ws
		, cyng::buffer_t const& server
		, std::string const& user
		, std::string const& pwd);

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
	cyng::vector_t client_req_query_gateway(boost::uuids::uuid tag
		, boost::uuids::uuid source
		, std::uint64_t seq
		, cyng::vector_t vec
		, boost::uuids::uuid tag_ws
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
