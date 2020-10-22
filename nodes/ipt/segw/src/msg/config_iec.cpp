/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "config_iec.h"
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
		config_iec::config_iec(cyng::logging::log_ptr logger
			, res_generator& sml_gen
			, cache& cfg)
		: logger_(logger)
			, sml_gen_(sml_gen)
			, cache_(cfg)
		{}

		void config_iec::get_proc_params(std::string trx, cyng::buffer_t srv_id) const
		{
			auto msg = sml_gen_.empty_get_proc_param(trx, srv_id, OBIS_IF_1107);

			//
			//	81 81 C7 93 01 FF - if true 1107 interface active otherwise SML interface active
			//
			auto const active = cache_.get_cfg(build_cfg_key({ OBIS_IF_1107, OBIS_IF_1107_ACTIVE }), false);
			append_get_proc_response(msg, {
				OBIS_IF_1107,
				OBIS_IF_1107_ACTIVE
				}, make_value(active));

			//
			//	81 81 C7 93 02 FF - Loop timeout in seconds
			//
			auto const loop_time = cache_.get_cfg(build_cfg_key({ OBIS_IF_1107, OBIS_IF_1107_LOOP_TIME }), std::chrono::seconds(60));
			append_get_proc_response(msg, {
				OBIS_IF_1107,
				OBIS_IF_1107_LOOP_TIME
				}, make_value(loop_time));

			//
			//	81 81 C7 93 03 FF - Retry count
			//
			auto const retries = cache_.get_cfg<std::uint32_t>(build_cfg_key({ OBIS_IF_1107, OBIS_IF_1107_RETRIES }), 3u);
			append_get_proc_response(msg, {
				OBIS_IF_1107,
				OBIS_IF_1107_RETRIES
				}, make_value(retries));

			//
			//	81 81 C7 93 04 FF - Minimal answer timeout(300) in milliseconds
			//
			auto const min_timeout = cache_.get_cfg(build_cfg_key({ OBIS_IF_1107, OBIS_IF_1107_MIN_TIMEOUT }), std::chrono::milliseconds(200u));
			append_get_proc_response(msg, {
				OBIS_IF_1107,
				OBIS_IF_1107_MIN_TIMEOUT
				}, make_value(min_timeout));

			//
			//	81 81 C7 93 05 FF - Maximal answer timeout(5000) in milliseconds
			//
			auto const max_timeout = cache_.get_cfg(build_cfg_key({ OBIS_IF_1107, OBIS_IF_1107_MAX_TIMEOUT }), std::chrono::milliseconds(5000u));
			append_get_proc_response(msg, {
				OBIS_IF_1107,
				OBIS_IF_1107_MAX_TIMEOUT
				}, make_value(max_timeout));

			//
			//	81 81 C7 93 06 FF - Maximum data bytes(10240)
			//
			auto const max_bytes = cache_.get_cfg<std::uint32_t>(build_cfg_key({ OBIS_IF_1107, OBIS_IF_1107_MAX_DATA_RATE }), 10240u);
			append_get_proc_response(msg, {
				OBIS_IF_1107,
				OBIS_IF_1107_MAX_DATA_RATE
				}, make_value(max_bytes));

			//
			//	81 81 C7 93 08 FF - Protocol mode(A ... D)
			//
			auto const mode = cache_.get_cfg<std::uint8_t>(build_cfg_key({ OBIS_IF_1107, OBIS_IF_1107_PROTOCOL_MODE }), 2u);
			append_get_proc_response(msg, {
				OBIS_IF_1107,
				OBIS_IF_1107_PROTOCOL_MODE
				}, make_value(mode));

			//
			//	81 81 C7 93 10 FF - auto activation
			//
			auto const auto_activation = cache_.get_cfg(build_cfg_key({ OBIS_IF_1107, OBIS_IF_1107_AUTO_ACTIVATION }), false);
			append_get_proc_response(msg, {
				OBIS_IF_1107,
				OBIS_IF_1107_AUTO_ACTIVATION
				}, make_value(auto_activation));

			//
			//	81 81 C7 93 11 FF - time grid of load profile readout in seconds
			//	default = 15min
			//
			auto const time_grid = cache_.get_cfg(build_cfg_key({ OBIS_IF_1107, OBIS_IF_1107_TIME_GRID }), std::chrono::seconds(900u));
			append_get_proc_response(msg, {
				OBIS_IF_1107,
				OBIS_IF_1107_TIME_GRID
				}, make_value(time_grid));

			//
			//	81 81 C7 93 13 FF - time sync in seconds
			//	default = 4h
			//
			auto const time_sync = cache_.get_cfg(build_cfg_key({ OBIS_IF_1107, OBIS_IF_1107_TIME_SYNC }), std::chrono::seconds(14400u));
			append_get_proc_response(msg, {
				OBIS_IF_1107,
				OBIS_IF_1107_TIME_SYNC
				}, make_value(time_sync));

			//
			//	81 81 C7 93 14 FF - seconds
			//
			auto const max_var = cache_.get_cfg(build_cfg_key({ OBIS_IF_1107, OBIS_IF_1107_MAX_VARIATION }), std::chrono::seconds(9u));
			append_get_proc_response(msg, {
				OBIS_IF_1107,
				OBIS_IF_1107_MAX_VARIATION
				}, make_value(max_var));

			//
			//	read IEC devices from "_IECDevs"
			//
			cache_.loop("_IECDevs", [&](cyng::table::record const& rec)->bool {

				auto const nr = cyng::value_cast<std::uint8_t>(rec.key().at(0), 1u);

				append_get_proc_response(msg, {
					OBIS_IF_1107,
					OBIS_IF_1107_METER_LIST,
					make_obis(0x81, 0x81, 0xC7, 0x93, 0x09, nr),
					OBIS_IF_1107_METER_ID
					}, make_value(rec["meterID"]));

				//
				//	81 81 C7 93 0B FF - baudrate
				//
				append_get_proc_response(msg, {
					OBIS_IF_1107,
					OBIS_IF_1107_METER_LIST,
					make_obis(0x81, 0x81, 0xC7, 0x93, 0x09, nr),
					OBIS_IF_1107_BAUDRATE
					}, make_value(rec["baudrate"]));

				//
				//	81 81 C7 93 0C FF - address
				//
				append_get_proc_response(msg, {
					OBIS_IF_1107,
					OBIS_IF_1107_METER_LIST,
					make_obis(0x81, 0x81, 0xC7, 0x93, 0x09, nr),
					OBIS_IF_1107_ADDRESS
					}, make_value(rec["address"]));

				//
				//	81 81 C7 93 0D FF - P1
				//
				append_get_proc_response(msg, {
					OBIS_IF_1107,
					OBIS_IF_1107_METER_LIST,
					make_obis(0x81, 0x81, 0xC7, 0x93, 0x09, nr),
					OBIS_IF_1107_P1
					}, make_value(rec["p1"]));

				//
				//	81 81 C7 93 0E FF - W5
				//
				append_get_proc_response(msg, {
					OBIS_IF_1107,
					OBIS_IF_1107_METER_LIST,
					make_obis(0x81, 0x81, 0xC7, 0x93, 0x09, nr),
					OBIS_IF_1107_W5
					}, make_value(rec["w5"]));


				return true;
				});
#ifdef __DEBUG
			//
			//	return some IEC devices
			//
			auto rnd = cyng::crypto::make_rnd_hex();
			for (std::uint8_t nr = 1; nr < 10; ++nr) {

				auto const meter = rnd(8);

				//
				//	81 81 C7 93 0A FF - meter id
				//
				append_get_proc_response(msg, {
					OBIS_IF_1107,
					OBIS_IF_1107_METER_LIST,
					make_obis(0x81, 0x81, 0xC7, 0x93, 0x09, nr),
					OBIS_IF_1107_METER_ID
					}, make_value(meter));

				//
				//	81 81 C7 93 0B FF - baudrate
				//
				append_get_proc_response(msg, {
					OBIS_IF_1107,
					OBIS_IF_1107_METER_LIST,
					make_obis(0x81, 0x81, 0xC7, 0x93, 0x09, nr),
					OBIS_IF_1107_BAUDRATE
					}, make_value(2400u));

				//
				//	81 81 C7 93 0C FF - address
				//
				append_get_proc_response(msg, {
					OBIS_IF_1107,
					OBIS_IF_1107_METER_LIST,
					make_obis(0x81, 0x81, 0xC7, 0x93, 0x09, nr),
					OBIS_IF_1107_ADDRESS
					}, make_value(meter));

				//
				//	81 81 C7 93 0D FF - P1
				//
				append_get_proc_response(msg, {
					OBIS_IF_1107,
					OBIS_IF_1107_METER_LIST,
					make_obis(0x81, 0x81, 0xC7, 0x93, 0x09, nr),
					OBIS_IF_1107_P1
					}, make_value("p1"));

				//
				//	81 81 C7 93 0E FF - W5
				//
				append_get_proc_response(msg, {
					OBIS_IF_1107,
					OBIS_IF_1107_METER_LIST,
					make_obis(0x81, 0x81, 0xC7, 0x93, 0x09, nr),
					OBIS_IF_1107_W5
					}, make_value("w5"));

			}
#endif
			//
			//	append to message queue
			//
			sml_gen_.append(std::move(msg));

		}

		void config_iec::set_meter(std::uint8_t nr, cyng::param_map_t&& map)
		{
			CYNG_LOG_DEBUG(logger_, "sml.set.proc.parameter.request (iec meter) #"
				<< +nr
				<< ": "
				<< cyng::io::to_str(map));

			//
			//	index is the table key
			//
			auto const key = cyng::table::key_generator(nr);

			cache_.write_table("_IECDevs", [&](cyng::store::table* tbl) {

				if (tbl->exist(key)) {
					if (map.empty()) {
						//
						//	delete 
						//
						tbl->erase(key, cache_.get_tag());
					}
					else {
						for (auto const& p : map) {
							auto const r = parse_obis(p.first);
							if (r.second) {
								if (OBIS_IF_1107_METER_ID == r.first) {
									//	[string]
									tbl->modify(key, cyng::param_t("meterID", p.second), cache_.get_tag());
								}
								else if (OBIS_IF_1107_BAUDRATE == r.first) {
									//	[u]
									tbl->modify(key, cyng::param_t("baudrate", p.second), cache_.get_tag());
								}
								else if (OBIS_IF_1107_ADDRESS == r.first) {
									//	[string]
									tbl->modify(key, cyng::param_t("address", p.second), cache_.get_tag());
								}
								else if (OBIS_IF_1107_P1 == r.first) {
									//	[string]
									tbl->modify(key, cyng::param_t("p1", p.second), cache_.get_tag());
								}
								else if (OBIS_IF_1107_W5 == r.first) {
									//	[string]
									tbl->modify(key, cyng::param_t("w5", p.second), cache_.get_tag());
								}
							}
						}
					}
				}
				else {
					if (map.size() == 5) {
						//
						//	insert
						//
						tbl->insert(key
							, cyng::table::data_generator(map.at(OBIS_IF_1107_METER_ID.to_str())
								, map.at(OBIS_IF_1107_ADDRESS.to_str())
								, std::string("description")	//	description
								, map.at(OBIS_IF_1107_BAUDRATE.to_str())
								, map.at(OBIS_IF_1107_P1.to_str())
								, map.at(OBIS_IF_1107_W5.to_str()))
							, 0u	//	gen
							, cache_.get_tag());
					}
				}
			});
		}

		void config_iec::set_param(obis const& code, cyng::param_t param)
		{
			CYNG_LOG_DEBUG(logger_, "sml.set.proc.parameter.request (if 1107) "
				<< param.first
				<< " = "
				<< cyng::io::to_str(param.second));

			if (code == OBIS_IF_1107_LOOP_TIME) {
				auto const val = cyng::numeric_cast<std::uint32_t>(param.second, 60u);
				cache_.set_cfg(build_cfg_key({ OBIS_IF_1107, code }), cyng::make_seconds(val));
			}
			else if (code == OBIS_IF_1107_MIN_TIMEOUT) {
				auto const val = cyng::numeric_cast<std::uint32_t>(param.second, 200u);
				cache_.set_cfg(build_cfg_key({ OBIS_IF_1107, code }), cyng::make_milliseconds(val));
			}
			else if (code == OBIS_IF_1107_MAX_TIMEOUT) {
				auto const val = cyng::numeric_cast<std::uint32_t>(param.second, 5000u);
				cache_.set_cfg(build_cfg_key({ OBIS_IF_1107, code }), cyng::make_milliseconds(val));
			}
			else if (code == OBIS_IF_1107_TIME_GRID) {
				auto const val = cyng::numeric_cast<std::uint32_t>(param.second, 900u);
				cache_.set_cfg(build_cfg_key({ OBIS_IF_1107, code }), cyng::make_seconds(val));
			}
			else if (code == OBIS_IF_1107_TIME_SYNC) {
				auto const val = cyng::numeric_cast<std::uint32_t>(param.second, 14400u);
				cache_.set_cfg(build_cfg_key({ OBIS_IF_1107, code }), cyng::make_seconds(val));
			}
			else if (code == OBIS_IF_1107_MAX_VARIATION) {
				auto const val = cyng::numeric_cast<std::uint32_t>(param.second, 9u);
				cache_.set_cfg(build_cfg_key({ OBIS_IF_1107, code }), cyng::make_seconds(val));
			}
			else {
				cache_.set_cfg(build_cfg_key({ OBIS_IF_1107, code }), param.second);
			}
		}

	}	//	sml
}

