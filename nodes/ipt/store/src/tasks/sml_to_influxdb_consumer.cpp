/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "sml_to_influxdb_consumer.h"
#include "../message_ids.h"
#include <smf/shared/db_meta.h>

#include <smf/sml/defs.h>
#include <NODE_project_info.h>

#include <cyng/async/task/base_task.h>
#include <cyng/dom/reader.h>
#include <cyng/io/serializer.h>
#include <cyng/db/connection_types.h>
#include <cyng/db/session.h>
#include <cyng/db/interface_session.h>
#include <cyng/db/sql_table.h>
#include <cyng/value_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/sql.h>
#include <cyng/table/meta.hpp>
#include <cyng/vm/generator.h>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace node
{
	sml_influxdb_consumer::sml_influxdb_consumer(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, std::size_t ntid	//	network task id
		, std::string host
		, std::string service
		, bool tls
		, std::string cert
		, std::chrono::seconds interval)
	: base_(*btp)
		, logger_(logger)
		, ntid_(ntid)
		, host_(host)
		, service_(service)
		, tls_(tls)
		, cert_(cert)
		, interval_(interval)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");

	}

	cyng::continuation sml_influxdb_consumer::run()
	{

		base_.suspend(interval_);
		return cyng::continuation::TASK_CONTINUE;
	}

	void sml_influxdb_consumer::stop(bool shutdown)
	{
		//
		//	remove all open lines
		//
		//lines_.clear();

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " stopped");
	}

	cyng::continuation sml_influxdb_consumer::process(std::uint64_t line
		, std::string target)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " create line "
			<< line
			<< ':'
			<< target);

		//lines_.emplace(std::piecewise_construct,
		//	std::forward_as_tuple(line),
		//	std::forward_as_tuple(meta_map_
		//		, schema_
		//		, (std::uint32_t)((line & 0xFFFFFFFF00000000LL) >> 32)
		//		, (std::uint32_t)(line & 0xFFFFFFFFLL)
		//		, target));

		//CYNG_LOG_TRACE(logger_, "task #"
		//	<< base_.get_id()
		//	<< " <"
		//	<< base_.get_class_name()
		//	<< "> has "
		//	<< lines_.size()
		//	<< " active lines");

		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation sml_influxdb_consumer::process(std::uint64_t line
		, std::uint16_t code
		, cyng::tuple_t msg)
	{
		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " line "
			<< line
			<< " received: "
			<< sml::messages::name(code));

		//auto pos = lines_.find(line);
		//if (pos != lines_.end()) {

		//	try {
		//		//
		//		//	write to DB
		//		//
		//		pos->second.write(pool_.get_session(), msg);
		//	}
		//	catch (std::exception const& ex) {

		//		CYNG_LOG_ERROR(logger_, "task #"
		//			<< base_.get_id()
		//			<< " <"
		//			<< base_.get_class_name()
		//			<< " line "
		//			<< line
		//			<< " error: "
		//			<< ex.what());
		//	}
		//}
		//else {

		//	CYNG_LOG_ERROR(logger_, "task #"
		//		<< base_.get_id()
		//		<< " <"
		//		<< base_.get_class_name()
		//		<< " line "
		//		<< line
		//		<< " not found");
		//}

		return cyng::continuation::TASK_CONTINUE;
	}

	//	slot [2] - CONSUMER_REMOVE_LINE
	cyng::continuation sml_influxdb_consumer::process(std::uint64_t line)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " close line "
			<< line);

		//auto pos = lines_.find(line);
		//if (pos != lines_.end()) {

		//	//
		//	//	remove this line
		//	//
		//	lines_.erase(pos);
		//}
		//else {

		//	CYNG_LOG_ERROR(logger_, "task #"
		//		<< base_.get_id()
		//		<< " <"
		//		<< base_.get_class_name()
		//		<< " line "
		//		<< line
		//		<< " not found");
		//}

		//CYNG_LOG_TRACE(logger_, "task #"
		//	<< base_.get_id()
		//	<< " <"
		//	<< base_.get_class_name()
		//	<< "> has "
		//	<< lines_.size()
		//	<< " active lines");

		return cyng::continuation::TASK_CONTINUE;
	}

	//	EOM
	cyng::continuation sml_influxdb_consumer::process(std::uint64_t line, std::size_t idx, std::uint16_t crc)
	{
		return cyng::continuation::TASK_CONTINUE;
	}
}
