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

		cache_.read_tables("_Readout", "_ReadoutData", [&](cyng::store::table const* tbl_ro, cyng::store::table const* tbl_data) {

			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> with "
				<< tbl_ro->size()
				<< " readout record(s)");

			tbl_ro->loop([&](cyng::table::record const& rec_ro) {

				//
				//	get primary key
				//
				auto const pk = rec_ro["pk"];

				//
				//	collect all data
				//
				tbl_data->loop([&](cyng::table::record const& rec_data) {

					CYNG_LOG_TRACE(logger_, "task #"
						<< base_.get_id()
						<< " <"
						<< base_.get_class_name()
						<< "> data "
						<< cyng::io::to_str(rec_data.convert()));

					return true;	//	continue
				});

				return true;	//	continue
			});
		});

		//
		//	clean up readout cache
		//
		cache_.clear_table("_Readout");
		cache_.clear_table("_ReadoutData");
	}

	void readout::distribute(cyng::buffer_t const& srv)
	{

	}

}

