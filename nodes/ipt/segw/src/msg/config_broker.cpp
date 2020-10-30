/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "config_broker.h"
#include "../segw.h"
#include "../cache.h"
#include "../cfg_wmbus.h"
#include "../cfg_rs485.h"

#include <smf/sml/protocol/generator.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/intrinsics/obis_factory.hpp>
#include <smf/sml/parser/obis_parser.h>
#include <smf/sml/obis_io.h>
#include <smf/serial/parity.h>
#include <smf/serial/flow_control.h>
#include <smf/serial/stopbits.h>
#include <cyng/set_cast.h>

#include <cyng/numeric_cast.hpp>
#include <cyng/buffer_cast.h>
#include <cyng/json.h>

namespace node
{
	namespace sml
	{
		config_broker::config_broker(cyng::logging::log_ptr logger
			, res_generator& sml_gen
			, cache& cfg)
		: logger_(logger)
			, sml_gen_(sml_gen)
			, cache_(cfg)
			, cfg_(cfg)
		{}

		cyng::tuple_t config_broker::get_proc_params(std::string trx, cyng::buffer_t srv_id) const
		{
			auto msg = sml_gen_.empty_get_proc_param(trx, srv_id, OBIS_ROOT_BROKER);

			//CYNG_LOG_DEBUG(logger_, cyng::io::to_type(msg));

			//
			//	wireless
			//
			auto const lmn_wireless = cfg_.get_server(source::WIRELESS_LMN);
			if (!lmn_wireless.empty()) {

				merge_msg(msg, { OBIS_ROOT_BROKER, make_obis(OBIS_ROOT_BROKER, 1u), sml::OBIS_BROKER_LOGIN }, make_value(cfg_.is_login_required(source::WIRELESS_LMN)));
				merge_msg(msg, { OBIS_ROOT_BROKER, make_obis(OBIS_ROOT_BROKER, 1u), sml::OBIS_SERIAL_NAME }, make_value(cfg_.get_port_name(source::WIRELESS_LMN)));

				std::uint8_t idx{ 1 };
				for (auto const& lmn : lmn_wireless) {
					merge_msg(msg, { OBIS_ROOT_BROKER, make_obis(OBIS_ROOT_BROKER, 1u),  make_obis(OBIS_BROKER_SERVER, idx) }, make_value(lmn.get_address()));
					merge_msg(msg, { OBIS_ROOT_BROKER, make_obis(OBIS_ROOT_BROKER, 1u),  make_obis(OBIS_BROKER_SERVICE, idx) }, make_value(lmn.get_port()));
					merge_msg(msg, { OBIS_ROOT_BROKER, make_obis(OBIS_ROOT_BROKER, 1u),  make_obis(OBIS_BROKER_USER, idx) }, make_value(lmn.get_account()));
					merge_msg(msg, { OBIS_ROOT_BROKER, make_obis(OBIS_ROOT_BROKER, 1u),  make_obis(OBIS_BROKER_PWD, idx) }, make_value(lmn.get_pwd()));
					++idx;
				}
			}

			//
			//	wired (rs485)
			//	reset index
			//
			auto const lmn_wired = cfg_.get_server(source::WIRED_LMN);	//	rs485
			if (!lmn_wired.empty()) {

				std::uint8_t idx{ 1 };
				merge_msg(msg, { OBIS_ROOT_BROKER, make_obis(OBIS_ROOT_BROKER, 2u), sml::OBIS_BROKER_LOGIN }, make_value(cfg_.is_login_required(source::WIRED_LMN)));
				merge_msg(msg, { OBIS_ROOT_BROKER, make_obis(OBIS_ROOT_BROKER, 2u), sml::OBIS_SERIAL_NAME }, make_value(cfg_.get_port_name(source::WIRED_LMN)));

				for (auto const& lmn : lmn_wired) {
					merge_msg(msg, { OBIS_ROOT_BROKER, make_obis(OBIS_ROOT_BROKER, 2u),  make_obis(OBIS_BROKER_SERVER, idx) }, make_value(lmn.get_address()));
					merge_msg(msg, { OBIS_ROOT_BROKER, make_obis(OBIS_ROOT_BROKER, 2u),  make_obis(OBIS_BROKER_SERVICE, idx) }, make_value(lmn.get_port()));
					merge_msg(msg, { OBIS_ROOT_BROKER, make_obis(OBIS_ROOT_BROKER, 2u),  make_obis(OBIS_BROKER_USER, idx) }, make_value(lmn.get_account()));
					merge_msg(msg, { OBIS_ROOT_BROKER, make_obis(OBIS_ROOT_BROKER, 2u),  make_obis(OBIS_BROKER_PWD, idx) }, make_value(lmn.get_pwd()));
					++idx;
				}
			}

			CYNG_LOG_DEBUG(logger_, cyng::io::to_type(msg));

			//
			//	append to message queue
			//
			return msg;

		}

		cyng::tuple_t config_broker::get_proc_params_port(std::string trx, cyng::buffer_t srv_id) const
		{
			auto msg = sml_gen_.empty_get_proc_param(trx, srv_id, OBIS_ROOT_SERIAL);

			cfg_wmbus wmbus(cache_);
			cfg_rs485 rs485(cache_);

			merge_msg(msg, { OBIS_ROOT_SERIAL, make_obis(OBIS_ROOT_SERIAL, wmbus.port_idx), OBIS_SERIAL_NAME }, make_value(wmbus.get_port()));
			merge_msg(msg, { OBIS_ROOT_SERIAL, make_obis(OBIS_ROOT_SERIAL, wmbus.port_idx), make_obis(OBIS_SERIAL_DATABITS, wmbus.port_idx) }, make_value(wmbus.get_databits().value()));
			merge_msg(msg, { OBIS_ROOT_SERIAL, make_obis(OBIS_ROOT_SERIAL, wmbus.port_idx), make_obis(OBIS_SERIAL_PARITY, wmbus.port_idx) }, make_value(node::serial::to_str(wmbus.get_parity())));
			merge_msg(msg, { OBIS_ROOT_SERIAL, make_obis(OBIS_ROOT_SERIAL, wmbus.port_idx), make_obis(OBIS_SERIAL_FLOW_CONTROL, wmbus.port_idx) }, make_value(node::serial::to_str(wmbus.get_flow_control())));
			merge_msg(msg, { OBIS_ROOT_SERIAL, make_obis(OBIS_ROOT_SERIAL, wmbus.port_idx), make_obis(OBIS_SERIAL_STOPBITS, wmbus.port_idx) }, make_value(node::serial::to_str(wmbus.get_stopbits())));
			merge_msg(msg, { OBIS_ROOT_SERIAL, make_obis(OBIS_ROOT_SERIAL, wmbus.port_idx), make_obis(OBIS_SERIAL_SPEED, wmbus.port_idx) }, make_value(wmbus.get_baud_rate().value()));

			merge_msg(msg, { OBIS_ROOT_SERIAL, make_obis(OBIS_ROOT_SERIAL, wmbus.port_idx), make_obis(OBIS_SERIAL_NAME, rs485.port_idx) }, make_value(rs485.get_port()));
			merge_msg(msg, { OBIS_ROOT_SERIAL, make_obis(OBIS_ROOT_SERIAL, wmbus.port_idx), make_obis(OBIS_SERIAL_DATABITS, rs485.port_idx) }, make_value(rs485.get_databits().value()));
			merge_msg(msg, { OBIS_ROOT_SERIAL, make_obis(OBIS_ROOT_SERIAL, wmbus.port_idx), make_obis(OBIS_SERIAL_PARITY, rs485.port_idx) }, make_value(node::serial::to_str(rs485.get_parity())));
			merge_msg(msg, { OBIS_ROOT_SERIAL, make_obis(OBIS_ROOT_SERIAL, wmbus.port_idx), make_obis(OBIS_SERIAL_FLOW_CONTROL, rs485.port_idx) }, make_value(node::serial::to_str(rs485.get_flow_control())));
			merge_msg(msg, { OBIS_ROOT_SERIAL, make_obis(OBIS_ROOT_SERIAL, wmbus.port_idx), make_obis(OBIS_SERIAL_STOPBITS, rs485.port_idx) }, make_value(node::serial::to_str(rs485.get_stopbits())));
			merge_msg(msg, { OBIS_ROOT_SERIAL, make_obis(OBIS_ROOT_SERIAL, wmbus.port_idx), make_obis(OBIS_SERIAL_SPEED, rs485.port_idx) }, make_value(rs485.get_baud_rate().value()));

			//
			//	append to message queue
			//
			return msg;
		}

		void config_broker::set_proc_params(obis_path_t const& path, cyng::buffer_t srv_id, cyng::param_t const& values)
		{
			CYNG_LOG_DEBUG(logger_, sml::transform_to_str(path, true, ':') << " = " << cyng::io::to_type(values.second));

			BOOST_ASSERT(!path.empty());
			auto const code = sml::parse_obis(values.first);

			BOOST_ASSERT(code.second);
			BOOST_ASSERT(code.first == path.back());

			if (values.second.get_class().tag() == cyng::TC_PARAM_MAP) {
				auto const pm = cyng::to_param_map(values.second);
				for (auto const& param : pm) {
					auto const c = sml::parse_obis(param.first);
					if (c.second) {
						obis_path_t p(path);
						p.push_back(c.first);
						//CYNG_LOG_TRACE(logger_, "build path <" << sml::transform_to_str(p, true, ':') << ">");
						set_proc_params(p, srv_id, param);
					}
				}
			}
			else {

				if (path.back().is_matching_5(OBIS_BROKER_LOGIN).second) {
					//	BROKER_LOGIN
					cache_.set_cfg(build_cfg_key(path), values.second);
				}
				else if (path.back().is_matching_5(OBIS_BROKER_SERVER).second) {
					//	BROKER_SERVER
					cache_.set_cfg(build_cfg_key(path), cyng::io::to_str(values.second));
				}
				else if (path.back().is_matching_5(OBIS_BROKER_SERVICE).second) {
					//	BROKER_SERVICE (IP port)
					cache_.set_cfg(build_cfg_key(path), cyng::numeric_cast<std::uint16_t>(values.second, 12001u));
				}
				else if (path.back().is_matching_5(OBIS_BROKER_USER).second) {
					//	BROKER_USER
					cache_.set_cfg(build_cfg_key(path), cyng::io::to_str(values.second));
				}
				else if (path.back().is_matching_5(OBIS_BROKER_PWD).second) {
					//	BROKER_PWD
					cache_.set_cfg(build_cfg_key(path), cyng::io::to_str(values.second));
				}
				else {
					CYNG_LOG_WARNING(logger_, "unknown broker value: " << sml::transform_to_str(path, true, ':') << " = " << cyng::io::to_type(values.second));
				}
			}

		}

		void config_broker::set_proc_params_port(obis_path_t const& path, cyng::buffer_t srv_id, cyng::param_t const& values) const
		{
			if (path.back().is_matching_5(OBIS_SERIAL_DATABITS).second) {
				//	SERIAL_DATABITS
				auto const val = cyng::numeric_cast<std::uint32_t>(values.second, 8u);
				cache_.set_cfg<std::uint32_t>(build_cfg_key(path), val);
			}
			else if (path.back().is_matching_5(OBIS_SERIAL_PARITY).second) {
				//	SERIAL_PARITY
				cache_.set_cfg(build_cfg_key(path), values.second);
			}
			else if (path.back().is_matching_5(OBIS_SERIAL_FLOW_CONTROL).second) {
				//	SERIAL_FLOW_CONTROL
				cache_.set_cfg(build_cfg_key(path), values.second);
			}
			else if (path.back().is_matching_5(OBIS_SERIAL_STOPBITS).second) {
				//	SERIAL_STOPBITS
				cache_.set_cfg(build_cfg_key(path), values.second);
			}
			else if (path.back().is_matching_5(OBIS_SERIAL_SPEED).second) {
				//	SERIAL_SPEED
				auto const val = cyng::numeric_cast<std::uint32_t>(values.second, 8u);
				cache_.set_cfg<std::uint32_t>(build_cfg_key(path), val);
			}
			else {
				CYNG_LOG_WARNING(logger_, "unknown hardware port value: " << to_hex(path, ':') << " = " << cyng::io::to_type(values.second));

			}
		}

	}	//	sml
}

