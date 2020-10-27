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
#include <smf/sml/intrinsics/obis_factory.hpp>
#include <smf/sml/parser/obis_parser.h>
#include <smf/sml/obis_io.h>

#include <cyng/numeric_cast.hpp>
#include <cyng/buffer_cast.h>
#include <cyng/set_cast.h>

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

		cyng::tuple_t config_ipt::get_proc_params(std::string trx, cyng::buffer_t srv_id) const
		{
			//	81 81 11 06 FF FF
			auto msg = sml_gen_.empty_get_proc_param(trx, srv_id, node::sml::OBIS_ROOT_IPT_PARAM);

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
					node::sml::merge_msg(msg, {
						node::sml::OBIS_ROOT_IPT_PARAM,
						node::sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, nr),
						node::sml::make_obis(node::sml::OBIS_TARGET_IP_ADDRESS, nr)
						}, node::sml::make_value(rec.host_));	//	ip address of IP-T master 

					//
					//	target port
					//
					std::uint16_t const port = static_cast<std::uint16_t>(std::stoul(rec.service_));
					node::sml::merge_msg(msg, {
						node::sml::OBIS_ROOT_IPT_PARAM,
						node::sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, nr),
						node::sml::make_obis(node::sml::OBIS_TARGET_PORT, nr)
						}, node::sml::make_value(port));

					//
					//	source port
					//	0 == free choice
					//
					node::sml::merge_msg(msg, {
						node::sml::OBIS_ROOT_IPT_PARAM,
						node::sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, nr),
						node::sml::make_obis(node::sml::OBIS_SOURCE_PORT, nr)
						}, node::sml::make_value(0u));

					//
					//	user
					//
					node::sml::merge_msg(msg, {
						node::sml::OBIS_ROOT_IPT_PARAM,
						node::sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, nr),
						node::sml::make_obis(node::sml::OBIS_IPT_ACCOUNT, nr)
						}, node::sml::make_value(rec.account_));

					//
					//	password
					//
					node::sml::merge_msg(msg, {
						node::sml::OBIS_ROOT_IPT_PARAM,
						node::sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, nr),
						node::sml::make_obis(node::sml::OBIS_IPT_PASSWORD, nr)
						}, node::sml::make_value(rec.pwd_));

					//
					//	scrambled
					//
					//node::sml::merge_msg(msg, {
					//	node::sml::OBIS_ROOT_IPT_PARAM,
					//	node::sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, nr),
					//	node::sml::make_obis(node::sml::OBIS_IPT_SCRAMBLED, nr)
					//	}, node::sml::make_value(rec.scrambled_));

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
			node::sml::merge_msg(msg, {
				node::sml::OBIS_ROOT_IPT_PARAM,
				node::sml::OBIS_TCP_WAIT_TO_RECONNECT
				}, node::sml::make_value(ipt.get_ipt_tcp_wait_to_reconnect().count()));

			//
			//	repetitions
			//	81 48 31 32 02 01 TCP_CONNECT_RETRIES
			//
			node::sml::merge_msg(msg, {
				node::sml::OBIS_ROOT_IPT_PARAM,
				node::sml::OBIS_TCP_CONNECT_RETRIES
				}, node::sml::make_value(ipt.get_ipt_tcp_connect_retries()));

			//
			//	SSL
			//
			node::sml::merge_msg(msg, {
				node::sml::OBIS_ROOT_IPT_PARAM,
				node::sml::OBIS_HAS_SSL_CONFIG
				}, node::sml::make_value(ipt.has_ipt_ssl()));

			//
			//	certificates (none)
			//
			node::sml::merge_msg(msg, {
				node::sml::OBIS_ROOT_IPT_PARAM,
				node::sml::OBIS_SSL_CERTIFICATES
				}, node::sml::make_value());

			//
			//	append to message queue
			//
			return msg;
		}

		void config_ipt::set_param(obis_path_t const& path, cyng::param_t const& values)
		{
			//	[3024699-2,81490D0700FF,0500155D40A430,operator,operator,
			//	("81490D0700FF":%(("81490D070002":%(("814917070002":127.0.0.1),("81491A070002":68ef),("8149633C0102":segw),("8149633C0202":!BMyUve!),("idx":2))),("idx":ff)))]

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
						set_param(p, param);
					}
				}
			}
			else {
				if (path.back().is_matching(0x81, 0x49, 0x17, 0x07, 0x00).second) {
					//	IP address
					if (values.second.get_class().tag() == cyng::TC_STRING) {
						cache_.set_cfg(build_cfg_key(path), cyng::io::to_str(values.second));
					}
					else {
						auto const tmp = cyng::to_buffer(values.second);
						cache_.set_cfg(build_cfg_key(path), cyng::io::to_ascii(tmp));
					}
				}
				else if (path.back().is_matching(0x81, 0x49, 0x1A, 0x07, 0x00).second) {
					//	target port
					cache_.set_cfg(build_cfg_key(path), std::to_string(cyng::numeric_cast<std::uint16_t>(values.second, 26862)));
				}
				else if (path.back().is_matching(0x81, 0x49, 0x63, 0x3C, 0x01).second) {
					//	account
					cache_.set_cfg(build_cfg_key(path), cyng::io::to_str(values.second));
				}
				else if (code.first.is_matching(0x81, 0x49, 0x63, 0x3C, 0x02).second) {
					//	pwd
					cache_.set_cfg(build_cfg_key(path), cyng::io::to_str(values.second));
				}
				else if (path.back().is_matching(0x81, 0x49, 0x63, 0x3C, 0x03).second) {
					//	scrambled
					cache_.set_cfg(build_cfg_key(path), values.second);
				}
				else if (path.back() == sml::OBIS_TCP_WAIT_TO_RECONNECT) {
					//	WAIT_TO_RECONNECT
					cache_.set_cfg(build_cfg_key(path), cyng::numeric_cast<std::uint8_t>(values.second, 1u));
				}
				else if (path.back() == sml::OBIS_TCP_CONNECT_RETRIES) {
					//	TCP_CONNECT_RETRIES
					cache_.set_cfg(build_cfg_key(path), cyng::numeric_cast<std::uint32_t>(values.second, 1u));
				}
				else if (path.back().to_uint64() == 0x0080800003FF) {
					//	use SSL
					cache_.set_cfg(build_cfg_key({ sml::OBIS_ROOT_IPT_PARAM
						, sml::OBIS_HAS_SSL_CONFIG }), values.second);
				}
			}
		}
	}	//	ipt
}

