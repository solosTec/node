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
			, config_iec& iec)
		: logger_(logger)
			, sml_gen_(sml_gen)
			, cache_(cfg)
			, config_ipt_(ipt)
			, config_sensor_params_(sensor_params)
			, config_data_collector_(data_collector)
			, config_security_(security)
			, config_access_(access)
			, config_iec_(iec)
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

				auto pos = path.begin();
				auto end = path.end();
				obis const code(*pos);
				++pos;

				switch (code.to_uint64()) {
				case CODE_ROOT_IPT_PARAM:	//	IP-T (0x81490D0700FF)
					if (pos != end)	_81490d0700ff(pos, end, trx, srv_id, user, pwd, param);
					break;
				case CODE_ROOT_SENSOR_PARAMS:	//	0x8181C78600FF
					if (pos != end)	config_sensor_params_.set_param(*pos, srv_id, param);
					break;
				case CODE_ROOT_DATA_COLLECTOR:	//	 0x8181C78620FF (Datenspiegel)
					BOOST_ASSERT(pos->to_str() == param.first);
					if (pos != end)	config_data_collector_.set_param(srv_id, obis(*pos).get_data().at(obis::VG_STORAGE), cyng::to_param_map(param.second));
					break;
				case CODE_CLEAR_DATA_COLLECTOR:	//	8181C7838205
					config_data_collector_.clear_data_collector(srv_id, cyng::to_param_map(param.second));
					break;
				case CODE_STORAGE_TIME_SHIFT:	//	0x0080800000FF
					storage_time_shift(pos, end, trx, srv_id, user, pwd, param);
					break;
				case CODE_PUSH_OPERATIONS:	//	0x8181C78A01FF
					BOOST_ASSERT(pos->to_str() == param.first);
					if (pos != end)	config_data_collector_.set_push_operations(srv_id, user, pwd, obis(*pos).get_data().at(obis::VG_STORAGE), cyng::to_param_map(param.second));
					break;
				case CODE_ROOT_SECURITY:	//	00 80 80 01 00 FF
					//config_data_security_.set_proc_params(trx, srv_id);
					break;
				case CODE_CLASS_MBUS:	//	0x00B000020000
					BOOST_ASSERT(pos != end);
					BOOST_ASSERT(pos->to_str() == param.first);
					class_mbus(obis(*pos), param);
					break;
				case CODE_IF_1107:	//	81 81 C7 93 00 FF
					BOOST_ASSERT(pos != end);
					if (OBIS_IF_1107_METER_LIST == obis(*pos)) {
						//
						//	add/modify meter list
						//
						++pos;
						auto const nr = obis(*pos).get_data().at(obis::VG_STORAGE);
						BOOST_ASSERT(pos != end);
						BOOST_ASSERT(pos->to_str() == param.first);
						//iec_meter(nr, cyng::to_param_map(param.second));
						config_iec_.set_meter(nr, cyng::to_param_map(param.second));
					}
					else {
						BOOST_ASSERT(pos->to_str() == param.first);
						config_iec_.set_param(obis(*pos), param);
					}
					break;
				default:
					CYNG_LOG_ERROR(logger_, "sml.set.proc.parameter.request - unknown OBIS code "
						<< code.to_str()
						<< " : "
						<< to_string(*pos)
						<< " / "
						<< get_name(code));
					break;
				}
			}
			else {
				CYNG_LOG_ERROR(logger_, "set_proc_parameter(empty path)");
			}
		}

		void set_proc_parameter::_81490d0700ff(obis_path_t::const_iterator pos
			, obis_path_t::const_iterator end
			, std::string trx
			, cyng::buffer_t srv_id
			, std::string user
			, std::string pwd
			, cyng::param_t	param)
		{
			switch (pos->to_uint64()) {
			case 0x81490D070001:
			case 0x81490D070002:
				if (pos != end)	config_ipt_.set_param(*++pos, param);
				break;
			case 0x814827320601:	//	WAIT_TO_RECONNECT
			case 0x814831320201:	//	TCP_CONNECT_RETRIES
			case 0x0080800003FF:	//	use SSL
				config_ipt_.set_param(*pos, param);
				break;
			default:
				CYNG_LOG_ERROR(logger_, "sml.set.proc.parameter.request <_81490d0700ff> - unknown OBIS code "
					<< to_string(*pos)
					<< " / "
					<< cyng::io::to_hex(pos->to_buffer()));
				break;
			}
		}

		void set_proc_parameter::storage_time_shift(obis_path_t::const_iterator
			, obis_path_t::const_iterator
			, std::string trx
			, cyng::buffer_t srv_id
			, std::string user
			, std::string pwd
			, cyng::param_t	param)
		{
			CYNG_LOG_DEBUG(logger_, "sml.set.proc.parameter.request <0080800000FF> - storage time shift "
				<< param.first
				<< " = "
				<< cyng::io::to_str(param.second));

			auto const sts(cyng::numeric_cast<std::int32_t>(param.second, 0u));
			cache_.set_cfg(sml::OBIS_STORAGE_TIME_SHIFT.to_str(), sts);
		}

		void set_proc_parameter::class_mbus(obis&& code
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
	}	//	sml
}

