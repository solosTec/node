/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "core.h"
#include <smf/sml/protocol/reader.h>
#include <smf/sml/protocol/message.h>
#include <smf/sml/protocol/value.hpp>
#include <smf/sml/obis_db.h>
#include <smf/sml/srv_id_io.h>
#include <smf/sml/obis_io.h>
#include <smf/shared/db_cfg.h>

#include <cyng/io/serializer.h>
#include <cyng/vm/generator.h>
#include <cyng/tuple_cast.hpp>
#include <cyng/numeric_cast.hpp>
#include <cyng/io/swap.h>
#include <cyng/io/io_buffer.h>
#include <cyng/util/slice.hpp>

//#ifdef SMF_IO_DEBUG
#include <cyng/io/hex_dump.hpp>
//#endif

#include <boost/algorithm/string.hpp>

namespace node
{
	namespace sml
	{

		kernel::kernel(cyng::logging::log_ptr logger
			, cyng::controller& vm
			, cyng::store::db& config_db
			, node::ipt::redundancy const& cfg
			, bool server_mode
			, std::string account
			, std::string pwd
			, bool accept_all)
		: logger_(logger)
			, config_db_(config_db)
			, server_mode_(server_mode)
			, account_(account)
			, pwd_(pwd)
			, accept_all_(accept_all)
			, sml_gen_()
			, data_(logger, sml_gen_, vm, config_db)
			, ipt_(logger, sml_gen_, vm, config_db, cfg)
			, push_(logger, sml_gen_, vm, config_db)
			, attention_(logger, sml_gen_, vm, config_db)
			, get_proc_parameter_(logger, sml_gen_, config_db, ipt_)
			, set_proc_parameter_(logger, sml_gen_, config_db, ipt_)
			, get_profile_list_(logger, sml_gen_, config_db)
		{
			reset();

			//
			//	SML transport
			//
			vm.register_function("sml.msg", 3, std::bind(&kernel::sml_msg, this, std::placeholders::_1));
			vm.register_function("sml.eom", 2, std::bind(&kernel::sml_eom, this, std::placeholders::_1));
			vm.register_function("sml.log", 1, [this](cyng::context& ctx){
				const cyng::vector_t frame = ctx.get_frame();
				CYNG_LOG_INFO(logger_, "sml.log - " << cyng::value_cast<std::string>(frame.at(0), ""));
			});
			
			//
			//	SML data
			//
			vm.register_function("sml.public.open.request", 6, std::bind(&kernel::sml_public_open_request, this, std::placeholders::_1));
			vm.register_function("sml.public.close.request", 1, std::bind(&kernel::sml_public_close_request, this, std::placeholders::_1));
			vm.register_function("sml.public.close.response", 1, std::bind(&kernel::sml_public_close_response, this, std::placeholders::_1));

			vm.register_function("sml.get.proc.parameter.request", 6, std::bind(&kernel::sml_get_proc_parameter_request, this, std::placeholders::_1));
			vm.register_function("sml.set.proc.parameter.request", 6, std::bind(&kernel::sml_set_proc_parameter_request, this, std::placeholders::_1));
			vm.register_function("sml.get.profile.list.request", 8, std::bind(&kernel::sml_get_profile_list_request, this, std::placeholders::_1));

		}

		void kernel::reset()
		{
			//
			//	reset reader
			//
			sml_gen_.reset();
		}

		void kernel::sml_msg(cyng::context& ctx)
		{
			//	[{31383035323232323535353231383235322D31,0,0,{0100,{null,005056C00008,3230313830353232323235353532,0500153B02297E,6F70657261746F72,6F70657261746F72,null}},9c91},0]
			//	[{31383035323232323535353231383235322D32,1,0,{0500,{0500153B02297E,6F70657261746F72,6F70657261746F72,{810060050000},null}},f8b5},1]
			//	[{31383035323232323535353231383235322D33,2,0,{0500,{0500153B02297E,6F70657261746F72,6F70657261746F72,{8181C78201FF},null}},4d37},2]
			//	[{31383035323232323535353231383235322D34,0,0,{0200,{null}},e6e8},3]
			//
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

			//
			//	get message body
			//
			cyng::tuple_t msg;
			msg = cyng::value_cast(frame.at(0), msg);

			//
			//	add generated instruction vector
			//
			ctx.queue(reader::read(msg));
		}

		void kernel::sml_eom(cyng::context& ctx)
		{
			//[4aa5,4]
			//
			//	* CRC
			//	* message counter
			//
			const cyng::vector_t frame = ctx.get_frame();
			const std::size_t idx = cyng::value_cast<std::size_t>(frame.at(1), 0);
			//CYNG_LOG_TRACE(logger_, "sml.eom - " << cyng::io::to_str(frame));
			CYNG_LOG_INFO(logger_, "sml.eom #" << idx);

			//
			//	build SML message frame
			//
			//cyng::buffer_t buf = boxing(msg_);
			cyng::buffer_t buf = sml_gen_.boxing();

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
			if (server_mode_)
			{
				ctx.queue(cyng::generate_invoke("stream.serialize", buf));
			}
			else
			{
				ctx.queue(cyng::generate_invoke("ipt.transfer.data", buf));
			}

			ctx.queue(cyng::generate_invoke("stream.flush"));

			reset();
		}

		void kernel::sml_public_open_request(cyng::context& ctx)
		{
			//	[34481794-6866-4776-8789-6f914b4e34e7,180301091943374505-1,0,005056c00008,00:15:3b:02:23:b3,20180301092332,operator,operator]
			//	[874c991e-cfc0-4348-83ab-064f59363229,190131160312334117-1,0,005056C00008,0500FFB04B94F8,20190131160312,operator,operator]
			//
			//	* pk
			//	* transaction id
			//	* SML message id
			//	* client ID - MAC from client
			//	* server ID - target server/gateway
			//	* request file id
			//	* username
			//	* password
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

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

			if (!accept_all_) {

				cyng::buffer_t server_id;
				server_id = cyng::value_cast(get_config(config_db_, OBIS_SERVER_ID.to_str()), server_id);

				//
				//	test server ID
				//
				if (!boost::algorithm::equals(server_id, std::get<2>(tpl))) {

					sml_gen_.attention_msg(frame.at(1)	// trx
						, std::get<2>(tpl)	//	server ID
						, OBIS_ATTENTION_NO_SERVER_ID.to_buffer()
						, "wrong server id"
						, cyng::tuple_t());

					CYNG_LOG_WARNING(logger_, "sml.public.open.request - wrong server ID: "
						<< cyng::io::to_hex(std::get<2>(tpl))
						<< " ("
						<< cyng::io::to_hex(server_id)
						<< ")")	;

					return;
				}

				//
				//	test login credentials
				//
				if (!boost::algorithm::equals(account_, std::get<4>(tpl)) ||
					!boost::algorithm::equals(pwd_, std::get<5>(tpl))) {

					sml_gen_.attention_msg(frame.at(1)	// trx
						, std::get<2>(tpl)	//	server ID
						, OBIS_ATTENTION_NOT_AUTHORIZED.to_buffer()
						, "login failed"
						, cyng::tuple_t());

					CYNG_LOG_WARNING(logger_, "sml.public.open.request - login failed: "
						<< std::get<4>(tpl)
						<< ":"
						<< std::get<5>(tpl))
						;
					return;
				}
			}

			//
			//	linearize and set CRC16
			//	append to current SML message
			//
			sml_gen_.public_open(frame.at(1)	// trx
				, frame.at(3)	//	client id
				, frame.at(5)	//	req file id
				, frame.at(4));
		}

		void kernel::sml_public_close_request(cyng::context& ctx)
		{
			//	[5fb21ea1-ac66-4a00-98b7-88edd57addb7,180523152649286995-4,3]
			//
			//	* transaction id
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

			//
			//	linearize and set CRC16
			//	append to current SML message
			//
			sml_gen_.public_close(frame.at(0));
		}

		void kernel::sml_public_close_response(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));
		}

		void kernel::sml_get_proc_parameter_request(cyng::context& ctx)
		{
			//	[b5cfc8a0-0bf4-4afe-9598-ded99f71469c,180301094328243436-3,2,05:00:15:3b:02:23:b3,operator,operator,81 81 c7 82 01 ff,null]
			//	[50cfab74-eef0-4c92-89d4-46b28ab9da20,180522233943303816-2,1,00:15:3b:02:29:7e,operator,operator,81 00 60 05 00 00,null]
			//
			//	* trx - transaction id
			//	* server ID
			//	* username
			//	* password
			//	* OBIS (parameterTreePath)
			//	* attribute (should be null)
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

			auto const tpl = cyng::tuple_cast<
				std::string,		//	[0] trx
				cyng::buffer_t,		//	[1] server id
				std::string,		//	[2] user
				std::string,		//	[3] password
				cyng::buffer_t		//	[4] path (OBIS)
			>(frame);

			obis const code(std::get<4>(tpl));

			get_proc_parameter_.generate_response(code
				, std::get<0>(tpl)
				, std::get<1>(tpl)
				, std::get<2>(tpl)
				, std::get<3>(tpl));

		}

		void kernel::sml_set_proc_parameter_request(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

			//	[]
			//
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

			auto const path = to_obis_path(std::get<1>(tpl), ' ');

			set_proc_parameter_.generate_response(path
				, std::get<0>(tpl)
				, std::get<2>(tpl)
				, std::get<3>(tpl)
				, std::get<4>(tpl)
				, std::get<5>(tpl));

		}

		void kernel::sml_get_profile_list_request(cyng::context& ctx) 
		{
			//	
			//	* transaction id
			//	* client ID
			//	* server ID
			//	* username
			//	* password
			//	* start time
			//	* end time
			//	* OBIS (requested parameter)
			//	
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

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

			obis const code(std::get<7>(tpl));
			get_profile_list_.generate_response(code
				, std::get<0>(tpl)
				, std::get<1>(tpl)
				, std::get<2>(tpl)
				, std::get<3>(tpl)
				, std::get<4>(tpl)
				, std::get<5>(tpl)
				, std::get<6>(tpl));
		}
	}	//	sml
}

