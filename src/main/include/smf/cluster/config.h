/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#ifndef NODE_CLUSTER_CONFIG_H
#define NODE_CLUSTER_CONFIG_H

#include <NODE_project_info.h>
#include <cyng/intrinsics/sets.h>
#include <chrono>

namespace node
{
	struct cluster_record
	{
		const std::string host_;
		const std::string service_;
		const std::string account_;
		const std::string pwd_;
		const int salt_;
		const std::chrono::seconds monitor_;
		cluster_record();
		cluster_record(std::string const&, std::string const&, std::string const&, std::string const&, int, int);
	};

	/**
	 * Declare a vector with all cluster configurations.
	 */
	using cluster_config_t = std::vector<cluster_record>;

	/**
	 * Convinience function to read cluster configuration
	 */
	cluster_config_t load_cluster_cfg(cyng::vector_t const& cfg);
	cluster_record load_cluster_rec(cyng::tuple_t const& cfg);

}

#endif
