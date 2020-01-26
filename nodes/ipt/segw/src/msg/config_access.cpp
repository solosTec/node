/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "config_access.h"
#include "../cache.h"

#include <smf/sml/protocol/generator.h>
#include <smf/sml/obis_db.h>

#include <cyng/numeric_cast.hpp>
#include <cyng/buffer_cast.h>

namespace node
{
	namespace sml
	{
		config_access::config_access(cyng::logging::log_ptr logger
			, res_generator& sml_gen
			, cache& cfg)
		: logger_(logger)
			, sml_gen_(sml_gen)
			, cache_(cfg)
		{}

		void config_access::get_proc_params(std::string trx, cyng::buffer_t srv_id) const
		{
			//	81 81 81 60 FF FF
			auto msg = sml_gen_.empty_get_proc_param_response(trx, srv_id, OBIS_ROOT_ACCESS_RIGHTS);

			//
			//	81 81 81 60 NN FF - role
			//
			append_get_proc_response(msg, {
				OBIS_ROOT_ACCESS_RIGHTS,
				OBIS_SECURITY_SERVER_ID
				}, make_value(srv_id));

			//
			//	append to message queue
			//
			sml_gen_.append(std::move(msg));

		}

		void config_access::set_param(node::sml::obis code
			, cyng::buffer_t srv_id
			, cyng::param_t const& param)
		{

		}


	}	//	sml
}

