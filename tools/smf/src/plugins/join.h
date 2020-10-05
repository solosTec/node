/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_TOOL_SMF_PLUGIN_JOIN_H
#define NODE_TOOL_SMF_PLUGIN_JOIN_H

#include <smf/cluster/config.h>

#include <cyng/vm/context.h>
#include <cyng/async/mux.h>
#include <cyng/log.h>

namespace node
{
	class cli;
	class join
	{
	public:
		join(cli*
			, cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, boost::uuids::uuid tag
			, cluster_config_t const& cfg);
		virtual ~join();
		void stop();

	private:
		void cmd(cyng::context& ctx);
		void connect();

	private:
		cli& cli_;
		cyng::async::mux& mux_;
		cyng::logging::log_ptr logger_;
		boost::uuids::uuid const tag_;
		cluster_config_t const config_;
		std::size_t task_;
	};

}

#endif	
