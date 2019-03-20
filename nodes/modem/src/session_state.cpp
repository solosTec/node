/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 


#include "session_state.h"
#include "session.h"
#include <smf/ipt/response.hpp>
#include <smf/ipt/scramble_key_io.hpp>
#include <smf/sml/status.h>
#include <smf/sml/srv_id_io.h>
#include <smf/sml/ip_io.h>
#include <smf/mbus/defs.h>
//#include <smf/ipt/defs.h>

#include <cyng/dom/reader.h>
#include <cyng/tuple_cast.hpp>
#include <cyng/numeric_cast.hpp>

#include <boost/assert.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace node 
{
	namespace modem
	{
		session_state::session_state(session* sp, bool auto_answer)
			: sp_(sp)
			, logger_(sp->logger_)
			, auto_answer_(auto_answer)
			, state_(S_IDLE)
			, idle_()
			, authorized_()
			, wait_for_open_response_()
			, wait_for_close_response_()
			, local_()
			, remote_()
			, task_()
		{}

		void session_state::signal_wrong_state(std::string msg)
		{
			CYNG_LOG_ERROR(logger_, sp_->vm().tag() << " unexpected event " << msg  << " in state "<< *this );
		}

		void session_state::transit(internal_state state)
		{
			if (state_ == state) {
				CYNG_LOG_WARNING(logger_, ">> " << sp_->vm().tag() << " no transition " << *this);
			}
			else {
				CYNG_LOG_DEBUG(logger_, ">> " << sp_->vm().tag() << " leave state " << *this);
				state_ = state;
				CYNG_LOG_DEBUG(logger_, ">> " << sp_->vm().tag() << " enter state " << *this);
			}
		}

		void session_state::react(state::evt_init_complete evt)
		{
			switch (state_) {
			case S_IDLE:
				break;
			default:
				signal_wrong_state("evt_init_complete");
				return;
			}

			if (!evt.success_) {
				CYNG_LOG_FATAL(logger_, sp_->vm().tag() << " cannot start gatekeeper");
			}
			else {

				//
				//	store task ID
				//
				idle_.tsk_gatekeeper_ = evt.tsk_;

				//
				//	watchdog in minutes
				//
				//idle_.watchdog_ = evt.watchdog_;
			}

			//sp_->vm_.register_function("session.update.connection.state", 2, std::bind(&session_state::update_connection_state, this, std::placeholders::_1));
			sp_->vm_.register_function("session.redirect", 1, std::bind(&session_state::redirect, this, std::placeholders::_1));
		}

		void session_state::redirect(cyng::context& ctx)
		{
			if (S_AUTHORIZED == state_) {

				const cyng::vector_t frame = ctx.get_frame();
				auto tsk = cyng::value_cast<std::size_t>(frame.at(0), cyng::async::NO_TASK);

				//
				//	update state
				//
				transit(S_CONNECTED_TASK);
				task_.reset(sp_->mux_, tsk);

				CYNG_LOG_INFO(logger_, "session.redirect "
					<< ctx.tag()
					<< ": TASK");
			}
			else {
				CYNG_LOG_WARNING(logger_, "session.redirect "
					<< ctx.tag()
					<< " - session is busy: ");
					//<< connect_state_);
			}
		}


		void session_state::react(state::evt_shutdown)
		{
			switch (state_) {
			case S_CONNECTED_LOCAL:
			case S_CONNECTED_REMOTE:
			case S_CONNECTED_TASK:

				//
				//	clear connection map
				//
				sp_->bus_->vm_.async_run(cyng::generate_invoke("server.connection-map.clear", sp_->vm_.tag()));

				//
				//	note: NO BREAK here - continue with AUTHORIZED state
				//
				//break;

			case S_AUTHORIZED:
				authorized_.stop(sp_->mux_);
				break;

			case S_IDLE:
				idle_.stop(sp_->mux_);
				break;

			default:
				signal_wrong_state("evt_shutdown");
				return;
			}
		}

		void session_state::react(state::evt_activity)
		{
			switch (state_) {
			//case S_IDLE:
			case S_ERROR:
			//case S_SHUTDOWN:
			case S_AUTHORIZED:
			case S_WAIT_FOR_OPEN_RESPONSE:
			case S_WAIT_FOR_CLOSE_RESPONSE:
			case S_CONNECTED_LOCAL:
			case S_CONNECTED_REMOTE:
			case S_CONNECTED_TASK:
				authorized_.activity(sp_->mux_);
				break;
			default:
				break;
			}
		}

		void session_state::react(state::evt_proxy evt)
		{
			switch (state_) {
				//case S_IDLE:
			//case S_ERROR:
				//case S_SHUTDOWN:
			case S_AUTHORIZED:
			//case S_WAIT_FOR_OPEN_RESPONSE:
			//case S_WAIT_FOR_CLOSE_RESPONSE:
			//case S_CONNECTED_LOCAL:
			//case S_CONNECTED_REMOTE:
			case S_CONNECTED_TASK:
				if (authorized_.tsk_proxy_ != cyng::async::NO_TASK) {
					BOOST_ASSERT_MSG(evt.tpl_.size() == 11, "evt_proxy");
					sp_->mux_.post(authorized_.tsk_proxy_, 7u, std::move(evt.tpl_));
				}
				else {
					CYNG_LOG_WARNING(logger_, sp_->vm().tag() << " no proxy task");
				}
				break;
			default:
				signal_wrong_state("evt_proxy");
				break;
			}
		}

		void session_state::react(state::evt_data evt)
		{
			auto ptr = cyng::object_cast<cyng::buffer_t>(evt.obj_);
			BOOST_ASSERT(ptr != nullptr);

#ifdef SMF_IO_LOG
			std::stringstream ss;
			ss
				<< "ipt-rx-"
				<< boost::uuids::to_string(tag)
				<< "-"
				<< std::setw(4)
				<< std::setfill('0')
				<< std::dec
				<< ++log_counter_
				<< ".log"
				;
			const std::string file_name = ss.str();
			std::ofstream of(file_name, std::ios::out | std::ios::app);
			if (of.is_open())
			{
				cyng::io::hex_dump hd;
				auto ptr = cyng::object_cast<cyng::buffer_t>(frame.at(1));
				hd(of, ptr->begin(), ptr->end());

				CYNG_LOG_TRACE(logger_, "write debug log " << file_name);
				of.close();
			}
#endif

			switch (state_) {
				//case S_IDLE:
				//case S_ERROR:
				//case S_SHUTDOWN:
			case S_AUTHORIZED:
				CYNG_LOG_WARNING(logger_, sp_->vm().tag()
					<< " transmit "
					<< ptr->size()
					<< " bytes in connection state authorized");
				local_.transmit(sp_->bus_, sp_->vm().tag(), evt.obj_);
				break;

				//case S_WAIT_FOR_OPEN_RESPONSE:
				//case S_WAIT_FOR_CLOSE_RESPONSE:

			case S_CONNECTED_LOCAL:
				CYNG_LOG_DEBUG(logger_, sp_->vm().tag()
					<< " transmit "
					<< ptr->size()
					<< " bytes to local session");
				local_.transmit(sp_->bus_, sp_->vm().tag(), evt.obj_);
				break;

			case S_CONNECTED_REMOTE:

				if (sp_->bus_->is_online()) {
					CYNG_LOG_DEBUG(logger_, sp_->vm().tag()
						<< " transmit "
						<< ptr->size()
						<< " bytes to remote session");
					remote_.transmit(sp_->bus_, sp_->vm().tag(), evt.obj_);
				}
				else {

					//
					//	cannot deliver data
					//
					CYNG_LOG_WARNING(logger_, "ipt.req.transmit.data "
						<< sp_->vm().tag()
						<< " - no master");
				}
				break;

			case S_CONNECTED_TASK:
				//	send data to task
				CYNG_LOG_DEBUG(logger_, sp_->vm().tag()
					<< " transmit "
					<< ptr->size()
					<< " bytes to task #"
					<< task_.tsk_proxy_);
				BOOST_ASSERT(task_.tsk_proxy_ == authorized_.tsk_proxy_);
				task_.transmit(sp_->mux_, evt.obj_);
				break;

			default:
				signal_wrong_state("evt_data");
				CYNG_LOG_DEBUG(logger_, sp_->vm().tag()
					<< " "
					<< ptr->size()
					<< " bytes get lost");
				break;
			}

			//
			//	update watchdog timer
			//
			authorized_.activity(sp_->mux_);
		}

		cyng::vector_t session_state::react(state::evt_res_login evt)
		{
			//
			//	further actions
			//
			cyng::vector_t prg;

			switch (state_) {
			case S_IDLE:
				break;
			default:
				signal_wrong_state("evt_res_login");
				return prg;
			}

			if (evt.success_) {

				//
				//	update session state
				//
				transit(S_AUTHORIZED);

				//
				//	stop gatekeeper
				//
				if (idle_.stop(sp_->mux_)) {
					CYNG_LOG_TRACE(logger_, sp_->vm().tag()
						<< " gatekeeper stopped");
				}
				else {
					CYNG_LOG_WARNING(logger_, sp_->vm().tag()
						<< " cannot stop gatekeeper");
				}

				//
				//	send login response to device
				//
				prg << cyng::generate_invoke_unwinded("print.ok");

				//
				//	process optional queries
				//
				query(evt.query_);

				//
				//	start proxy
				//
				prg << cyng::generate_invoke_unwinded("session.start.proxy");

			}
			else {

				prg << cyng::generate_invoke_unwinded("print.error");

				//
				//	gatekeeper will terminating this session
				//
			}

			prg << cyng::generate_invoke_unwinded("stream.flush");
			return prg;
		}

		void session_state::query(std::uint32_t query)
		{
			//
			//	further actions
			//
			cyng::param_map_t const bag = cyng::param_map_factory("tp-layer", "modem");

			sp_->bus_->vm_.async_run(client_update_attr(sp_->vm().tag()
				, "TDevice.vFirmware"
				, cyng::make_object(NODE_VERSION)
				, bag));
			sp_->bus_->vm_.async_run(client_update_attr(sp_->vm().tag()
				, "TDevice.id"
				, cyng::make_object("modem")
				, bag));

#ifdef _DEBUG
			sp_->bus_->vm_.async_run(client_update_attr(sp_->vm().tag()
				, "device.time"
				, cyng::make_now()
				, bag));
#endif
		}

		void session_state::react(state::evt_watchdog_started evt)
		{
			if (evt.success_) {
				authorized_.tsk_watchdog_ = evt.tsk_;
			}
			else {
				CYNG_LOG_FATAL(logger_, sp_->vm().tag() << " cannot start watchdog");
			}
		}

		void session_state::react(state::evt_proxy_started evt)
		{
			if (evt.success_) {
				authorized_.tsk_proxy_ = evt.tsk_;

				//
				//	init SML comm
				//
				CYNG_LOG_DEBUG(logger_, sp_->vm().tag() << " initialize SML comm");
				//sp_->proxy_comm_.register_this(sp_->vm());

			}
			else {
				CYNG_LOG_FATAL(logger_, sp_->vm().tag() << " cannot start proxy");
			}
		}

		cyng::vector_t session_state::react(state::evt_req_login evt)
		{
			cyng::vector_t prg;

			switch (state_) {
			case S_IDLE:
				break;
			default:
				signal_wrong_state("evt_req_public");
				return prg;
			}

			CYNG_LOG_INFO(logger_, "ipt.req.login "
				<< evt.tag_
				<< ':'
				<< evt.name_
				<< ':'
				<< evt.pwd_);

			if (sp_->bus_->is_online()) {

				sp_->bus_->vm_.async_run(client_req_login(evt.tag_
					, evt.name_	//	name
					, evt.pwd_	//	pwd
					, "plain" //	login scheme
					, cyng::param_map_factory
						("tp-layer", "modem")
						("security", "public")
						("time", std::chrono::system_clock::now())
						("local-ep", evt.lep_)
						("remote-ep", evt.rep_)));

			}
			else {

				CYNG_LOG_WARNING(logger_, "cannot forward authorization request - no master");

				prg
					<< cyng::generate_invoke_unwinded("print.error")
					<< cyng::generate_invoke_unwinded("stream.flush")
					;

			}

			//
			//	update watchdog timer
			//
			authorized_.activity(sp_->mux_);

			return prg;
		}

		cyng::vector_t session_state::react(state::evt_client_req_open_connection evt)
		{
			cyng::vector_t prg;

			auto const dom = cyng::make_reader(evt.master_);
			auto const target = cyng::value_cast(dom.get("origin-tag"), boost::uuids::nil_uuid());

			switch (state_) {
			case S_AUTHORIZED:
				break;
			default:
				signal_wrong_state("evt_client_req_open_connection");
				if (auto_answer_) {
					sp_->bus_->vm_.async_run(node::client_res_open_connection(target
						, evt.seq_	//	sequence
						, false		//	no success
						, evt.master_
						, evt.client_));
				}
				return prg;
			}


			if (auto_answer_) {

				CYNG_LOG_INFO(logger_, "incoming call - auto answer mode is ON");

				//
				//	update parser state
				//
				sp_->parser_.set_stream_mode();

				//
				//	print response
				//
				prg
					<< cyng::generate_invoke_unwinded("print.connect")
					;

				//
				//	send connection open response response
				//			
				sp_->bus_->vm_.async_run(node::client_res_open_connection(target
					, evt.seq_	//	sequence
					, true		//	success
					, evt.master_
					, evt.client_));

				//
				//	update session state
				//
				transit(evt.local_ ? S_CONNECTED_LOCAL : S_CONNECTED_REMOTE);

			}
			else {
				//
				//	update state
				//
				transit(S_WAIT_FOR_OPEN_RESPONSE);

				//
				//	ring ring
				//
				CYNG_LOG_INFO(logger_, "incoming call - wait for ATA");
				prg
					<< cyng::generate_invoke_unwinded("print.ring")
					;

				wait_for_open_response_.init(evt.local_, evt.seq_, evt.master_, evt.client_);
			}

			return prg;
		}

		cyng::vector_t session_state::react(state::evt_client_res_close_connection evt)
		{
			cyng::vector_t prg;

			switch (state_) {
			case S_WAIT_FOR_CLOSE_RESPONSE:
				break;
			default:
				signal_wrong_state("evt_client_res_close_connection");
				return prg;
			}


			//
			//	send response
			//
			if (evt.success_) {
				prg
					<< cyng::generate_invoke_unwinded("print.ok")
					;
			}
			else {
				prg
					<< cyng::generate_invoke_unwinded("print.error")
					;

			}

			prg
				<< cyng::generate_invoke_unwinded("stream.flush")
				;

			//
			//	dom reader
			//
			auto dom = cyng::make_reader(evt.client_);

			//
			//	check lag/response time
			//
			const auto start = cyng::value_cast(dom.get("start"), std::chrono::system_clock::now());
			auto lag = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);

			if (lag > sp_->timeout_) {
				CYNG_LOG_WARNING(logger_, "client.req.close.connection.forward - high cluster lag: " << lag.count() << " millisec");
			}
			else {
				CYNG_LOG_TRACE(logger_, "client.req.close.connection.forward - cluster lag: " << lag.count() << " millisec");
			}

			//
			//	reset connection state
			//
			transit(S_AUTHORIZED);
			wait_for_close_response_.reset();

			return prg;
		}

		cyng::vector_t session_state::react(state::evt_client_res_open_connection evt)
		{
			cyng::vector_t prg;

			switch (state_) {
			//case S_WAIT_FOR_OPEN_RESPONSE:
			case S_AUTHORIZED:
				break;
			default:
				signal_wrong_state("evt_client_res_open_connection");
				return prg;
			}

			//
			//	dom reader
			//
			auto const dom = cyng::make_reader(evt.client_);

			//
			//	check lag/response time
			//
			const auto start = cyng::value_cast(dom.get("start"), std::chrono::system_clock::now());
			auto lag = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);

			if (lag > sp_->timeout_) {
				CYNG_LOG_WARNING(logger_, "client.res.open.connection.forward "
					<< " - high cluster lag: " << lag.count() << " millisec");
			}
			else {
				CYNG_LOG_TRACE(logger_, "client.res.open.connection.forward "
					<< " - cluster lag: " << lag.count() << " millisec");
			}

			if (evt.success_)
			{
				//
				//	hides outer variable dom!
				//
				auto const dom = cyng::make_reader(evt.master_);
				const auto local = cyng::value_cast(dom.get("local-connect"), false);

				CYNG_LOG_TRACE(logger_, sp_->vm().tag()
					<< " set connection state to "
					<< (local ? "local" : "remote"));

				//
				//	update parser state
				//
				sp_->parser_.set_stream_mode();

				//
				//	print response
				//
				prg
					<< cyng::generate_invoke_unwinded("print.connect")
					;
				//
				//	update state
				//
				transit(local ? S_CONNECTED_LOCAL : S_CONNECTED_REMOTE);
			}
			else
			{
				//
				//	print response
				//
				prg
					<< cyng::generate_invoke_unwinded("print.no-answer")
					;

				//
				//	update state
				//	To reset the session state to AUTHORIZED reflect the fact that this session is no longer part in any
				//	attempt to establish a connection.
				//
				transit(S_AUTHORIZED);
			}

			prg << cyng::generate_invoke_unwinded("stream.flush");


			return prg;
		}

		cyng::vector_t session_state::react(state::evt_modem_req_close_connection evt)
		{
			cyng::vector_t prg;

			switch (state_) {
			case S_CONNECTED_LOCAL:
			case S_CONNECTED_REMOTE:
				transit(S_WAIT_FOR_CLOSE_RESPONSE);
				break;
			default:
				signal_wrong_state("evt_modem_req_close_connection");
				return prg;
			}


			if (sp_->bus_->is_online())
			{
				CYNG_LOG_TRACE(logger_, sp_->vm().tag() << " received connection close request");
				sp_->bus_->vm_.async_run(node::client_req_close_connection(sp_->vm().tag()
					, false //	no shutdown
					, cyng::param_map_factory("tp-layer", "modem")("origin-tag", sp_->vm().tag())("start", std::chrono::system_clock::now())));
			}
			else
			{
				CYNG_LOG_WARNING(logger_, sp_->vm().tag() << " received connection close request - no master");
				prg
					<< cyng::generate_invoke_unwinded("print.error")
					<< cyng::generate_invoke_unwinded("stream.flush")
					;
			}

			//
			//	update watchdog timer
			//
			authorized_.activity(sp_->mux_);

			return prg;
		}

		cyng::vector_t session_state::react(state::evt_modem_res_open_connection evt)
		{
			cyng::vector_t prg;

			switch (state_) {
			case S_WAIT_FOR_OPEN_RESPONSE:
				break;
			default:
				signal_wrong_state("evt_modem_res_open_connection");
				return prg;
			}

			//
			//	stop open connection task
			//
			//BOOST_ASSERT(evt.tsk_ == wait_for_open_response_.tsk_connection_open_);
			//sp_->mux_.post(evt.tsk_, evt.slot_, cyng::tuple_factory(evt.res_));

			//
			//	update session state
			//
			//if (ipt::tp_res_open_connection_policy::is_success(evt.res_)) {
			//	switch (wait_for_open_response_.type_) {
			//	case state::state_wait_for_open_response::E_LOCAL:
			//		sp_->bus_->vm_.async_run(wait_for_open_response_.establish_local_connection());
			//		transit(S_CONNECTED_LOCAL);
			//		break;
			//	case state::state_wait_for_open_response::E_REMOTE:
			//		transit(S_CONNECTED_REMOTE);
			//		break;
			//	default:
			//		BOOST_ASSERT_MSG(false, "undefined connection state");
			//		break;
			//	}
			//}
			//else {

			//	//
			//	//	fallback to authorized
			//	//
			//	transit(S_AUTHORIZED);
			//}
			
			if (sp_->bus_->is_online()) {
				//sp_->bus_->vm_.async_run(client_res_open_connection(wait_for_open_response_.get_origin_tag()
				//	, wait_for_open_response_.seq_	//	cluster sequence
				//	, ipt::tp_res_open_connection_policy::is_success(evt.res_)
				//	, wait_for_open_response_.master_params_
				//	, wait_for_open_response_.client_params_));
			}

			//
			//	reset old state
			//
			wait_for_open_response_.reset();

			//
			//	update watchdog timer
			//
			authorized_.activity(sp_->mux_);

			return prg;
		}

		cyng::vector_t session_state::react(state::evt_modem_res_close_connection evt)
		{
			cyng::vector_t prg;

			switch (state_) {
			case S_WAIT_FOR_CLOSE_RESPONSE:
				break;
			default:
				signal_wrong_state("evt_modem_res_close_connection");
				return prg;
			}

			//
			//	stop close connection task
			//
			BOOST_ASSERT(evt.tsk_ == wait_for_close_response_.tsk_connection_close_);
			//sp_->mux_.post(evt.tsk_, evt.slot_, cyng::tuple_factory(evt.res_));

			//
			//	update master
			//
			//wait_for_close_response_.terminate(sp_->bus_, evt.res_);

			//
			//	update state
			//
			transit(S_AUTHORIZED);
			//wait_for_close_response_.reset();

			//
			//	update watchdog timer
			//
			authorized_.activity(sp_->mux_);

			return prg;
		}

		cyng::vector_t session_state::react(state::evt_client_req_close_connection evt)
		{
			cyng::vector_t prg;

			switch (state_) {
			case S_CONNECTED_LOCAL:
			case S_CONNECTED_REMOTE:
				break;
			default:
				signal_wrong_state("evt_client_req_close_connection");
				return prg;
			}

			//
			//	print NO CARRIER
			//
			prg
				<< cyng::generate_invoke_unwinded("print.no-carrier")
				<< cyng::generate_invoke_unwinded("stream.flush")
				;

			//
			//	update parser state
			//
			sp_->parser_.set_cmd_mode();

			//
			//	initialize next state
			//
			//wait_for_close_response_.init(evt.tsk_, evt.tag_, evt.shutdown_, evt.seq_, evt.master_, evt.client_, state_ == S_CONNECTED_LOCAL);


			if (!evt.shutdown_)
			{
				//
				//	don't send response in shutdown mode
				//

				sp_->bus_->vm_.async_run(client_res_close_connection(sp_->vm().tag()
					, evt.seq_	//	cluster sequence
					, true	//	success
					, evt.master_
					, evt.client_));
			}

			//
			//	update state
			//
			transit(S_AUTHORIZED);

			return prg;
		}

		cyng::vector_t session_state::react(state::evt_info evt)
		{
			cyng::vector_t prg;

			switch (state_) {
			case S_AUTHORIZED:
				break;
			default:
				signal_wrong_state("evt_info");
				return prg;
			}

			switch (evt.code_)
			{
			case 1:
				prg << cyng::generate_invoke_unwinded("print.text", cyng::invoke("ip.tcp.socket.ep.local"));
				break;
			case 2:
				prg << cyng::generate_invoke_unwinded("print.text", cyng::invoke("ip.tcp.socket.ep.remote"));
				break;
			case 3:
				prg << cyng::generate_invoke_unwinded("print.text", std::chrono::system_clock::now());
				break;
			case 4:
				prg << cyng::generate_invoke_unwinded("print.text", NODE_VERSION);
				break;
			case 5:
				prg << cyng::generate_invoke_unwinded("print.text", NODE_PLATFORM);
				break;
			case 6:
				prg << cyng::generate_invoke_unwinded("print.text", NODE_PROCESSOR);
				break;
			case 7:
				prg << cyng::generate_invoke_unwinded("print.text", NODE_SSL_VERSION);
				break;
			case 8:
				prg << cyng::generate_invoke_unwinded("print.text", NODE_BUILD_TYPE);
				break;
			case 9:
				prg << cyng::generate_invoke_unwinded("print.text", sp_->vm().tag());
				break;
			case 10:
				prg << cyng::generate_invoke_unwinded("print.text", sp_->bus_->vm_.tag());
				break;
			case 11:
			{
				std::stringstream ss;
				ss << state_;
				prg << cyng::generate_invoke_unwinded("print.text", ss.str());
			}
				break;
			case 12:
			{
				std::stringstream ss;
				ss << *this;
				prg << cyng::generate_invoke_unwinded("print.text", ss.str());
			}
				break;
			case 13:
				//	auto answer ON/OFF
				prg << cyng::generate_invoke_unwinded("print.text", (auto_answer_ ? "ON" : "OFF"));
				break;

			default:
				break;
			}

			prg
				<< cyng::generate_invoke_unwinded("print.ok")
				<< cyng::generate_invoke_unwinded("stream.flush")
				;
			return prg;
		}

		void session_state::react(state::evt_sml_msg evt)
		{
			switch (state_) {
			case S_CONNECTED_TASK:
				break;
			default:
				signal_wrong_state("evt_sml_msg");
				return;
			}

			CYNG_LOG_DEBUG(logger_, "SML message to #"
				<< task_.tsk_proxy_
				<< " - "
				<< cyng::io::to_str(evt.msg_));

			//
			//	message slot (3)
			//
			BOOST_ASSERT_MSG(evt.msg_.size() == 5, "evt_sml_msg");
			sp_->mux_.post(task_.tsk_proxy_, 3, std::move(evt.msg_));
		}

		void session_state::react(state::evt_sml_eom evt)
		{
			switch (state_) {
			case S_CONNECTED_TASK:
				break;
			default:
				signal_wrong_state("evt_sml_eom");
				return;
			}

			CYNG_LOG_DEBUG(logger_, "evt_sml_eom to #" << task_.tsk_proxy_);

			//
			//	message slot (1)
			//
			BOOST_ASSERT_MSG(evt.tpl_.size() == 2, "evt_sml_eom");
			sp_->mux_.post(task_.tsk_proxy_, 1, std::move(evt.tpl_));
		}

		void session_state::react(state::evt_sml_public_close_response evt)
		{
			switch (state_) {
			case S_CONNECTED_TASK:
				break;
			default:
				signal_wrong_state("evt_sml_public_close_response");
				return;
			}

			CYNG_LOG_DEBUG(logger_, "evt_sml_public_close_response to #" << task_.tsk_proxy_);

			//
			//	message slot (4)
			//
			BOOST_ASSERT_MSG(evt.tpl_.size() == 3, "evt_sml_public_close_response");
			sp_->mux_.post(task_.tsk_proxy_, 4, std::move(evt.tpl_));

			//
			//	disconnect task from data stream
			//
			transit(S_AUTHORIZED);

		}

		void session_state::react(state::evt_sml_get_proc_param_srv_visible evt)
		{
			switch (state_) {
			case S_CONNECTED_TASK:
				break;
			default:
				signal_wrong_state("evt_sml_get_proc_param_srv_visible");
				return;
			}

			//BOOST_ASSERT(evt.vec_.size() == 8);
			CYNG_LOG_DEBUG(logger_, "evt_sml_get_proc_param_srv_visible to #" << task_.tsk_proxy_);

			//
			//	message slot (5)
			//
			task_.get_proc_param_srv_device(sp_->mux_, evt.vec_);
		}

		void session_state::react(state::evt_sml_get_proc_param_srv_active evt)
		{
			switch (state_) {
			case S_CONNECTED_TASK:
				break;
			default:
				signal_wrong_state("evt_sml_get_proc_param_srv_active");
				return;
			}

			BOOST_ASSERT(evt.vec_.size() == 9);
			CYNG_LOG_DEBUG(logger_, "evt_sml_get_proc_param_srv_active to #" << task_.tsk_proxy_);

			//
			//	message slot (5)
			//
			task_.get_proc_param_srv_device(sp_->mux_, evt.vec_);
		}

		void session_state::react(state::evt_sml_get_proc_param_firmware evt)
		{
			switch (state_) {
			case S_CONNECTED_TASK:
				break;
			default:
				signal_wrong_state("evt_sml_get_proc_param_firmware");
				return;
			}

			BOOST_ASSERT(evt.vec_.size() == 9);
			CYNG_LOG_DEBUG(logger_, "evt_sml_get_proc_param_firmware to #" << task_.tsk_proxy_);

			//
			//	message slot (5)
			//
			task_.get_proc_param_srv_firmware(sp_->mux_, evt.vec_);
		}

		void session_state::react(state::evt_sml_get_proc_param_status_word evt)
		{
			switch (state_) {
			case S_CONNECTED_TASK:
				break;
			default:
				signal_wrong_state("evt_sml_get_proc_param_status_word");
				return;
			}

			//BOOST_ASSERT(evt.vec_.size() == 8);
			CYNG_LOG_DEBUG(logger_, "evt_sml_get_proc_param_status_word to #" << task_.tsk_proxy_);

			//
			//	message slot (5)
			//
			task_.get_proc_param_status_word(sp_->mux_, evt.vec_);
		}

		void session_state::react(state::evt_sml_get_proc_param_memory evt)
		{
			switch (state_) {
			case S_CONNECTED_TASK:
				break;
			default:
				signal_wrong_state("evt_sml_get_proc_param_memory");
				return;
			}

			BOOST_ASSERT(evt.vec_.size() == 7);
			CYNG_LOG_DEBUG(logger_, "evt_sml_get_proc_param_memory to #" << task_.tsk_proxy_);

			//
			//	message slot (5)
			//
			task_.get_proc_param_memory(sp_->mux_, evt.vec_);

		}

		void session_state::react(state::evt_sml_get_proc_param_wmbus_status evt)
		{
			switch (state_) {
			case S_CONNECTED_TASK:
				break;
			default:
				signal_wrong_state("evt_sml_get_proc_param_wmbus_status");
				return;
			}

			//BOOST_ASSERT(evt.vec_.size() == 8);
			CYNG_LOG_DEBUG(logger_, "evt_sml_get_proc_param_wmbus_status to #" << task_.tsk_proxy_);

			//
			//	message slot (5)
			//
			task_.get_proc_param_wmbus_status(sp_->mux_, evt.vec_);

		}

		void session_state::react(state::evt_sml_get_proc_param_wmbus_config evt)
		{
			switch (state_) {
			case S_CONNECTED_TASK:
				break;
			default:
				signal_wrong_state("evt_sml_get_proc_param_wmbus_config");
				return;
			}

			//BOOST_ASSERT(evt.vec_.size() == 8);
			CYNG_LOG_DEBUG(logger_, "evt_sml_get_proc_param_wmbus_config to #" << task_.tsk_proxy_);

			//
			//	message slot (5)
			//
			task_.get_proc_param_wmbus_config(sp_->mux_, evt.vec_);

		}

		void session_state::react(state::evt_sml_get_proc_param_ipt_status evt)
		{
			switch (state_) {
			case S_CONNECTED_TASK:
				break;
			default:
				signal_wrong_state("evt_sml_get_proc_param_ipt_status");
				return;
			}

			//BOOST_ASSERT(evt.vec_.size() == 8);
			CYNG_LOG_DEBUG(logger_, "evt_sml_get_proc_param_ipt_status to #" << task_.tsk_proxy_);

			//
			//	message slot (5)
			//
			task_.get_proc_param_ipt_status(sp_->mux_, evt.vec_);

		}

		void session_state::react(state::evt_sml_get_proc_param_ipt_param evt)
		{
			switch (state_) {
			case S_CONNECTED_TASK:
				break;
			default:
				signal_wrong_state("evt_sml_get_proc_param_ipt_param");
				return;
			}

			BOOST_ASSERT(evt.vec_.size() == 11);
			CYNG_LOG_DEBUG(logger_, "evt_sml_get_proc_param_ipt_param to #" << task_.tsk_proxy_);

			//
			//	message slot (5)
			//
			task_.get_proc_param_ipt_param(sp_->mux_, evt.vec_);

		}

		void session_state::react(state::evt_sml_get_list_response evt)
		{
			switch (state_) {
			case S_CONNECTED_TASK:
				break;
			default:
				signal_wrong_state("evt_sml_get_list_response");
				return;
			}

			BOOST_ASSERT(evt.vec_.size() == 9);
			//	[7dee1864-70a4-4029-9787-c5cae1ec52eb,4061719-2,0,,01E61E130900163C07,990000000003,%(("08 00 01 00 00 ff":0.758),("08 00 01 02 00 ff":0.758)),null,06988c71]
			CYNG_LOG_DEBUG(logger_, "evt_sml_get_list_response to #" << task_.tsk_proxy_);
			CYNG_LOG_DEBUG(logger_, "get_list_response: "	<< cyng::io::to_str(evt.vec_));

			//
			//	message slot (5)
			//
			task_.get_list_response(sp_->mux_, evt.vec_);

		}

		void session_state::react(state::evt_sml_attention_msg evt)
		{
			switch (state_) {
			case S_CONNECTED_TASK:
				break;
			default:
				signal_wrong_state("evt_sml_attention_msg");
				return;
			}

			//BOOST_ASSERT(evt.vec_.size() == 8);
			CYNG_LOG_DEBUG(logger_, "evt_sml_attention_msg to #" << task_.tsk_proxy_);

			//
			//	message slot (6)
			//
			task_.attention_msg(sp_->mux_, evt.vec_);

		}

		std::ostream& operator<<(std::ostream& os, session_state const& st)
		{
			switch (st.state_) {
			case session_state::S_IDLE:
				os << "IDLE";
				break;
			case session_state::S_ERROR:
				os << "ERROR";
				break;
			case session_state::S_SHUTDOWN:
				os << "SHUTDOWN";
				break;
			case session_state::S_AUTHORIZED:
				os << "AUTHORIZED";
				break;
			case session_state::S_WAIT_FOR_OPEN_RESPONSE:
				os << "WAIT_FOR_OPEN_RESPONSE";
				break;
			case session_state::S_WAIT_FOR_CLOSE_RESPONSE:
				os << "WAIT_FOR_CLOSE_RESPONSE";
				break;
			case session_state::S_CONNECTED_LOCAL:
				os << "CONNECTED_LOCAL";
				break;
			case session_state::S_CONNECTED_REMOTE:
				os << "CONNECTED_REMOTE";
				break;
			case session_state::S_CONNECTED_TASK:
				os << "CONNECTED_TASK";
				break;
			default:
				break;
			}
			return os;
		}


		//
		//	events are located in a separate namespace
		//
		namespace state
		{
			evt_init_complete::evt_init_complete(std::pair<std::size_t, bool> r)
				: tsk_(r.first)
				, success_(r.second)
			{}

			evt_watchdog_started::evt_watchdog_started(std::pair<std::size_t, bool> r)
				: tsk_(r.first)
			, success_(r.second)
			{}

			evt_proxy_started::evt_proxy_started(std::pair<std::size_t, bool> r)
				: tsk_(r.first)
				, success_(r.second)
			{}

			evt_req_login::evt_req_login(std::tuple
				<
				boost::uuids::uuid,		//	[0] peer tag
				std::string,			//	[1] name
				std::string,			//	[2] pwd
				boost::asio::ip::tcp::endpoint,	//	[3] local ep
				boost::asio::ip::tcp::endpoint	//	[4] remote ep
				> tpl)
			: tag_(std::get<0>(tpl))
				, name_(std::get<1>(tpl))
				, pwd_(std::get<2>(tpl))
				, lep_(std::get<3>(tpl))
				, rep_(std::get<4>(tpl))
			{}


			//
			//	EVENT: login response
			//
			evt_res_login::evt_res_login(std::tuple<
				boost::uuids::uuid,		//	[0] peer tag
				std::uint64_t,			//	[1] sequence number
				bool,					//	[2] success
				std::string,			//	[3] name
				std::string,			//	[4] msg
				std::uint32_t,			//	[5] query
				cyng::param_map_t		//	[6] client
			> tpl)
				: tag_(std::get<0>(tpl))
				, seq_(std::get<1>(tpl))
				, success_(std::get<2>(tpl))
				, name_(std::get<3>(tpl))
				, msg_(std::get<4>(tpl))
				, query_(std::get<5>(tpl))
				, bag_(std::get<6>(tpl))
			{}


			//
			//	EVENT: IP-T connection open request
			//
			evt_ipt_req_open_connection::evt_ipt_req_open_connection(boost::uuids::uuid tag
				//, sequence_type seq
				, std::string number)
				: tag_(tag)
				//, seq_(seq)
				, number_(number)
			{}


			//
			//	EVENT: IP-T connection open response
			//
			evt_modem_res_open_connection::evt_modem_res_open_connection(std::size_t tsk, std::size_t slot/*, response_type res*/)
				: tsk_(tsk)
			, slot_(slot)
			//, res_(res)
			{}


			//
			//	EVENT: modem connection close request
			//
			evt_modem_req_close_connection::evt_modem_req_close_connection(boost::uuids::uuid tag)		//	session tag
			: tag_(tag)
			{}

			//
			//	EVENT: IP-T connection close response
			//
			evt_modem_res_close_connection::evt_modem_res_close_connection(std::size_t tsk, std::size_t slot/*, response_type res*/)
				: tsk_(tsk)
				, slot_(slot)
				//, res_(res)
			{}


			//
			//	EVENT: incoming connection open request
			//
			evt_client_req_open_connection::evt_client_req_open_connection(bool local
				, std::size_t seq
				, cyng::param_map_t master
				, cyng::param_map_t client)
			: local_(local)
				, seq_(seq)
				, master_(master)
				, client_(client)
			{}

			//
			//	EVENT: incoming connection open response
			//
			evt_client_res_open_connection::evt_client_res_open_connection(std::tuple<
				boost::uuids::uuid,		//	[0] peer
				std::uint64_t,			//	[1] cluster sequence
				bool,					//	[2] success
				cyng::param_map_t,		//	[3] options
				cyng::param_map_t		//	[4] bag
			> tpl)
			: peer_(std::get<0>(tpl))
				, seq_(std::get<1>(tpl))
				, success_(std::get<2>(tpl))
				, master_(std::get<3>(tpl))
				, client_(std::get<4>(tpl))
			{}


			//
			//	EVENT: incoming connection close request
			//
			evt_client_req_close_connection::evt_client_req_close_connection(std::tuple<
				boost::uuids::uuid,		//	[0] peer
				std::uint64_t,			//	[1] cluster sequence
				boost::uuids::uuid,		//	[2] origin-tag
				bool,					//	[3] shutdown flag
				cyng::param_map_t,		//	[4] options
				cyng::param_map_t		//	[5] bag
			> tpl)	
			: seq_(std::get<1>(tpl))
				, tag_(std::get<2>(tpl))
				, shutdown_(std::get<3>(tpl))
				, master_(std::get<4>(tpl))
				, client_(std::get<5>(tpl))
			{}

			//
			//	EVENT: incoming connection close response
			//
			evt_client_res_close_connection::evt_client_res_close_connection(std::tuple<
				boost::uuids::uuid,		//	[0] peer
				std::uint64_t,			//	[1] cluster sequence
				bool,					//	[2] success flag from remote session
				cyng::param_map_t,		//	[3] master data
				cyng::param_map_t		//	[4] client data
			> tpl)
			: peer_(std::get<0>(tpl))
				, seq_(std::get<1>(tpl))
				, success_(std::get<2>(tpl))
				, master_(std::get<3>(tpl))
				, client_(std::get<4>(tpl))
			{}

			//
			//	EVENT: redirect communication
			//
			evt_redirect::evt_redirect()
			{}


			//
			//	EVENT: shutdown
			//
			evt_shutdown::evt_shutdown()
			{}


			//
			//	EVENT: transmit data
			//
			evt_data::evt_data(cyng::object obj)
				: obj_(obj)
			{}

			//
			//	EVENT: activity
			//
			evt_activity::evt_activity()
			{}

			//
			//	EVENT: proxy
			//
			evt_proxy::evt_proxy(cyng::tuple_t&& tpl)
				: tpl_(std::forward<cyng::tuple_t>(tpl))
			{}

			//
			//	EVENT: SML message
			//
			evt_sml_msg::evt_sml_msg(std::size_t idx, cyng::tuple_t msg)
				: idx_(idx)
				, msg_(msg)
			{}

			//
			//	EVENT: SML EOM
			//
			evt_sml_eom::evt_sml_eom(cyng::tuple_t tpl)
				: tpl_(tpl)
			{}

			//
			//	EVENT: sml_public_close_response
			//
			evt_sml_public_close_response::evt_sml_public_close_response(cyng::tuple_t tpl)
				: tpl_(tpl)
			{}

			//
			//	EVENT: sml_get_proc_param_srv_visible
			//
			evt_sml_get_proc_param_srv_visible::evt_sml_get_proc_param_srv_visible(cyng::vector_t vec)
				: vec_(vec)
			{}

			//
			//	EVENT: sml_get_proc_param_srv_active
			//
			evt_sml_get_proc_param_srv_active::evt_sml_get_proc_param_srv_active(cyng::vector_t vec)
				: vec_(vec)
			{}

			//
			//	EVENT: sml_get_proc_param_firmware
			//
			evt_sml_get_proc_param_firmware::evt_sml_get_proc_param_firmware(cyng::vector_t vec)
				: vec_(vec)
			{}

			//
			//	EVENT: sml_get_proc_param_status_word
			//
			evt_sml_get_proc_param_status_word::evt_sml_get_proc_param_status_word(cyng::vector_t vec)
				: vec_(vec)
			{}
			
			//
			//	EVENT: sml_get_proc_param_memory
			//
			evt_sml_get_proc_param_memory::evt_sml_get_proc_param_memory(cyng::vector_t vec)
				: vec_(vec)
			{}

			//
			//	EVENT: evt_sml_get_proc_param_wmbus_status
			//
			evt_sml_get_proc_param_wmbus_status::evt_sml_get_proc_param_wmbus_status(cyng::vector_t vec)
				: vec_(vec)
			{}

			//
			//	EVENT: evt_sml_get_proc_param_wmbus_config
			//
			evt_sml_get_proc_param_wmbus_config::evt_sml_get_proc_param_wmbus_config(cyng::vector_t vec)
				: vec_(vec)
			{}

			//
			//	EVENT: evt_sml_get_proc_param_ipt_status
			//
			evt_sml_get_proc_param_ipt_status::evt_sml_get_proc_param_ipt_status(cyng::vector_t vec)
				: vec_(vec)
			{}

			//
			//	EVENT: evt_sml_get_proc_param_ipt_param
			//
			evt_sml_get_proc_param_ipt_param::evt_sml_get_proc_param_ipt_param(cyng::vector_t vec)
				: vec_(vec)
			{}

			//
			//	EVENT: evt_sml_get_list_response
			//
			evt_sml_get_list_response::evt_sml_get_list_response(cyng::vector_t vec)
				: vec_(vec)
			{}

			//
			//	EVENT: evt_sml_attention_msg
			//
			evt_sml_attention_msg::evt_sml_attention_msg(cyng::vector_t vec)
				: vec_(vec)
			{}

			//
			//	EVENT: evt_info
			//
			evt_info::evt_info(std::uint32_t code)
				: code_(code)
			{}

			//
			//	STATE: idle
			//
			state_idle::state_idle()
				: tsk_gatekeeper_(cyng::async::NO_TASK)
			{}

			bool state_idle::stop(cyng::async::mux& mux)
			{
				if (cyng::async::NO_TASK != tsk_gatekeeper_) {
					return mux.stop(tsk_gatekeeper_);
				}
				return false;
			}


			//
			//	STATE: authorized
			//
			state_authorized::state_authorized()
				: tsk_watchdog_(cyng::async::NO_TASK)
				, tsk_proxy_(cyng::async::NO_TASK)

			{}

			void state_authorized::stop(cyng::async::mux& mux)
			{
				if (cyng::async::NO_TASK != tsk_watchdog_) mux.stop(tsk_watchdog_);
				if (cyng::async::NO_TASK != tsk_proxy_)	mux.stop(tsk_proxy_);
			}

			void state_authorized::activity(cyng::async::mux& mux)
			{
				if (cyng::async::NO_TASK != tsk_watchdog_)	mux.post(tsk_watchdog_, 0u, cyng::tuple_t());
			}

			//
			//	STATE: state_wait_for_open_response
			//
			state_wait_for_open_response::state_wait_for_open_response()
			: type_(E_UNDEF)
				, seq_(0u)
				, master_params_()
				, client_params_()
			{}

			void state_wait_for_open_response::init(bool local
				, std::uint64_t seq
				, cyng::param_map_t master
				, cyng::param_map_t client)
			{
				type_ = local ? E_LOCAL : E_REMOTE;
				seq_ = seq;
				master_params_ = master;
				client_params_ = client;
			}

			cyng::vector_t state_wait_for_open_response::establish_local_connection() const
			{
				auto const dom = cyng::make_reader(master_params_);

				auto const origin = cyng::value_cast(dom.get("origin-tag"), boost::uuids::nil_uuid());
				auto const remote = cyng::value_cast(dom.get("remote-tag"), boost::uuids::nil_uuid());
				BOOST_ASSERT(!origin.is_nil());
				BOOST_ASSERT(!remote.is_nil());
				BOOST_ASSERT(origin != remote);
				return cyng::generate_invoke("server.connection-map.insert", origin, remote);
			}

			boost::uuids::uuid state_wait_for_open_response::get_origin_tag() const
			{
				auto const dom = cyng::make_reader(master_params_);
				return cyng::value_cast(dom.get("origin-tag"), boost::uuids::nil_uuid());
			}

			void state_wait_for_open_response::reset()
			{
				type_ = E_UNDEF;
				seq_ = 0u;
				master_params_.clear();
				client_params_.clear();
			}

			//
			//	STATE: state_wait_for_close_response
			//
			state_wait_for_close_response::state_wait_for_close_response()
			: tsk_connection_close_(cyng::async::NO_TASK)
				, tag_(boost::uuids::nil_uuid())
				, shutdown_(false)
				, seq_(0u)
				, master_params_()
				, client_params_()
				//, local_(false)
			{}

			void state_wait_for_close_response::init(std::size_t tsk
				, boost::uuids::uuid tag
				, bool shutdown
				, std::uint64_t seq
				, cyng::param_map_t master
				, cyng::param_map_t client
				, bool local)
			{
				tsk_connection_close_ = tsk;
				tag_ = tag;
				shutdown_ = shutdown;
				seq_ = seq;
				master_params_ = master;
				client_params_ = client;

				//	otherwise connection type is inconsistent
				BOOST_ASSERT(is_connection_local() == local);
			}

			void state_wait_for_close_response::reset()
			{
				tsk_connection_close_ = cyng::async::NO_TASK;
				tag_ = boost::uuids::nil_uuid();
				shutdown_ = false;
				seq_ = 0u;
				master_params_.clear();
				client_params_.clear();
			}

			bool state_wait_for_close_response::is_connection_local() const
			{
				auto const dom = cyng::make_reader(master_params_);
				return cyng::value_cast(dom.get("local-connect"), false);
			}

			void state_wait_for_close_response::terminate(bus::shared_type bus/*, response_type res*/)
			{
				if (!shutdown_ && bus->is_online())
				{
					if (is_connection_local()) {

						//
						//	remove from connection_map_
						//
						bus->vm_.async_run(cyng::generate_invoke("server.connection-map.clear", tag_));
					}

					//	send "client.res.close.connection" to SMF master
					//bus->vm_.async_run(client_res_close_connection(tag_
					//	, seq_
					//	, ipt::tp_res_close_connection_policy::is_success(res)
					//	, master_params_
					//	, client_params_));
				}

				//
				//	reset data
				//
				reset();
			}

			//
			//	STATE: connected_local
			//
			state_connected_local::state_connected_local()
			{}

			void state_connected_local::transmit(bus::shared_type bus, boost::uuids::uuid tag, cyng::object obj)
			{
				bus->vm_.async_run(cyng::generate_invoke("server.transmit.data", tag, obj));
			}

			//
			//	STATE: connected_remote
			//
			state_connected_remote::state_connected_remote()
			{}
			void state_connected_remote::transmit(bus::shared_type bus, boost::uuids::uuid tag, cyng::object obj)
			{
				bus->vm_.async_run(client_req_transmit_data(tag
					, cyng::param_map_factory("tp-layer", "modem")("start", std::chrono::system_clock::now())
					, obj));
			}

			//
			//	STATE: connected_task
			//
			state_connected_task::state_connected_task()
				: tsk_proxy_(cyng::async::NO_TASK)
			{}

			void state_connected_task::reset(cyng::async::mux& mux, std::size_t tsk)
			{
				tsk_proxy_ = tsk;
				mux.post(tsk, 0, cyng::tuple_t{});
			}

			void state_connected_task::transmit(cyng::async::mux& mux, cyng::object obj)
			{
				mux.post(tsk_proxy_, 2, cyng::tuple_factory(obj));
			}

			void state_connected_task::get_proc_param_srv_device(cyng::async::mux& mux, cyng::vector_t vec)
			{
				cyng::buffer_t meter;
				meter = cyng::value_cast(vec.at(6), meter);

				auto const type = node::sml::get_srv_type(meter);
				std::string maker;
				if (type == node::sml::SRV_MBUS) {
					auto const code = node::sml::get_manufacturer_code(meter);
					maker = node::sml::decode(code);
				}

				mux.post(tsk_proxy_, 5, cyng::tuple_t{
					vec.at(1),	//	trx
					vec.at(2),	//	idx
					vec.at(3),	//	server ID
					vec.at(4),	//	OBIS code
								//	visible/active device
					cyng::param_map_factory("number", vec.at(5))
						("ident", node::sml::from_server_id(meter))
						("meter", node::sml::from_server_id(meter))
						("meterId", meter)
						("class", vec.at(7))
						("timestamp", vec.at(8))
						("type", node::sml::get_srv_type(meter))
						("maker", maker)
					()
				});

			}

			void state_connected_task::get_proc_param_srv_firmware(cyng::async::mux& mux, cyng::vector_t vec)
			{
				mux.post(tsk_proxy_, 5, cyng::tuple_t{
					vec.at(1),	//	trx
					vec.at(2),	//	idx
					vec.at(3),	//	server ID
					vec.at(4),	//	OBIS code
								//	firmware
					cyng::param_map_factory("number", vec.at(5))
						("firmware", vec.at(6))
						("version", vec.at(7))
						("active", vec.at(8))
					()
				});

			}

			void state_connected_task::get_proc_param_status_word(cyng::async::mux& mux, cyng::vector_t vec)
			{
				auto const word = cyng::numeric_cast<std::uint32_t>(vec.at(5), 0u);

				node::sml::status stat;
				stat.reset(word);

				mux.post(tsk_proxy_, 5, cyng::tuple_t{
					vec.at(1),	//	trx
					vec.at(2),	//	idx
					vec.at(3),	//	server ID
					vec.at(4),	//	OBIS code
								//	status word
					cyng::param_map_factory("word", node::sml::to_param_map(stat))()
				});

			}

			void state_connected_task::get_proc_param_memory(cyng::async::mux& mux, cyng::vector_t vec)
			{
				mux.post(tsk_proxy_, 5, cyng::tuple_t{
					vec.at(1),	//	trx
					vec.at(2),	//	idx
					vec.at(3),	//	server ID
					vec.at(4),	//	OBIS code
								//	status word
					cyng::param_map_factory("mirror", vec.at(5))("tmp", vec.at(6)) ()
				});
			}


			void state_connected_task::get_proc_param_wmbus_status(cyng::async::mux& mux, cyng::vector_t vec)
			{
				mux.post(tsk_proxy_, 5, cyng::tuple_t{
					vec.at(1),	//	trx
					vec.at(2),	//	idx
					vec.at(3),	//	server ID
					vec.at(4),	//	OBIS code
								//	firmware
					cyng::param_map_factory("manufacturer", vec.at(5))
						("id", vec.at(6))
						("firmware", vec.at(7))
						("hardware", vec.at(8))
					()
				});
			}

			void state_connected_task::get_proc_param_wmbus_config(cyng::async::mux& mux, cyng::vector_t vec)
			{
				mux.post(tsk_proxy_, 5, cyng::tuple_t{
					vec.at(1),	//	trx
					vec.at(2),	//	idx
					vec.at(3),	//	server ID
					vec.at(4),	//	OBIS code
								//	wireless M-Bus configuration
					cyng::param_map_factory("protocol", vec.at(5))
						("sMode", cyng::numeric_cast<std::uint16_t>(vec.at(6), 0))
						("tMode", cyng::numeric_cast<std::uint16_t>(vec.at(7), 0))
						("reboot", vec.at(8))
						("power", vec.at(9))
						("installMode", vec.at(10))
					()
				});
			}

			void state_connected_task::get_proc_param_ipt_status(cyng::async::mux& mux, cyng::vector_t vec)
			{
				mux.post(tsk_proxy_, 5, cyng::tuple_t{
					vec.at(1),	//	trx
					vec.at(2),	//	idx
					vec.at(3),	//	server ID
					vec.at(4),	//	OBIS code
								//	current IP-T address parameters
					cyng::param_map_factory
						("address", node::sml::to_ip_address_v4(vec.at(5)))
						("local", cyng::numeric_cast<std::uint16_t>(vec.at(6), 0))
						("remote", cyng::numeric_cast<std::uint16_t>(vec.at(7), 0))
					()
				});
			}

			void state_connected_task::get_proc_param_ipt_param(cyng::async::mux& mux, cyng::vector_t vec)
			{
				mux.post(tsk_proxy_, 5, cyng::tuple_t{
					vec.at(1),	//	trx
					vec.at(2),	//	idx
					vec.at(3),	//	server ID
					vec.at(4),	//	OBIS code
								//	IP-T configuration record
					cyng::param_map_factory("idx", vec.at(5))
						("address", node::sml::ip_address_to_str(vec.at(6)))
						("local", cyng::numeric_cast<std::uint16_t>(vec.at(7), 0))
						("remote", cyng::numeric_cast<std::uint16_t>(vec.at(8), 0))
						("name", vec.at(9))
						("pwd", vec.at(10))
					()
				});
			}

			void state_connected_task::get_list_response(cyng::async::mux& mux, cyng::vector_t vec)
			{

				//	[7dee1864-70a4-4029-9787-c5cae1ec52eb,4061719-2,0,,01E61E130900163C07,990000000003,%(("08 00 01 00 00 ff":0.758),("08 00 01 02 00 ff":0.758)),null,06988c71]
				mux.post(tsk_proxy_, 5, cyng::tuple_t{
					vec.at(1),	//	trx
					vec.at(2),	//	idx
					vec.at(4),	//	server ID - mostly empty
					vec.at(5),	//	OBIS_CODE(99, 00, 00, 00, 00, 03)
					vec.at(6)	//	last data record
					});
			}

			void state_connected_task::attention_msg(cyng::async::mux& mux, cyng::vector_t vec)
			{
				mux.post(tsk_proxy_, 6, cyng::tuple_t{ vec.at(3), vec.at(4) });
			}

		}

	}
}



