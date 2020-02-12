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
#include <smf/sml/obis_io.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/intrinsics/obis_factory.hpp>
#include <smf/shared/db_cfg.h>

#include <cyng/io/serializer.h>
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
				profile_1_minute(trx, client_id, srv_id, start, end);
				break;
			case 0x8181C78611FF:	//	PROFILE_15_MINUTE
				profile_15_minute(trx, client_id, srv_id, start, end);
				break;
			case 0x8181C78612FF:	//	PROFILE_60_MINUTE
				profile_60_minute(trx, client_id, srv_id, start, end);
				break;
			case 0x8181C78613FF:	//	PROFILE_24_HOUR
				profile_24_hour(trx, client_id, srv_id, start, end);
				break;
			case 0x8181C78614FF:	//	PROFILE_LAST_2_HOURS
				profile_last_2_hours(trx, client_id, srv_id, start, end);
				break;
			case 0x8181C78615FF:	//	PROFILE_LAST_WEEK
				profile_last_week(trx, client_id, srv_id, start, end);
				break;
			case 0x8181C78616FF:	//	PROFILE_1_MONTH
				profile_1_month(trx, client_id, srv_id, start, end);
				break;
			case 0x8181C78617FF:	//	PROFILE_1_YEAR
				profile_1_year(trx, client_id, srv_id, start, end);
				break;
			case 0x8181C78618FF:	//	PROFILE_INITIAL
				profile_initial(trx, client_id, srv_id, start, end);
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
			//
			//	send operation logs - 81 81 C7 89 E1 FF 
			//
			cyng::buffer_t tmp;
			storage_.loop("TOpLog", [&](cyng::table::record const& rec)->bool {

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

		void get_profile_list::profile_1_minute(std::string trx
			, cyng::buffer_t client_id
			, cyng::buffer_t srv_id
			, std::chrono::system_clock::time_point start
			, std::chrono::system_clock::time_point end)
		{
			CYNG_LOG_WARNING(logger_, "OBIS_PROFILE_1_MINUTE not implemented yet");
		}

		void get_profile_list::profile_15_minute(std::string trx
			, cyng::buffer_t client_id
			, cyng::buffer_t srv_id
			, std::chrono::system_clock::time_point start
			, std::chrono::system_clock::time_point end)
		{
			CYNG_LOG_WARNING(logger_, "OBIS_PROFILE_15_MINUTE not implemented yet");

#ifdef _DEBUG

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
#endif
		}

		void get_profile_list::profile_60_minute(std::string trx
			, cyng::buffer_t client_id
			, cyng::buffer_t srv_id
			, std::chrono::system_clock::time_point start
			, std::chrono::system_clock::time_point end)
		{
			CYNG_LOG_WARNING(logger_, "OBIS_PROFILE_60_MINUTE not implemented yet");
		}

		void get_profile_list::profile_24_hour(std::string trx
			, cyng::buffer_t client_id
			, cyng::buffer_t srv_id
			, std::chrono::system_clock::time_point start
			, std::chrono::system_clock::time_point end)
		{
			CYNG_LOG_WARNING(logger_, "OBIS_PROFILE_24_HOUR not implemented yet");
		}

		void get_profile_list::profile_last_2_hours(std::string trx
			, cyng::buffer_t client_id
			, cyng::buffer_t srv_id
			, std::chrono::system_clock::time_point start
			, std::chrono::system_clock::time_point end)
		{
			CYNG_LOG_WARNING(logger_, "OBIS_PROFILE_LAST_2_HOURS not implemented yet");
		}

		void get_profile_list::profile_last_week(std::string trx
			, cyng::buffer_t client_id
			, cyng::buffer_t srv_id
			, std::chrono::system_clock::time_point start
			, std::chrono::system_clock::time_point end)
		{
			CYNG_LOG_WARNING(logger_, "OBIS_PROFILE_LAST_WEEK not implemented yet");
		}

		void get_profile_list::profile_1_month(std::string trx
			, cyng::buffer_t client_id
			, cyng::buffer_t srv_id
			, std::chrono::system_clock::time_point start
			, std::chrono::system_clock::time_point end)
		{
			CYNG_LOG_WARNING(logger_, "OBIS_PROFILE_1_MONTH not implemented yet");
		}

		void get_profile_list::profile_1_year(std::string trx
			, cyng::buffer_t client_id
			, cyng::buffer_t srv_id
			, std::chrono::system_clock::time_point start
			, std::chrono::system_clock::time_point end)
		{
			CYNG_LOG_WARNING(logger_, "OBIS_PROFILE_1_YEAR not implemented yet");
		}

		void get_profile_list::profile_initial(std::string trx
			, cyng::buffer_t client_id
			, cyng::buffer_t srv_id
			, std::chrono::system_clock::time_point start
			, std::chrono::system_clock::time_point end)
		{
			CYNG_LOG_WARNING(logger_, "OBIS_PROFILE_INITIAL not implemented yet");
		}


	}	//	sml
}

