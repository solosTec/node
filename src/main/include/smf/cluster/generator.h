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
	cyng::vector_t client_res_close(boost::uuids::uuid tag, std::uint64_t seq);

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

	cyng::vector_t client_res_register_push_target(boost::uuids::uuid tag
		, std::uint64_t seq
		, bool success
		, std::uint32_t channel
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
		, cyng::param_map_t const& bag);

	/**
	 * If shutdown is true, no response is required
	 */
	cyng::vector_t client_req_close_connection_forward(boost::uuids::uuid tag
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
