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
	, tsk_(tsk)
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

	void sync::run()
	{	
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> run "
			<< table_);

		//subscribe();
	}

	void sync::stop()
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

		bus_->vm_.async_run(bus_req_subscribe(table_, base_.get_id()));
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
			tbl->loop([this, tbl](cyng::table::record const& rec) {

				//
				//	upload	
				//
				CYNG_LOG_TRACE(logger_, cyng::io::to_str(rec[0]));
				bus_->vm_.async_run(bus_db_insert(tbl->meta().get_name()
					, rec.key()
					, rec.data()
					, rec.get_generation()));
			});

		}, cyng::store::read_access(table_));

		//
		//	clear table
		//
		cache_.clear(table_);

		//
		//	subscribe cache
		//
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> subscribe cache "
			<< table_);

		cache_.get_listener(table_
			, std::bind(&sync::isig, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)
			, std::bind(&sync::rsig, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&sync::csig, this, std::placeholders::_1)
			, std::bind(&sync::msig, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

		//
		//	manage table state
		//
		cache_.set_state(table_, TS_READY);

		return cyng::continuation::TASK_CONTINUE;
	}

	//cyng::continuation sync::process(std::string name, cyng::vector_t const& key)
	//{
	//	CYNG_LOG_INFO(logger_, "cache "
	//		<< table_
	//		<< " sync data from "
	//		<< name);

	//	//
	//	//	clean up cache
	//	//
	//	cache_.access([this, &key](cyng::store::table* tbl)->void {

	//		if (tbl->exist(key))
	//		{
	//			CYNG_LOG_WARNING(logger_, "cache "
	//				<< tbl->meta().get_name()
	//				<< " clean up record "
	//				<< cyng::io::to_str(key));

	//			tbl->erase(key);
	//		}
	//		else
	//		{
	//			//
	//			//	insert into SQL database
	//			//
	//			CYNG_LOG_TRACE(logger_, "cache "
	//				<< tbl->meta().get_name()
	//				<< " insert record "
	//				<< cyng::io::to_str(key));
	//		}

	//	}, cyng::store::write_access(table_));

	//	return cyng::continuation::TASK_CONTINUE;
	//}


	void sync::isig(cyng::store::table const* tbl, cyng::table::key_type const&, cyng::table::data_type const&, std::uint64_t)
	{
		CYNG_LOG_INFO(logger_, "isig " << table_);
		BOOST_ASSERT(tbl->meta().get_name() == table_);
	}

	void sync::rsig(cyng::store::table const* tbl, cyng::table::key_type const&)
	{
		CYNG_LOG_INFO(logger_, "rsig " << table_);
		BOOST_ASSERT(tbl->meta().get_name() == table_);
	}

	void sync::csig(cyng::store::table const* tbl)
	{
		CYNG_LOG_INFO(logger_, "csig " << table_);
		BOOST_ASSERT(tbl->meta().get_name() == table_);
	}

	void sync::msig(cyng::store::table const* tbl, cyng::table::key_type const&, cyng::attr_t const&)
	{
		CYNG_LOG_INFO(logger_, "msig " << table_);
		BOOST_ASSERT(tbl->meta().get_name() == table_);
	}

}
