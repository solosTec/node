/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "virtual_meter.h"
#include <smf/sml/srv_id_io.h>
#include <smf/sml/obis_db.h>
#include <smf/mbus/units.h>

#include <cyng/async/task/base_task.h>

#include <boost/uuid/nil_generator.hpp>

namespace node
{

	virtual_meter::virtual_meter(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cyng::store::db& config_db
		, cyng::buffer_t server_id
		, std::chrono::seconds interval)
	: base_(*btp)
		, logger_(logger)
		, config_db_(config_db)
		, server_id_(server_id)
		, interval_(interval)
		, p_value_(0)
		, current_(0u, 120u)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< sml::from_server_id(server_id));
	}

	cyng::continuation virtual_meter::run()
	{
		//
		//	restart timer
		//
		base_.suspend(interval_);

		//
		//	generate random readout values
		//
		generate_values();

		return cyng::continuation::TASK_CONTINUE;
	}

	void virtual_meter::stop(bool shutdown)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is stopped");
	}

	void virtual_meter::generate_values()
	{
		//
		//	generate meter values
		//
		auto const val = current_();
		p_value_ += val;

		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> generate readings: "
			<< val
			<< " / "
			<< p_value_);

		config_db_.access([&](cyng::store::table* tbl) {

			//
			//	manufacturer
			//
			tbl->merge(cyng::table::key_generator(server_id_, sml::OBIS_DATA_MANUFACTURER.to_buffer()),
				cyng::table::data_generator(0, 0, "SOL", "SOL", 0u, std::chrono::system_clock::now()), 
				1, boost::uuids::nil_uuid());


			//	01 00 01 08 00 FF
			tbl->merge(cyng::table::key_generator(server_id_, sml::OBIS_REG_POS_AE_NO_TARIFF.to_buffer()),
				cyng::table::data_generator(static_cast<std::uint8_t>(mbus::units::WATT_HOUR)	//	unit code
					, static_cast<std::int8_t>(0)	//	scaler
					, p_value_	//	raw value
					, std::to_string(p_value_)	//	formatted value
					, 0u	//	status
					, std::chrono::system_clock::now()),
				1, boost::uuids::nil_uuid());

			//	01 00 10 07 00 FF
			tbl->merge(cyng::table::key_generator(server_id_, sml::OBIS_REG_CUR_AP.to_buffer()),
				cyng::table::data_generator(static_cast<std::uint8_t>(mbus::units::WATT)	//	unit code
					, static_cast<std::int8_t>(0)	//	scaler
					, val	//	raw value
					, std::to_string(val)	//	formatted value
					, 0u	//	status
					, std::chrono::system_clock::now()),
				1, boost::uuids::nil_uuid());

			//	00 00 60 01 01 FF - SERIAL_NR_SECOND

		}, cyng::store::write_access("readout"));
	}

}
