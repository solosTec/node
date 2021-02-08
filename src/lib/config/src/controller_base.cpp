/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/controller_base.h>
#include <smf.h>

#include <cyng/obj/factory.hpp>
#include <cyng/io/ostream.h>
#include <cyng/io/serialize.h>

#include <fstream>
#include <iostream>

#include <boost/uuid/random_generator.hpp>

namespace smf {
	namespace config {

		controller_base::controller_base(startup const& config)
			: config_(config)
		{}

		bool controller_base::run_options(boost::program_options::variables_map& vars) {

			if (vars["default"].as< bool >())	{
				//	write default configuration
				write_config(create_default_config(std::chrono::system_clock::now()
					, std::filesystem::temp_directory_path()
					, std::filesystem::current_path()));
				return true;
			}
			if (vars["show"].as< bool >())	{
				//	show configuration
				print_configuration(std::cout);
				return true;
			}
#if defined(BOOST_OS_WINDOWS_AVAILABLE)
			if (vars["service.enabled"].as< bool >()) {
				//	run as service 
				//::OutputDebugString("start master node");
				//const std::string srv_name = vm["service.name"].as< std::string >();
				//::OutputDebugString(srv_name.c_str());
				//return node::run_as_service(std::move(ctrl), srv_name);
			}
#endif

			return false;
		}

		void controller_base::write_config(cyng::vector_t&& vec) {

			//std::cout << cyng::to_string(vec) << std::endl;

			std::fstream fso(config_.json_path_, std::ios::trunc | std::ios::out);
			if (fso.is_open()) {

				std::cout
					<< "write to file "
					<< config_.json_path_
					<< std::endl;

				//
				//	get default values
				//
				if (vec.empty()) {
					std::cerr
						<< "** warning: configuration is empty"
						<< std::endl;
				}
				auto const obj = cyng::make_object(std::move(vec));
				cyng::io::serialize_json(fso, obj);
				cyng::io::serialize_json(std::cout, obj);
				std::cout << std::endl;
			}
			else
			{
				std::cerr
					<< "** error: unable to open file "
					<< config_.json_path_
					<< std::endl;
			}
		}

		boost::uuids::uuid controller_base::get_random_tag() const {
			boost::uuids::uuid u;
			int pos = 0;
			auto const start = std::chrono::system_clock::now();
			for (boost::uuids::uuid::iterator it = u.begin(), end = u.end(); it != end; ++it, ++pos) {

				unsigned long random_value{ 0 };
				std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - start;
				if (pos == sizeof(unsigned long)) {
					random_value = static_cast<unsigned long>(elapsed_seconds.count() * 4294967296.0);
					pos = 0;
				}

				// static_cast gets rid of warnings of converting unsigned long to boost::uint8_t
				*it = static_cast<boost::uuids::uuid::value_type>((random_value >> (pos * 8)) & 0xFF);
			}

			boost::uuids::detail::set_uuid_random_vv(u);
			return u;

		}

	}
}

