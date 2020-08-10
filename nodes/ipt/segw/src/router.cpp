/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "router.h"
#include "cache.h"
#include "storage.h"

#include <smf/sml/protocol/reader.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/obis_io.h>
#include <smf/sml/intrinsics/obis_factory.hpp>

#include <cyng/vm/controller.h>
#include <cyng/vm/context.h>
#include <cyng/vm/generator.h>
#include <cyng/tuple_cast.hpp>
#include <cyng/set_cast.h>
#if defined(SMF_IO_DEBUG) || defined(_DEBUG)
#include <cyng/io/hex_dump.hpp>
#include <cyng/io/cpp_dump.hpp>
#endif

#include <boost/core/ignore_unused.hpp>

namespace node
{
	router::router(cyng::logging::log_ptr logger
		, bool const server_mode
		, cyng::controller& vm
		, cache& cfg
		, storage& db
		, std::string const& account
		, std::string const& pwd
		, bool accept_all)
	: logger_(logger)
		, server_mode_(server_mode)
		, cache_(cfg)
		, sml_gen_()
		, account_(account)
		, pwd_(pwd)
		, accept_all_(accept_all)
		, config_ipt_(logger, sml_gen_, cfg)
		, config_sensor_params_(logger, sml_gen_, cfg)
		, config_data_collector_(logger, sml_gen_, cfg, vm)
		, config_security_(logger, sml_gen_, cfg)
		, config_access_(logger, sml_gen_, cfg)
		, config_iec_(logger, sml_gen_, cfg)
		, config_broker_(logger, sml_gen_, cfg)
		, get_proc_parameter_(logger
			, sml_gen_
			, cfg
			, db
			, config_ipt_
			, config_sensor_params_
			, config_data_collector_
			, config_security_
			, config_access_
			, config_iec_
			, config_broker_)
		, set_proc_parameter_(logger
			, sml_gen_
			, cfg
			, config_ipt_
			, config_sensor_params_
			, config_data_collector_
			, config_security_
			, config_access_
			, config_iec_
			, config_broker_)
		, get_profile_list_(logger, sml_gen_, cfg, db)
		, get_list_(logger, sml_gen_, cfg, db)
		, attention_(logger, sml_gen_, cfg)
	{
		//
		//	SML transport
		//
		vm.register_function("sml.msg", 3, std::bind(&router::sml_msg, this, std::placeholders::_1));
		vm.register_function("sml.eom", 2, std::bind(&router::sml_eom, this, std::placeholders::_1));
		vm.register_function("sml.log", 1, [this](cyng::context& ctx) {
			cyng::vector_t const frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.log - " << cyng::value_cast<std::string>(frame.at(0), ""));
		});

		//
		//	SML data
		//
		vm.register_function("sml.public.open.request", 6, std::bind(&router::sml_public_open_request, this, std::placeholders::_1));
		vm.register_function("sml.public.close.request", 1, std::bind(&router::sml_public_close_request, this, std::placeholders::_1));
		vm.register_function("sml.public.close.response", 1, std::bind(&router::sml_public_close_response, this, std::placeholders::_1));

		vm.register_function("sml.get.proc.parameter.request", 6, std::bind(&router::sml_get_proc_parameter_request, this, std::placeholders::_1));
		vm.register_function("sml.set.proc.parameter.request", 6, std::bind(&router::sml_set_proc_parameter_request, this, std::placeholders::_1));
		vm.register_function("sml.get.profile.list.request", 8, std::bind(&router::sml_get_profile_list_request, this, std::placeholders::_1));
		vm.register_function("sml.get.list.request", 7, std::bind(&router::sml_get_list_request, this, std::placeholders::_1));

		attention_.register_this(vm);

	}

	router::~router()
	{}

	void router::sml_msg(cyng::context& ctx)
	{
		//	example:
		//	[]
		//
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - " << cyng::io::to_type(frame));

		//
		//	get message body
		//
		auto const msg = cyng::to_tuple(frame.at(0));

		//
		//	add generated instruction vector
		//
		ctx.queue(sml::reader::read(msg));
	}

	void router::sml_eom(cyng::context& ctx)
	{
		//	example:
		//	[]
		//
		//	* CRC
		//	* message counter
		//
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		const std::size_t idx = cyng::value_cast<std::size_t>(frame.at(1), 0);
		CYNG_LOG_INFO(logger_, "sml.eom #" << idx);

		//
		//	build SML message frame
		//
		cyng::buffer_t buf = sml_gen_.boxing();

#ifdef _DEBUG
		cyng::io::cpp_dump cd;
		std::stringstream ss;
		cd(ss, buf.begin(), buf.end(), "var");
		CYNG_LOG_TRACE(logger_, "response:\n" << ss.str());
#endif

#ifdef SMF_IO_DEBUG
		cyng::io::hex_dump hd;
		std::stringstream ss;
		hd(ss, buf.begin(), buf.end());
		CYNG_LOG_TRACE(logger_, "response:\n" << ss.str());
#endif

		//
		//	transfer data
		//	"stream.serialize"
		//
		ctx.queue(server_mode_ 
			? cyng::generate_invoke("stream.serialize", buf) 
			: cyng::generate_invoke("ipt.transfer.data", buf));

		ctx.queue(cyng::generate_invoke("stream.flush"));

		//
		//	reset generator
		//
		sml_gen_.reset();
	}

	void router::sml_public_open_request(cyng::context& ctx)
	{
		//	example:
		//	[]
		//
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			std::string,		//	[0] trx
			cyng::buffer_t,		//	[1] client ID
			cyng::buffer_t,		//	[2] server ID
			std::string,		//	[3] request file id
			std::string,		//	[4] username
			std::string			//	[5] password
		>(frame);

		//	sml.public.open.request - trx: 190131160312334117-1, msg id: 0, client id: , server id: , file id: 20190131160312, user: operator, pwd: operator
		CYNG_LOG_INFO(logger_, "sml.public.open.request - trx: "
			<< std::get<0>(tpl)
			<< ", client id: "
			<< cyng::io::to_hex(std::get<1>(tpl))
			<< ", server id: "
			<< cyng::io::to_hex(std::get<2>(tpl))
			<< ", file id: "
			<< std::get<3>(tpl)
			<< ", user: "
			<< std::get<4>(tpl)
			<< ", pwd: "
			<< std::get<5>(tpl))
			;


		if (accept_all_) {

			CYNG_LOG_WARNING(logger_, "sml.public.open.request - Login is disabled for "
				<< cyng::io::to_hex(cache_.get_srv_id()))
				;

			//
			//	linearize and set CRC16
			//	append to current SML message
			//
			sml_gen_.public_open(frame.at(0)	// trx
				, frame.at(1)	//	client id
				, frame.at(3)	//	req file id
				, frame.at(2));

		}
		else {

			//
			//	test server ID
			//
			if (cache_.get_srv_id() == std::get<2>(tpl)) {

				//
				//	test login credentials
				//
				if (!boost::algorithm::equals(account_, std::get<4>(tpl)) ||
					!boost::algorithm::equals(pwd_, std::get<5>(tpl))) {

					sml_gen_.attention_msg(frame.at(1)	// trx
						, std::get<2>(tpl)	//	server ID
						, sml::OBIS_ATTENTION_NOT_AUTHORIZED.to_buffer()
						, "login failed"
						, cyng::tuple_t());

					CYNG_LOG_WARNING(logger_, "sml.public.open.request - login failed: "
						<< std::get<4>(tpl)
						<< ":"
						<< std::get<5>(tpl))
						;
				}
				else {

					//
					//	linearize and set CRC16
					//	append to current SML message
					//
					sml_gen_.public_open(frame.at(0)	// trx
						, frame.at(1)	//	client id
						, frame.at(3)	//	req file id
						, frame.at(2));
				}
			}
			else
			{
				CYNG_LOG_INFO(logger_, "sml.public.open.request - server ID: "
					<< cyng::io::to_hex(std::get<2>(tpl))
					<< " (expected "
					<< cyng::io::to_hex(cache_.get_srv_id())
					<< ")");

				sml_gen_.attention_msg(frame.at(0)	// trx
					, std::get<2>(tpl)	//	server ID
					, sml::OBIS_ATTENTION_NO_SERVER_ID.to_buffer()
					, "wrong server id"
					, cyng::tuple_t());
			}
		}
	}

	void router::sml_public_close_request(cyng::context& ctx)
	{
		//	example:
		//	[]
		//
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		//
		//	linearize and set CRC16
		//	append to current SML message
		//
		sml_gen_.public_close(frame.at(0));
	}

	void router::sml_public_close_response(cyng::context& ctx)
	{
		//	example:
		//	[]
		//
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		//
		//	ignored
		//
	}

	void router::sml_get_proc_parameter_request(cyng::context& ctx)
	{
		//	example:
		//	[]
		//
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			std::string,		//	[0] trx
			cyng::buffer_t,		//	[1] server id
			std::string,		//	[2] user
			std::string,		//	[3] password
			cyng::vector_t		//	[4] path (OBIS)
		>(frame);

		auto const path = sml::vector_to_path(std::get<4>(tpl));
		BOOST_ASSERT(!path.empty());

		//
		//	routed to "get proc parameter" handler
		//
		get_proc_parameter_.generate_response(path
			, std::get<0>(tpl)
			, std::get<1>(tpl)
			, std::get<2>(tpl)
			, std::get<3>(tpl));

	}

	void router::sml_set_proc_parameter_request(cyng::context& ctx)
	{
		//	example:
		//	[]
		//
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		//	* [string] trx - transaction id
		//	* [string] path
		//	* [buffer] server ID
		//	* [string] username
		//	* [string] password
		//	* [param] attribute (should be null)

		auto const tpl = cyng::tuple_cast<
			std::string,		//	[0] trx
			std::string,		//	[1] path
			cyng::buffer_t,		//	[2] server id
			std::string,		//	[3] user
			std::string,		//	[4] password
			cyng::param_t		//	[5] param
		>(frame);

		//
		//	build a obis path from string
		//
		auto const path = sml::to_obis_path(std::get<1>(tpl), ' ');

		set_proc_parameter_.generate_response(path
			, std::get<0>(tpl)
			, std::get<2>(tpl)
			, std::get<3>(tpl)
			, std::get<4>(tpl)
			, std::get<5>(tpl));

	}

	void router::sml_get_profile_list_request(cyng::context& ctx)
	{
		//	example:
		//	[]
		//
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			std::string,		//	[0] trx
			cyng::buffer_t,		//	[1] client id
			cyng::buffer_t,		//	[2] server id
			std::string,		//	[3] user
			std::string,		//	[4] password
			std::chrono::system_clock::time_point,		//	[5] start time
			std::chrono::system_clock::time_point,		//	[6] end time
			cyng::buffer_t		//	[7] path (OBIS)
		>(frame);

		sml::obis const code(std::get<7>(tpl));

		get_profile_list_.generate_response(code
			, std::get<0>(tpl)
			, std::get<1>(tpl)
			, std::get<2>(tpl)
			, std::get<3>(tpl)
			, std::get<4>(tpl)
			, std::get<5>(tpl)
			, std::get<6>(tpl));

	}

	void router::sml_get_list_request(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			std::string,		//	[0] trx
			cyng::buffer_t,		//	[1] client id
			cyng::buffer_t,		//	[2] server id <- meter/sensor ID with the requested data
			std::string,		//	[3] reqFileId
			std::string,		//	[4] user
			std::string,		//	[5] password
			cyng::buffer_t		//	[6] path (OBIS)
		>(frame);

		sml::obis const code(std::get<6>(tpl));

		get_list_.generate_response(sml::obis(std::get<6>(tpl))
			, std::get<0>(tpl)
			, std::get<1>(tpl)
			, std::get<2>(tpl)
			, std::get<3>(tpl)
			, std::get<4>(tpl)
			, std::get<5>(tpl));
	}
}
