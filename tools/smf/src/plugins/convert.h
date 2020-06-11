/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_TOOL_SMF_PLUGIN_CONVERT_H
#define NODE_TOOL_SMF_PLUGIN_CONVERT_H

#include <smf/sml/intrinsics/obis.h>
#include <cyng/vm/context.h>
#include <cyng/compatibility/file_system.hpp>

namespace node
{
	class cli;
	class convert
	{
	public:
		convert(cli*);
		virtual ~convert();

	private:
		void csv_abl(cyng::context& ctx);
		void csv_abl_file(cyng::filesystem::path);
		void csv_abl_dir(cyng::filesystem::path);
		void csv_abl_data(cyng::filesystem::path, std::string, cyng::buffer_t const&);

	private:
		cli& cli_;
	};

	/**
	 * create a vector of all OBIS codes
	 */
	std::map<sml::obis, std::size_t> analyse_csv_header(cyng::tuple_t);
	std::chrono::system_clock::time_point get_ro_time(std::map<sml::obis, std::size_t> const&, cyng::tuple_t const&);

}

#endif	
