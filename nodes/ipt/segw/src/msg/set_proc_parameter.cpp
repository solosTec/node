/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "set_proc_parameter.h"
#include "config_ipt.h"
#include "config_sensor_params.h"
#include "config_data_collector.h"
#include "config_access.h"
#include "config_iec.h"
#include "config_broker.h"
#include "../cache.h"
#include "../segw.h"

#include <smf/sml/protocol/generator.h>
#include <smf/sml/obis_io.h>
#include <smf/sml/obis_db.h>

#include <cyng/io/serializer.h>
#include <cyng/buffer_cast.h>
#include <cyng/numeric_cast.hpp>
#include <cyng/set_cast.h>

namespace node
{
	namespace sml
	{
		set_proc_parameter::set_proc_parameter(cyng::logging::log_ptr logger
			, res_generator& sml_gen
			, cache& cfg
			, node::ipt::config_ipt& ipt
			, config_sensor_params& sensor_params
			, config_data_collector& data_collector
			, config_security& security
			, config_access& access
			, config_iec& iec
			, config_broker& broker)
		: logger_(logger)
			, sml_gen_(sml_gen)
			, cache_(cfg)
			, config_ipt_(ipt)
			, config_sensor_params_(sensor_params)
			, config_data_collector_(data_collector)
			, config_security_(security)
			, config_access_(access)
			, config_iec_(iec)
			, config_broker_(broker)
		{}

		void set_proc_parameter::generate_response(obis_path_t const& path
			, std::string trx
			, cyng::buffer_t srv_id
			, std::string user
			, std::string pwd
			, cyng::param_t	param)
		{

			//	[190919215155673187-2,81490D0700FF 81490D070002 81491A070002,0500FFB04B94F8,operator,operator,("81491A070002":68f0)]
			if (!path.empty()) {

				BOOST_ASSERT(path.back().to_str() == param.first);

				switch (path.front().to_uint64()) {
				case CODE_ROOT_IPT_PARAM:	//	IP-T (0x81490D0700FF)
					config_ipt_.set_param(path, param);
					break;
				case CODE_ROOT_SENSOR_PARAMS:	//	0x8181C78600FF
					config_sensor_params_.set_param(path, srv_id, param);
					break;
				case CODE_ROOT_DATA_COLLECTOR:	//	 0x8181C78620FF (Datenspiegel)
					config_data_collector_.set_param(path.back().get_data().at(obis::VG_STORAGE), srv_id, cyng::to_param_map(param.second));
					break;
				case CODE_CLEAR_DATA_COLLECTOR:	//	8181C7838205
					config_data_collector_.clear_data_collector(srv_id, cyng::to_param_map(param.second));
					break;
				case CODE_STORAGE_TIME_SHIFT:	//	0x0080800000FF
					storage_time_shift(cyng::numeric_cast<std::int32_t>(param.second, 0u));
					break;
				case CODE_ROOT_PUSH_OPERATIONS:	//	0x8181C78A01FF
					config_data_collector_.set_push_operations(path.back().get_data().at(obis::VG_STORAGE), srv_id, cyng::to_param_map(param.second));
					break;
				case CODE_ROOT_SECURITY:	//	00 80 80 01 00 FF
					//config_data_security_.set_proc_params(trx, srv_id);
					break;
				case CODE_CLASS_MBUS:	//	0x00B000020000
					class_mbus(path.back(), param);
					break;
				case CODE_IF_1107:	//	81 81 C7 93 00 FF
					if (OBIS_IF_1107_METER_LIST == path.back()) {
						//
						//	add/modify meter list
						//
						auto const nr = path.back().get_data().at(obis::VG_STORAGE);
						config_iec_.set_meter(nr, cyng::to_param_map(param.second));
					}
					else {
						config_iec_.set_param(path.back(), param);
					}
					break;
				case CODE_REBOOT:
					CYNG_LOG_WARNING(logger_, "sml.set.proc.parameter.request - reboot");
					reboot(trx, srv_id);
					break;
				case CODE_ROOT_BROKER:
					config_broker_.set_proc_params(path, srv_id, param.second);
					break;
				default:
					CYNG_LOG_ERROR(logger_, "sml.set.proc.parameter.request - unknown OBIS path "
						<< sml::to_hex(path, ':')
						<< " = "
						<< cyng::io::to_type(param.second));
					break;
				}
			}
			else {
				CYNG_LOG_ERROR(logger_, "set_proc_parameter(empty path)");
			}
		}

		void set_proc_parameter::storage_time_shift(std::int32_t sts)
		{
			CYNG_LOG_DEBUG(logger_, "sml.set.proc.parameter.request <0080800000FF> - storage time shift "
				<< sml::OBIS_STORAGE_TIME_SHIFT.to_str()
				<< " = "
				<< sts);

			cache_.set_cfg(sml::OBIS_STORAGE_TIME_SHIFT.to_str(), sts);
		}

		void set_proc_parameter::class_mbus(obis const& code
			, cyng::param_t param)
		{
			CYNG_LOG_DEBUG(logger_, "sml.set.proc.parameter.request (m-bus) "
				<< param.first
				<< " = "
				<< cyng::io::to_str(param.second));

			if (OBIS_CLASS_MBUS_RO_INTERVAL == code) {
				auto const val = cyng::numeric_cast<std::uint32_t> (param.second, 3600u);
				cache_.set_cfg(build_cfg_key({ OBIS_CLASS_MBUS, code }), std::chrono::seconds(val));
			}
			else if (OBIS_CLASS_MBUS_SEARCH_INTERVAL == code) {
				auto const val = cyng::numeric_cast<std::uint32_t> (param.second, 0u);
				cache_.set_cfg(build_cfg_key({ OBIS_CLASS_MBUS, code }), std::chrono::seconds(val));
			}
			else if (sml::OBIS_CLASS_MBUS_BITRATE == code) {
				//	[u32] bismask
				//	bit 1 = 150 baud
				//	bit 2 = 300 baud
				//	bit 5 = 2400 baud
				//	bit 7 = 9600 baud
				auto const val = cyng::numeric_cast<std::uint32_t> (param.second, 0u);
				cache_.set_cfg(build_cfg_key({ OBIS_CLASS_MBUS, code }), val);
			}
			else {
				cache_.set_cfg(build_cfg_key({ OBIS_CLASS_MBUS, code }), param.second);
			}
		}

		void set_proc_parameter::reboot(std::string trx
			, cyng::buffer_t srv_id)
		{
#if defined(NODE_CROSS_COMPILE)
			//
			//	generate an attention as response
			//
			sml_gen_.attention_msg(cyng::make_object(trx)
				, srv_id	//	server ID
				, sml::OBIS_ATTENTION_OK.to_buffer()
				, "reboot"
				, cyng::tuple_t());

			//
			//	ToDo: this has to be executed later
			//
			system("/usr/sbin/reboot.sh");
#else 
			sml_gen_.attention_msg(cyng::make_object(trx)
				, srv_id	//	server ID
				, sml::OBIS_ATTENTION_NOT_EXECUTED.to_buffer()
				, "reboot"
				, cyng::tuple_t());
#endif
		}

	}	//	sml
}

