/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_IPT_BUS_CONFIG_H
#define SMF_IPT_BUS_CONFIG_H

#include <smf/ipt/scramble_key.h>

#include <cyng/obj/intrinsics/container.h>

#include <chrono>
#include <string>

namespace smf
{
	namespace ipt	
	{
		struct server
		{
			server();
			server(server const&) = default;
			server(std::string const&, std::string const&, std::string const&, std::string const&, scramble_key const&, bool, int);

			std::string const host_;
			std::string const service_;
			std::string const account_;
			std::string const pwd_;
			scramble_key const sk_;	//!< default scramble key
			bool const scrambled_;
			std::chrono::seconds const monitor_;
		};

		/**
		 * read a configuration map with "service", "host", "account", "pwd",
		 * "def-sk", scrambled" and "monitor"
		 */
		server read_config(cyng::param_map_t const& pmap);

	}	//	ipt
}


#endif
