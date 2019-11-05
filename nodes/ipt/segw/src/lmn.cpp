/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 

#include "lmn.h"
#include <smf/sml/srv_id_io.h>
#include <smf/mbus/defs.h>
//#include <smf/mbus/header.h>
//#include <smf/mbus/variable_data_block.h>
//#include <smf/mbus/aes.h>

#include <cyng/vm/domain/log_domain.h>
#include <cyng/tuple_cast.hpp>
#include <cyng/io/serializer.h>
#include <cyng/io/hex_dump.hpp>

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
		vm_.register_function("wmbus.push.frame", 7, std::bind(&lmn::wmbus_push_frame, this, std::placeholders::_1));

	}

	void lmn::wmbus_push_frame(cyng::context& ctx)
	{
		//
		//	incoming wireless M-Bus data
		//

		cyng::vector_t const frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		//	[01242396072000630E,HYD,63,e,0003105c,72,29436587E61EBF03B900200540C83A80A8E0668CAAB369804FBEFBA35725B34A55369C7877E42924BD812D6D]
		auto const tpl = cyng::tuple_cast<
			cyng::buffer_t,		//	[0] server id
			std::string,		//	[1] manufacturer
			std::uint8_t,		//	[2] version
			std::uint8_t,		//	[3] media
			std::uint32_t,		//	[4] device id
			std::uint8_t,		//	[5] frame_type
			cyng::buffer_t		//	[6] payload
		>(frame);

		//
		//	extract server ID and payload
		//
		cyng::buffer_t const& server_id = std::get<0>(tpl);
		cyng::buffer_t const& payload = std::get<6>(tpl);

		CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - server id: " << sml::from_server_id(server_id));
		CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - manufacturer: " << std::get<1>(tpl));
		CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - version: " << +std::get<2>(tpl));
		CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - media: " << +std::get<3>(tpl) << " - " << node::mbus::get_medium_name(std::get<3>(tpl)));
		CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - device id: " << std::get<4>(tpl));
		CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - frame type: " << +std::get<5>(tpl));

		//
		//	decoding depends on frame type and AES key from
		//	device table
		//
		switch (std::get<5>(tpl)) {
		case node::mbus::FIELD_CI_HEADER_LONG:	//	0x72
			//	e.g. HYD
			//read_frame_header_long(ctx, server_id, payload);
			break;
		case node::mbus::FIELD_CI_HEADER_SHORT:	//	0x7A
			//read_frame_header_short(ctx, server_id, payload);
			break;
		case node::mbus::FIELD_CI_RES_SHORT_SML:	//	0x7F
			//	e.g. ED300L
			//read_frame_header_short_sml(ctx, server_id, payload);
			break;
		default:
		{
			//
			//	frame type not supported
			//
			std::stringstream ss;
			ss
				<< " frame type ["
				<< std::hex
				<< std::setw(2)
				<< +std::get<5>(tpl)
				<< "] not supported - data\n"
				;

			cyng::io::hex_dump hd;
			hd(ss, std::get<6>(tpl).cbegin(), std::get<6>(tpl).cend());

			CYNG_LOG_WARNING(logger_, ctx.get_name()
				<< ss.str());
		}
		//
		// update device table
		//
		//update_device_table(std::get<0>(tpl)
		//	, std::get<1>(tpl)
		//	, std::get<2>(tpl)
		//	, std::get<3>(tpl)
		//	, std::get<5>(tpl)
		//	, cyng::crypto::aes_128_key{}
		//, ctx.tag());
		break;
		}

	}

}

