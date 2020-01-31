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

#include <cyng/io/io_chrono.hpp>
#include <cyng/buffer_cast.h>

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
		, std::string target)
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
			, cyng::store::table const* tbl_dm
			, cyng::store::table* tbl_req) {

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
							collect_profile_8181C78610FF(tbl_req);
							break;
						case sml::CODE_PROFILE_15_MINUTE:
							collect_profile_8181C78611FF(tbl_req);
							break;
						case sml::CODE_PROFILE_60_MINUTE:
							collect_profile_8181C78612FF(tbl_req);
							break;
						case sml::CODE_PROFILE_24_HOUR:
							collect_profile_8181C78613FF(tbl_req);
							break;
						case sml::CODE_PROFILE_LAST_2_HOURS:
							collect_profile_8181C78614FF(tbl_req);
							break;
						case sml::CODE_PROFILE_LAST_WEEK:
							collect_profile_8181C78615FF(tbl_req);
							break;
						case sml::CODE_PROFILE_1_MONTH:
							collect_profile_8181C78616FF(tbl_req);
							break;
						case sml::CODE_PROFILE_1_YEAR:
							collect_profile_8181C78617FF(tbl_req);
							break;
						case sml::CODE_PROFILE_INITIAL:
							collect_profile_8181C78618FF(tbl_req);
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
			, cyng::store::read_access("_DataMirror")
			, cyng::store::write_access("_PushReq"));

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

	}

	void push::collect_profile_8181C78611FF(cyng::store::table* tbl)
	{	//	15 minutes

	}

	void push::collect_profile_8181C78612FF(cyng::store::table* tbl)
	{	//	60 minutes

	}

	void push::collect_profile_8181C78613FF(cyng::store::table* tbl)
	{	//	24 hours

	}

	void push::collect_profile_8181C78614FF(cyng::store::table* tbl)
	{	//	last 2 hours

	}

	void push::collect_profile_8181C78615FF(cyng::store::table* tbl)
	{	//	last week

	}

	void push::collect_profile_8181C78616FF(cyng::store::table* tbl)
	{	//	1 month

	}

	void push::collect_profile_8181C78617FF(cyng::store::table* tbl)
	{	//	1 year

	}

	void push::collect_profile_8181C78618FF(cyng::store::table* tbl)
	{	//	initial

	}

}

