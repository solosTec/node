/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_CONFIG_H
#define NODE_IPT_CONFIG_H

#include <NODE_project_info.h>
#include <smf/ipt/scramble_key.h>
#include <cyng/intrinsics/sets.h>
#include <chrono>

namespace node
{
	namespace ipt
	{
		struct master_record
		{
			master_record();
			master_record(master_record const&) = default;
			master_record(std::string const&, std::string const&, std::string const&, std::string const&, scramble_key const&, bool, int);

			std::string const host_;
			std::string const service_;
			std::string const account_;
			std::string const pwd_;
			scramble_key const sk_;	//!< default scramble key
			bool const scrambled_;
			std::chrono::seconds const monitor_;
		};

		/**
		 * Declare a vector with all cluster configurations.
		 */
		using master_config_t = std::vector<master_record>;

		/**
		 * Convinience function to read ipt master configuration
		 */
		master_record load_cluster_rec(cyng::tuple_t const& cfg);

		/**
		 * managing ipt master redundancy
		 */
		struct redundancy
		{
			//redundancy(master_config_t const&);
			redundancy(master_config_t const&, std::uint8_t idx);
			redundancy(redundancy const&);

			/**
			 * switch to next redundancy.
			 *
			 * @return true if other redundancy was available
			 */
			bool next() const;

			/**
			 * get current reduncancy
			 */
			master_record const& get() const;

			/**
			 * Sythesize a string with format: host ':' service
			 */
			std::string get_address() const;

			/**
			 * config data
			 */
			master_config_t config_;

			/**
			 * index of current ipt master configuration
			 */
			mutable std::uint8_t master_;
			 
		};

		/**
		 * Convinience function to read ipt master configuration
		 */
		redundancy load_cluster_cfg(cyng::vector_t const& cfg);
	}
}

#endif
