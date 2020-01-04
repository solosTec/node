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

			cache_.read_table("_DataCollector", [&](cyng::store::table const* tbl_dc) {

				std::uint8_t nr{ 1 };	//	data collector index
				tbl_dc->loop([&](cyng::table::record const& rec) {

					auto const id = cyng::to_buffer(rec["serverID"]);
					if (id == srv_id) {

						nr = cyng::numeric_cast<std::uint8_t>(rec["nr"], nr);

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

						std::uint8_t idx{ 1 };	//	OBIS counter
						//
						//	81 81 C7 8A 23 FF - entries
						//
						auto const entries = cyng::value_cast<std::string>(rec["entries"], "");
						auto const vec = to_obis_path(entries, ':');
						for (auto const& code : vec) {

							append_get_proc_response(msg, {
								OBIS_ROOT_DATA_COLLECTOR,
								make_obis(0x81, 0x81, 0xC7, 0x86, 0x20, nr),
								OBIS_PROFILE,
								make_obis(0x81, 0x81, 0xC7, 0x8A, 0x23, idx)
								}, make_value(code));

							//
							//	update OBIS counter
							//
							++idx;
						}

						//
						//	update data collector index
						//
						++nr;
					}
					return true;	//	continue
					});

				});

			//
			//	append to message queue
			//
			sml_gen_.append(std::move(msg));

		}

		void config_data_collector::set_param(cyng::buffer_t srv_id
			, std::uint8_t nr
			, cyng::param_map_t&& params)
		{

			cache_.write_table("_DataCollector", [&](cyng::store::table* tbl) {

				auto const key = cyng::table::key_generator(srv_id, nr);
				auto const rec = tbl->lookup(key);
				if (rec.empty()) {
					insert_data_collector(tbl, key, params, cache_.get_tag());
				}
				else {
					update_data_collector(tbl, key, params, cache_.get_tag());
				}
			});
		}

		void config_data_collector::clear_data_collector(cyng::buffer_t srv_id
			, cyng::param_map_t&& params)
		{
			//%(("8181C78204FF":01E61E571406213603), ("8181C78A83FF":8181C78612FF))

			auto const id = cyng::to_buffer(lookup(params, OBIS_SERVER_ID));
			obis const profile(cyng::to_buffer(lookup(params, OBIS_PROFILE)));

			cache_.write_table("_DataCollector", [&](cyng::store::table* tbl_dc) {

				cyng::table::key_list_t keys;
				tbl_dc->loop([&](cyng::table::record const& rec) {
					if (cyng::to_buffer(rec["serverID"]) == id && cyng::to_buffer(rec["profile"]) == profile) {
						keys.push_back(rec.key());
					}
					return true;
					});

				for (auto const key : keys) {
					tbl_dc->erase(key, cache_.get_tag());
				}

				});
		}


		void insert_data_collector(cyng::store::table* tbl, cyng::table::key_type const& key, cyng::param_map_t const& params, boost::uuids::uuid source)
		{
			//	[19122321172417116-2,8181C78620FF 8181C7862001,01A815743145040102,operator,operator,("8181C7862001":%(("8181C78621FF":true),("8181C78622FF":64),("8181C78781FF":0),("8181C78A23FF":%(("8181C78A2301":070003010001),("8181C78A2302":0700030100FF))),("8181C78A83FF":8181C78611FF)))]

			//%(("8181C78621FF":true),("8181C78622FF":64),("8181C78781FF":0),("8181C78A23FF":%(("8181C78A2301":070003010001),("8181C78A2302":0700030100FF))),("8181C78A83FF":8181C78611FF))

			auto const obj = lookup(params, OBIS_DATA_COLLECTOR_OBIS);
			

			tbl->insert(key
				, cyng::table::data_generator(lookup(params, OBIS_PROFILE), lookup(params, OBIS_DATA_COLLECTOR_ACTIVE), lookup(params, OBIS_DATA_COLLECTOR_SIZE), lookup(params, OBIS_DATA_REGISTER_PERIOD), get_entries(obj))
				, 0u
				, source);

		}

		void update_data_collector(cyng::store::table* tbl, cyng::table::key_type const& key, cyng::param_map_t const& params, boost::uuids::uuid source)
		{
			for (auto const& param : params) {
				if (boost::algorithm::equals(param.first, OBIS_DATA_COLLECTOR_ACTIVE.to_str())) {
					tbl->modify(key, cyng::param_t("active", param.second), source);
				}
				else if (boost::algorithm::equals(param.first, OBIS_DATA_COLLECTOR_SIZE.to_str())) {
					tbl->modify(key, cyng::param_t("maxSize", param.second), source);
				}
				else if (boost::algorithm::equals(param.first, OBIS_DATA_REGISTER_PERIOD.to_str())) {
					tbl->modify(key, cyng::param_t("regPeriod", param.second), source);
				}
				else if (boost::algorithm::equals(param.first, OBIS_DATA_COLLECTOR_OBIS.to_str())) {
					//
					//	update entries
					//
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

		std::string get_entries(cyng::object obj)
		{
			obis_path vec;
			auto const params = cyng::to_param_map(obj);
			std::transform(params.begin(), params.end(), std::back_inserter(vec), [](cyng::param_t const& p) {
				return obis(cyng::to_buffer(p.second));
				});
			return to_hex(vec, ':');
		}


	}	//	sml
}

