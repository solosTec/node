/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "readout.h"
#include "../cache.h"
#include "../storage.h"

#include <cyng/io/io_chrono.hpp>

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
				//	Hint: an optimization could be to check if there is any
				//	data collector with this serverID and to skip to skip
				//	the loop is not.
				//

				//
				//	look for matching and active data mirrors
				//
				auto collectors = tbl_collector->find_all(cyng::param_t("pk", rec_meta["serverID"]));

				//
				//	remove all inactive collectors from result set
				//
				cleanup_collectors(tbl_collector, collectors);

				if (!collectors.empty()) {

					//
					//	processing readout data
					//
					for (auto const& key : result) {
						auto const rec_data = tbl_data->lookup(key);
						BOOST_ASSERT(!rec_data.empty());

						CYNG_LOG_TRACE(logger_, "task #"
							<< base_.get_id()
							<< " <"
							<< base_.get_class_name()
							<< "> data "
							<< cyng::io::to_str(rec_data.convert()));

						process_readout_data(tbl_collector
							, tbl_mirror
							, collectors
							, rec_meta
							, rec_data);
					}
				}
				else {
					CYNG_LOG_WARNING(logger_, "task #"
						<< base_.get_id()
						<< " <"
						<< base_.get_class_name()
						<< "> no data collector defined for "
						<< cyng::io::to_str(rec_meta["serverID"]));
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

	void readout::process_readout_data(cyng::store::table const* tbl_collector
		, cyng::store::table const* tbl_mirror
		, cyng::table::key_list_t const& collectors
		, cyng::table::record const& rec_meta
		, cyng::table::record const& rec_data)
	{
		//
		//	look active data collectors
		//
	}

}

