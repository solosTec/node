/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_CLUSTER_CONFIG_H
#define SMF_CLUSTER_CONFIG_H


#include <cyng/obj/intrinsics/container.h>

#include <chrono>
#include <string>
#include <ostream>

namespace smf
{
	struct server
	{
		server();
		server(server const&) = default;
		server(std::string const&, std::string const&, std::string const&, std::string const&, std::uint64_t);

		std::string const host_;
		std::string const service_;
		std::string const account_;
		std::string const pwd_;
		std::uint64_t const salt_;
	};

	/**
	 * read a configuration map with "service", "host", "account", "pwd",
	 * "def-sk", scrambled" and "monitor"
	 */
	server read_config(cyng::param_map_t const& pmap);

	/**
	 * debug helper
	 */
	std::ostream& operator<<(std::ostream& os, server const&);


	class toggle
	{
	public:
		/**
		 * Declare a vector with all cluster configurations.
		 */
		using server_vec_t = std::vector<server>;

	public:
		toggle(server_vec_t&&);
		toggle(toggle const&) = default;

		/**
		 * Switch redundancy
		 */
		server changeover() const;

		/**
		 * get current reduncancy
		 */
		server get() const;


	private:
		server_vec_t const cfg_;
		mutable std::size_t	active_;	//!<	index of active configuration
	};

	/**
	 * read config vector
	 */
	toggle::server_vec_t read_config(cyng::vector_t const& vec);

}


#endif
