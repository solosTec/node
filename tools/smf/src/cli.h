/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_TOOL_SMF_CLI_H
#define NODE_TOOL_SMF_CLI_H

#include "console.h"
#include "plugins/convert.h"
#include "plugins/tracking.h"
#include "plugins/cleanup-v4.h"
#include "plugins/send.h"
#include "plugins/join.h"
#include <cyng/compatibility/io_service.h>

namespace node
{
	class cli : public console
	{
		friend class convert;
		friend class tracking;
		friend class cleanup;
		friend class send;
		friend class join;

	public:
		cli(cyng::async::mux&
			, cyng::logging::log_ptr logger
			, boost::uuids::uuid
			, cluster_config_t const& cfg
			, std::ostream&
			, std::istream&);
		virtual ~cli();

		int run();
		int run(std::string const& file_name);

	protected:
		virtual bool parse(std::string const&);
		virtual void shutdown();
		void call(std::string, std::vector<std::string> const&);

	private:
		convert		plugin_convert_;
		tracking	plugin_tracking_;
		cleanup		plugin_cleanup_;
		send		plugin_send_;
		join		plugin_join_;
	};
}

#endif	
