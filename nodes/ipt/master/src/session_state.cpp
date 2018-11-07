/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 


#include "session.h"
#include <smf/ipt/response.hpp>
#include <smf/sml/status.h>
#include <smf/sml/srv_id_io.h>

#include <cyng/io/serializer.h>
#include <cyng/tuple_cast.hpp>
#include <cyng/factory/set_factory.h>
#include <cyng/set_cast.h>
#include <cyng/numeric_cast.hpp>
#include <cyng/io/swap.h>

#include <boost/assert.hpp>

namespace node 
{
	namespace ipt
	{
		session::connect_state::connect_state(session* sp, std::size_t tsk)
			: state_(STATE_START)
			, sr_(*sp)
			, tsk_(tsk)
		{}

		response_type session::connect_state::set_authorized(bool b)
		{
			BOOST_ASSERT(state_ == STATE_START);

			const response_type res = b
				? ctrl_res_login_public_policy::SUCCESS
				: ctrl_res_login_public_policy::UNKNOWN_ACCOUNT
				;

			//
			//	stop gatekeeper
			//
			sr_.mux_.post(tsk_, 0, cyng::tuple_factory(res));

			if (b) {
				state_ = STATE_AUTHORIZED;
			}

			return res;
		}

		void session::connect_state::open_connection(bool b)
		{
			state_ = b
				? STATE_LOCAL
				: STATE_REMOTE
				;
		}

		void session::connect_state::open_connection(std::size_t tsk)
		{
			state_ = STATE_TASK;
			tsk_ = tsk;
		}

		void session::connect_state::close_connection()
		{
			state_ = STATE_OFFLINE;
			tsk_ = 0u;
		}

		void session::connect_state::stop()
		{
			if (!is_authorized()) {

				BOOST_ASSERT((STATE_AUTHORIZED == state_) || (STATE_START == state_));
				state_ = STATE_START;

				CYNG_LOG_INFO(sr_.logger_, "stop gatekeeper #"
					<< tsk_);

				const response_type res = ctrl_res_login_public_policy::GENERAL_ERROR;
				sr_.mux_.post(tsk_, 0, cyng::tuple_factory(res));
			}
		}

		bool session::connect_state::is_authorized() const
		{
			return STATE_START != state_;
		}

		bool session::connect_state::is_connected() const
		{
			return state_ > STATE_OFFLINE;
		}

		std::ostream& operator<<(std::ostream& os, session::connect_state const& cs)
		{
			switch (cs.state_) {
			case session::connect_state::STATE_START:
				os << "start";
				break;
			case session::connect_state::STATE_AUTHORIZED:
				os << "authorized";
				break;
			case session::connect_state::STATE_OFFLINE:
				os << "offline";
				break;
			case session::connect_state::STATE_LOCAL:
				os << "local";
				break;
			case session::connect_state::STATE_REMOTE:
				os << "remote";
				break;
			case session::connect_state::STATE_TASK:
				os << "task #" << cs.tsk_;
				break;
			default:
				os << "ERROR";
				break;
			}
			return os;
		}

		void session::connect_state::register_this(cyng::controller& vm)
		{
			vm.register_function("sml.msg", 2, std::bind(&connect_state::sml_msg, this, std::placeholders::_1));
			vm.register_function("sml.eom", 2, std::bind(&connect_state::sml_eom, this, std::placeholders::_1));
			vm.register_function("sml.public.open.response", 6, std::bind(&connect_state::sml_public_open_response, this, std::placeholders::_1));
			vm.register_function("sml.public.close.response", 3, std::bind(&connect_state::sml_public_close_response, this, std::placeholders::_1));
			vm.register_function("sml.get.proc.param.srv.visible", 8, std::bind(&connect_state::sml_get_proc_param_srv_visible, this, std::placeholders::_1));
			vm.register_function("sml.get.proc.param.srv.active", 8, std::bind(&connect_state::sml_get_proc_param_srv_active, this, std::placeholders::_1));
			vm.register_function("sml.get.proc.param.firmware", 8, std::bind(&connect_state::sml_get_proc_param_firmware, this, std::placeholders::_1));
			vm.register_function("sml.get.proc.param.simple", 6, std::bind(&connect_state::sml_get_proc_param_simple, this, std::placeholders::_1));
			vm.register_function("sml.get.proc.status.word", 6, std::bind(&connect_state::sml_get_proc_status_word, this, std::placeholders::_1));
			vm.register_function("sml.get.proc.param.memory", 7, std::bind(&connect_state::sml_get_proc_param_memory, this, std::placeholders::_1));
			vm.register_function("sml.get.proc.param.wmbus.status", 9, std::bind(&connect_state::sml_get_proc_param_wmbus_status, this, std::placeholders::_1));
			vm.register_function("sml.get.proc.param.wmbus.config", 11, std::bind(&connect_state::sml_get_proc_param_wmbus_config, this, std::placeholders::_1));
			vm.register_function("sml.get.proc.param.ipt.state", 8, std::bind(&connect_state::sml_get_proc_param_ipt_status, this, std::placeholders::_1));
			vm.register_function("sml.get.proc.param.ipt.param", 11, std::bind(&connect_state::sml_get_proc_param_ipt_param, this, std::placeholders::_1));
			vm.register_function("sml.attention.msg", 6, std::bind(&connect_state::sml_attention_msg, this, std::placeholders::_1));

		}

		void session::connect_state::sml_msg(cyng::context& ctx)
		{
			//
			//	1. tuple containing the SML data tree
			//	2. message index
			//

			const cyng::vector_t frame = ctx.get_frame();

			//
			//	print sml message number
			//
			const std::size_t idx = cyng::value_cast<std::size_t>(frame.at(1), 0);
			CYNG_LOG_INFO(sr_.logger_, "SML processor sml.msg #"
				<< idx);

			//
			//	get message body
			//
			cyng::tuple_t msg;
			msg = cyng::value_cast(frame.at(0), msg);

			CYNG_LOG_DEBUG(sr_.logger_, "SML message to #"
				<< tsk_
				<< " - "
				<< cyng::io::to_str(msg));

			if (state_ == STATE_TASK) {
				sr_.mux_.post(tsk_, 3, std::move(msg));
			}
			else {
				CYNG_LOG_ERROR(sr_.logger_, "sml.msg #"
					<< idx
					<< " - session in wrong state: "
					<< *this);

			}
		}

		void session::connect_state::sml_eom(cyng::context& ctx)
		{
			//	[5213,3]
			//
			//	* CRC
			//	* message counter
			//
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(sr_.logger_, "sml.eom "
				<< *this
				<< " - "
				<< cyng::io::to_str(frame));

			auto const tpl = cyng::tuple_cast<
				std::uint16_t,	//	[0] CRC
				std::size_t		//	[1] message index
			>(frame);

			if (state_ == STATE_TASK) {
				sr_.mux_.post(tsk_, 1, cyng::tuple_factory(std::get<0>(tpl), std::get<1>(tpl)));
			}
			else {
				CYNG_LOG_ERROR(sr_.logger_, "sml.eom - session in wrong state: "
					<< *this
					<< " - "
					<< cyng::io::to_str(frame));

			}
		}

		void session::connect_state::sml_public_open_response(cyng::context& ctx)
		{
			//	 [6f5fade9-5364-4c7c-b8e3-cb74f16361d8,,0,000000000000,0500153B0223B3,20180828184625]
			//
			//	* [uuid] pk
			//	* [buffer] trx
			//	* [size_t] idx
			//	* clientId
			//	* serverId
			//	* reqFileId

			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(sr_.logger_, "sml.public.open.response "
				<< *this
				<< " - "
				<< cyng::io::to_str(frame));

		}

		void session::connect_state::sml_public_close_response(cyng::context& ctx)
		{
			//	 [d937611f-1d91-41c0-84e7-52f0f6ab36bc,,0]
			//
			//	* [uuid] pk
			//	* [buffer] trx
			//	* [size_t] idx

			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(sr_.logger_, "sml.public.close.response "
				<< *this
				<< " - "
				<< cyng::io::to_str(frame));

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,	//	[0] pk
				cyng::buffer_t,		//	[1] trx
				std::size_t			//	[2] idx
			>(frame);

			if (state_ == STATE_TASK) {
				//sr_.mux_.post(tsk_, 4, cyng::tuple_factory(std::get<0>(tpl), std::get<1>(tpl), std::get<2>(tpl)));
				sr_.mux_.post(tsk_, 4, cyng::to_tuple(frame));
				
				//
				//	disconnect task from data stream
				//
				close_connection();
			}
			else {
				CYNG_LOG_ERROR(sr_.logger_, "sml.public.close.response - session in wrong state: "
					<< *this
					<< " - "
					<< cyng::io::to_str(frame));
			}

		}

		void session::connect_state::sml_get_proc_status_word(cyng::context& ctx)
		{
			//	[b1913f62-6cb3-4586-ad43-24b20f0e7d7e,3616596-2,0,0500153B02297E,810060050000,cc070202]
			//
			//	* [uuid] pk
			//	* [string] trx
			//	* [size_t] idx
			//	* [buffer] server id
			//	* [buffer] OBIS code (81 00 60 05 00 00)
			//	* [u32] status word
			const cyng::vector_t frame = ctx.get_frame();
			const auto word = cyng::numeric_cast<std::uint32_t>(frame.at(5), 0u);
			CYNG_LOG_TRACE(sr_.logger_, "sml.get.proc.status.word "
				<< *this
				<< " - "
				<< cyng::io::to_str(frame)
				<< " - "
				<< frame.at(5).get_class().type_name()
				<< " - "
				<< word);

			node::sml::status stat;
			stat.reset();

			if (state_ == STATE_TASK) {
				sr_.mux_.post(tsk_, 5, cyng::tuple_t{ 
					frame.at(1),	//	trx
					frame.at(2),	//	idx
					frame.at(3),	//	server ID
					frame.at(4),	//	OBIS code
									//	status word
					cyng::param_map_factory("word", node::sml::to_param_map(stat))()
				});
			}
			else {
				CYNG_LOG_ERROR(sr_.logger_, "sml.get.proc.status.word - session in wrong state: "
					<< *this
					<< " - "
					<< cyng::io::to_str(frame));
			}
		}

		void session::connect_state::sml_get_proc_param_memory(cyng::context& ctx)
		{
			//	[eef2de9e-775a-4819-95f2-665b95da1078,9030605-2,0,0500153B01EC46,0080800010FF,14,0]
			//	
			//	* [uuid] pk
			//	* [string] trx
			//	* [size_t] idx
			//	* [buffer] server id
			//	* [buffer] OBIS code (81 00 60 05 00 00)
			//	* [u8] mirror
			//	* [u8] tmp
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_TRACE(sr_.logger_, "sml.get.proc.param.memory "
				<< *this
				<< " - "
				<< cyng::io::to_str(frame)
				<< " - "
				<< frame.at(5).get_class().type_name()
				<< " - "
				<< cyng::numeric_cast<std::uint32_t>(frame.at(5), 0u));

			if (state_ == STATE_TASK) {
				sr_.mux_.post(tsk_, 5, cyng::tuple_t{ 
					frame.at(1),	//	trx
					frame.at(2),	//	idx
					frame.at(3),	//	server ID
					frame.at(4),	//	OBIS code
									//	status word
					cyng::param_map_factory("mirror", frame.at(5))("tmp", frame.at(6))()
				});
			}
			else {
				CYNG_LOG_ERROR(sr_.logger_, "sml.get.proc.status.word - session in wrong state: "
					<< *this
					<< " - "
					<< cyng::io::to_str(frame));
			}
		}

		void session::connect_state::sml_get_proc_param_srv_visible(cyng::context& ctx)
		{
			//	[63b54276-1872-484a-b282-1de45d045f58,7027958-2,0,0500153B0223B3,000c,01E61E29436587BF03,2D2D2D,2018-08-29 17:59:22.00000000]
			//
			//	* [uuid] pk
			//	* [string] trx
			//	* [size_t] idx
			//	* [buffer] server id
			//	* [buffer] OBIS code
			//	* [u16] number
			//	* [buffer] meter ID
			//	* [buffer] device class (always "---")
			//	* [timestamp] UTC/last status message

			const cyng::vector_t frame = ctx.get_frame();
			//CYNG_LOG_TRACE(sr_.logger_, "sml.get.proc.param.srv.visible "
			//	<< *this
			//	<< " - "
			//	<< cyng::io::to_str(frame));

			if (state_ == STATE_TASK) {

				cyng::buffer_t meter;
				meter = cyng::value_cast(frame.at(6), meter);

				sr_.mux_.post(tsk_, 5, cyng::tuple_t{ 
					frame.at(1),	//	trx
					frame.at(2),	//	idx
					frame.at(3),	//	server ID
					frame.at(4),	//	OBIS code
									//	visible device
					cyng::param_map_factory("number", frame.at(5))
					("meter", node::sml::from_server_id(meter))
					("meterId", meter)
					("class", frame.at(7))
					("timestamp", frame.at(8))
					("type", node::sml::get_srv_type(meter))
					()
				});
			}
			else {
				CYNG_LOG_ERROR(sr_.logger_, "sml.get.proc.param.srv.visible - session in wrong state: "
					<< *this
					<< " - "
					<< cyng::io::to_str(frame));

			}
		}

		void session::connect_state::sml_get_proc_param_srv_active(cyng::context& ctx)
		{
			//	[63b54276-1872-484a-b282-1de45d045f58,7027958-2,0,0500153B0223B3,000c,01E61E29436587BF03,2D2D2D,2018-08-29 17:59:22.00000000]
			//
			//	* [uuid] pk
			//	* [string] trx
			//	* [size_t] idx
			//	* [buffer] server id
			//	* [u16] number
			//	* [buffer] meter ID
			//	* [buffer] device class (always "---")
			//	* [timestamp] UTC/last status message

			const cyng::vector_t frame = ctx.get_frame();
			//CYNG_LOG_TRACE(sr_.logger_, "sml.get.proc.param.srv.active "
			//	<< *this
			//	<< " - "
			//	<< cyng::io::to_str(frame));

			if (state_ == STATE_TASK) {

				cyng::buffer_t meter;
				meter = cyng::value_cast(frame.at(6), meter);

				sr_.mux_.post(tsk_, 5, cyng::tuple_t{
					frame.at(1),	//	trx
					frame.at(2),	//	idx
					frame.at(3),	//	server ID
					frame.at(4),	//	OBIS code
									//	visible device
					cyng::param_map_factory("number", frame.at(5))
					("meter", node::sml::from_server_id(meter))
					("class", frame.at(7))
					("timestamp", frame.at(8))
					("type", node::sml::get_srv_type(meter))
					()
				});
			}
			else {
				CYNG_LOG_ERROR(sr_.logger_, "sml.get.proc.param.srv.active - session in wrong state: "
					<< *this
					<< " - "
					<< cyng::io::to_str(frame));
			}
		}

		void session::connect_state::sml_get_proc_param_firmware(cyng::context& ctx)
		{
			//	[63b54276-1872-484a-b282-1de45d045f58,7027958-2,0,0500153B0223B3,000c,01E61E29436587BF03,2D2D2D,2018-08-29 17:59:22.00000000]
			//
			//	* [uuid] pk
			//	* [string] trx
			//	* [size_t] idx
			//	* [buffer] server id
			//	* [u8] number
			//	* [buffer] firmware name/section
			//	* [buffer] version
			//	* [bool] active/inactive

			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_TRACE(sr_.logger_, "sml.get.proc.param.firmware "
				<< *this
				<< " - "
				<< cyng::io::to_str(frame));

			if (state_ == STATE_TASK) {
				//sr_.mux_.post(tsk_, 7, cyng::tuple_t{ frame.at(3), frame.at(4), frame.at(5), frame.at(6), frame.at(7) });
				sr_.mux_.post(tsk_, 5, cyng::tuple_t{ 
					frame.at(1),	//	trx
					frame.at(2),	//	idx
					frame.at(3),	//	server ID
					frame.at(4),	//	OBIS code
									//	firmware
					cyng::param_map_factory("number", frame.at(5))
					("firmware", frame.at(6))
					("version", frame.at(7))
					("active", frame.at(8))
					()
				});
			}
			else {
				CYNG_LOG_ERROR(sr_.logger_, "sml.get.proc.param.firmware - session in wrong state: "
					<< *this
					<< " - "
					<< cyng::io::to_str(frame));
			}
		}

		void session::connect_state::sml_get_proc_param_wmbus_status(cyng::context& ctx)
		{
			//	 [c5870024-e64c-43dd-aac4-3f3209a291b9,7228963-2,0,0500153B021774,81060F0600FF,RC1180-MBUS3,A815919638040131,332E3038,322E3030]
			//
			//	* [uuid] pk
			//	* [string] trx
			//	* [size_t] idx
			//	* [buffer] server id
			//	* [buffer] OBIS
			//	* [string] manufacturer
			//	* [buffer] device ID
			//	* [string] firmware version
			//	* [string] hardware version
			//
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_TRACE(sr_.logger_, "sml.get.proc.param.wmbus.status "
				<< *this
				<< " - "
				<< cyng::io::to_str(frame));

			if (state_ == STATE_TASK) {
				sr_.mux_.post(tsk_, 5, cyng::tuple_t{ 
					frame.at(1),	//	trx
					frame.at(2),	//	idx
					frame.at(3),	//	server ID
					frame.at(4),	//	OBIS code
									//	firmware
					cyng::param_map_factory("manufacturer", frame.at(5))
					("id", frame.at(6))
					("firmware", frame.at(7))
					("hardware", frame.at(8))
					()
				});
			}
			else {
				CYNG_LOG_ERROR(sr_.logger_, "sml.get.proc.param.firmware - session in wrong state: "
					<< *this
					<< " - "
					<< cyng::io::to_str(frame));
			}
		}

		void session::connect_state::sml_get_proc_param_wmbus_config(cyng::context& ctx)
		{
			//	[0cbb85fa-7c66-4f5f-891e-9a409c054137,1701175-3,0,0500153B01EC46,8106190700FF,0,1e,14,00015180,0,null]
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_TRACE(sr_.logger_, "sml.get.proc.param.wmbus.config "
				<< *this
				<< " - "
				<< cyng::io::to_str(frame));

			if (state_ == STATE_TASK) {
				sr_.mux_.post(tsk_, 5, cyng::tuple_t{
					frame.at(1),	//	trx
					frame.at(2),	//	idx
					frame.at(3),	//	server ID
					frame.at(4),	//	OBIS code
									//	wireless M-Bus configuration
					cyng::param_map_factory("protocol", frame.at(5))
					("sMode", cyng::numeric_cast<std::uint16_t>(frame.at(6), 0))
					("tMode", cyng::numeric_cast<std::uint16_t>(frame.at(7), 0))
					("reboot", frame.at(8))
					("power", frame.at(9))
					("installMode", frame.at(9))
					()
					});
			}
			else {
				CYNG_LOG_ERROR(sr_.logger_, "sml.get.proc.param.firmware - session in wrong state: "
					<< *this
					<< " - "
					<< cyng::io::to_str(frame));
			}
		}

		void session::connect_state::sml_get_proc_param_ipt_status(cyng::context& ctx)
		{
			//	 [97197e6c-a8cb-496a-8749-6bf25867758f,1360299-2,0,00:15:3b:01:ec:46,81490D0600FF,1501a8c0,68ee,10c6]
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_TRACE(sr_.logger_, "sml.get.proc.param.ipt.status "
				<< *this
				<< " - "
				<< cyng::io::to_str(frame));

			//	boost::asio::ip::make_address_v4() expects host byte ordering - but SML delivers network ordering
			const boost::asio::ip::address_v4::uint_type ia = cyng::value_cast<boost::asio::ip::address_v4::uint_type>(frame.at(5), 0u);
			const boost::asio::ip::address address = boost::asio::ip::make_address_v4(cyng::swap_num(ia));

			sr_.mux_.post(tsk_, 5, cyng::tuple_t{
				frame.at(1),	//	trx
				frame.at(2),	//	idx
				frame.at(3),	//	server ID
				frame.at(4),	//	OBIS code
								//	current IP-T address parameters
				cyng::param_map_factory("address", address)
				("local", cyng::numeric_cast<std::uint16_t>(frame.at(6), 0))
				("remote", cyng::numeric_cast<std::uint16_t>(frame.at(7), 0))
				()
				});
		}

		void session::connect_state::sml_get_proc_param_ipt_param(cyng::context& ctx)
		{
			//	 [1b2164c5-ee0f-44d4-a4eb-6f3a78e47767,0980651-3,0,00:15:3b:02:23:b3,81490D0700FF,2,1501a8c0,68ef,0,LSMTest4,LSMTest4]
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_TRACE(sr_.logger_, "sml.get.proc.param.ipt.param "
				<< *this
				<< " - "
				<< cyng::io::to_str(frame));

			//	boost::asio::ip::make_address_v4() expects host byte ordering - but SML delivers network ordering
			const boost::asio::ip::address_v4::uint_type ia = cyng::value_cast<boost::asio::ip::address_v4::uint_type>(frame.at(6), 0u);
			const boost::asio::ip::address address = boost::asio::ip::make_address_v4(cyng::swap_num(ia));

			sr_.mux_.post(tsk_, 5, cyng::tuple_t{
				frame.at(1),	//	trx
				frame.at(2),	//	idx
				frame.at(3),	//	server ID
				frame.at(4),	//	OBIS code
								//	IP-T configuration record
				cyng::param_map_factory("idx", frame.at(5))
				("address", address)
				("local", cyng::numeric_cast<std::uint16_t>(frame.at(7), 0))
				("remote", cyng::numeric_cast<std::uint16_t>(frame.at(8), 0))
				("name", frame.at(9))
				("pwd", frame.at(10))
				()
				});
		}

		void session::connect_state::sml_get_proc_param_simple(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_TRACE(sr_.logger_, "sml.get.proc.param.simple "
				<< *this
				<< " - "
				<< cyng::io::to_str(frame));
		}

		void session::connect_state::sml_attention_msg(cyng::context& ctx)
		{
			//	[7020aae0-7cbe-46a0-aaf6-3aa4b037a35e,9858052-2,0,0500153B02297E,8181C7C7FD00,null]
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_WARNING(sr_.logger_, "sml.attention.msg "
				<< *this
				<< " - "
				<< cyng::io::to_str(frame));

			if (state_ == STATE_TASK) {
				sr_.mux_.post(tsk_, 6, cyng::tuple_t{ frame.at(3), frame.at(4) });
			}
			else {
				CYNG_LOG_ERROR(sr_.logger_, "sml.attention.msg - session in wrong state: "
					<< *this
					<< " - "
					<< cyng::io::to_str(frame));
			}
		}


	}
}



