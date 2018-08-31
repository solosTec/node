/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 


#include "session.h"
#include <cyng/io/serializer.h>
#include <cyng/tuple_cast.hpp>
#include <cyng/factory/set_factory.h>
#include <cyng/set_cast.h>

namespace node 
{
	namespace ipt
	{
		session::connect_state::connect_state(session* sp)
			: state_(STATE_OFFLINE)
			, sr_(*sp)
			, tsk_(cyng::async::NO_TASK)
		{}

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

		bool session::connect_state::is_connected() const
		{
			return state_ != STATE_OFFLINE;
		}

		std::ostream& operator<<(std::ostream& os, session::connect_state const& cs)
		{
			switch (cs.state_) {
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
				break;
			}
			return os;
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

			//CYNG_LOG_DEBUG(logger_, "SML message "
			//	<< cyng::io::to_str(msg));

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

		void session::connect_state::sml_get_proc_param_srv_visible(cyng::context& ctx)
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
			//CYNG_LOG_TRACE(sr_.logger_, "sml.get.proc.param.srv.visible "
			//	<< *this
			//	<< " - "
			//	<< cyng::io::to_str(frame));

			if (state_ == STATE_TASK) {
				sr_.mux_.post(tsk_, 5, cyng::tuple_t{ frame.at(3), frame.at(4), frame.at(5), frame.at(6), frame.at(7) });
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
				sr_.mux_.post(tsk_, 5, cyng::tuple_t{ frame.at(3), frame.at(4), frame.at(5), frame.at(6), frame.at(7) });
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
				sr_.mux_.post(tsk_, 5, cyng::tuple_t{ frame.at(3), frame.at(4), frame.at(5), frame.at(6), frame.at(7) });
			}
			else {
				CYNG_LOG_ERROR(sr_.logger_, "sml.get.proc.param.firmware - session in wrong state: "
					<< *this
					<< " - "
					<< cyng::io::to_str(frame));

			}
		}

		void session::connect_state::sml_get_proc_param_simple(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_TRACE(sr_.logger_, "sml.get.proc.param.simple "
				<< *this
				<< " - "
				<< cyng::io::to_str(frame));

		}


	}
}



