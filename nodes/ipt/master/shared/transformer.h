/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_MASTER_TRANSFORMER_H
#define NODE_IPT_MASTER_TRANSFORMER_H

#include <cyng/object.h>
#include <cyng/intrinsics/sets.h>
#include <cyng/log.h>

namespace node
{
	/**
	 * transform OBIS code tree into a web-friendly JSON like structure
	 */
	cyng::param_map_t transform_status_word(cyng::object);
	cyng::param_map_t transform_broker_params(cyng::logging::log_ptr, cyng::param_map_t const&);
	cyng::param_map_t transform_broker_hw_params(cyng::logging::log_ptr, cyng::param_map_t const&);
	cyng::param_map_t transform_data_collector_params(cyng::logging::log_ptr, cyng::param_map_t const&);

}


#endif

