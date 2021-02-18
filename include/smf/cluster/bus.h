/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_CLUSTER_BUS_H
#define SMF_CLUSTER_BUS_H


#include <smf/cluster/config.h>

#include <cyng/log/logger.h>
#include <cyng/task/channel.h>

namespace smf
{
	class bus
	{
	public:
		bus(cyng::logger, toggle cfg, cyng::channel_weak wp);

		void start();
		void stop();

	private:
		cyng::logger logger_;
		toggle const cfg_;
		cyng::channel_weak wchannel_;
	};
}


#endif
