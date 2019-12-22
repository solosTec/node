/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "cfg_ipt.h"
#include <smf/mbus/defs.h>
#include <smf/sml/srv_id_io.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/protocol/message.h>

#include <cyng/tuple_cast.hpp>
#include <cyng/numeric_cast.hpp>
#include <cyng/io/serializer.h>
#include <cyng/io/hex_dump.hpp>
#include <cyng/vm/generator.h>

#include <boost/algorithm/string.hpp>

namespace node
{
	namespace sml
	{

		cfg_ipt::cfg_ipt(cyng::logging::log_ptr logger
			, res_generator& sml_gen
			, cyng::controller& vm
			, cyng::store::db& config_db
			, node::ipt::redundancy const& cfg)
		: logger_(logger)
			, sml_gen_(sml_gen)
			, config_db_(config_db)
			, cfg_ipt_(cfg)
			, wait_time_(12)	//	minutes
			, repetitions_(120)	//	counter
			, ssl_(false)
		{}

		void cfg_ipt::get_proc_ipt_params(std::string trx, cyng::buffer_t srv_id) const
		{

			//	81 81 11 06 FF FF
			auto msg = sml_gen_.empty_get_proc_param_response(trx, srv_id, OBIS_ROOT_IPT_PARAM);

			std::uint8_t nr{ 1 };
			for (auto const& rec : cfg_ipt_.config_) {
				try {


					//
					//	host
					//
					append_get_proc_response(msg, {
						OBIS_ROOT_IPT_PARAM,
						make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, nr),
						make_obis(0x81, 0x49, 0x17, 0x07, 0x00, nr)
						}, make_value(rec.host_));

					//
					//	target port
					//
					std::uint16_t const port = static_cast<std::uint16_t>(std::stoul(rec.service_));
					append_get_proc_response(msg, {
						OBIS_ROOT_IPT_PARAM,
						make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, nr),
						make_obis(0x81, 0x49, 0x1A, 0x07, 0x00, nr)
						}, make_value(port));

					//
					//	source port
					//	0 == free choice
					//
					append_get_proc_response(msg, {
						OBIS_ROOT_IPT_PARAM,
						make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, nr),
						make_obis(0x81, 0x49, 0x19, 0x07, 0x00, nr)
						}, make_value(0u));

					//
					//	user
					//
					append_get_proc_response(msg, {
						OBIS_ROOT_IPT_PARAM,
						make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, nr),
						make_obis(0x81, 0x49, 0x63, 0x3C, 0x01, nr)
						}, make_value(rec.account_));

					//
					//	password
					//
					append_get_proc_response(msg, {
						OBIS_ROOT_IPT_PARAM,
						make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, nr),
						make_obis(0x81, 0x49, 0x63, 0x3C, 0x02, nr)
						}, make_value(rec.pwd_));

					//
					//	update index
					//
					++nr;
				}
				catch (std::exception const& ex) {
					CYNG_LOG_ERROR(logger_, " get_proc_ipt_params <" << ex.what() << ">");
				}
			}

			//
			//	waiting time (Wartezeit)
			//	81 48 27 32 06 01 - TCP_WAIT_TO_RECONNECT
			//
			append_get_proc_response(msg, {
				OBIS_ROOT_IPT_PARAM,
				OBIS_TCP_WAIT_TO_RECONNECT
				}, make_value(wait_time_));

			//
			//	repetitions
			//	81 48 31 32 02 01 TCP_CONNECT_RETRIES
			//
			append_get_proc_response(msg, {
				OBIS_ROOT_IPT_PARAM,
				OBIS_TCP_CONNECT_RETRIES
				}, make_value(repetitions_));

			//
			//	SSL
			//
			append_get_proc_response(msg, {
				OBIS_ROOT_IPT_PARAM,
				OBIS_CODE(00, 80, 80, 00, 03, FF)
				}, make_value(ssl_));
			
			//
			//	certificates (none)
			//
			append_get_proc_response(msg, {
				OBIS_ROOT_IPT_PARAM,
				OBIS_CODE(00, 80, 80, 00, 04, FF)
				}, make_value());

			//
			//	append to message queue
			//
			sml_gen_.append(std::move(msg));
		}

		void cfg_ipt::set_param(obis code, cyng::param_t const& param)
		{
			switch (code.to_uint64()) {
			case 0x814917070001:
				if (param.second.get_class().tag() == cyng::TC_STRING) {
					const_cast<std::string&>(cfg_ipt_.config_.at(0).host_) = cyng::io::to_str(param.second);
				}
				else {
					cyng::buffer_t tmp;
					tmp = cyng::value_cast(param.second, tmp);
					const_cast<std::string&>(cfg_ipt_.config_.at(0).host_) = cyng::io::to_ascii(tmp);
				}
			break;
			case 0x81491A070001:	//	target port
				const_cast<std::string&>(cfg_ipt_.config_.at(0).service_) = std::to_string(cyng::numeric_cast<std::uint16_t>(param.second, 26862));
				break;
			case 0x8149633C0101:	//	account
				const_cast<std::string&>(cfg_ipt_.config_.at(0).account_) = cyng::io::to_str(param.second);
				break;
			case 0x8149633C0201:	//	pwd
				const_cast<std::string&>(cfg_ipt_.config_.at(0).pwd_) = cyng::io::to_str(param.second);
				break;

			case 0x814917070002:	//	host
				if (param.second.get_class().tag() == cyng::TC_STRING) {
					const_cast<std::string&>(cfg_ipt_.config_.at(1).host_) = cyng::io::to_str(param.second);
				}
				else {
					cyng::buffer_t tmp;
					tmp = cyng::value_cast(param.second, tmp);
					const_cast<std::string&>(cfg_ipt_.config_.at(1).host_) = cyng::io::to_ascii(tmp);
				}
				break;
			case 0x81491A070002:	//	target port
				const_cast<std::string&>(cfg_ipt_.config_.at(1).service_) = std::to_string(cyng::numeric_cast<std::uint16_t>(param.second, 26862));
				break;
			case 0x8149633C0102:	//	account
				const_cast<std::string&>(cfg_ipt_.config_.at(1).account_) = cyng::io::to_str(param.second);
				break;
			case 0x8149633C0202:	//	pwd
				const_cast<std::string&>(cfg_ipt_.config_.at(1).pwd_) = cyng::io::to_str(param.second);
				break;

			case 0x814827320601:	//	WAIT_TO_RECONNECT
				wait_time_ = cyng::numeric_cast(param.second, wait_time_);
				break;
			case 0x814831320201:	//	TCP_CONNECT_RETRIES
				repetitions_ = cyng::numeric_cast(param.second, repetitions_);
				break;
			case 0x0080800003FF:	//	use SSL
				ssl_ = cyng::value_cast(param.second, ssl_);
				break;
			default:
				break;
			}
		}
	}	//	sml
}

