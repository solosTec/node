/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_CONFIG_H
#define SMF_CONFIG_H

#include <boost/predef.h>
#include <string>
#if defined(BOOST_OS_LINUX_AVAILABLE)
#include <sys/time.h>
#include <sys/resource.h>
#endif
#include <boost/program_options.hpp>
#include <cyng/obj/intrinsics/severity.h>

namespace smf {
	namespace config {

		/**
		 * Takes the node name and derives a default path to the .cfg file
		 */
		std::string derive_cfg_file_name(std::string node);

#if defined(BOOST_OS_WINDOWS_AVAILABLE)
		/**
		 * Build a clean service name with an uppercase first character.
		 * There are different names for debug and release builds. 
		 */
		std::string derive_service_name(std::string node);
#endif

		/**
		 * Store generic config/startup data
		 */
		struct startup
		{
			startup(std::string node);
			startup(startup const&) = default;

			/**
			 * @return log level as string
			 */
			cyng::severity get_log_level() const noexcept;

			/**
			 * node name
			 */
			std::string const node_;
			std::string const cfg_default_;

			/**
			 * .cfg file with more options
			 */
			std::string config_file_;

			/**
			 * path to JSON formatted configuration file
			 */
			std::string json_path_;

			/**
			 * The JSON configuration files (can) contains a vector
			 * of different configuration options. The config index allows
			 * to select one of these entries.
			 * Default is 0.
			 */
			std::size_t config_index_;

			/**
			 * Thread pool size.
			 * Default is 2.
			 */
			std::size_t pool_size_;

#if defined(BOOST_OS_LINUX_AVAILABLE)
			struct rlimit rl_;
#endif

			std::string log_level_str_;

		};

		/**
		 * Get the RLIMIT_NOFILE on linux. On Windows effectively nothing happens
		 */
		bool get_resource_limit(startup&);
		bool set_resource_limit(startup const&);

		/**
		 * get generic options
		 */
		boost::program_options::options_description get_generic_options(startup&);

		/**
		 * set default options
		 */
		boost::program_options::options_description get_default_options(startup&);

		/**
		 * Handle some generic options and read cfg file.
		 * --version, --build, --ip
		 * 
		 * @return true if all requested options are satisfied.
		 */
		bool complete_generic_options(startup const&, boost::program_options::options_description&, boost::program_options::variables_map&);

		/**
		 * read .cfg file
		 */
		void read_cfg_file(std::string, boost::program_options::options_description&, boost::program_options::variables_map&);

		void print_version_info(std::ostream& os, std::string const& name);
		void print_build_info(std::ostream& os);
		void print_net_config(std::ostream& os);
	}
}

#endif
