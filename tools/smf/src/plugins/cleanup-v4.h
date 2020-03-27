/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_TOOL_SMF_PLUGIN_CLEANUP_V4_H
#define NODE_TOOL_SMF_PLUGIN_CLEANUP_V4_H

#include <cyng/vm/context.h>
#include <boost/filesystem.hpp>

namespace node
{
	class cli;
	class cleanup
	{
	public:
		cleanup(cli*);
		virtual ~cleanup();

	private:
		void cmd(cyng::context& ctx);
		void read_log();
		void read_e350(boost::filesystem::path);
		void read_ipt(boost::filesystem::path);
		void intersection();
		void import_data();
		void dump_result();
	private:
		cli& cli_;
		boost::filesystem::path root_dir_;
		cyng::vector_t	devices_;
		std::map<std::string, std::string>	active_e350_;
		std::map<std::string, std::string>	active_ipt_;
		std::map<std::string, std::pair<std::string, std::string>>	inactive_devs_;
	};

}

#endif	
