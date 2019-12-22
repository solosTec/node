/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "config_ipt.h"
#include "../cache.h"
#include "../segw.h"
#include "../cfg_ipt.h"

#include <smf/sml/protocol/generator.h>
#include <smf/sml/obis_db.h>

#include <cyng/numeric_cast.hpp>

namespace node
{
	namespace ipt
	{
		config_ipt::config_ipt(cyng::logging::log_ptr logger
			, node::sml::res_generator& sml_gen
			, cache& cfg)
		: logger_(logger)
			, sml_gen_(sml_gen)
			, cache_(cfg)
		{}

		void config_ipt::get_proc_params(std::string trx, cyng::buffer_t srv_id) const
		{
			//	81 81 11 06 FF FF
			auto msg = sml_gen_.empty_get_proc_param_response(trx, srv_id, node::sml::OBIS_ROOT_IPT_PARAM);

			std::uint8_t nr{ 1 };

			//
			//	get IP-T configuration
			//
			cfg_ipt ipt(cache_);
			auto const cfg = ipt.get_ipt_redundancy();

			for (auto const& rec : cfg.config_) {
				try {

					//
					//	host
					//
					node::sml::append_get_proc_response(msg, {
						node::sml::OBIS_ROOT_IPT_PARAM,
						node::sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, nr),
						node::sml::make_obis(0x81, 0x49, 0x17, 0x07, 0x00, nr)
						}, node::sml::make_value(rec.host_));

					//
					//	target port
					//
					std::uint16_t const port = static_cast<std::uint16_t>(std::stoul(rec.service_));
					node::sml::append_get_proc_response(msg, {
						node::sml::OBIS_ROOT_IPT_PARAM,
						node::sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, nr),
						node::sml::make_obis(0x81, 0x49, 0x1A, 0x07, 0x00, nr)
						}, node::sml::make_value(port));

					//
					//	source port
					//	0 == free choice
					//
					node::sml::append_get_proc_response(msg, {
						node::sml::OBIS_ROOT_IPT_PARAM,
						node::sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, nr),
						node::sml::make_obis(0x81, 0x49, 0x19, 0x07, 0x00, nr)
						}, node::sml::make_value(0u));

					//
					//	user
					//
					node::sml::append_get_proc_response(msg, {
						node::sml::OBIS_ROOT_IPT_PARAM,
						node::sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, nr),
						node::sml::make_obis(0x81, 0x49, 0x63, 0x3C, 0x01, nr)
						}, node::sml::make_value(rec.account_));

					//
					//	password
					//
					node::sml::append_get_proc_response(msg, {
						node::sml::OBIS_ROOT_IPT_PARAM,
						node::sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, nr),
						node::sml::make_obis(0x81, 0x49, 0x63, 0x3C, 0x02, nr)
						}, node::sml::make_value(rec.pwd_));

					//
					//	update index
					//
					++nr;
				}
				catch (std::exception const& ex) {
					CYNG_LOG_ERROR(logger_, " config_ipt::get_proc_params <" << ex.what() << ">");
				}
			}

			//
			//	waiting time (Wartezeit)
			//	81 48 27 32 06 01 - TCP_WAIT_TO_RECONNECT
			//
			node::sml::append_get_proc_response(msg, {
				node::sml::OBIS_ROOT_IPT_PARAM,
				node::sml::OBIS_TCP_WAIT_TO_RECONNECT
				}, node::sml::make_value(ipt.get_ipt_tcp_wait_to_reconnect().count()));

			//
			//	repetitions
			//	81 48 31 32 02 01 TCP_CONNECT_RETRIES
			//
			node::sml::append_get_proc_response(msg, {
				node::sml::OBIS_ROOT_IPT_PARAM,
				node::sml::OBIS_TCP_CONNECT_RETRIES
				}, node::sml::make_value(ipt.get_ipt_tcp_connect_retries()));

			//
			//	SSL
			//
			node::sml::append_get_proc_response(msg, {
				node::sml::OBIS_ROOT_IPT_PARAM,
				node::sml::OBIS_HAS_SSL_CONFIG
				}, node::sml::make_value(ipt.has_ipt_ssl()));

			//
			//	certificates (none)
			//
			node::sml::append_get_proc_response(msg, {
				node::sml::OBIS_ROOT_IPT_PARAM,
				node::sml::OBIS_SSL_CERTIFICATES
				}, node::sml::make_value());

			//
			//	append to message queue
			//
			sml_gen_.append(std::move(msg));

		}

		void config_ipt::set_param(node::sml::obis code, cyng::param_t const& param)
		{
			switch (code.to_uint64()) {

			//
			//	reduncancy 
			//
			case 0x814917070001:
				if (param.second.get_class().tag() == cyng::TC_STRING) {
					cache_.set_cfg(build_cfg_key({ node::sml::OBIS_ROOT_IPT_PARAM
						, node::sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, 1)
						, code }), cyng::io::to_str(param.second));
				}
				else {
					cyng::buffer_t tmp;
					tmp = cyng::value_cast(param.second, tmp);
					cache_.set_cfg(build_cfg_key({ node::sml::OBIS_ROOT_IPT_PARAM
						, node::sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, 1)
						, code }), cyng::io::to_ascii(tmp));
				}
				break;

			case 0x81491A070001:	//	target port
				cache_.set_cfg(build_cfg_key({ sml::OBIS_ROOT_IPT_PARAM
					, sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, 1)
					, code }), std::to_string(cyng::numeric_cast<std::uint16_t>(param.second, 26862)));
				break;

			case 0x8149633C0101:	//	account
				cache_.set_cfg(build_cfg_key({ sml::OBIS_ROOT_IPT_PARAM
					, sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, 1)
					, code }), cyng::io::to_str(param.second));
				break;

			case 0x8149633C0201:	//	pwd
				cache_.set_cfg(build_cfg_key({ sml::OBIS_ROOT_IPT_PARAM
					, sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, 1)
					, code }), cyng::io::to_str(param.second));
				break;

			//
			//	reduncancy 2
			//
			case 0x814917070002:	//	host
				if (param.second.get_class().tag() == cyng::TC_STRING) {
					cache_.set_cfg(build_cfg_key({ node::sml::OBIS_ROOT_IPT_PARAM
						, node::sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, 2)
						, code }), cyng::io::to_str(param.second));
				}
				else {
					cyng::buffer_t tmp;
					tmp = cyng::value_cast(param.second, tmp);
					cache_.set_cfg(build_cfg_key({ node::sml::OBIS_ROOT_IPT_PARAM
						, node::sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, 2)
						, code }), cyng::io::to_ascii(tmp));
				}
				break;

			case 0x81491A070002:	//	target port
				cache_.set_cfg(build_cfg_key({ sml::OBIS_ROOT_IPT_PARAM
					, sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, 2)
					, code }), std::to_string(cyng::numeric_cast<std::uint16_t>(param.second, 26862)));
				break;

			case 0x8149633C0102:	//	account
				cache_.set_cfg(build_cfg_key({ sml::OBIS_ROOT_IPT_PARAM
					, sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, 2)
					, code }), cyng::io::to_str(param.second));
				break;

			case 0x8149633C0202:	//	pwd
				cache_.set_cfg(build_cfg_key({ sml::OBIS_ROOT_IPT_PARAM
					, sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, 2)
					, code }), cyng::io::to_str(param.second));
				break;

			//
			//	general values
			//
			case 0x814827320601:	//	WAIT_TO_RECONNECT
				cache_.set_cfg(build_cfg_key({ sml::OBIS_ROOT_IPT_PARAM
					, sml::OBIS_TCP_WAIT_TO_RECONNECT }), cyng::numeric_cast<std::uint8_t>(param.second, 1u));
				break;

			case 0x814831320201:	//	TCP_CONNECT_RETRIES
				cache_.set_cfg(build_cfg_key({ sml::OBIS_ROOT_IPT_PARAM
					, sml::OBIS_TCP_CONNECT_RETRIES }), cyng::numeric_cast<std::uint32_t>(param.second, 1u));
				break;

			case 0x0080800003FF:	//	use SSL
				cache_.set_cfg(build_cfg_key({ sml::OBIS_ROOT_IPT_PARAM
					, sml::OBIS_HAS_SSL_CONFIG }), param.second);
				break;

			default:
				break;
			}

		}


	}	//	ipt
}

