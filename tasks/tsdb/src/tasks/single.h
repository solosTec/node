/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_TSDB_TASK_SINGLE_H
#define NODE_TSDB_TASK_SINGLE_H

#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/policy.h>
#include <cyng/intrinsics/version.h>

namespace node
{

	class single
	{
	public:
		using msg_0 = std::tuple<std::string
			, std::size_t
			, std::chrono::system_clock::time_point	//	time stamp
			, boost::uuids::uuid	//	session tag
			, std::string	// device name
			, std::string	// event
			, std::string	// description
		>;
		using signatures_t = std::tuple<msg_0>;

	public:
		single(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, boost::filesystem::path root
			, std::string prefix
			, std::string suffix
			, std::chrono::seconds period);

		cyng::continuation run();
		void stop(bool shutdown);

		/**
		 * @brief slot [0]
		 *
		 * insert
		 */
		cyng::continuation process(std::string
			, std::size_t
			, std::chrono::system_clock::time_point	//	time stamp
			, boost::uuids::uuid	//	session tag
			, std::string	// device name
			, std::string	// event
			, std::string);


	private:
		void test_file_size();
		void create_backup_file();

	private:
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;
		boost::filesystem::path const file_name_;
		std::chrono::seconds const period_;
		std::ofstream ofs_;
	};	
}

#endif