/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "get_profile_list.h"
#include "../segw.h"
#include "../cache.h"
#include "../storage.h"

#include <smf/sml/protocol/generator.h>
#include <smf/sml/srv_id_io.h>
#include <smf/sml/obis_io.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/intrinsics/obis_factory.hpp>
#include <smf/shared/db_cfg.h>
#include <smf/mbus/units.h>

#include <cyng/io/serializer.h>
#include <cyng/io/io_chrono.hpp>
#ifdef _DEBUG
#include <cyng/rnd.h>
#endif

namespace node
{
	namespace sml
	{

		get_profile_list::get_profile_list(cyng::logging::log_ptr logger
			, res_generator& sml_gen
			, cache& cfg
			, storage& db)
		: logger_(logger)
			, sml_gen_(sml_gen)
			, cache_(cfg)
			, storage_(db)
		{
		}

		void get_profile_list::generate_response(obis code
			, std::string trx
			, cyng::buffer_t client_id
			, cyng::buffer_t srv_id
			, std::string name
			, std::string pwd
			, std::chrono::system_clock::time_point start
			, std::chrono::system_clock::time_point end)
		{
			switch (code.to_uint64()) {
			case 0x8181C789E1FF:	//	CLASS_OP_LOG
				class_op_log(trx, client_id, srv_id, start, end);
				break;
			case 0x8181C78610FF:	//	PROFILE_1_MINUTE
			case 0x8181C78611FF:	//	PROFILE_15_MINUTE
				//profile_15_minute(trx, client_id, srv_id, start, end);
			case 0x8181C78612FF:	//	PROFILE_60_MINUTE
			case 0x8181C78613FF:	//	PROFILE_24_HOUR
			case 0x8181C78614FF:	//	PROFILE_LAST_2_HOURS
			case 0x8181C78615FF:	//	PROFILE_LAST_WEEK
			case 0x8181C78616FF:	//	PROFILE_1_MONTH
			case 0x8181C78617FF:	//	PROFILE_1_YEAR
			case 0x8181C78618FF:	//	PROFILE_INITIAL
				get_profile(code, trx, client_id, srv_id, start, end);
				break;

			default:
				CYNG_LOG_ERROR(logger_, "sml.get.profile.list.request - unknown OBIS code "
					<< to_string(code)
					<< " / "
					<< cyng::io::to_hex(code.to_buffer()));
				break;
			}
		}

		void get_profile_list::class_op_log(std::string trx
			, cyng::buffer_t client_id
			, cyng::buffer_t srv_id
			, std::chrono::system_clock::time_point start
			, std::chrono::system_clock::time_point end)
		{
			CYNG_LOG_TRACE(logger_, "query op log from "
				<< cyng::to_str(start)
				<< " to "
				<< cyng::to_str(end));

			//
			//	send operation logs - 81 81 C7 89 E1 FF 
			//
			storage_.loop_oplog(start, end, [&](cyng::table::record const& rec)->bool {

				cyng::buffer_t tmp;
				obis peer = cyng::value_cast(rec["peer"], tmp);
				auto server = cyng::value_cast(rec["serverId"], tmp);
				auto idx = cyng::value_cast<std::uint64_t>(rec["ROWID"], 0u);

				sml_gen_.get_profile_op_log(trx
					, client_id
					, cyng::value_cast(rec["actTime"], std::chrono::system_clock::now()) //	act_time
					, cyng::value_cast<std::uint32_t>(rec["regPeriod"], 900u) //	reg_period
					, cyng::value_cast(rec["valTime"], std::chrono::system_clock::now())
					, cyng::value_cast<std::uint64_t>(rec["status"], 0u) //	status
					, cyng::value_cast<std::uint32_t>(rec["event"], 0u) //	evt
					, obis(peer) //	peer_address
					, cyng::value_cast(rec["utc"], std::chrono::system_clock::now())
					, server
					, cyng::value_cast<std::string>(rec["target"], "")
					, cyng::value_cast<std::uint8_t>(rec["pushNr"], 1u)
					, cyng::value_cast<std::string>(rec["details"], "")
				);

				return true;	//	continue
			});
		}

#ifdef _DEBUG
		void get_profile_list::profile_15_minute(std::string trx
			, cyng::buffer_t client_id
			, cyng::buffer_t srv_id
			, std::chrono::system_clock::time_point start
			, std::chrono::system_clock::time_point end)
		{
			CYNG_LOG_WARNING(logger_, "OBIS_PROFILE_15_MINUTE not implemented yet");

			auto rndi = cyng::crypto::make_rnd(-10, +10);
			auto val = rndi();

			for (auto pos = 0; pos < 100; ++pos) {
				auto msg = sml_gen_.empty_get_profile_list(trx
					, client_id
					, OBIS_PROFILE_15_MINUTE
					, std::chrono::system_clock::now()	// act_time
					, 0u //	reg_period
					, std::chrono::system_clock::now()	// val_time
					, 0u);	//	status

				append_period_entry(msg
					, OBIS_PROFILE_15_MINUTE
					, period_entry(OBIS_CODE(01, 00, 01, 08, 00, FF), 30, -1, cyng::make_object(pos + rndi())));

				append_period_entry(msg
					, OBIS_PROFILE_15_MINUTE
					, period_entry(OBIS_CODE(01, 00, 02, 08, 00, FF), 30, -1, cyng::make_object(pos + rndi())));

				append_period_entry(msg
					, OBIS_PROFILE_15_MINUTE
					, period_entry(OBIS_CODE(01, 00, 03, 08, 00, FF), 30, -1, cyng::make_object(val)));
				val += rndi();

				append_period_entry(msg
					, OBIS_PROFILE_15_MINUTE
					, period_entry(OBIS_CODE(81, 81, C7, 82, 03, FF), 255, 0, cyng::make_object("solos::Tec")));

				append_period_entry(msg
					, OBIS_PROFILE_15_MINUTE
					, period_entry(OBIS_CODE(01, 00, 00, 09, 0B, 00), 7, 0, cyng::make_object(1568906997+pos)));

				append_period_entry(msg
					, OBIS_PROFILE_15_MINUTE
					, period_entry(OBIS_CODE(81, 06, 2B, 07, 00, 00), 255, 0, cyng::make_object(-53)));

				//
				//	append to message queue
				//
				sml_gen_.append(std::move(msg));
			}
		}
#endif

		void get_profile_list::get_profile(obis profile
			, std::string trx
			, cyng::buffer_t client_id
			, cyng::buffer_t srv_id
			, std::chrono::system_clock::time_point start
			, std::chrono::system_clock::time_point end)
		{
			std::uint64_t idx_prev{ 0u };
			cyng::tuple_t msg;

			storage_.loop_profile(profile, start, end, [&](cyng::buffer_t srv_id
				, std::uint64_t idx
				, std::chrono::system_clock::time_point tsidx
				, std::chrono::system_clock::time_point act_time
				, std::uint32_t status
				, sml::obis code
				, std::int8_t scaler 
				, std::uint8_t unit
				, cyng::object&& obj)->bool {

					if (idx_prev != idx) {

						if (idx_prev != 0u) {
							//
							//	append to message queue
							//
							sml_gen_.append(std::move(msg));
						}

						msg = sml_gen_.empty_get_profile_list(trx
							, srv_id
							, profile
							, act_time	// act_time
							, 0u //	reg_period
							, std::chrono::system_clock::now()	// val_time
							, 0u);	//	status

						idx_prev = idx;
					}

					append_period_entry(msg
						, profile
						, period_entry(code, unit, scaler, obj));

					CYNG_LOG_TRACE(logger_, sml::from_server_id(srv_id)
						<< ", "
						<< cyng::to_str(tsidx)
						<< ", "
						<< cyng::to_str(act_time)
						<< ", "
						<< status
						<< ", "
						<< code.to_str()
						<< ", "
						<< cyng::io::to_str(obj)
						<< ' '
						<< mbus::get_unit_name(unit));

				return true;	//	continue
			});

			//
			//	append to message queue
			//
			if (!msg.empty())	sml_gen_.append(std::move(msg));

		}
	}	//	sml
}

