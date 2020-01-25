/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "readout.h"
#include "../cache.h"
#include "../storage.h"
#include <smf/sml/srv_id_io.h>
#include <smf/sml/obis_db.h>

#include <cyng/io/io_chrono.hpp>
#include <cyng/buffer_cast.h>
#include <cyng/numeric_cast.hpp>

namespace node
{
	readout::readout(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cache& db
		, storage& store
		, std::chrono::seconds interval)
	: base_(*btp) 
		, logger_(logger)
		, cache_(db)
		, storage_(store)
		, interval_(interval)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> with an interval of "
			<< cyng::to_str(interval_));
	}

	cyng::continuation readout::run()
	{
#ifdef _DEBUG
		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");
#endif
		//
		//	re-start timer
		//
		base_.suspend(interval_);

		//
		//	check for new readout data
		//
		distribute();

		return cyng::continuation::TASK_CONTINUE;
	}

	void readout::stop(bool shutdown)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> ");
	}

	cyng::continuation readout::process(cyng::buffer_t srv)
	{
		distribute(srv);

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}

	void readout::distribute()
	{
		//
		//	Collect all cached readout data and store all records
		//	into SQL database that are listed in the data collector.
		//
		cache_.get_db().access([&](cyng::store::table* tbl_ro
			, cyng::store::table * tbl_data
			, cyng::store::table const* tbl_collector
			, cyng::store::table const* tbl_mirror) {

			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> with "
				<< tbl_ro->size()
				<< " readout record(s)");

			tbl_ro->loop([&](cyng::table::record const& rec_meta) {

				CYNG_LOG_TRACE(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> meta "
					<< cyng::io::to_str(rec_meta.convert()));

				//
				//	get primary key
				//
				auto const pk = rec_meta["pk"];

				//
				//	collect all data from this readout
				//
				auto const result = tbl_data->find_all(cyng::param_t("pk", pk));

				//
				//	An optimization could be to check if there is any
				//	data collector with this serverID and to skip to skip
				//	the loop is not.
				//
				auto const srv_id = cyng::to_buffer(rec_meta["serverID"]);

				//
				//	look for matching and active data mirrors
				//
				auto collectors = tbl_collector->find_all(cyng::param_t("serverID", rec_meta["serverID"]));

				//
				//	remove all inactive collectors from result set
				//
				cleanup_collectors(tbl_collector, collectors);

				if (collectors.empty()) {
					CYNG_LOG_WARNING(logger_, "task #"
						<< base_.get_id()
						<< " <"
						<< base_.get_class_name()
						<< "> no data collector defined for "
						<< sml::from_server_id(srv_id));
				}

				for (auto const& key_collector : collectors) {
					auto const rec_collector = tbl_collector->lookup(key_collector);
					BOOST_ASSERT(!rec_collector.empty());

					//
					//	get profile and make readout data permanent
					//
					sml::obis const profile = cyng::to_buffer(rec_collector["profile"]);

					CYNG_LOG_TRACE(logger_, "task #"
						<< base_.get_id()
						<< " <"
						<< base_.get_class_name()
						<< "> profile "
						<< sml::get_profile_name(profile)
						<< " of "
						<< sml::from_server_id(srv_id));

					//
					//	processing readout data
					//

					switch (profile.to_uint64()) {
					case sml::CODE_PROFILE_1_MINUTE:
						profile_1_minute(tbl_data, tbl_mirror, rec_collector, rec_meta, result);
						break;
					case sml::CODE_PROFILE_15_MINUTE:
						profile_15_minute(tbl_data, tbl_mirror, rec_collector, rec_meta, result);
						break;
					case sml::CODE_PROFILE_60_MINUTE:
						profile_60_min(tbl_data, tbl_mirror, rec_collector, rec_meta, result);
						break;
					case sml::CODE_PROFILE_24_HOUR:
						profile_24_hour(tbl_data, tbl_mirror, rec_collector, rec_meta, result);
						break;
					case sml::CODE_PROFILE_LAST_2_HOURS:
						break;
					case sml::CODE_PROFILE_LAST_WEEK:
						break;
					case sml::CODE_PROFILE_1_MONTH:
						break;
					case sml::CODE_PROFILE_1_YEAR:
						break;
					case sml::CODE_PROFILE_INITIAL:
						break;
					default:
						CYNG_LOG_ERROR(logger_, "task #"
							<< base_.get_id()
							<< " <"
							<< base_.get_class_name()
							<< "> unknown profile "
							<< profile.to_str()
							<< " for "
							<< sml::from_server_id(srv_id));
						break;
					}
				}

				//
				//	clean up readout data cache
				//
				cyng::erase(tbl_data, result, cache_.get_tag());

				return true;	//	continue
			});

			//
			//	clean up readout cache
			//
			tbl_ro->clear(cache_.get_tag());

		}	, cyng::store::write_access("_Readout")
			, cyng::store::write_access("_ReadoutData")
			, cyng::store::read_access("_DataCollector")
			, cyng::store::read_access("_DataMirror"));

	}

	void readout::distribute(cyng::buffer_t const& srv)
	{
		CYNG_LOG_ERROR(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> slot [0] not implemented yet");
	}

	void readout::cleanup_collectors(cyng::store::table const* tbl_collector
		, cyng::table::key_list_t& collectors)
	{
		collectors.remove_if([&](cyng::table::key_type const& key) {

			auto const rec_collector = tbl_collector->lookup(key);
			BOOST_ASSERT(!rec_collector.empty());
			auto const active = cyng::value_cast(rec_collector["active"], false);
			return !active;
		});
	}

	void readout::profile_1_minute(cyng::store::table const* tbl_data
		, cyng::store::table const* tbl_mirror
		, cyng::table::record const& rec_collector
		, cyng::table::record const& rec_meta
		, cyng::table::key_list_t const& result)
	{
		auto const srv_id = cyng::to_buffer(rec_meta["serverID"]);
		auto const ts = cyng::value_cast(rec_meta["ts"], std::chrono::system_clock::now());
		auto const minutes = cyng::chrono::minutes_since_epoch(ts);
		auto const status = cyng::numeric_cast<std::uint32_t>(rec_meta["status"], 0);

		storage_.merge_profile_meta_8181C78610FF(srv_id, minutes, ts, status);

		for (auto const& key : result) {
			auto const rec_data = tbl_data->lookup(key);
			BOOST_ASSERT(!rec_data.empty());

			CYNG_LOG_TRACE(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> data "
				<< cyng::io::to_str(rec_data.convert()));
		}
	}

	void readout::profile_15_minute(cyng::store::table const* tbl_data
		, cyng::store::table const* tbl_mirror
		, cyng::table::record const& rec_collector
		, cyng::table::record const& rec_meta
		, cyng::table::key_list_t const& result)
	{
		auto const srv_id = cyng::to_buffer(rec_meta["serverID"]);
		auto const ts = cyng::value_cast(rec_meta["ts"], std::chrono::system_clock::now());
		auto const quarters = cyng::chrono::minutes_since_epoch(ts) / 4;
		auto const status = cyng::numeric_cast<std::uint32_t>(rec_meta["status"], 0);

		storage_.merge_profile_meta_8181C78611FF(srv_id, quarters, ts, status);

		for (auto const& key : result) {
			auto const rec_data = tbl_data->lookup(key);
			BOOST_ASSERT(!rec_data.empty());

			auto const code = cyng::to_buffer(rec_data["OBIS"]);

			CYNG_LOG_TRACE(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> data "
				<< sml::obis(code).to_str()
				<< " - "
				<< cyng::io::to_str(rec_data.convert()));

			storage_.merge_profile_storage_8181C78611FF(srv_id
				, quarters
				, code
				, rec_data["val"]
				, rec_data["scaler"]
				, rec_data["unit"]
				, rec_data["type"]);

		}
	}

	void readout::profile_60_min(cyng::store::table const* tbl_data
		, cyng::store::table const* tbl_mirror
		, cyng::table::record const& rec_collector
		, cyng::table::record const& rec_meta
		, cyng::table::key_list_t const& result)
	{
		//
		//	store meta data
		//
		auto const srv_id = cyng::to_buffer(rec_meta["serverID"]);
		auto const ts = cyng::value_cast(rec_meta["ts"], std::chrono::system_clock::now());
		auto const hours = cyng::chrono::minutes_since_epoch(ts);
		auto const status = cyng::numeric_cast<std::uint32_t>(rec_meta["status"], 0);

		storage_.merge_profile_meta_8181C78612FF(srv_id, hours, ts, status);

		//
		//	store readout data
		//
		for (auto const& key : result) {
			auto const rec_data = tbl_data->lookup(key);
			BOOST_ASSERT(!rec_data.empty());

			auto const code = cyng::to_buffer(rec_data["OBIS"]);

			CYNG_LOG_TRACE(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> data "
				<< sml::obis(code).to_str()
				<< " - "
				<< cyng::io::to_str(rec_data.convert()));

				//	[2020-01-24 14:26:03.09515740] TRACE  2084 -- task #2 <node::readout> profile PROFILE_60_MINUTE of 01-e61e-57140621-36-03
				//	[2020-01-24 14:26:03.09344180] TRACE  2084 -- task #2 <node::readout> meta {("key":%(("pk":07979b37-afb8-4c16-8bbd-cd1e8564c0c5))),("data":%(("gen":1),("serverID":01E61E571406213603),("status":0),("ts":2020-01-24 14:25:58.10786690)))}
				//	[2020-01-24 14:26:03.09678630] TRACE  2084 -- task #2 <node::readout> data {("key":%(("OBIS":070003010000),("pk":07979b37-afb8-4c16-8bbd-cd1e8564c0c5))),("data":%(("gen":1),("scaler":-2),("type":9),("unit":d),("val":0018e6db)))}
				//	[2020-01-24 14:26:03.09865190] TRACE  2084 -- task #2 <node::readout> data {("key":%(("OBIS":8106000374FF),("pk":07979b37-afb8-4c16-8bbd-cd1e8564c0c5))),("data":%(("gen":1),("scaler":0),("type":9),("unit":7),("val":0000011e)))}

			storage_.merge_profile_storage_8181C78612FF(srv_id
				, hours
				, code
				, rec_data["val"]
				, rec_data["scaler"]
				, rec_data["unit"]
				, rec_data["type"]);

		}
	}

	void readout::profile_24_hour(cyng::store::table const* tbl_data
		, cyng::store::table const* tbl_mirror
		, cyng::table::record const& rec_collector
		, cyng::table::record const& rec_meta
		, cyng::table::key_list_t const& result)
	{
		//
		//	store meta data
		//
		auto const srv_id = cyng::to_buffer(rec_meta["serverID"]);
		auto const ts = cyng::value_cast(rec_meta["ts"], std::chrono::system_clock::now());
		auto const days = cyng::chrono::days_since_epoch(ts);
		auto const status = cyng::numeric_cast<std::uint32_t>(rec_meta["status"], 0);

		storage_.merge_profile_meta_8181C78613FF(srv_id, days, ts, status);


		for (auto const& key : result) {
			auto const rec_data = tbl_data->lookup(key);
			BOOST_ASSERT(!rec_data.empty());

			auto const code = cyng::to_buffer(rec_data["OBIS"]);

			CYNG_LOG_TRACE(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> data "
				<< sml::obis(code).to_str()
				<< " - "
				<< cyng::io::to_str(rec_data.convert()));

			storage_.merge_profile_storage_8181C78613FF(srv_id
				, days
				, code
				, rec_data["val"]
				, rec_data["scaler"]
				, rec_data["unit"]
				, rec_data["type"]);

		}
	}

}

