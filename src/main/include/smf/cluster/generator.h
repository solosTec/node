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
//#include <cyng/vm/manip.h>
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
		, std::string const& node_class);

	/**
	 * Close socket 
	 */
	cyng::vector_t bus_shutdown();

	/**
	 * Subscribe specified table
	 * The task ID is optional.
	 */
	cyng::vector_t bus_req_subscribe(std::string const&, std::size_t tsk);
	cyng::vector_t bus_req_unsubscribe(std::string const&);

	/**
	 * database operations
	 * The task ID is optional.
	 */
	cyng::vector_t bus_db_insert(std::string const&
		, cyng::vector_t const&
		, cyng::vector_t const&
		, std::uint64_t generation);

	cyng::vector_t bus_db_insert(std::string const&
		, cyng::vector_t const&
		, cyng::vector_t const&
		, std::uint64_t generation
		, boost::uuids::uuid tag
		, std::size_t tsk);
	/**
	 * Send an arbitrary function call to receiver, which will send it back.
	 */
	template < typename ...Args >
	cyng::vector_t bus_reflect(std::string const& name, Args&&... args)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke("stream.serialize"

			//	reflect
			, cyng::generate_invoke_remote("stream.serialize", cyng::generate_invoke_reflect(name, std::forward<Args>(args)...))
			, cyng::generate_invoke_remote("stream.flush")
			)

			<< cyng::generate_invoke("stream.flush")
			;

	}


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
		, std::string const& msg 
		, cyng::param_map_t const& bag);

	cyng::vector_t client_req_open_push_channel(boost::uuids::uuid tag
		, std::string const& target
		, std::string const& device
		, std::string const& number
		, std::string const& version
		, std::string const& id
		, std::uint16_t timeout
		, cyng::param_map_t const& bag);

	cyng::vector_t client_res_open_push_channel(boost::uuids::uuid tag
		, std::uint64_t seq
		, bool success
		, std::uint32_t channel
		, std::uint32_t source
		, cyng::param_map_t const& options
		, cyng::param_map_t const& bag);
}

#endif
