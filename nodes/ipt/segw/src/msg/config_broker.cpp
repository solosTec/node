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

		void config_broker::get_proc_params(std::string trx, cyng::buffer_t srv_id) const
		{
			auto msg = sml_gen_.empty_get_proc_param_response(trx, srv_id, OBIS_ROOT_BROKER);

			//CYNG_LOG_DEBUG(logger_, cyng::io::to_type(msg));

			//
			//	wireless
			//
			auto const lmn_wireless = cfg_.get_broker(cfg_broker::source::WIRELESS_LMN);
			if (!lmn_wireless.empty()) {

				append_get_proc_response(msg, { OBIS_ROOT_BROKER, make_obis(OBIS_ROOT_BROKER, 1u), sml::OBIS_BROKER_LOGIN }, make_value(cfg_.is_login_required(cfg_broker::source::WIRELESS_LMN)));
				append_get_proc_response(msg, { OBIS_ROOT_BROKER, make_obis(OBIS_ROOT_BROKER, 1u), sml::OBIS_HARDWARE_PORT_NAME }, make_value(cfg_.get_port_name(cfg_broker::source::WIRELESS_LMN)));

				std::uint8_t idx{ 1 };
				for (auto const& lmn : lmn_wireless) {
					append_get_proc_response(msg, { OBIS_ROOT_BROKER, make_obis(OBIS_ROOT_BROKER, 1u),  make_obis(OBIS_BROKER_SERVER, idx) }, make_value(lmn.get_address()));
					append_get_proc_response(msg, { OBIS_ROOT_BROKER, make_obis(OBIS_ROOT_BROKER, 1u),  make_obis(OBIS_BROKER_SERVICE, idx) }, make_value(lmn.get_port()));
					append_get_proc_response(msg, { OBIS_ROOT_BROKER, make_obis(OBIS_ROOT_BROKER, 1u),  make_obis(OBIS_BROKER_USER, idx) }, make_value(lmn.get_account()));
					append_get_proc_response(msg, { OBIS_ROOT_BROKER, make_obis(OBIS_ROOT_BROKER, 1u),  make_obis(OBIS_BROKER_PWD, idx) }, make_value(lmn.get_pwd()));
					++idx;
				}
			}

			//
			//	wired (rs485)
			//	reset index
			//
			auto const lmn_wired = cfg_.get_broker(cfg_broker::source::WIRED_LMN);	//	rs485
			if (!lmn_wired.empty()) {

				std::uint8_t idx{ 1 };
				append_get_proc_response(msg, { OBIS_ROOT_BROKER, make_obis(OBIS_ROOT_BROKER, 2u), sml::OBIS_BROKER_LOGIN }, make_value(cfg_.is_login_required(cfg_broker::source::WIRED_LMN)));
				append_get_proc_response(msg, { OBIS_ROOT_BROKER, make_obis(OBIS_ROOT_BROKER, 2u), sml::OBIS_HARDWARE_PORT_NAME }, make_value(cfg_.get_port_name(cfg_broker::source::WIRED_LMN)));

				for (auto const& lmn : lmn_wired) {
					append_get_proc_response(msg, { OBIS_ROOT_BROKER, make_obis(OBIS_ROOT_BROKER, 2u),  make_obis(OBIS_BROKER_SERVER, idx) }, make_value(lmn.get_address()));
					append_get_proc_response(msg, { OBIS_ROOT_BROKER, make_obis(OBIS_ROOT_BROKER, 2u),  make_obis(OBIS_BROKER_SERVICE, idx) }, make_value(lmn.get_port()));
					append_get_proc_response(msg, { OBIS_ROOT_BROKER, make_obis(OBIS_ROOT_BROKER, 2u),  make_obis(OBIS_BROKER_USER, idx) }, make_value(lmn.get_account()));
					append_get_proc_response(msg, { OBIS_ROOT_BROKER, make_obis(OBIS_ROOT_BROKER, 2u),  make_obis(OBIS_BROKER_PWD, idx) }, make_value(lmn.get_pwd()));
					++idx;
				}
			}

			CYNG_LOG_DEBUG(logger_, cyng::io::to_type(msg));

			//
			//	append to message queue
			//
			sml_gen_.append(std::move(msg));

		}

		void config_broker::get_proc_params_port(std::string trx, cyng::buffer_t srv_id) const
		{
			auto msg = sml_gen_.empty_get_proc_param_response(trx, srv_id, OBIS_ROOT_HARDWARE_PORT);

			cfg_wmbus wmbus(cache_);
			cfg_rs485 rs485(cache_);

			append_get_proc_response(msg, { OBIS_ROOT_HARDWARE_PORT, make_obis(OBIS_HARDWARE_PORT_NAME, wmbus.port_idx) }, make_value(wmbus.get_port()));
			append_get_proc_response(msg, { OBIS_ROOT_HARDWARE_PORT, make_obis(OBIS_HARDWARE_PORT_DATABITS, wmbus.port_idx) }, make_value(wmbus.get_databits().value()));
			append_get_proc_response(msg, { OBIS_ROOT_HARDWARE_PORT, make_obis(OBIS_HARDWARE_PORT_PARITY, wmbus.port_idx) }, make_value(node::serial::to_str(wmbus.get_parity())));
			append_get_proc_response(msg, { OBIS_ROOT_HARDWARE_PORT, make_obis(OBIS_HARDWARE_PORT_FLOW_CONTROL, wmbus.port_idx) }, make_value(node::serial::to_str(wmbus.get_flow_control())));
			append_get_proc_response(msg, { OBIS_ROOT_HARDWARE_PORT, make_obis(OBIS_HARDWARE_PORT_STOPBITS, wmbus.port_idx) }, make_value(node::serial::to_str(wmbus.get_stopbits())));
			append_get_proc_response(msg, { OBIS_ROOT_HARDWARE_PORT, make_obis(OBIS_HARDWARE_PORT_SPEED, wmbus.port_idx) }, make_value(wmbus.get_baud_rate().value()));

			append_get_proc_response(msg, { OBIS_ROOT_HARDWARE_PORT, make_obis(OBIS_HARDWARE_PORT_NAME, rs485.port_idx) }, make_value(rs485.get_port()));
			append_get_proc_response(msg, { OBIS_ROOT_HARDWARE_PORT, make_obis(OBIS_HARDWARE_PORT_DATABITS, rs485.port_idx) }, make_value(rs485.get_databits().value()));
			append_get_proc_response(msg, { OBIS_ROOT_HARDWARE_PORT, make_obis(OBIS_HARDWARE_PORT_PARITY, rs485.port_idx) }, make_value(node::serial::to_str(rs485.get_parity())));
			append_get_proc_response(msg, { OBIS_ROOT_HARDWARE_PORT, make_obis(OBIS_HARDWARE_PORT_FLOW_CONTROL, rs485.port_idx) }, make_value(node::serial::to_str(rs485.get_flow_control())));
			append_get_proc_response(msg, { OBIS_ROOT_HARDWARE_PORT, make_obis(OBIS_HARDWARE_PORT_STOPBITS, rs485.port_idx) }, make_value(node::serial::to_str(rs485.get_stopbits())));
			append_get_proc_response(msg, { OBIS_ROOT_HARDWARE_PORT, make_obis(OBIS_HARDWARE_PORT_SPEED, rs485.port_idx) }, make_value(rs485.get_baud_rate().value()));

			//
			//	append to message queue
			//
			sml_gen_.append(std::move(msg));
		}

		void config_broker::set_proc_params(obis_path_t const& path, cyng::buffer_t srv_id, cyng::object obj)
		{
			CYNG_LOG_DEBUG(logger_, to_hex(path, ':') << " = " << cyng::io::to_type(obj));

			if (path.back().is_matching_5(OBIS_BROKER_LOGIN).second) {
				//	BROKER_LOGIN
				cache_.set_cfg(build_cfg_key(path), obj);
			}
			else if (path.back().is_matching_5(OBIS_BROKER_SERVER).second) {
				//	BROKER_SERVER
				cache_.set_cfg(build_cfg_key(path), cyng::io::to_str(obj));
			}
			if (path.back().is_matching_5(OBIS_BROKER_SERVICE).second) {
				//	BROKER_SERVICE (IP port)
				cache_.set_cfg(build_cfg_key(path), cyng::numeric_cast<std::uint16_t>(obj, 12001u));
			}
			else if (path.back().is_matching_5(OBIS_BROKER_USER).second) {
				//	BROKER_USER
				cache_.set_cfg(build_cfg_key(path), cyng::io::to_str(obj));
			}
			else if (path.back().is_matching_5(OBIS_BROKER_PWD).second) {
				//	BROKER_PWD
				cache_.set_cfg(build_cfg_key(path), cyng::io::to_str(obj));
			}
			else {
				CYNG_LOG_WARNING(logger_, "unknown broker value: " << to_hex(path, ':') << " = " << cyng::io::to_type(obj));
			}

		}

		void config_broker::set_proc_params_port(obis_path_t const& path, cyng::buffer_t srv_id, cyng::object obj) const
		{
			if (path.back().is_matching_5(OBIS_HARDWARE_PORT_DATABITS).second) {
				//	HARDWARE_PORT_DATABITS
				auto const val = cyng::numeric_cast<std::uint32_t>(obj, 8u);
				cache_.set_cfg<std::uint32_t>(build_cfg_key(path), val);
			}
			else if (path.back().is_matching_5(OBIS_HARDWARE_PORT_PARITY).second) {
				//	HARDWARE_PORT_PARITY
				cache_.set_cfg(build_cfg_key(path), obj);
			}
			else if (path.back().is_matching_5(OBIS_HARDWARE_PORT_FLOW_CONTROL).second) {
				//	HARDWARE_PORT_FLOW_CONTROL
				cache_.set_cfg(build_cfg_key(path), obj);
			}
			else if (path.back().is_matching_5(OBIS_HARDWARE_PORT_STOPBITS).second) {
				//	HARDWARE_PORT_STOPBITS
				cache_.set_cfg(build_cfg_key(path), obj);
			}
			else if (path.back().is_matching_5(OBIS_HARDWARE_PORT_SPEED).second) {
				//	HARDWARE_PORT_SPEED
				auto const val = cyng::numeric_cast<std::uint32_t>(obj, 8u);
				cache_.set_cfg<std::uint32_t>(build_cfg_key(path), val);
			}
			else {
				CYNG_LOG_WARNING(logger_, "unknown hardware port value: " << to_hex(path, ':') << " = " << cyng::io::to_type(obj));

			}
		}

	}	//	sml
}

