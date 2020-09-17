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
			  
			append_get_proc_response(msg, { OBIS_ROOT_BROKER, make_obis(0x90, 0x00, 0x00, 0x00, 0x01, 1u) }, make_value(cfg_.is_login_required(cfg_broker::source::WIRELESS_LMN)));
			append_get_proc_response(msg, { OBIS_ROOT_BROKER, make_obis(0x90, 0x00, 0x00, 0x00, 0x02, 1u) }, make_value(cfg_.get_port_name(cfg_broker::source::WIRELESS_LMN)));

			std::uint8_t idx{ 1 };
			for (auto const& lmn : lmn_wireless) {
				append_get_proc_response(msg, { OBIS_ROOT_BROKER, make_obis(0x90, 0x00, 0x00, 0x00, 0x03, 1u),  make_obis(0x90, 0x00, 0x00, 0x00, 0x03, idx) }, make_value(lmn.get_address()));
				append_get_proc_response(msg, { OBIS_ROOT_BROKER, make_obis(0x90, 0x00, 0x00, 0x00, 0x03, 1u),  make_obis(0x90, 0x00, 0x00, 0x00, 0x04, idx) }, make_value(lmn.get_port()));
				append_get_proc_response(msg, { OBIS_ROOT_BROKER, make_obis(0x90, 0x00, 0x00, 0x00, 0x03, 1u),  make_obis(0x90, 0x00, 0x00, 0x00, 0x05, idx) }, make_value(lmn.get_account()));
				append_get_proc_response(msg, { OBIS_ROOT_BROKER, make_obis(0x90, 0x00, 0x00, 0x00, 0x03, 1u),  make_obis(0x90, 0x00, 0x00, 0x00, 0x06, idx) }, make_value(lmn.get_pwd()));
				++idx;
			}

			//
			//	wired (rs485)
			//	reset index
			//
			auto const lmn_wired = cfg_.get_broker(cfg_broker::source::WIRED_LMN);	//	rs485
			idx = 1;

			append_get_proc_response(msg, { OBIS_ROOT_BROKER, make_obis(0x90, 0x00, 0x00, 0x00, 0x01, 2u) }, make_value(cfg_.is_login_required(cfg_broker::source::WIRED_LMN)));
			append_get_proc_response(msg, { OBIS_ROOT_BROKER, make_obis(0x90, 0x00, 0x00, 0x00, 0x02, 2u) }, make_value(cfg_.get_port_name(cfg_broker::source::WIRED_LMN)));
			for (auto const& lmn : lmn_wired) {
				append_get_proc_response(msg, { OBIS_ROOT_BROKER, make_obis(0x90, 0x00, 0x00, 0x00, 0x03, 2u),  make_obis(0x90, 0x00, 0x00, 0x00, 0x03, idx) }, make_value(lmn.get_address()));
				append_get_proc_response(msg, { OBIS_ROOT_BROKER, make_obis(0x90, 0x00, 0x00, 0x00, 0x03, 2u),  make_obis(0x90, 0x00, 0x00, 0x00, 0x04, idx) }, make_value(lmn.get_port()));
				append_get_proc_response(msg, { OBIS_ROOT_BROKER, make_obis(0x90, 0x00, 0x00, 0x00, 0x03, 2u),  make_obis(0x90, 0x00, 0x00, 0x00, 0x05, idx) }, make_value(lmn.get_account()));
				append_get_proc_response(msg, { OBIS_ROOT_BROKER, make_obis(0x90, 0x00, 0x00, 0x00, 0x03, 2u),  make_obis(0x90, 0x00, 0x00, 0x00, 0x06, idx) }, make_value(lmn.get_pwd()));
				++idx;
			}

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

			append_get_proc_response(msg, { OBIS_ROOT_HARDWARE_PORT, make_obis(0x91, 0x00, 0x00, 0x00, 0x01, wmbus.port_idx) }, make_value(wmbus.get_port()));
			append_get_proc_response(msg, { OBIS_ROOT_HARDWARE_PORT, make_obis(0x91, 0x00, 0x00, 0x00, 0x02, wmbus.port_idx) }, make_value(wmbus.get_databits().value()));
			append_get_proc_response(msg, { OBIS_ROOT_HARDWARE_PORT, make_obis(0x91, 0x00, 0x00, 0x00, 0x03, wmbus.port_idx) }, make_value(node::serial::to_str(wmbus.get_parity())));
			append_get_proc_response(msg, { OBIS_ROOT_HARDWARE_PORT, make_obis(0x91, 0x00, 0x00, 0x00, 0x04, wmbus.port_idx) }, make_value(node::serial::to_str(wmbus.get_flow_control())));
			append_get_proc_response(msg, { OBIS_ROOT_HARDWARE_PORT, make_obis(0x91, 0x00, 0x00, 0x00, 0x05, wmbus.port_idx) }, make_value(node::serial::to_str(wmbus.get_stopbits())));
			append_get_proc_response(msg, { OBIS_ROOT_HARDWARE_PORT, make_obis(0x91, 0x00, 0x00, 0x00, 0x06, wmbus.port_idx) }, make_value(wmbus.get_baud_rate().value()));

			append_get_proc_response(msg, { OBIS_ROOT_HARDWARE_PORT, make_obis(0x91, 0x00, 0x00, 0x00, 0x01, rs485.port_idx) }, make_value(rs485.get_port()));
			append_get_proc_response(msg, { OBIS_ROOT_HARDWARE_PORT, make_obis(0x91, 0x00, 0x00, 0x00, 0x02, rs485.port_idx) }, make_value(rs485.get_databits().value()));
			append_get_proc_response(msg, { OBIS_ROOT_HARDWARE_PORT, make_obis(0x91, 0x00, 0x00, 0x00, 0x03, rs485.port_idx) }, make_value(node::serial::to_str(rs485.get_parity())));
			append_get_proc_response(msg, { OBIS_ROOT_HARDWARE_PORT, make_obis(0x91, 0x00, 0x00, 0x00, 0x04, rs485.port_idx) }, make_value(node::serial::to_str(rs485.get_flow_control())));
			append_get_proc_response(msg, { OBIS_ROOT_HARDWARE_PORT, make_obis(0x91, 0x00, 0x00, 0x00, 0x05, rs485.port_idx) }, make_value(node::serial::to_str(rs485.get_stopbits())));
			append_get_proc_response(msg, { OBIS_ROOT_HARDWARE_PORT, make_obis(0x91, 0x00, 0x00, 0x00, 0x06, rs485.port_idx) }, make_value(rs485.get_baud_rate().value()));

			//
			//	append to message queue
			//
			sml_gen_.append(std::move(msg));
		}

		void config_broker::set_proc_params(obis_path_t const& path, cyng::buffer_t srv_id, cyng::object obj)
		{
			CYNG_LOG_DEBUG(logger_, to_hex(path, ':') << " = " << cyng::io::to_type(obj));

			if (path.back().is_matching(0x90, 0x00, 0x00, 0x00, 0x01).second) {
				//	login
				cache_.set_cfg(build_cfg_key(path), obj);
			}
			else if (path.back().is_matching(0x90, 0x00, 0x00, 0x00, 0x03).second) {
				//	address/host
				cache_.set_cfg(build_cfg_key(path), cyng::io::to_str(obj));
			}
			if (path.back().is_matching(0x90, 0x00, 0x00, 0x00, 0x04).second) {
				//	IP port
				cache_.set_cfg(build_cfg_key(path), cyng::numeric_cast<std::uint16_t>(obj, 12001u));
			}
			else if (path.back().is_matching(0x90, 0x00, 0x00, 0x00, 0x05).second) {
				//	account
				cache_.set_cfg(build_cfg_key(path), cyng::io::to_str(obj));
			}
			else if (path.back().is_matching(0x90, 0x00, 0x00, 0x00, 0x06).second) {
				//	pwd
				cache_.set_cfg(build_cfg_key(path), cyng::io::to_str(obj));
			}

		}

	}	//	sml
}

