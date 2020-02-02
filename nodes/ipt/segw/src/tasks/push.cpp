/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "push.h"
#include "../cache.h"
#include "../storage.h"
#include <smf/sml/event.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/srv_id_io.h>
#include <smf/mbus/units.h>

#include <cyng/io/io_chrono.hpp>
#include <cyng/buffer_cast.h>
#include <cyng/db/session.h>
#include <cyng/db/sql_table.h>
#include <cyng/db/interface_statement.h>
#include <cyng/table/restore.h>

namespace node
{
	push::push(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cache& db
		, storage& store
		, cyng::buffer_t srv_id
		, std::uint8_t nr
		, cyng::buffer_t profile
		, std::chrono::seconds interval
		, std::chrono::seconds delay
		, std::string target
		, std::size_t tsk)
	: base_(*btp) 
		, logger_(logger)
		, cache_(db)
		, storage_(store)
		, srv_id_(srv_id)
		, nr_(nr)
		, profile_(profile)
		, interval_(interval)
		, delay_(delay)
		, target_(target)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> with "
			<< cyng::to_str(interval_)
			<< " interval and "
			<< cyng::to_str(delay_)
			<< " delay to ["
			<< target_	//	push target name
			<< "]");
	}

	cyng::continuation push::run()
	{
#ifdef _DEBUG
		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< sml::from_server_id(srv_id_)
			<< ":"
			<< sml::get_profile_name(profile_)
			<< " ==> "
			<< target_);
#endif
		//
		//	recalibrate start time dependent from profile type
		//


		//
		//	start/restart timer
		//
		base_.suspend(interval_);

		//
		//	push data
		//
		if (cache_.is_authorized()) {

			if (send_push_data()) {

				//
				//	inconsistent data - stop task
				//
				return cyng::continuation::TASK_STOP;
			}
		}
		else {
			CYNG_LOG_WARNING(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> cannot push "
				<< sml::get_profile_name(profile_)
				<< " ==> "
				<< target_);

			//
			//	OBIS log entry
			//

			auto const sw = cache_.get_status_word();
			auto srv = cache_.get_srv_id();

			storage_.generate_op_log(sw
				, sml::LOG_CODE_16	//	Push - Vorgang nicht erfolgreich
				, sml::OBIS_PEER_ADDRESS	//	ToDo: use correct OBIS code
				, srv	//	server ID
				, target_
				, nr_	//	nr 
				, "unable to open push target");

		}

		return cyng::continuation::TASK_CONTINUE;
	}

	void push::stop(bool shutdown)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is stopped");
	}

	cyng::continuation push::process()
	{

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}

	bool push::send_push_data()
	{
		bool stop{ false };
		//
		//	Write data into a memory table "_PushReq" triggers listener
		//	that will send the push data to an ipt master.
		//	After receiving a response the data will be marked
		//	as obsolete and will be removed. Otherwise data remain
		//	in table will be send again.
		//
		cache_.get_db().access([&](cyng::store::table const* tbl_dc
			, cyng::store::table const* tbl_dm) {

			auto const key = cyng::table::key_generator(srv_id_, nr_);
			auto const rec = tbl_dc->lookup(key);
			if (!rec.empty()) {

				auto const active = cyng::value_cast(rec["active"], false);
				if (active) {

					//
					//	collect OBIS codes from "_DataMirror"
					//
					auto const codes = this->collect_obis_codes(tbl_dm);

					if (!codes.empty()) {
						//
						//	collect data records from TProfile_... and TStorage_...
						//	and insert this records into "_PushReq"
						//
						switch (profile_.to_uint64()) {
						case sml::CODE_PROFILE_1_MINUTE:
							//collect_profile_8181C78610FF(tbl_req);
							break;
						case sml::CODE_PROFILE_15_MINUTE:
							//collect_profile_8181C78611FF(tbl_req);
							break;
						case sml::CODE_PROFILE_60_MINUTE:
							collect_profile_8181C78612FF();
							break;
						case sml::CODE_PROFILE_24_HOUR:
							//collect_profile_8181C78613FF(tbl_req);
							break;
						case sml::CODE_PROFILE_LAST_2_HOURS:
							//collect_profile_8181C78614FF(tbl_req);
							break;
						case sml::CODE_PROFILE_LAST_WEEK:
							//collect_profile_8181C78615FF(tbl_req);
							break;
						case sml::CODE_PROFILE_1_MONTH:
							//collect_profile_8181C78616FF(tbl_req);
							break;
						case sml::CODE_PROFILE_1_YEAR:
							//collect_profile_8181C78617FF(tbl_req);
							break;
						case sml::CODE_PROFILE_INITIAL:
							//collect_profile_8181C78618FF(tbl_req);
							break;
						default:
							BOOST_ASSERT_MSG(false, "undefined profile");
							break;
						}
					}
					else {
						CYNG_LOG_WARNING(logger_, "task #"
							<< base_.get_id()
							<< " <"
							<< base_.get_class_name()
							<< "> "
							<< sml::from_server_id(srv_id_)
							<< ":"
							<< sml::get_profile_name(profile_)
							<< " ==> "
							<< target_
							<< " has no OBIS codes");

					}
				}
				else {
					CYNG_LOG_WARNING(logger_, "task #"
						<< base_.get_id()
						<< " <"
						<< base_.get_class_name()
						<< "> "
						<< sml::from_server_id(srv_id_)
						<< ":"
						<< sml::get_profile_name(profile_)
						<< " ==> "
						<< target_
						<< " is inactive");
				}
			}
			else {

				CYNG_LOG_WARNING(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> no record in \"_DataCollector\" for "
					<< sml::from_server_id(srv_id_)
					<< ":"
					<< +nr_);

				//
				//	stop task
				//
				stop = true;

			}
			
		}	, cyng::store::read_access("_DataCollector")
			, cyng::store::read_access("_DataMirror"));

		return stop;
	}

	std::vector<sml::obis> push::collect_obis_codes(cyng::store::table const* tbl)
	{
		std::vector<sml::obis> codes;

		tbl->loop([&](cyng::table::record const& rec) {

			auto const srv_id = cyng::to_buffer(rec["serverID"]);
			auto const nr = cyng::value_cast<std::uint8_t>(rec["nr"], 0);

			if ((this->srv_id_ == srv_id) && (this->nr_ == nr)) {

				//
				//	match
				//
				sml::obis code(cyng::to_buffer(rec["code"]));
				codes.push_back(code);
			}

			return true;	//	continue
		});

		return codes;
	}

	void push::collect_profile_8181C78610FF(cyng::store::table* tbl)
	{	//	1 minute
		std::string sql = "SELECT P10.clientID, P10.tsidx, datetime(P10.actTime), status, S10.OBIS, S10.val, S10.type, S10.scaler, S10.unit from TProfile_8181C78610FF P10 JOIN TStorage_8181C78610FF S10 ON P10.clientID = S10.clientID AND P10.tsidx = S10.tsidx;";

	}

	void push::collect_profile_8181C78611FF(cyng::store::table* tbl)
	{	//	15 minutes
		std::string sql = "SELECT P11.clientID, P11.tsidx, datetime(P11.actTime), status, S11.OBIS, S11.val, S11.type, S11.scaler, S11.unit from TProfile_8181C78611FF P11 JOIN TStorage_8181C78611FF S11 ON P11.clientID = S11.clientID AND P11.tsidx = S11.tsidx;";

	}

	void push::collect_profile_8181C78612FF()
	{	//	60 minutes
		std::string sql = "SELECT P12.clientID, P12.tsidx, datetime(P12.actTime), status, S12.OBIS, S12.val, S12.type, S12.scaler, S12.unit from TProfile_8181C78612FF P12 JOIN TStorage_8181C78612FF S12 ON P12.clientID = S12.clientID AND P12.tsidx = S12.tsidx;";
		auto s = storage_.get_session();
		auto stmt = s.create_statement();
		std::pair<int, bool> r = stmt->prepare(sql);
		if (r.second) {

			//
			//	read all results
			//
			std::uint8_t counter{ 0 };
			while (auto res = stmt->get_result()) {

				//
				//	Convert SQL result to record
				//
				auto const size = res->column_count();
				BOOST_ASSERT(size == 9);
				auto const srv_id = cyng::to_buffer(res->get(1, cyng::TC_BUFFER, 9));
				auto const tsdix = cyng::value_cast<std::uint64_t>(res->get(2, cyng::TC_UINT64, 0), 0);
				//	3. actTime
				//	4. status
				sml::obis const code(cyng::to_buffer(res->get(5, cyng::TC_BUFFER, 6)));
				//	6. val
				auto const val = cyng::value_cast<std::string>(res->get(6, cyng::TC_STRING, 0), "");
				//	7. type
				auto const type = cyng::value_cast<std::uint32_t>(res->get(7, cyng::TC_UINT32, 0), 0);
				auto const scaler = cyng::value_cast<std::int8_t>(res->get(8, cyng::TC_INT8, 0), 0);
				auto const unit = cyng::value_cast<std::uint8_t>(res->get(9, cyng::TC_UINT8, 0), 0);

				//
				//	restore object from string and type info
				//
				auto const obj = cyng::table::restore(val, type);

				std::cout
					<< sml::from_server_id(srv_id)
					<< ", "
					<< tsdix
					<< " - "
					<< cyng::to_str(get_ts(profile_, tsdix))
					<< ", "
					<< cyng::io::to_str(res->get(3, cyng::TC_TIME_POINT, 0)).substr(0, 19)
					<< ", "
					<< cyng::value_cast<std::uint32_t>(res->get(4, cyng::TC_UINT32, 0), -1)
					<< ", "
					<< code.to_str()
					<< ", "
					<< cyng::io::to_str(obj)
					<< " "
					<< mbus::get_unit_name(unit)
					<< std::endl;

				//
				//	ToDo: build push msg
				//

				++counter;

			}

		}
	}

	void push::collect_profile_8181C78613FF(cyng::store::table* tbl)
	{	//	24 hours
		std::string sql = "SELECT P13.clientID, P13.tsidx, datetime(P13.actTime), status, S13.OBIS, S13.val, S13.type, S13.scaler, S13.unit from TProfile_8181C78613FF P13 JOIN TStorage_8181C78613FF S13 ON P13.clientID = S13.clientID AND P13.tsidx = S13.tsidx;";

	}

	void push::collect_profile_8181C78614FF(cyng::store::table* tbl)
	{	//	last 2 hours
		std::string sql = "SELECT P14.clientID, P14.tsidx, datetime(P14.actTime), status, S14.OBIS, S14.val, S14.type, S14.scaler, S14.unit from TProfile_8181C78614FF P14 JOIN TStorage_8181C78614FF S14 ON P14.clientID = S14.clientID AND P14.tsidx = S14.tsidx;";

	}

	void push::collect_profile_8181C78615FF(cyng::store::table* tbl)
	{	//	last week
		std::string sql = "SELECT P15.clientID, P15.tsidx, datetime(P15.actTime), status, S15.OBIS, S15.val, S15.type, S15.scaler, S15.unit from TProfile_8181C78615FF P15 JOIN TStorage_8181C78615FF S15 ON P15.clientID = S15.clientID AND P15.tsidx = S15.tsidx;";

	}

	void push::collect_profile_8181C78616FF(cyng::store::table* tbl)
	{	//	1 month
		std::string sql = "SELECT P16.clientID, P16.tsidx, datetime(P16.actTime), status, S16.OBIS, S16.val, S16.type, S16.scaler, S16.unit from TProfile_8181C78616FF P16 JOIN TStorage_8181C78616FF S16 ON P16.clientID = S16.clientID AND P16.tsidx = S16.tsidx;";

	}

	void push::collect_profile_8181C78617FF(cyng::store::table* tbl)
	{	//	1 year
		std::string sql = "SELECT P17.clientID, P17.tsidx, datetime(P17.actTime), status, S17.OBIS, S17.val, S17.type, S17.scaler, S17.unit from TProfile_8181C78617FF P17 JOIN TStorage_8181C78617FF S17 ON P17.clientID = S17.clientID AND P17.tsidx = S17.tsidx;";

	}

	void push::collect_profile_8181C78618FF(cyng::store::table* tbl)
	{	//	initial

	}

	std::chrono::system_clock::time_point get_ts(sml::obis profile, std::uint64_t tsidx)
	{
		switch (profile.to_uint64()) {
		case sml::CODE_PROFILE_1_MINUTE:	return cyng::chrono::ts_since_passed_minutes(tsidx);
		case sml::CODE_PROFILE_15_MINUTE:	return cyng::chrono::ts_since_passed_minutes(tsidx / 15u);
		case sml::CODE_PROFILE_60_MINUTE:	return cyng::chrono::ts_since_passed_hours(tsidx);
		case sml::CODE_PROFILE_24_HOUR:	return cyng::chrono::ts_since_passed_days(tsidx);
			//case 14:	return cyng::chrono::ts_since_passed_minutes(profile);
			//case 15:	return cyng::chrono::ts_since_passed_minutes(profile);
		case sml::CODE_PROFILE_1_MONTH:	return cyng::chrono::ts_since_passed_days(tsidx / 30);
		case sml::CODE_PROFILE_1_YEAR:	return cyng::chrono::ts_since_passed_days(tsidx / 365u);
			//case 18:	return cyng::chrono::ts_since_passed_minutes(profile);
		default:
			//std::cerr << "unknown profile: " << profile << std::endl;
			return std::chrono::system_clock::now();
		}
	}
}

