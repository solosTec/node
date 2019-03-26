/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "sync.h"
#include "setup_defs.h"
#include <smf/cluster/generator.h>
#include <cyng/async/task/task_builder.hpp>
#include <cyng/dom/reader.h>
#include <cyng/io/serializer.h>
#include <cyng/table/meta.hpp>
#include <cyng/vm/generator.h>
#include <boost/algorithm/string/predicate.hpp>

namespace node
{
	sync::sync(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger 
		, bus::shared_type cl_bus
		, std::size_t tsk
		, std::string const& table
		, cyng::store::db& cache)
	: base_(*btp)
	, logger_(logger)
	, bus_(cl_bus)
	, tsk_cluster_(tsk)
	, table_(table)
	, cache_(cache)
	{
		CYNG_LOG_INFO(logger_, "task #"
		<< base_.get_id()
		<< " <"
		<< base_.get_class_name()
		<< "> is syncing "
		<< table_);

	}

	cyng::continuation sync::run()
	{	
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> run "
			<< table_);
		return cyng::continuation::TASK_CONTINUE;
	}

	void sync::stop(bool shutdown)
	{
		CYNG_LOG_INFO(logger_, "sync "
			<< table_
			<< " is stopped");
	}

	void sync::subscribe()
	{
		//
		//	manage table state
		//
		cache_.set_state(table_, TS_SYNC);

		//
		//	Get existing records from master. This could be setup data
		//	from another redundancy or data collected during a line disruption.
		//
		bus_->vm_.async_run(bus_req_subscribe(table_, base_.get_id()));

		//	
		//	Continue uplading remaining records
		//	
		bus_->vm_.async_run(bus_reflect("cluster.task.resume"
			, base_.get_id()
			, static_cast<std::size_t>(1)	//	slot
			, base_.get_id()));
	}

	cyng::continuation sync::process(std::string name, std::size_t sync_tsk)
	{
		BOOST_ASSERT(name == table_);
		CYNG_LOG_INFO(logger_, table_
			<< " cache complete - start sync");

		subscribe();
		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation sync::process(std::size_t size)
	{
		CYNG_LOG_INFO(logger_, "cache "
			<< table_
			<< " complete - send "
			<< cache_.size(table_)
			<< " record(s)");

		//BOOST_ASSERT(size == cache_.size());

		//
		//	upload cache
		//
		cache_.access([this](const cyng::store::table* tbl)->void {

			BOOST_ASSERT(tbl->meta().get_name() == table_);

			//CYNG_LOG_INFO(logger_, tbl->meta().get_name() << "->size(" << tbl->size() << ")");
			tbl->loop([this, tbl](cyng::table::record const& rec) -> bool {

#ifdef _DEBUG
				BOOST_ASSERT_MSG(tbl->meta().check_key(rec.key()), "invalid key");
				BOOST_ASSERT_MSG((tbl->meta().size() == (rec.key().size() + rec.data().size())), "invalid key or data");
#endif

				//
				//	upload	
				//
				CYNG_LOG_TRACE(logger_, cyng::io::to_str(rec[0]));
				bus_->vm_.async_run(bus_req_db_insert(tbl->meta().get_name()
					, rec.key()
					, rec.data()
					, rec.get_generation()
					, bus_->vm_.tag()));

				//	continue
				return true;
			});

		}, cyng::store::read_access(table_));

		//
		//	clear table
		//
		cache_.clear(table_, bus_->vm_.tag());

		//
		//	manage table state
		//
		cache_.set_state(table_, TS_READY);

		base_.mux_.post(tsk_cluster_, 2, cyng::tuple_factory(table_, base_.get_id()));


		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation sync::process()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> unsubscribe from cache "
			<< table_);

		cache_.clear(table_, bus_->vm_.tag());
		return cyng::continuation::TASK_STOP;
	}
}
