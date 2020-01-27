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

#include <cyng/io/io_chrono.hpp>

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

			send_push_data();
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
				, sml::OBIS_PEER_OBISLOG	//	ToDo: use correct OBIS code
				, srv	//	server ID
				, target_
				, 0	//	nr ToDo: increment
				, "cannot push");

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

	void push::send_push_data()
	{
		//
		//	Write data into a memory table "_PushReq" triggers listener
		//	that will send the push data to an ipt master.
		//	After receiving a response the data will be marked
		//	as obsolete and will be removed. Otherwise data remain
		//	in table will be send again.
		//
		cache_.get_db().access([&](cyng::store::table const* tbl_dc, cyng::store::table* tbl_req) {

			auto const key = cyng::table::key_generator(srv_id_, nr_);
			auto const rec = tbl_dc->lookup(key);
			if (!rec.empty()) {

				auto const active = cyng::value_cast(rec["active"], false);
				if (active) {

					//
					//	collect OBIS codes from "_DataMirror"
					//

					//
					//	collect data records from TProfile_... and TStorage_...
					//	and insert this records into "_PushReq"
					//
				}
			}
			
		}	, cyng::store::read_access("_DataCollector")
			, cyng::store::write_access("_PushReq"));

	}

}

