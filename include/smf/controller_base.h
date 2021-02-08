/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_CONFIG_CONTROLLER_BASE_H
#define SMF_CONFIG_CONTROLLER_BASE_H

#include <smf/config.h>
#include <cyng/obj/intrinsics/container.h>

#include <filesystem>
#include <boost/uuid/uuid.hpp>

namespace smf {
	namespace config {

		class controller_base
		{
		public:
			controller_base(startup const&);

			bool run_options(boost::program_options::variables_map&);

			virtual int run() = 0;

		protected:
			virtual cyng::vector_t create_default_config(std::chrono::system_clock::time_point&&
				, std::filesystem::path&& tmp
				, std::filesystem::path&& cwd) = 0;
			virtual void print_configuration(std::ostream&) = 0;

			void write_config(cyng::vector_t&&);

			/**
			 * generate a random UUID without requiring system entropy
			 */
			boost::uuids::uuid get_random_tag() const;

		protected:
			startup const& config_;

		};
	}
}

#endif
