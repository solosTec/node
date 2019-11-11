/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 

#include "segw.h"
#include "lmn.h"
#include "bridge.h"
#include "cache.h"
#include "tasks/lmn_wired.h"
#include "tasks/lmn_wireless.h"

#include <smf/sml/srv_id_io.h>
#include <smf/sml/obis_db.h>
#include <smf/mbus/defs.h>
 //#include <smf/mbus/header.h>
//#include <smf/mbus/variable_data_block.h>
//#include <smf/mbus/aes.h>

#include <cyng/vm/domain/log_domain.h>
#include <cyng/tuple_cast.hpp>
#include <cyng/io/serializer.h>
#include <cyng/io/hex_dump.hpp>
#include <cyng/async/task/task_builder.hpp>

#include <boost/uuid/random_generator.hpp>

namespace node
{
	lmn::lmn(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, boost::uuids::uuid tag
		, bridge& b)
	: mux_(mux)
		, logger_(logger)
		, vm_(mux.get_io_service(), tag)
		, bridge_(b)
	{
		cyng::register_logger(logger_, vm_);
		vm_.async_run(cyng::generate_invoke("log.msg.info", "log domain is running"));

		//
		//	callback from wireless LMN
		//
		//vm_.register_function("wmbus.push.frame", 7, std::bind(&lmn::wmbus_push_frame, this, std::placeholders::_1));

	}

	void lmn::start()
	{
		auto const wired = bridge_.cache_.get_cfg(build_cfg_key({ sml::OBIS_CODE_IF_1107 }, "enabled"), false);
		if (wired) {
			start_lmn_wired();
		}
		else {
			CYNG_LOG_WARNING(logger_, "LMN wired (IF_1107) is disabled");
		}

		auto const wireless = bridge_.cache_.get_cfg(build_cfg_key({ sml::OBIS_W_MBUS_PROTOCOL }, "enabled"), false);
		if (wireless) {
			start_lmn_wireless();
		}
		else {
			CYNG_LOG_WARNING(logger_, "LMN wireless (M-Bus) is disabled");
		}
	}

	void lmn::start_lmn_wired()
	{
		try {
#if BOOST_OS_WINDOWS
			auto const port = bridge_.cache_.get_cfg(build_cfg_key({ sml::OBIS_CODE_IF_1107 }, "port"), std::string("COM1"));
#else
			auto const port = bridge_.cache_.get_cfg(build_cfg_key({ sml::OBIS_CODE_IF_1107 }, "port"), std::string("/dev/ttyAPP0"));
#endif
			CYNG_LOG_INFO(logger_, "LMN wired is running on port: " << port);

			cyng::async::start_task_delayed<lmn_wired>(mux_
				, std::chrono::seconds(1)
				, logger_
				, port		//	port
				, bridge_.cache_.get_cfg(build_cfg_key({ sml::OBIS_CODE_IF_1107 }, "databits"), 8u)		//	[u8] databits
				, bridge_.cache_.get_cfg(build_cfg_key({ sml::OBIS_CODE_IF_1107 }, "parity"), "none")	//	[s] parity
				, bridge_.cache_.get_cfg(build_cfg_key({ sml::OBIS_CODE_IF_1107 }, "flow_control"), "none")	//	[s] flow_control
				, bridge_.cache_.get_cfg(build_cfg_key({ sml::OBIS_CODE_IF_1107 }, "stopbits"), "one")	//	[s] stopbits
				, bridge_.cache_.get_cfg(build_cfg_key({ sml::OBIS_CODE_IF_1107 }, "speed"), 8u)		//	[u32] speed
				, cyng::async::NO_TASK);
		}
		catch (boost::system::system_error const& ex) {
			CYNG_LOG_FATAL(logger_, ex.what() << ":" << ex.code());
		}
	}

	void lmn::start_lmn_wireless()
	{
		try {
#if BOOST_OS_WINDOWS
			auto const port = bridge_.cache_.get_cfg(build_cfg_key({ sml::OBIS_W_MBUS_PROTOCOL }, "port"), std::string("COM3"));
#else
			auto const port = bridge_.cache_.get_cfg(build_cfg_key({ sml::OBIS_W_MBUS_PROTOCOL }, "port"), std::string("/dev/ttyAPP1"));
#endif
			CYNG_LOG_INFO(logger_, "LMN wireless is running on port: " << port);

			cyng::async::start_task_delayed<lmn_wireless>(mux_
				, std::chrono::seconds(1)
				, logger_
				, port		//	port
				, bridge_.cache_.get_cfg(build_cfg_key({ sml::OBIS_W_MBUS_PROTOCOL }, "databits"), 8u)		//	[u8] databits
				, bridge_.cache_.get_cfg(build_cfg_key({ sml::OBIS_W_MBUS_PROTOCOL }, "parity"), "none")	//	[s] parity
				, bridge_.cache_.get_cfg(build_cfg_key({ sml::OBIS_W_MBUS_PROTOCOL }, "flow_control"), "none")	//	[s] flow_control
				, bridge_.cache_.get_cfg(build_cfg_key({ sml::OBIS_W_MBUS_PROTOCOL }, "stopbits"), "one")	//	[s] stopbits
				, bridge_.cache_.get_cfg(build_cfg_key({ sml::OBIS_W_MBUS_PROTOCOL }, "speed"), 8u)		//	[u32] speed
				, cyng::async::NO_TASK);
		}
		catch (boost::system::system_error const& ex) {
			CYNG_LOG_FATAL(logger_, ex.what() << ":" << ex.code());
		}
	}

	//void lmn::wmbus_push_frame(cyng::context& ctx)
	//{
	//	//
	//	//	incoming wireless M-Bus data
	//	//

	//	cyng::vector_t const frame = ctx.get_frame();
	//	CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

	//	//	[01242396072000630E,HYD,63,e,0003105c,72,29436587E61EBF03B900200540C83A80A8E0668CAAB369804FBEFBA35725B34A55369C7877E42924BD812D6D]
	//	auto const tpl = cyng::tuple_cast<
	//		cyng::buffer_t,		//	[0] server id
	//		std::string,		//	[1] manufacturer
	//		std::uint8_t,		//	[2] version
	//		std::uint8_t,		//	[3] media
	//		std::uint32_t,		//	[4] device id
	//		std::uint8_t,		//	[5] frame_type
	//		cyng::buffer_t		//	[6] payload
	//	>(frame);

	//	//
	//	//	extract server ID and payload
	//	//
	//	cyng::buffer_t const& server_id = std::get<0>(tpl);
	//	cyng::buffer_t const& payload = std::get<6>(tpl);

	//	CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - server id: " << sml::from_server_id(server_id));
	//	CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - manufacturer: " << std::get<1>(tpl));
	//	CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - version: " << +std::get<2>(tpl));
	//	CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - media: " << +std::get<3>(tpl) << " - " << node::mbus::get_medium_name(std::get<3>(tpl)));
	//	CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - device id: " << std::get<4>(tpl));
	//	CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - frame type: " << +std::get<5>(tpl));

	//	//
	//	//	decoding depends on frame type and AES key from
	//	//	device table
	//	//
	//	switch (std::get<5>(tpl)) {
	//	case node::mbus::FIELD_CI_HEADER_LONG:	//	0x72
	//		//	e.g. HYD
	//		//read_frame_header_long(ctx, server_id, payload);
	//		break;
	//	case node::mbus::FIELD_CI_HEADER_SHORT:	//	0x7A
	//		//read_frame_header_short(ctx, server_id, payload);
	//		break;
	//	case node::mbus::FIELD_CI_RES_SHORT_SML:	//	0x7F
	//		//	e.g. ED300L
	//		//read_frame_header_short_sml(ctx, server_id, payload);
	//		break;
	//	default:
	//	{
	//		//
	//		//	frame type not supported
	//		//
	//		std::stringstream ss;
	//		ss
	//			<< " frame type ["
	//			<< std::hex
	//			<< std::setw(2)
	//			<< +std::get<5>(tpl)
	//			<< "] not supported - data\n"
	//			;

	//		cyng::io::hex_dump hd;
	//		hd(ss, std::get<6>(tpl).cbegin(), std::get<6>(tpl).cend());

	//		CYNG_LOG_WARNING(logger_, ctx.get_name()
	//			<< ss.str());
	//	}
	//	//
	//	// update device table
	//	//
	//	//update_device_table(std::get<0>(tpl)
	//	//	, std::get<1>(tpl)
	//	//	, std::get<2>(tpl)
	//	//	, std::get<3>(tpl)
	//	//	, std::get<5>(tpl)
	//	//	, cyng::crypto::aes_128_key{}
	//	//, ctx.tag());
	//	break;
	//	}

	//}

}

