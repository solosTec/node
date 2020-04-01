/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "config_data_collector.h"
#include "../cache.h"

#include <smf/sml/protocol/generator.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/obis_io.h>
#include <smf/sml/intrinsics/obis_factory.hpp>
#include <smf/sml/srv_id_io.h>

#include <cyng/numeric_cast.hpp>
#include <cyng/buffer_cast.h>
#include <cyng/set_cast.h>
 
namespace node
{
	namespace sml
	{
		//
		//	forward declaration
		//
		cyng::object lookup(cyng::param_map_t const&, obis);

		config_data_collector::config_data_collector(cyng::logging::log_ptr logger
			, res_generator& sml_gen
			, cache& cfg)
		: logger_(logger)
			, sml_gen_(sml_gen)
			, cache_(cfg)
		{}

		void config_data_collector::get_proc_params(std::string trx, cyng::buffer_t srv_id) const
		{
			//	81 81 C7 86 20 FF
			auto msg = sml_gen_.empty_get_proc_param_response(trx, srv_id, OBIS_ROOT_DATA_COLLECTOR);

			cache_.read_tables("_DataCollector", "_DataMirror", [&](cyng::store::table const* tbl_dc, cyng::store::table const* tbl_reg) {

				tbl_dc->loop([&](cyng::table::record const& rec) {

					auto const id = cyng::to_buffer(rec["serverID"]);
					if (id == srv_id) {

						auto const nr = cyng::numeric_cast<std::uint8_t>(rec["nr"], 1);

						//
						//	81 81 C7 86 21 FF - active
						//
						append_get_proc_response(msg, {
							OBIS_ROOT_DATA_COLLECTOR,
							make_obis(0x81, 0x81, 0xC7, 0x86, 0x20, nr),
							OBIS_DATA_COLLECTOR_ACTIVE
							}, make_value(rec["active"]));

						//
						//	81 81 C7 86 22 FF - Einträge
						//
						append_get_proc_response(msg, {
							OBIS_ROOT_DATA_COLLECTOR,
							make_obis(0x81, 0x81, 0xC7, 0x86, 0x20, nr),
							OBIS_DATA_COLLECTOR_SIZE
							}, make_value(rec["maxSize"]));

						//
						//	81 81 C7 87 81 FF  - Registerperiode (seconds)
						//
						append_get_proc_response(msg, {
							OBIS_ROOT_DATA_COLLECTOR,
							make_obis(0x81, 0x81, 0xC7, 0x86, 0x20, nr),
							OBIS_DATA_REGISTER_PERIOD
							}, make_value(rec["regPeriod"]));

						//
						//	81 81 C7 8A 83 FF - profile
						//
						append_get_proc_response(msg, {
							OBIS_ROOT_DATA_COLLECTOR,
							make_obis(0x81, 0x81, 0xC7, 0x86, 0x20, nr),
							OBIS_PROFILE
							}, make_value(rec["profile"]));

						//
						//	collect all available OBIS codes for this meter
						//
						loop_data_mirror(tbl_reg, msg, srv_id, nr);

					}
					return true;	//	continue
				});

			});

			//
			//	append to message queue
			//
			sml_gen_.append(std::move(msg));
		}

		void config_data_collector::get_push_operations(std::string trx, cyng::buffer_t srv_id) const
		{
			auto msg = sml_gen_.empty_get_proc_param_response(trx, srv_id, OBIS_PUSH_OPERATIONS);

			//
			//	81 81 C7 8A 01 FF - service push
			//	read from SQLite database table "TPushOps"
			//
			cache_.loop("_PushOps", [&](cyng::table::record const& rec) {

				cyng::buffer_t const id = cyng::to_buffer(rec["serverID"]);

				//
				//	only matching records
				//
				if (srv_id == id) {

					auto const nr = cyng::numeric_cast<std::uint8_t>(rec["idx"], 1);

					//
					//	81 81 C7 8A 02 FF - push interval in seconds
					//
					append_get_proc_response(msg, {
						OBIS_PUSH_OPERATIONS,
						make_obis(0x81, 0x81, 0xC7, 0x8A, 0x01, nr),
						OBIS_PUSH_INTERVAL
						}, make_value(rec["interval"]));

					//
					//	81 81 C7 8A 03 FF - push delay in seconds
					//
					append_get_proc_response(msg, {
						OBIS_PUSH_OPERATIONS,
						make_obis(0x81, 0x81, 0xC7, 0x8A, 0x01, nr),
						OBIS_PUSH_DELAY
						}, make_value(rec["delay"]));

					//
					//	81 47 17 07 00 FF - target name
					//
					append_get_proc_response(msg, {
						OBIS_PUSH_OPERATIONS,
						make_obis(0x81, 0x81, 0xC7, 0x8A, 0x01, nr),
						OBIS_PUSH_TARGET
						}, make_value(rec["target"]));

					//	push service:
					//	* 81 81 C7 8A 21 FF == IP-T
					//	* 81 81 C7 8A 22 FF == SML client address
					//	* 81 81 C7 8A 23 FF == KNX ID
					append_get_proc_response(msg, {
						OBIS_PUSH_OPERATIONS,
						make_obis(0x81, 0x81, 0xC7, 0x8A, 0x01, nr),
						OBIS_PUSH_SERVICE
						}, make_value(OBIS_PUSH_SERVICE_IPT));

					//
					//	push source (81 81 C7 8A 04 FF) has both, an
					//	* SML value (OBIS code) with the push source and
					//	* SML tree with addition informations about the
					//	push server ID (81 81 C7 8A 81 FF), the profile (81 81 C7 8A 83 FF)
					//	and the list of identifiers of the values to be delivered by the push source. 
					//

					//	* 81 81 C7 8A 42 FF == profile (PUSH_SOURCE_PROFILE)
					//	* 81 81 C7 8A 43 FF == installation parameters (PUSH_SOURCE_INSTALL)
					//	* 81 81 C7 8A 44 FF == list of visible sensors/actors (PUSH_SOURCE_SENSOR_LIST)
					append_get_proc_response(msg, {
						OBIS_PUSH_OPERATIONS,
						make_obis(0x81, 0x81, 0xC7, 0x8A, 0x01, nr),
						OBIS_PUSH_SOURCE
						}
						, make_value(OBIS_PUSH_SOURCE_PROFILE)
						, cyng::tuple_factory(
							child_list_tree(OBIS_PUSH_SERVER_ID, make_value(srv_id)),
							empty_tree(OBIS_PUSH_IDENTIFIERS),
							child_list_tree(OBIS_PROFILE, make_value(OBIS_PROFILE_15_MINUTE))));

				}

				return true;	//	continue
			});

			//
			//	append to message queue
			//
			sml_gen_.append(std::move(msg));

		}

		void config_data_collector::set_push_operations(cyng::buffer_t srv_id
			, std::string user
			, std::string pwd
			, std::uint8_t nr
			, cyng::param_map_t&& params)
		{
			cache_.write_table("_PushOps", [&](cyng::store::table* tbl) {
				auto const key = cyng::table::key_generator(srv_id, nr);
				auto const rec = tbl->lookup(key);
				if (!rec.empty()) {
					if (params.empty()) {

						//
						//	an empty parameter set indicates that the record hast be removed
						//
						tbl->erase(key, cache_.get_tag());
					}
					else {
						update_push_ops(tbl, key, params, cache_.get_tag());
					}
				}
				else {
					BOOST_ASSERT(params.size() == 5);
					insert_push_ops(tbl, key, params, cache_.get_tag());
				}
			});
		}

		void config_data_collector::set_param(cyng::buffer_t srv_id
			, std::uint8_t nr
			, cyng::param_map_t&& params)
		{

			cache_.write_tables("_DataCollector", "_DataMirror", [&](cyng::store::table* tbl_dc, cyng::store::table* tbl_reg) {

				auto const key = cyng::table::key_generator(srv_id, nr);
				auto const rec = tbl_dc->lookup(key);
				if (rec.empty()) {
					insert_data_collector(tbl_dc, key, params, tbl_reg, cache_.get_tag());
				}
				else {
					update_data_collector(tbl_dc, key, params, tbl_reg, cache_.get_tag());
				}
			});
		}

		void config_data_collector::clear_data_collector(cyng::buffer_t srv_id
			, cyng::param_map_t&& params)
		{
			//%(("8181C78204FF":01E61E571406213603), ("8181C78A83FF":8181C78612FF))

			auto const id = cyng::to_buffer(lookup(params, OBIS_SERVER_ID));
			obis const profile(cyng::to_buffer(lookup(params, OBIS_PROFILE)));

			CYNG_LOG_INFO(logger_, sml::from_server_id(srv_id)
				<< " - delete data collector "
				<< sml::from_server_id(id)
				<< ", profile: "
				<< sml::get_profile_name(profile));

			cache_.write_tables("_DataCollector", "_DataMirror", [&](cyng::store::table* tbl_dc, cyng::store::table* tbl_mirr) {

				//
				//	clear _DataCollector
				//
				cyng::table::key_type pk;
				tbl_dc->loop([&](cyng::table::record const& rec) {
					if (!rec.empty()) {
						if (cyng::to_buffer(rec["serverID"]) == id && cyng::to_buffer(rec["profile"]) == profile) {
							pk = rec.key();
							return false;	//	data collector found - stop searching
						}
					}
					return true;	//	continue
				});

				if (pk.size() == 2) {

					if (!tbl_dc->erase(pk, cache_.get_tag())) {
						CYNG_LOG_WARNING(logger_, "cannot delete data collector: "
							<< sml::from_server_id(id)
							<< ", profile: "
							<< sml::get_profile_name(profile));
					}

					//
					//	find attached _DataMirror
					//
					cyng::table::key_list_t keys;
					auto const nr = cyng::value_cast<std::uint8_t>(pk.at(1), 0);
					BOOST_ASSERT(nr != 0);
					tbl_mirr->loop([&](cyng::table::record const& rec) {
						if (!rec.empty()) {
							if (nr == cyng::value_cast<std::uint8_t>(rec["nr"], 0) && cyng::to_buffer(rec["serverID"]) == id) {
								keys.push_back(rec.key());
							}
						}
						return true;
						});

					//
					//	clear _DataMirror
					//
					CYNG_LOG_TRACE(logger_, sml::from_server_id(srv_id)
						<< " - delete "
						<< keys.size()
						<< " data mirrors "
						<< sml::from_server_id(id)
						<< ", profile: "
						<< sml::get_profile_name(profile));

					for (auto const& key : keys) {
						tbl_mirr->erase(key, cache_.get_tag());
					}
				}
				else {
					CYNG_LOG_ERROR(logger_, "primary key has invalid size: "
						<< pk.size());

				}
			});
		}


		void insert_data_collector(cyng::store::table* tbl_dc
			, cyng::table::key_type const& key
			, cyng::param_map_t const& params
			, cyng::store::table* tbl_reg
			, boost::uuids::uuid source)
		{
			//	[19122321172417116-2,8181C78620FF 8181C7862001,01A815743145040102,operator,operator,("8181C7862001":%(("8181C78621FF":true),("8181C78622FF":64),("8181C78781FF":0),("8181C78A23FF":%(("8181C78A2301":070003010001),("8181C78A2302":0700030100FF))),("8181C78A83FF":8181C78611FF)))]

			//%(("8181C78621FF":true),("8181C78622FF":64),("8181C78781FF":0),("8181C78A23FF":%(("8181C78A2301":070003010001),("8181C78A2302":0700030100FF))),("8181C78A83FF":8181C78611FF))	

			//
			//	data collector
			//
			tbl_dc->insert(key
				, cyng::table::data_generator(lookup(params, OBIS_PROFILE)
					, lookup(params, OBIS_DATA_COLLECTOR_ACTIVE)
					, lookup(params, OBIS_DATA_COLLECTOR_SIZE)
					, lookup(params, OBIS_DATA_REGISTER_PERIOD))
				, 0u
				, source);

			//
			//	data mirror / registers
			//
			auto const obj = lookup(params, OBIS_DATA_COLLECTOR_OBIS);
			auto const regs = cyng::to_param_map(obj);
			for (auto const& p : regs) {
				obis const code(to_obis(p.first));
				obis const reg(cyng::to_buffer(p.second));
				tbl_reg->insert(
					cyng::table::key_generator(key.at(0), key.at(1), code.get_data().at(obis::VG_STORAGE)), 
					cyng::table::data_generator(reg.to_buffer(), true), 0u, source);
			}
		}

		void update_data_collector(cyng::store::table* tbl_dc
			, cyng::table::key_type const& key
			, cyng::param_map_t const& params
			, cyng::store::table* tbl_reg
			, boost::uuids::uuid source)
		{
			for (auto const& param : params) {
				if (boost::algorithm::equals(param.first, OBIS_DATA_COLLECTOR_ACTIVE.to_str())) {
					tbl_dc->modify(key, cyng::param_t("active", param.second), source);
				}
				else if (boost::algorithm::equals(param.first, OBIS_DATA_COLLECTOR_SIZE.to_str())) {
					tbl_dc->modify(key, cyng::param_t("maxSize", param.second), source);
				}
				else if (boost::algorithm::equals(param.first, OBIS_DATA_REGISTER_PERIOD.to_str())) {
					tbl_dc->modify(key, cyng::param_t("regPeriod", param.second), source);
				}
				else if (boost::algorithm::equals(param.first, OBIS_DATA_COLLECTOR_OBIS.to_str())) {

					//
					//	update register entries / data mirror
					//
					auto const regs = cyng::to_param_map(param.second);
					update_data_mirror(tbl_reg, key.at(0), key.at(1), regs, source);

				}
			}
		}

		void update_data_mirror(cyng::store::table* tbl
			, cyng::object srv_id
			, cyng::object nr
			, cyng::param_map_t const& params
			, boost::uuids::uuid source)
		{
			//	("8181C78A2301":070003010001),("8181C78A2302":0700030100FF)

			for (auto const& p : params) {
				obis const code = to_obis(p.first);
				auto const key = cyng::table::key_generator(srv_id, nr, code.get_data().at(obis::VG_STORAGE));
				auto const rec = tbl->lookup(key);
				if (rec.empty()) {

					//
					//	insert
					//
					tbl->insert(key, cyng::table::data_generator(p.second, true), 0u, source);
				}
				else {

					//
					//	delete
					//
					tbl->erase(key, source);
				}
			}
		}

		cyng::object lookup(cyng::param_map_t const& params, obis code)
		{
			auto const pos = params.find(code.to_str());
			return (pos != params.end())
				? pos->second
				: cyng::make_object()
				;
		}

		void loop_data_mirror(cyng::store::table const* tbl
			, cyng::tuple_t& msg
			, cyng::buffer_t srv_id
			, std::uint8_t nr)
		{
			tbl->loop([&](cyng::table::record const& rec) {

				auto const id = cyng::to_buffer(rec["serverID"]);
				auto const idx = cyng::numeric_cast<std::uint8_t>(rec["nr"], 1u);

				//
				//	select matching registers
				//
				if ((id == srv_id) && (nr == idx)) {

					obis const code(cyng::to_buffer(rec["code"]));
					auto const reg = cyng::numeric_cast<std::uint8_t>(rec["reg"], 1);

					append_get_proc_response(msg, {
						OBIS_ROOT_DATA_COLLECTOR,
						make_obis(0x81, 0x81, 0xC7, 0x86, 0x20, nr),
						OBIS_PROFILE,
						make_obis(0x81, 0x81, 0xC7, 0x8A, 0x23, reg)
						}, make_value(code));

				}
				return true;	//	continue
			});

		}

		void insert_push_ops(cyng::store::table* tbl
			, cyng::table::key_type const& key
			, cyng::param_map_t const& params
			, boost::uuids::uuid source)
		{
			tbl->insert(key
				, cyng::table::data_generator(lookup(params, OBIS_PUSH_INTERVAL)
					, lookup(params, OBIS_PUSH_DELAY)
					, lookup(params, OBIS_PUSH_SOURCE)
					, lookup(params, OBIS_PUSH_TARGET)
					, lookup(params, OBIS_PUSH_SERVICE)
					, static_cast<std::uint64_t>(0)		//	lowerBound
					, static_cast<std::uint64_t>(0))	//	task
				, 0u
				, source);

		}

		void update_push_ops(cyng::store::table* tbl
			, cyng::table::key_type const& key
			, cyng::param_map_t const& params
			, boost::uuids::uuid source)
		{
			for (auto const& param : params) {
				if (boost::algorithm::equals(param.first, OBIS_PUSH_INTERVAL.to_str())) {
					tbl->modify(key, cyng::param_factory("interval", cyng::numeric_cast<std::uint32_t>(param.second, 0)), source);
				}
				else if (boost::algorithm::equals(param.first, OBIS_PUSH_DELAY.to_str())) {
					tbl->modify(key, cyng::param_factory("delay", cyng::numeric_cast<std::uint32_t>(param.second, 0)), source);
				}
				else if (boost::algorithm::equals(param.first, OBIS_PUSH_SOURCE.to_str())) {
					auto const soure_params = cyng::to_param_map(param.second);
					for (auto const& soure_param : soure_params) {
						if (boost::algorithm::equals(soure_param.first, OBIS_PUSH_SOURCE.to_str())) {
							tbl->modify(key, cyng::param_t("source", soure_param.second), source);
						}
						else if (boost::algorithm::equals(soure_param.first, OBIS_PUSH_SERVER_ID.to_str())) {
							//CYNG_LOG_TRACE(logger_, "modify push.ops OBIS_PUSH_SERVER_ID "
							//	<< soure_param.first
							//	<< " = "
							//	<< cyng::io::to_str(soure_param.second));
						}
						else if (boost::algorithm::equals(soure_param.first, OBIS_PROFILE.to_str())) {

						}
					}
				}
				else if (boost::algorithm::equals(param.first, OBIS_PUSH_TARGET.to_str())) {
					tbl->modify(key, cyng::param_t("target", param.second), source);
				}
				else if (boost::algorithm::equals(param.first, OBIS_PUSH_SERVICE.to_str())) {
					tbl->modify(key, cyng::param_t("service", param.second), source);
				}
			}
		}
	}	//	sml
}

