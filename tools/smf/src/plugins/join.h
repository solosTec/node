/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_TOOL_SMF_PLUGIN_JOIN_H
#define NODE_TOOL_SMF_PLUGIN_JOIN_H

 #include <smf/cluster/bus.h>
#include <smf/cluster/config.h>

#include <cyng/vm/context.h>
#include <cyng/compatibility/file_system.hpp>

namespace node
{
	class cli;
	class join
	{
	public:
		join(cli*, cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, boost::uuids::uuid tag
			, cluster_config_t const& cfg);
		virtual ~join();

	private:
		void cmd(cyng::context& ctx);
		void reconfigure(cyng::context& ctx);
		void reconfigure_impl();
		void connect();

	private:
		cli& cli_;
		bus::shared_type bus_;
		cyng::logging::log_ptr logger_;
		cluster_redundancy const config_;

	};

}

#endif	
