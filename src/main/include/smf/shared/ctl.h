/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */
#ifndef NODE_CTL_H
#define NODE_CTL_H

#include <cyng/log.h>
#include <cyng/intrinsics/sets.h>
#include <string>
#include <cstdint>
#include <boost/predef.h>	//	requires Boost 1.55
#if BOOST_OS_WINDOWS
#include <boost/asio.hpp>
#include <cyng/scm/service.hpp>
#endif
#include <boost/uuid/random_generator.hpp>

namespace cyng
{
	template <typename T> class reader;

	namespace async {
		class mux;
	}
}

namespace node
{

	/**
	 * wait for system signals
	 */
	bool wait(cyng::logging::log_ptr logger);

	/**
	 * controller base class
	 */
	class ctl
	{
	public:
		/**
		 * @param index index of the configuration vector (mostly == 0)
		 * @param pool_size thread pool size
		 * @param json_path path to JSON configuration file
		 */
		ctl(unsigned int index
			, unsigned int pool_size
			, std::string const& json_path
			, std::string node_name);

		/**
		 * Start controller loop.
		 * The controller loop controls startup and shutdown of the server.
		 *
		 * @return EXIT_FAILURE in case of an error, otherwise EXIT_SUCCESS.
		 */
		int run(bool console);

		/**
		 * create a configuration file with default values.
		 *
		 * @return EXIT_FAILURE in case of an error, otherwise EXIT_SUCCESS.
		 */
		int create_config() const;

		/**
		 * Read JSON configuration file and prints content
		 *
		 * @return EXIT_FAILURE in case of an error, otherwise EXIT_SUCCESS.
		 */
		int print_config(std::ostream&) const;

		/**
		 * Connect to an database and create all required tables
		 *
		 * @param section section name that defines the connection string
		 * @return EXIT_FAILURE in case of an error, otherwise EXIT_SUCCESS.
		 */
		int init_config_db(std::string section);

#if BOOST_OS_WINDOWS
		/**
		 * Implementation of the control handler.
		 * Forward events from service controller to service.
		 */
		void control_handler(DWORD);
#endif

	protected:
		virtual bool start(cyng::async::mux&, cyng::logging::log_ptr, cyng::reader<cyng::object> const& cfg, boost::uuids::uuid);
		virtual cyng::vector_t create_config(std::fstream&, boost::filesystem::path&& tmp, boost::filesystem::path&& cwd) const;

		/**
		 * Overwrite this method to implement a database initialization
		 */
		virtual int prepare_config_db(cyng::param_map_t&&);


	private:
		bool pre_start(cyng::async::mux&, cyng::logging::log_ptr, cyng::object const&);
		std::string get_logfile_name() const;

	protected:
		unsigned int const config_index_;
		unsigned int const pool_size_;
		std::string const json_path_;
		std::string const node_name_;
		mutable boost::uuids::random_generator uidgen_;
	};

#if BOOST_OS_WINDOWS
	/**
	 * run as windows service
	 */
	template <typename CTL >
	int run_as_service(CTL&& ctrl, std::string const& srv_name)
	{
		//
		//	define service type
		//
		typedef service< CTL >	service_type;

		//
		//	messages
		//
		static std::string msg_01 = "startup service [" + srv_name + "]";
		static std::string msg_02 = "An instance of the [" + srv_name + "] service is already running";
		static std::string msg_03 = "***Error 1063: The [" + srv_name + "] service process could not connect to the service controller";
		static std::string msg_04 = "The [" + srv_name + "] service is configured to run in does not implement the service";

		//
		//	create service
		//
		::OutputDebugString(msg_01.c_str());
		service_type srv(std::move(ctrl), srv_name);

		//
		//	starts dispatcher and calls service main() function 
		//
		const DWORD r = srv.run();
		switch (r)
		{
		case ERROR_SERVICE_ALREADY_RUNNING:
			//	An instance of the service is already running.
			::OutputDebugString(msg_02.c_str());
			break;
		case ERROR_FAILED_SERVICE_CONTROLLER_CONNECT:
			//
			//	The service process could not connect to the service controller.
			//	Typical error message, when running in console mode.
			//
			::OutputDebugString(msg_03.c_str());
			std::cerr
				<< msg_03
				<< std::endl
				;
			break;
		case ERROR_SERVICE_NOT_IN_EXE:
			//	The executable program that this service is configured to run in does not implement the service.
			::OutputDebugString(msg_04.c_str());
			break;
		default:
		{
			std::stringstream ss;
			ss
				<< '['
				<< srv_name
				<< "] service dispatcher stopped: "
				<< r;
			const std::string msg = ss.str();
			::OutputDebugString(msg.c_str());
		}
		break;
		}

		return EXIT_SUCCESS;
	}

#endif

	std::pair<cyng::object, bool> get_config_section(cyng::object config, unsigned int config_index);
	std::pair<cyng::object, bool> read_config_section(std::string const& json_path, unsigned int config_index);

}
#endif
