/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "config_broker.h"
#include "../segw.h"
#include "../cache.h"

#include <smf/sml/protocol/generator.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/intrinsics/obis_factory.hpp>
#include <smf/sml/parser/obis_parser.h>

#include <cyng/numeric_cast.hpp>
#include <cyng/buffer_cast.h>

namespace node
{
	namespace sml
	{
		config_broker::config_broker(cyng::logging::log_ptr logger
			, res_generator& sml_gen
			, cache& cfg)
		: logger_(logger)
			, sml_gen_(sml_gen)
			, cache_(cfg)
			, cfg_(cfg)
		{}

		void config_broker::get_proc_params(std::string trx, cyng::buffer_t srv_id) const
		{
			auto msg = sml_gen_.empty_get_proc_param_response(trx, srv_id, OBIS_ROOT_BROKER);

			append_get_proc_response(msg, { OBIS_ROOT_BROKER, OBIS_BROKER_ENABLED, make_obis(0x90, 0x00, 0x00, 0x00, 0x01, 1u) }, make_value(cfg_.is_transparent_mode(0)));
			append_get_proc_response(msg, { OBIS_ROOT_BROKER, OBIS_BROKER_SERVER, make_obis(0x90, 0x00, 0x00, 0x00, 0x02, 1u) }, make_value("segw.ch"));
			append_get_proc_response(msg, { OBIS_ROOT_BROKER, OBIS_BROKER_PORT, make_obis(0x90, 0x00, 0x00, 0x00, 0x03, 1u) }, make_value(12001u));

			append_get_proc_response(msg, { OBIS_ROOT_BROKER, OBIS_BROKER_ENABLED, make_obis(0x90, 0x00, 0x00, 0x00, 0x01, 2u) }, make_value(cfg_.is_transparent_mode(1)));
			append_get_proc_response(msg, { OBIS_ROOT_BROKER, OBIS_BROKER_SERVER, make_obis(0x90, 0x00, 0x00, 0x00, 0x02, 2u) }, make_value("192.168.1.21"));
			append_get_proc_response(msg, { OBIS_ROOT_BROKER, OBIS_BROKER_PORT, make_obis(0x90, 0x00, 0x00, 0x00, 0x03, 2u) }, make_value(12002u));


			//
			//	append to message queue
			//
			sml_gen_.append(std::move(msg));

		}
	}	//	sml
}

