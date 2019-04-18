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
		{
			vm.register_function("sml.set.proc.ipt.param.address", 4, std::bind(&cfg_ipt::sml_set_proc_ipt_param_address, this, std::placeholders::_1));
			vm.register_function("sml.set.proc.ipt.param.port.target", 4, std::bind(&cfg_ipt::sml_set_proc_ipt_param_port_target, this, std::placeholders::_1));
			vm.register_function("sml.set.proc.ipt.param.user", 4, std::bind(&cfg_ipt::sml_set_proc_ipt_param_user, this, std::placeholders::_1));
			vm.register_function("sml.set.proc.ipt.param.pwd", 4, std::bind(&cfg_ipt::sml_set_proc_ipt_param_pwd, this, std::placeholders::_1));
		}

		void cfg_ipt::get_proc_ipt_params(cyng::object trx, cyng::object server_id)
		{
			sml_gen_.get_proc_ipt_params(trx, server_id, cfg_ipt_);
		}

		void cfg_ipt::sml_set_proc_ipt_param_address(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_TRACE(logger_, "sml.set.proc.ipt.param.address " << cyng::io::to_str(frame));

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,			//	[0] pk
				std::string,				//	[1] trx
				std::uint8_t,				//	[2] record index (0..1)
				cyng::buffer_t				//	[3] address
			>(frame);

			auto const idx = std::get<2>(tpl);
			if (idx < cfg_ipt_.config_.size()) {

				CYNG_LOG_INFO(logger_, " sml.set.proc.ipt.param.address["
					<< +idx
					<< "] "
					<< cfg_ipt_.config_.at(idx).host_
					<< " => "
					<< cyng::io::to_ascii(std::get<3>(tpl)));

				*const_cast<std::string*>(&const_cast<node::ipt::master_config_t&>(cfg_ipt_.config_).at(idx).host_) = cyng::io::to_ascii(std::get<3>(tpl));
			}
		}

		void cfg_ipt::sml_set_proc_ipt_param_port_target(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			//CYNG_LOG_TRACE(logger_, "sml.set.proc.ipt.param.port.target " << cyng::io::to_str(frame));

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,	//	[0] pk
				std::string,		//	[1] trx
				std::uint8_t,		//	[2] record index (1..2)
				std::uint16_t		//	[3] port
			>(frame);

			const auto idx = std::get<2>(tpl);
			if (idx < cfg_ipt_.config_.size()) {

				CYNG_LOG_INFO(logger_, " sml.set.proc.ipt.param.port.target["
					<< +idx
					<< "] "
					<< cfg_ipt_.config_.at(idx).service_
					<< " => "
					<< std::get<3>(tpl));

				*const_cast<std::string*>(&const_cast<node::ipt::master_config_t&>(cfg_ipt_.config_).at(idx).service_) = std::to_string(std::get<3>(tpl));
			}
		}

		void cfg_ipt::sml_set_proc_ipt_param_user(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			//CYNG_LOG_TRACE(logger_, "sml.set.proc.ipt.param.user " << cyng::io::to_str(frame));

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,	//	[0] pk
				std::string,		//	[1] trx
				std::uint8_t,		//	[2] record index (1..2)
				std::string			//	[3] user
			>(frame);

			const auto idx = std::get<2>(tpl);
			if (idx < cfg_ipt_.config_.size()) {

				CYNG_LOG_INFO(logger_, " sml.set.proc.ipt.param.user["
					<< +idx
					<< "] "
					<< cfg_ipt_.config_.at(idx).account_
					<< " => "
					<< std::get<3>(tpl));

				*const_cast<std::string*>(&const_cast<node::ipt::master_config_t&>(cfg_ipt_.config_).at(idx).account_) = std::get<3>(tpl);
			}
		}

		void cfg_ipt::sml_set_proc_ipt_param_pwd(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			//CYNG_LOG_TRACE(logger_, "sml.set.proc.ipt.param.pwd " << cyng::io::to_str(frame));

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,	//	[0] pk
				std::string,		//	[1] trx
				std::uint8_t,		//	[2] record index (1..2)
				std::string			//	[3] pwd
			>(frame);

			const auto idx = std::get<2>(tpl);
			if (idx < cfg_ipt_.config_.size()) {

				CYNG_LOG_INFO(logger_, " sml.set.proc.ipt.param.pwd["
					<< +idx
					<< "] "
					<< cfg_ipt_.config_.at(idx).pwd_
					<< " => "
					<< std::get<3>(tpl));

				*const_cast<std::string*>(&const_cast<node::ipt::master_config_t&>(cfg_ipt_.config_).at(idx).pwd_) = std::get<3>(tpl);
			}
		}

	}	//	sml
}

