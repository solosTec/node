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
#include <ostream>

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
		 * debug helper
		 */
		std::ostream& operator<<(std::ostream& os, server const&);

		/**
		 * read a configuration map with "service", "host", "account", "pwd",
		 * and "salt"
		 */
		server read_config(cyng::param_map_t const& pmap);

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

	}	//	cluster
}


#endif
