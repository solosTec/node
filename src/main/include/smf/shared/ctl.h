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
#endif
#include <boost/uuid/random_generator.hpp>

namespace cyng
{
	//class object;

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
		 * @param pool_size thread pool size
		 * @param json_path path to JSON configuration file
		 */
		ctl(unsigned int pool_size, std::string const& json_path, std::string node_name);

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


#if BOOST_OS_WINDOWS
		/**
		 * run as windows service
		 */
		static int run_as_service(ctl&&, std::string const&);
		virtual void control_handler(DWORD);

#endif

	protected:
		virtual bool start(cyng::async::mux&, cyng::logging::log_ptr, cyng::reader<cyng::object> const& cfg, boost::uuids::uuid);
		virtual cyng::vector_t create_config(std::fstream&, boost::filesystem::path&& tmp, boost::filesystem::path&& cwd) const;


	private:
		bool pre_start(cyng::async::mux&, cyng::logging::log_ptr, cyng::object const&);
		std::string get_logfile_name() const;

	protected:
		unsigned int const pool_size_;
		std::string const json_path_;
		std::string const node_name_;
		mutable boost::uuids::random_generator uidgen_;
	};
}
#endif
