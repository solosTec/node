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
#include <smf/sml/obis_io.h>
#include <smf/sml/intrinsics/obis_factory.hpp>
#include <smf/sml/srv_id_io.h>

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

		void config_access::get_proc_params(std::string trx
			, cyng::buffer_t srv_id
			, obis_path_t path) const
		{
			CYNG_LOG_DEBUG(logger_, "get access rights: " 
				<< from_server_id(srv_id)
				<< " - "
				<< to_hex(path, ':'));

			BOOST_ASSERT(path.front() == OBIS_ROOT_ACCESS_RIGHTS);

			//	81 81 81 60 FF FF
			auto msg = sml_gen_.empty_get_proc_param_response(trx, srv_id, OBIS_ROOT_ACCESS_RIGHTS);

			if (path.size() == 1) {

				//
				//	ToDo: check server ID
				//


				//
				//	GUEST
				//
				append_get_proc_response(msg, {
					OBIS_ROOT_ACCESS_RIGHTS,
					make_obis(0x81, 0x81, 0x81, 0x60, static_cast<std::uint32_t>(role::GUEST), 0xFF)
					}, make_value());

				//
				//	USER - end user
				//
				append_get_proc_response(msg, {
					OBIS_ROOT_ACCESS_RIGHTS,
					make_obis(0x81, 0x81, 0x81, 0x60, static_cast<std::uint32_t>(role::USER), 0xFF)
					}, make_value());


				//
				//	GW_OPERATOR,
				//	get the special rights of the gateway operator
				//
				append_get_proc_response(msg, {
					OBIS_ROOT_ACCESS_RIGHTS,
					make_obis(0x81, 0x81, 0x81, 0x60, static_cast<std::uint32_t>(role::GW_OPERATOR), 0xFF),
					make_obis(0x81, 0x81, 0x81, 0x60, 0x03, 0x01),
					OBIS_ACCESS_USER_NAME
					}, make_value("operator"));
				append_get_proc_response(msg, {
					OBIS_ROOT_ACCESS_RIGHTS,
					make_obis(0x81, 0x81, 0x81, 0x60, static_cast<std::uint32_t>(role::GW_OPERATOR), 0xFF),
					make_obis(0x81, 0x81, 0x81, 0x60, 0x03, 0x01),
					OBIS_ACCESS_PASSWORD
					}, make_value("06e55b633481f7bb072957eabcf110c972e86691c3cfedabe088024bffe42f23"));

				append_get_proc_response(msg, {
					OBIS_ROOT_ACCESS_RIGHTS,
					make_obis(0x81, 0x81, 0x81, 0x60, static_cast<std::uint32_t>(role::GW_OPERATOR), 0xFF),
					make_obis(0x81, 0x81, 0x81, 0x60, 0x03, 0x01),
					make_obis(0x81, 0x81, 0x81, 0x63, 0xFF, 0xFF)
					}, make_value());	//	no certificate

				append_get_proc_response(msg, {
					OBIS_ROOT_ACCESS_RIGHTS,
					make_obis(0x81, 0x81, 0x81, 0x60, static_cast<std::uint32_t>(role::GW_OPERATOR), 0xFF),
					make_obis(0x81, 0x81, 0x81, 0x60, 0x03, 0x01),
					make_obis(0x81, 0x81, 0x81, 0x64, 0x01, 0x01)
					}, make_value(cache_.get_srv_id()));	//	this server ID

				//
				//	list of server/meter
				//
				append_get_proc_response(msg, {
					OBIS_ROOT_ACCESS_RIGHTS,
					make_obis(0x81, 0x81, 0x81, 0x60, static_cast<std::uint32_t>(role::GW_OPERATOR), 0xFF),
					make_obis(0x81, 0x81, 0x81, 0x60, 0x03, 0x01),
					make_obis(0x81, 0x81, 0x81, 0x64, 0x01, 0x02)
					}, make_value("04045057"));	//	
				append_get_proc_response(msg, {
					OBIS_ROOT_ACCESS_RIGHTS,
					make_obis(0x81, 0x81, 0x81, 0x60, static_cast<std::uint32_t>(role::GW_OPERATOR), 0xFF),
					make_obis(0x81, 0x81, 0x81, 0x60, 0x03, 0x01),
					make_obis(0x81, 0x81, 0x81, 0x64, 0x01, 0x01)
					}, make_value(srv_id));	//	gateway itself as server #1


				//	DEV_OPERATOR,
				append_get_proc_response(msg, {
					OBIS_ROOT_ACCESS_RIGHTS,
					make_obis(0x81, 0x81, 0x81, 0x60, static_cast<std::uint32_t>(role::DEV_OPERATOR), 0xFF)
					}, make_value());

				//	SERVICE_PROVIDER,
				append_get_proc_response(msg, {
					OBIS_ROOT_ACCESS_RIGHTS,
					make_obis(0x81, 0x81, 0x81, 0x60, static_cast<std::uint32_t>(role::SERVICE_PROVIDER), 0xFF)
					}, make_value());

				//	SUPPLIER,
				append_get_proc_response(msg, {
					OBIS_ROOT_ACCESS_RIGHTS,
					make_obis(0x81, 0x81, 0x81, 0x60, static_cast<std::uint32_t>(role::SUPPLIER), 0xFF)
					}, make_value());

				//	MANUFACTURER,
				append_get_proc_response(msg, {
					OBIS_ROOT_ACCESS_RIGHTS,
					make_obis(0x81, 0x81, 0x81, 0x60, static_cast<std::uint32_t>(role::MANUFACTURER), 0xFF)
					}, make_value());

				//	RESERVED
				append_get_proc_response(msg, {
					OBIS_ROOT_ACCESS_RIGHTS,
					make_obis(0x81, 0x81, 0x81, 0x60, static_cast<std::uint32_t>(role::RESERVED), 0xFF)
					}, make_value());
			}
			else {
				//
				//	81 81 81 60 NN FF - role
				//
				
				//append_get_proc_response(msg, {
				//	path.back(),
				//	OBIS_FTP_UPDATE	//	99, 00, 00, 00, 00, 05
				//	}, make_value(7));

				//append_get_proc_response(msg, {
				//	path.back(),
				//	OBIS_OBISLOG_INTERVAL	//	81, 81, 27, 32, 07, 01
				//	}, make_value(7));
				//
				//append_get_proc_response(msg, {
				//	path.back(),
				//	OBIS_ROOT_CUSTOM_INTERFACE	//	81 02 00 07 00 FF
				//	}, make_value(7));

				//append_get_proc_response(msg, {
				//	path.back(),
				//	OBIS_ROOT_CUSTOM_PARAM	//	81 02 00 07 10 FF
				//	}, make_value(1));

			}

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

