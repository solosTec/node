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
#include <cyng/log/logger.h>
#include <cyng/obj/tag.hpp>

#include <filesystem>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/random_generator.hpp>

namespace cyng {
	class controller;
}

namespace smf {
	namespace config {

		class controller_base
		{
		public:
			controller_base(startup const&);

			virtual bool run_options(boost::program_options::variables_map&);

			/**
			 * Prepare a default environment and call the derived run() method
			 */
			virtual int run();

		protected:
			virtual cyng::vector_t create_default_config(std::chrono::system_clock::time_point&&
				, std::filesystem::path&& tmp
				, std::filesystem::path&& cwd) = 0;
			virtual void print_configuration(std::ostream&);

			virtual void run(cyng::controller&, cyng::logger, cyng::object const& cfg, std::string const& node_name) = 0;
			virtual void shutdown(cyng::logger, cyng::registry&) = 0;


			void write_config(cyng::vector_t&&);

			/**
			 * generate a random UUID without requiring system entropy
			 */
			[[nodiscard]]
			boost::uuids::uuid get_random_tag() const;

			/**
			 * Convert an object of type string into an UUID
			 */
			[[nodiscard]]
			boost::uuids::uuid read_tag(cyng::object) const;

			[[nodiscard]]
			cyng::object read_config_section(std::string const& json_path, std::size_t config_index);

		protected:
			startup const& config_;

		private:
			mutable boost::uuids::random_generator_mt19937 uidgen_;

		};

		void stop_tasks(cyng::logger, cyng::registry&, std::string);
	}
}

#endif
