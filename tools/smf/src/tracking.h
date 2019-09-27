/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_TOOL_SMF_PLUGIN_TRACKING_H
#define NODE_TOOL_SMF_PLUGIN_TRACKING_H

#include <smf/sml/intrinsics/obis.h>
#include <cyng/vm/context.h>
#include <boost/filesystem.hpp>

namespace node
{
	class cli;
	class tracking
	{
	public:
		tracking(cli*);
		virtual ~tracking();

	private:
		void cmd(cyng::context& ctx);
		std::string get_status() const;
		void load(std::string);
		void list() const;
		void start(std::string);
		void flush(std::string);
		void stop();

	private:
		cli& cli_;
		enum {
			TRACKING_INITIAL,	//	closed
			TRACKING_OPEN,
			TRACKING_RUNNING
		} status_;
		cyng::vector_t data_;
	};

	std::string get_parameter(std::size_t, cyng::vector_t const&, std::string);
	std::string get_filename(std::size_t, cyng::vector_t const&);
	cyng::tuple_t create_record(std::string);
	std::string get_time(std::chrono::system_clock::time_point);
}

#endif	
