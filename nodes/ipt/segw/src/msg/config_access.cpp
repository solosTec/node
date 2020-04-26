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
			cyng::tuple_t msg = sml_gen_.empty_get_proc_param_response(trx, srv_id, path);

			if (path.size() == 1) {

				//
				//	deliver an overview of all roles, users and meters 
				//
				cache_.read_tables("_User", "_Privileges", [&](cyng::store::table const* tblUser, cyng::store::table const* tblPriv) {

					collect_role(msg, role::GUEST, tblUser, tblPriv);
					collect_role(msg,role::USER, tblUser, tblPriv);
					collect_role(msg,role::GW_OPERATOR, tblUser, tblPriv);
					collect_role(msg,role::DEV_OPERATOR, tblUser, tblPriv);
					collect_role(msg,role::SERVICE_PROVIDER, tblUser, tblPriv);
					collect_role(msg,role::SUPPLIER, tblUser, tblPriv);
					collect_role(msg,role::MANUFACTURER, tblUser, tblPriv);
					collect_role(msg,role::RESERVED, tblUser, tblPriv);

				});
			}
			else if (path.size() == 4) {

				//
				//	get role
				//
				auto const role = path.at(1).get_quantities();
				BOOST_ASSERT(path.at(2).get_quantities() == role);

				//
				//	get user
				//
				auto const user = path.at(2).get_storage();

				//
				//	get meter
				//
				auto const meter = path.at(3).get_storage();
				BOOST_ASSERT(meter != 0);

				//
				//	get meter id
				//
				auto const id = cache_.get_meter_by_id(meter);

				cache_.read_tables("_User", "_Privileges", [&](cyng::store::table const* tblUser, cyng::store::table const* tblPriv) {

					//
					//	set server ID / meter
					//
					set_get_proc_response_attribute(msg, { path.at(3) }, make_value(srv_id));

					//collect_privs(msg, role, user, meter, tblUser, tblPriv);

				});

				//CYNG_LOG_DEBUG(logger_, "get access rights (2-long-path) "
				//	<< cyng::io::to_str(msg));

				append_get_proc_response(msg, {
						path.at(3),
						OBIS_SECONDS_INDEX
					}, make_value(7));

				//CYNG_LOG_DEBUG(logger_, "get access rights (3-long-path) "
				//	<< cyng::io::to_str(msg));

				append_get_proc_response(msg, {
						path.at(3),
						OBIS_CURRENT_UTC
					}, make_value(7));

				append_get_proc_response(msg, {
						path.at(3),
						OBIS_TZ_OFFSET
					}, make_value(7));

				append_get_proc_response(msg, {
						path.at(3),
						OBIS_CLASS_OP_LOG_STATUS_WORD
					}, make_value(7));

				append_get_proc_response(msg, {
						path.at(3),
						OBIS_ROOT_CUSTOM_PARAM
					}, make_value(1));

				//CYNG_LOG_DEBUG(logger_, "get access rights (4-long-path) "
				//	<< cyng::io::to_str(msg));
			}
			else {

				CYNG_LOG_WARNING(logger_, "get access rights - invalid path: "
					<< from_server_id(srv_id)
					<< " - "
					<< to_hex(path, ':'));

			}

			//
			//	append to message queue
			//
			sml_gen_.append(std::move(msg));

		}

		void config_access::collect_role(cyng::tuple_t& msg
			//, std::uint8_t role
			, role e
			, cyng::store::table const* tblUser
			, cyng::store::table const* tblPriv) const
		{
			auto const bitpos = get_bit_position(e);
			auto const rn = static_cast<std::uint8_t>(e);

			std::size_t counter{ 0 };
			tblUser->loop([&](cyng::table::record const& rec) {

				auto const mask = cyng::value_cast<std::uint8_t>(rec["role"], 0u);
				if ((mask & bitpos) == bitpos) {

					//
					//	user with the specified role found - collect more data
					//

					auto const nr = cyng::value_cast<std::uint8_t>(rec["nr"], 1u);
					auto const name = cyng::value_cast<std::string>(rec["user"], "");
					auto const pwd = cyng::value_cast(rec["pwd"], cyng::crypto::digest_sha256());
					auto const h = cyng::io::to_str(pwd);	//	hash as string

					CYNG_LOG_DEBUG(logger_, "get access rights (1-"
						<< +nr
						<< ") "
						<< cyng::io::to_str(msg));

					append_get_proc_response(msg, {
						OBIS_ROOT_ACCESS_RIGHTS,
						make_obis(0x81, 0x81, 0x81, 0x60, rn, 0xFF),
						make_obis(0x81, 0x81, 0x81, 0x60, rn, nr),
						OBIS_ACCESS_USER_NAME
						}, make_value(name));

					CYNG_LOG_DEBUG(logger_, "get access rights (2-"
						<< +nr
						<< ") "
						<< cyng::io::to_str(msg));


					append_get_proc_response(msg, {
						OBIS_ROOT_ACCESS_RIGHTS,
						make_obis(0x81, 0x81, 0x81, 0x60, rn, 0xFF),
						make_obis(0x81, 0x81, 0x81, 0x60, rn, nr),
						OBIS_ACCESS_PASSWORD
						}, make_value(h));

					CYNG_LOG_DEBUG(logger_, "get access rights (3-"
						<< +nr
						<< ") "
						<< cyng::io::to_str(msg));

					append_get_proc_response(msg, {
						OBIS_ROOT_ACCESS_RIGHTS,
						make_obis(0x81, 0x81, 0x81, 0x60, rn, 0xFF),
						make_obis(0x81, 0x81, 0x81, 0x60, rn, nr),
						OBIS_ACCESS_PUBLIC_KEY
						}, make_value());	//	no certificate

					CYNG_LOG_DEBUG(logger_, "get access rights (4-"
						<< +nr
						<< ") "
						<< cyng::io::to_str(msg));

					//
					//	list of devices
					//
					auto const meters = collect_meters_of_user(tblPriv, nr);

					CYNG_LOG_DEBUG(logger_, "get access rights (X-"
						<< +nr
						<< ") + "
						<< meters.size()
						<< " meter(s)");

					std::uint8_t idx{ 1 };
					for(auto const& id: meters) {

						append_get_proc_response(msg, {
							OBIS_ROOT_ACCESS_RIGHTS,
							make_obis(0x81, 0x81, 0x81, 0x60, rn, 0xFF),
							make_obis(0x81, 0x81, 0x81, 0x60, rn, nr),
							make_obis(0x81, 0x81, 0x81, 0x64, 0x01, idx)
							}, make_value(id));	

						++idx;
					}

					//
					//	update counter
					//
					++counter;
				}

				return true;
			});

			if (counter == 0u) {

				CYNG_LOG_DEBUG(logger_, "get access rights (A) "
					<< cyng::io::to_str(msg));

				//
				//	there is no user with this role configured
				//
				append_get_proc_response(msg, {
					OBIS_ROOT_ACCESS_RIGHTS,
					make_obis(0x81, 0x81, 0x81, 0x60, rn, 0xFF)
					}, cyng::tuple_factory());	//	NULL
					//}, make_value());	//	NULL

				CYNG_LOG_DEBUG(logger_, "get access rights (B) "
					<< cyng::io::to_str(msg));
			}
		}


		void config_access::set_param(node::sml::obis code
			, cyng::buffer_t srv_id
			, cyng::param_t const& param)
		{

		}


	}	//	sml
}

