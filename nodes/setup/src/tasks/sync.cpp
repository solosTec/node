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
				bus_->vm_.run(bus_req_db_insert(tbl->meta().get_name()
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
		//	subscribe cache
		//
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> subscribe cache "
			<< table_);

		cache_.get_listener(table_
			, std::bind(&sync::sig_ins, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
			, std::bind(&sync::sig_del, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			, std::bind(&sync::sig_clr, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&sync::sig_mod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));

		//
		//	manage table state
		//
		cache_.set_state(table_, TS_READY);

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

		cache_.disconnect(table_);
		cache_.clear(table_, bus_->vm_.tag());
		return cyng::continuation::TASK_STOP;
	}

	void sync::sig_ins(cyng::store::table const* tbl
		, cyng::table::key_type const&
		, cyng::table::data_type const&
		, std::uint64_t
		, boost::uuids::uuid source)
	{
		CYNG_LOG_INFO(logger_, "sig.ins " << table_);
		BOOST_ASSERT(tbl->meta().get_name() == table_);
	}

	void sync::sig_del(cyng::store::table const* tbl, cyng::table::key_type const&, boost::uuids::uuid source)
	{
		CYNG_LOG_INFO(logger_, "sig.gel " << table_);
		BOOST_ASSERT(tbl->meta().get_name() == table_);
	}

	void sync::sig_clr(cyng::store::table const* tbl, boost::uuids::uuid source)
	{
		CYNG_LOG_INFO(logger_, "sig.clr " << table_);
		BOOST_ASSERT(tbl->meta().get_name() == table_);
	}

	void sync::sig_mod(cyng::store::table const* tbl
		, cyng::table::key_type const&
		, cyng::attr_t const&
		, std::uint64_t gen
		, boost::uuids::uuid source)
	{
		CYNG_LOG_INFO(logger_, "sig.mod " << table_);
		BOOST_ASSERT(tbl->meta().get_name() == table_);
	}

}
