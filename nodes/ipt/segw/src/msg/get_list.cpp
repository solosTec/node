/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "get_list.h"
#include "../segw.h"
#include "../cache.h"
#include "../storage.h"

#include <smf/sml/protocol/generator.h>
#include <smf/sml/obis_io.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/intrinsics/obis_factory.hpp>
#include <smf/shared/db_cfg.h>

#include <cyng/io/serializer.h>
#include <cyng/io/io_chrono.hpp>
#include <cyng/buffer_cast.h>
#include <cyng/numeric_cast.hpp>
#include <cyng/table/restore.h>
#ifdef _DEBUG
#include <cyng/rnd.h>
#endif

namespace node
{
	namespace sml
	{

		get_list::get_list(cyng::logging::log_ptr logger
			, res_generator& sml_gen
			, cache& cfg
			, storage& db)
		: logger_(logger)
			, sml_gen_(sml_gen)
			, cache_(cfg)
			, storage_(db)
		{
		}

		cyng::tuple_t get_list::generate_response(obis&& code
			, std::string trx
			, cyng::buffer_t client_id
			, cyng::buffer_t srv_id //	<- meter/sensor ID with the requested data
			, std::string req_field
			, std::string user
			, std::string pwd)
		{
			switch (code.to_uint64()) {
			case CODE_LIST_CURRENT_DATA_RECORD:	
				return current_data_record(trx, client_id, srv_id, req_field);

			default:
				CYNG_LOG_ERROR(logger_, "sml.get.list.request - unknown OBIS code "
					<< to_string(code)
					<< " / "
					<< cyng::io::to_hex(code.to_buffer()));
				break;
			}

			return cyng::tuple_factory();
		}

		cyng::tuple_t get_list::current_data_record(std::string trx
			, cyng::buffer_t client_id
			, cyng::buffer_t srv_id
			, std::string req_field)
		{
			cyng::tuple_t cdr;	//	current data record

			cache_.read_tables("_Readout", "_ReadoutData", [&](cyng::store::table const* tbl_ro, cyng::store::table const* tbl_data) {
				tbl_ro->loop([&](cyng::table::record const& rec_meta) {

					auto const tmp = cyng::to_buffer(rec_meta["serverID"]);
					BOOST_ASSERT_MSG(tmp.size() == 9, "invalid server ID");
					if (srv_id == tmp) {

						//
						//	get primary key
						//
						auto const pk = rec_meta["pk"];

						//
						//	collect all data from this readout
						//
						auto const result = tbl_data->find_all(cyng::param_t("pk", pk));

						for (auto const& key : result) {

							auto const rec_data = tbl_data->lookup(key);
							obis const code(cyng::to_buffer(rec_data["OBIS"]));

							auto const type = cyng::value_cast<std::uint32_t>(rec_data["type"], 0);
							auto const value = cyng::value_cast(rec_data["val"], "");
							cyng::object obj;
							try {
								obj = cyng::table::restore(value, type);
							}
							catch (std::exception const& ex) {
								obj = cyng::make_object(ex.what());
							}

							CYNG_LOG_TRACE(logger_, "current data record "
								<< code.to_str()
								<< " = "
								<< cyng::io::to_str(obj)
								<< ':'
								<< obj.get_class().type_name()
								<< " ("
								<< cyng::io::to_str(rec_data["val"])
								<< ")");

							cdr.push_back(list_entry(code
								, 0	//	status
								, cyng::value_cast(rec_meta["ts"], std::chrono::system_clock::now())
								, cyng::numeric_cast<std::uint8_t>(rec_data["unit"], 0)	//	unit
								, cyng::numeric_cast<std::int8_t>(rec_data["scaler"], 0)	//	scaler
								, obj));
						}
						return false;	//	stop
					}
					
					return true;	//	continue
				});
			});

			return sml_gen_.get_list(trx
				, client_id
				, srv_id
				, OBIS_LIST_CURRENT_DATA_RECORD
				, make_timestamp(std::chrono::system_clock::now())
				, make_timestamp(std::chrono::system_clock::now())
				, cdr);

		}

	}	//	sml
}

