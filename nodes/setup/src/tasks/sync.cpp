/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#include "sync.h"
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

		//
		//	create table
		//
		create_table();

	}

	void sync::run()
	{	
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> subscribe "
			<< table_);

		subscribe();
	}

	void sync::stop()
	{
		CYNG_LOG_INFO(logger_, "sync "
			<< table_
			<< " is stopped");
	}

	void sync::create_table()
	{
		CYNG_LOG_TRACE(logger_, "create table cache " << table_);

		if (boost::algorithm::equals(table_, "TDevice"))
		{
			cache_.create_table(cyng::table::make_meta_table<1, 7>("TDevice",
				{ "pk", "name", "number", "descr", "id", "vFirmware", "enabled", "created" },
				{ cyng::TC_UUID, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_BOOL, cyng::TC_TIME_POINT },
				{ 36, 128, 128, 512, 64, 64, 0, 0 }));

		}
		else if (boost::algorithm::equals(table_, "TGateway"))
		{
			cache_.create_table(cyng::table::make_meta_table<1, 13>("TGateway", { "pk"	//	primary key
				, "id"	//	(1) Server-ID (i.e. 0500153B02517E)
				, "manufacturer"	//	(2) manufacturer (i.e. EMH)
				, "model"	//	(3) Typbezeichnung (i.e. Variomuc ETHERNET)
				, "made"	//	(4) production date
				, "vFirmware"	//	(5) firmwareversion (i.e. 11600000)
				, "factoryNr"	//	(6) fabrik nummer (i.e. 06441734)
				, "ifService"	//	(7) MAC of service interface
				, "ifData"	//	(8) MAC od data interface
				, "pwdDef"	//	(9) Default PW
				, "pwdRoot"	//	(10)
				, "mbus"	//	(11)
				, "userName"	//	(12)
				, "userPwd"	//	(13)
				},
				{ cyng::TC_UUID, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_TIME_POINT, cyng::TC_STRING, cyng::TC_MAC48, cyng::TC_MAC48, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING },
				{ 36, 23, 64, 64, 0, 8, 18, 18, 32, 32, 16, 32, 32 }));

		}
		else
		{
			CYNG_LOG_ERROR(logger_, table_ << " is an undefined data scheme")
		}
	}

	void sync::subscribe()
	{
		bus_->vm_.async_run(bus_req_subscribe(table_));
		bus_->vm_.async_run(bus_reflect("cluster.task.resume", tsk_, static_cast<std::size_t>(0), table_, base_.get_id()));
	}

	cyng::continuation sync::process(std::size_t size)
	{
		CYNG_LOG_INFO(logger_, "cache complete - send "
			<< cache_.size()
			<< " record(s)");

		BOOST_ASSERT(size == cache_.size());

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

		return cyng::continuation::TASK_CONTINUE;
	}

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
