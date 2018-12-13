/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
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

#include <cyng/dom/reader.h>
#include <cyng/tuple_cast.hpp>
#include <cyng/numeric_cast.hpp>

#include <boost/assert.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace node 
{
	namespace ipt
	{
		session_state::session_state(session* sp)
			: sp_(sp)
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
			CYNG_LOG_ERROR(sp_->logger_, sp_->vm().tag() << " unexpected event " << msg  << " in state "<< *this );
		}

		void session_state::transit(internal_state state)
		{
			if (state_ == state) {
				CYNG_LOG_WARNING(sp_->logger_, ">> " << sp_->vm().tag() << " no transition " << *this);
			}
			else {
				CYNG_LOG_DEBUG(sp_->logger_, ">> " << sp_->vm().tag() << " leave state " << *this);
				state_ = state;
				CYNG_LOG_DEBUG(sp_->logger_, ">> " << sp_->vm().tag() << " enter state " << *this);
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
				CYNG_LOG_FATAL(sp_->logger_, sp_->vm().tag() << " cannot start gatekeeper");
			}
			else {

				//
				//	store task ID
				//
				idle_.tsk_gatekeeper_ = evt.tsk_;

				//
				//	watchdog in minutes
				//
				idle_.watchdog_ = evt.watchdog_;
			}

			//sp_->vm_.register_function("session.update.connection.state", 2, std::bind(&session_state::update_connection_state, this, std::placeholders::_1));
			sp_->vm_.register_function("session.redirect", 1, std::bind(&session_state::redirect, this, std::placeholders::_1));
		}

		//void session_state::update_connection_state(cyng::context& ctx)
		//{
		//	if (S_AUTHORIZED == state_) {

		//		auto const tpl = cyng::tuple_cast<
		//			boost::uuids::uuid,		//	[0] remote tag
		//			bool					//	[1] new state
		//		>(ctx.get_frame());

		//		//
		//		//	update state
		//		//
		//		transit(std::get<1>(tpl) ? S_CONNECTED_LOCAL : S_CONNECTED_REMOTE);
		//	}
		//	else {
		//		signal_wrong_state("update_connection_state");
		//	}
		//}

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

				CYNG_LOG_INFO(sp_->logger_, "session.redirect "
					<< ctx.tag()
					<< ": TASK");
			}
			else {
				CYNG_LOG_WARNING(sp_->logger_, "session.redirect "
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
				sp_->bus_->vm_.async_run(cyng::generate_invoke("server.clear.connection.map", sp_->vm_.tag()));

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
					sp_->mux_.post(authorized_.tsk_proxy_, 7u, std::move(evt.tpl_));
				}
				else {
					CYNG_LOG_WARNING(sp_->logger_, sp_->vm().tag() << " no proxy task");
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

			if (sp_->bus_->is_online()) {

				switch (state_) {
				//case S_IDLE:
				//case S_ERROR:
				//case S_SHUTDOWN:
				case S_AUTHORIZED:
					CYNG_LOG_WARNING(sp_->logger_, sp_->vm().tag()
						<< " transmit "
						<< ptr->size()
						<< " bytes in connection state authorized");
					local_.transmit(sp_->bus_, sp_->vm().tag(), evt.obj_);
					break;

				//case S_WAIT_FOR_OPEN_RESPONSE:
				//case S_WAIT_FOR_CLOSE_RESPONSE:

				case S_CONNECTED_LOCAL:
					CYNG_LOG_DEBUG(sp_->logger_, sp_->vm().tag()
						<< " transmit "
						<< ptr->size()
						<< " bytes to local session");
					local_.transmit(sp_->bus_, sp_->vm().tag(), evt.obj_);
					break;

				case S_CONNECTED_REMOTE:

					CYNG_LOG_DEBUG(sp_->logger_, sp_->vm().tag()
						<< " transmit "
						<< ptr->size()
						<< " bytes to remote session");
					remote_.transmit(sp_->bus_, sp_->vm().tag(), evt.obj_);
					break;

				case S_CONNECTED_TASK:
					//	send data to task
					CYNG_LOG_DEBUG(sp_->logger_, sp_->vm().tag()
						<< " transmit "
						<< ptr->size()
						<< " bytes to task #"
						<< task_.tsk_proxy_);
					BOOST_ASSERT(task_.tsk_proxy_ == authorized_.tsk_proxy_);
					task_.transmit(sp_->mux_, evt.obj_);
					break;

				default:
					signal_wrong_state("evt_data");
					break;
				}
			}
			else {

				//
				//	cannot deliver data
				//
				CYNG_LOG_WARNING(sp_->logger_, "ipt.req.transmit.data "
					<< sp_->vm().tag()
					<< " - no master");
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


			//
			//	read original data
			//
			auto dom = cyng::make_reader(evt.bag_);

			//
			//	IP-T result type
			//
			const response_type res = evt.success_
				? ctrl_res_login_public_policy::SUCCESS
				: ctrl_res_login_public_policy::UNKNOWN_ACCOUNT
				;

			//
			//	send login response to device
			//
			const std::string security = cyng::value_cast<std::string>(dom.get("security"), "undef");
			if (boost::algorithm::equals(security, "scrambled")) {
				prg << cyng::generate_invoke_unwinded("res.login.scrambled", res, idle_.watchdog_, "");
			}
			else {
				prg << cyng::generate_invoke_unwinded("res.login.public", res, idle_.watchdog_, "");
			}
			prg << cyng::generate_invoke_unwinded("stream.flush");

			if (evt.success_) {

				//
				//	update session state
				//
				transit(S_AUTHORIZED);

				//
				//	stop gatekeeper
				//
				idle_.stop(sp_->mux_);

				//
				//	start watchdog
				//
				if (idle_.watchdog_ != 0) {
					prg << cyng::generate_invoke_unwinded("session.start.watchdog", idle_.watchdog_, evt.name_);
				}

				//
				//	send query
				//
				prg << cyng::unwind(query(evt.query_));

				//
				//	start proxy
				//
				prg << cyng::generate_invoke_unwinded("session.start.proxy");

			}

			//
			//	gatekeeper will terminating this session
			//
			return prg;
		}

		cyng::vector_t session_state::query(std::uint32_t query)
		{
			//
			//	further actions
			//
			cyng::vector_t prg;
			if ((query & QUERY_PROTOCOL_VERSION) == QUERY_PROTOCOL_VERSION)
			{
				CYNG_LOG_DEBUG(sp_->logger_, sp_->vm().tag() << " QUERY_PROTOCOL_VERSION");
				prg << cyng::generate_invoke_unwinded("req.protocol.version");
			}
			if ((query & QUERY_FIRMWARE_VERSION) == QUERY_FIRMWARE_VERSION)
			{
				CYNG_LOG_DEBUG(sp_->logger_, sp_->vm().tag() << " QUERY_FIRMWARE_VERSION");
				prg << cyng::generate_invoke_unwinded("req.software.version");
			}
			if ((query & QUERY_DEVICE_IDENTIFIER) == QUERY_DEVICE_IDENTIFIER)
			{
				CYNG_LOG_DEBUG(sp_->logger_, sp_->vm().tag() << " QUERY_DEVICE_IDENTIFIER");
				prg << cyng::generate_invoke_unwinded("req.device.id");
			}
			if ((query & QUERY_NETWORK_STATUS) == QUERY_NETWORK_STATUS)
			{
				CYNG_LOG_DEBUG(sp_->logger_, sp_->vm().tag() << " QUERY_NETWORK_STATUS");
				prg << cyng::generate_invoke_unwinded("req.net.status");
			}
			if ((query & QUERY_IP_STATISTIC) == QUERY_IP_STATISTIC)
			{
				CYNG_LOG_DEBUG(sp_->logger_, sp_->vm().tag() << " QUERY_IP_STATISTIC");
				prg << cyng::generate_invoke_unwinded("req.ip.statistics");
			}
			if ((query & QUERY_DEVICE_AUTHENTIFICATION) == QUERY_DEVICE_AUTHENTIFICATION)
			{
				CYNG_LOG_DEBUG(sp_->logger_, sp_->vm().tag() << " QUERY_DEVICE_AUTHENTIFICATION");
				prg << cyng::generate_invoke_unwinded("req.device.auth");
			}
			if ((query & QUERY_DEVICE_TIME) == QUERY_DEVICE_TIME)
			{
				CYNG_LOG_DEBUG(sp_->logger_, sp_->vm().tag() << " QUERY_DEVICE_TIME");
				prg << cyng::generate_invoke_unwinded("req.device.time");
			}
			prg << cyng::generate_invoke_unwinded("stream.flush");
			return prg;
		}

		void session_state::react(state::evt_watchdog_started evt)
		{
			if (evt.success_) {
				authorized_.tsk_watchdog_ = evt.tsk_;
			}
			else {
				CYNG_LOG_FATAL(sp_->logger_, sp_->vm().tag() << " cannot start watchdog");
			}
		}

		void session_state::react(state::evt_proxy_started evt)
		{
			if (evt.success_) {
				authorized_.tsk_proxy_ = evt.tsk_;

				//
				//	init SML comm
				//
				CYNG_LOG_DEBUG(sp_->logger_, sp_->vm().tag() << " initialize SML comm");
				sp_->proxy_comm_.register_this(sp_->vm());

			}
			else {
				CYNG_LOG_FATAL(sp_->logger_, sp_->vm().tag() << " cannot start proxy");
			}
		}

		cyng::vector_t session_state::react(state::evt_req_login_public evt)
		{
			cyng::vector_t prg;

			switch (state_) {
			case S_IDLE:
				break;
			default:
				signal_wrong_state("evt_req_login_public");
				return prg;
			}

			CYNG_LOG_INFO(sp_->logger_, "ipt.req.login.public "
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
					, cyng::param_map_factory("tp-layer", "ipt")("security", "public")("time", std::chrono::system_clock::now())));

			}
			else {

				//
				//	reject login - faulty master
				//
				const response_type res = ctrl_res_login_public_policy::MALFUNCTION;

				prg
					<< cyng::generate_invoke_unwinded("res.login.public", res, static_cast<std::uint16_t>(0), "")
					<< cyng::generate_invoke_unwinded("stream.flush")
					<< cyng::generate_invoke_unwinded("log.msg.error", "public login failed - no master")
					;

			}

			//
			//	update watchdog timer
			//
			authorized_.activity(sp_->mux_);

			return prg;
		}

		cyng::vector_t session_state::react(state::evt_req_login_scrambled evt)
		{
			cyng::vector_t prg;

			switch (state_) {
			case S_IDLE:
				break;
			default:
				signal_wrong_state("evt_req_login_scrambled");
				return prg;
			}

			CYNG_LOG_INFO(sp_->logger_, "ipt.req.login.scrambled "
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
					, cyng::param_map_factory("tp-layer", "ipt")("security", "scrambled")("time", std::chrono::system_clock::now())));

			}
			else {

				//
				//	reject login - faulty master
				//
				const response_type res = ctrl_res_login_public_policy::MALFUNCTION;

				prg
					<< cyng::generate_invoke_unwinded("res.login.public", res, static_cast<std::uint16_t>(0), "")
					<< cyng::generate_invoke_unwinded("stream.flush")
					<< cyng::generate_invoke_unwinded("log.msg.error", "scrambled login failed - no master")
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

			switch (state_) {
			case S_AUTHORIZED:
				break;
			default:
				signal_wrong_state("evt_client_req_open_connection");
				return prg;
			}

			if (evt.success_) {

				//
				//	update state
				//
				transit(S_WAIT_FOR_OPEN_RESPONSE);
				wait_for_open_response_.init(evt.tsk_, evt.origin_tag_, evt.local_, evt.seq_, evt.master_, evt.client_);
				//authorized_.tsk_connection_open_ = evt.tsk_;
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
			//	dom reader
			//
			auto dom = cyng::make_reader(evt.client_);
			const sequence_type seq = cyng::value_cast<sequence_type>(dom.get("seq"), 0);
			BOOST_ASSERT(seq != 0);	//	values of 0 not allowed

			//
			//	use success flag to get an IP-T response code
			//
			const response_type res = (evt.success_)
				? tp_res_close_connection_policy::CONNECTION_CLEARING_SUCCEEDED
				: tp_res_close_connection_policy::CONNECTION_CLEARING_FORBIDDEN
				;

			//
			//	send response
			//
			prg
				<< cyng::generate_invoke_unwinded("res.close.connection", seq, res)
				<< cyng::generate_invoke_unwinded("stream.flush")
				;

			//
			//	check lag/response time
			//
			const auto start = cyng::value_cast(dom.get("start"), std::chrono::system_clock::now());
			auto lag = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);

			if (lag > sp_->timeout_) {
				CYNG_LOG_WARNING(sp_->logger_, "client.req.close.connection.forward - high cluster lag: " << lag.count() << " millisec");
			}
			else {
				CYNG_LOG_TRACE(sp_->logger_, "client.req.close.connection.forward - cluster lag: " << lag.count() << " millisec");
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
			auto dom = cyng::make_reader(evt.client_);
			const sequence_type seq = cyng::value_cast<sequence_type>(dom.get("seq"), 0);
			BOOST_ASSERT(seq != 0);	//	values of 0 not allowed

			//
			//	check lag/response time
			//
			const auto start = cyng::value_cast(dom.get("start"), std::chrono::system_clock::now());
			auto lag = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);

			if (lag > sp_->timeout_) {
				CYNG_LOG_WARNING(sp_->logger_, "client.res.open.connection.forward "
					<< " - high cluster lag: " << lag.count() << " millisec");
			}
			else {
				CYNG_LOG_TRACE(sp_->logger_, "client.res.open.connection.forward "
					<< " - cluster lag: " << lag.count() << " millisec");
			}

			if (evt.success_)
			{
				//
				//	hides outer variable dom
				//
				auto dom = cyng::make_reader(evt.master_);
				const auto local = cyng::value_cast(dom.get("local-connect"), false);

				CYNG_LOG_TRACE(sp_->logger_, sp_->vm().tag()
					<< " set connection state to "
					<< (local ? "local" : "remote"));

				//
				//	update state
				//
				transit(local ? S_CONNECTED_LOCAL : S_CONNECTED_REMOTE);
				prg << cyng::generate_invoke_unwinded("res.open.connection", seq, static_cast<response_type>(tp_res_open_connection_policy::DIALUP_SUCCESS));
			}
			else
			{
				//
				//	update state
				//	To reset the session state to AUTHORIZED reflect the fact that this session is no longer part in any
				//	attempt to establish a connection.
				//
				transit(S_AUTHORIZED);
				prg << cyng::generate_invoke_unwinded("res.open.connection", seq, static_cast<response_type>(tp_res_open_connection_policy::DIALUP_FAILED));
			}

			prg << cyng::generate_invoke_unwinded("stream.flush");

			return prg;
		}

		cyng::vector_t session_state::react(state::evt_ipt_req_close_connection evt)
		{
			cyng::vector_t prg;

			switch (state_) {
			case S_CONNECTED_LOCAL:
			case S_CONNECTED_REMOTE:
			//case S_CONNECTED_TASK:
				break;
			default:
				signal_wrong_state("evt_ipt_req_close_connection");
				return prg;
			}

			//const cyng::vector_t frame = ctx.get_frame();
			if (sp_->bus_->is_online())
			{
				sp_->bus_->vm_.async_run(node::client_req_close_connection(sp_->vm().tag()
					, false //	no shutdown
					, cyng::param_map_factory("tp-layer", "ipt")("origin-tag", sp_->vm().tag())("seq", evt.seq_)("start", std::chrono::system_clock::now())));
			}
			else
			{
				//ctx.queue(cyng::generate_invoke("log.msg.error", "ipt.req.close.connection - no master", frame))
				prg
					<< cyng::generate_invoke_unwinded("res.close.connection", evt.seq_, static_cast<response_type>(tp_res_close_connection_policy::CONNECTION_CLEARING_FORBIDDEN))
					<< cyng::generate_invoke_unwinded("stream.flush");
			}

			//
			//	update watchdog timer
			//
			authorized_.activity(sp_->mux_);

			return prg;
		}

		cyng::vector_t session_state::react(state::evt_ipt_res_open_connection evt)
		{
			cyng::vector_t prg;

			switch (state_) {
			case S_WAIT_FOR_OPEN_RESPONSE:
				break;
			default:
				signal_wrong_state("evt_ipt_res_open_connection");
				return prg;
			}

			//
			//	stop open connection task
			//
			BOOST_ASSERT(evt.tsk_ == wait_for_open_response_.tsk_connection_open_);
			//authorized_.tsk_connection_open_ = cyng::async::NO_TASK;
			sp_->mux_.post(evt.tsk_, evt.slot_, cyng::tuple_factory(evt.res_));

			//
			//	update session state
			//
			if (ipt::tp_res_open_connection_policy::is_success(evt.res_)) {
				switch (wait_for_open_response_.type_) {
				case state::state_wait_for_open_response::E_LOCAL:
					transit(S_CONNECTED_LOCAL);
					break;
				case state::state_wait_for_open_response::E_REMOTE:
					transit(S_CONNECTED_REMOTE);
					break;
				default:
					BOOST_ASSERT_MSG(false, "undefined connection state");
					break;
				}
			}
			else {

				//
				//	fallback to authorized
				//
				transit(S_AUTHORIZED);
			}
			
			if (sp_->bus_->is_online()) {
				sp_->bus_->vm_.async_run(client_res_open_connection(wait_for_open_response_.tag_
					, wait_for_open_response_.seq_	//	cluster sequence
					, ipt::tp_res_open_connection_policy::is_success(evt.res_)
					, wait_for_open_response_.master_params_
					, wait_for_open_response_.client_params_));
			}

			wait_for_open_response_.reset();

			//
			//	update watchdog timer
			//
			authorized_.activity(sp_->mux_);

			return prg;
		}

		cyng::vector_t session_state::react(state::evt_ipt_res_close_connection evt)
		{
			cyng::vector_t prg;

			switch (state_) {
			case S_WAIT_FOR_CLOSE_RESPONSE:
				break;
			default:
				signal_wrong_state("evt_ipt_res_close_connection");
				return prg;
			}

			//
			//	stop close connection task
			//
			BOOST_ASSERT(evt.tsk_ == wait_for_close_response_.tsk_connection_close_);
			sp_->mux_.post(evt.tsk_, evt.slot_, cyng::tuple_factory(evt.res_));

			//
			//	update master
			//
			wait_for_close_response_.terminate(sp_->bus_, evt.res_);

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

			if (evt.success_) {
				//
				//	update state
				//
				transit(S_WAIT_FOR_CLOSE_RESPONSE);
				wait_for_close_response_.init(evt.tsk_, evt.tag_, evt.shutdown_, evt.seq_, evt.master_, evt.client_, state_ == S_CONNECTED_LOCAL);
			}
			else {
				CYNG_LOG_FATAL(sp_->logger_, "cannot start close_connection");
			}

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

			CYNG_LOG_DEBUG(sp_->logger_, "SML message to #"
				<< task_.tsk_proxy_
				<< " - "
				<< cyng::io::to_str(evt.msg_));

			//
			//	message slot (3)
			//
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

			CYNG_LOG_DEBUG(sp_->logger_, "evt_sml_eom to #" << task_.tsk_proxy_);

			//
			//	message slot (1)
			//
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

			CYNG_LOG_DEBUG(sp_->logger_, "evt_sml_public_close_response to #" << task_.tsk_proxy_);

			//
			//	message slot (4)
			//
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
			CYNG_LOG_DEBUG(sp_->logger_, "evt_sml_get_proc_param_srv_visible to #" << task_.tsk_proxy_);

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
			CYNG_LOG_DEBUG(sp_->logger_, "evt_sml_get_proc_param_srv_active to #" << task_.tsk_proxy_);

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
			CYNG_LOG_DEBUG(sp_->logger_, "evt_sml_get_proc_param_firmware to #" << task_.tsk_proxy_);

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
			CYNG_LOG_DEBUG(sp_->logger_, "evt_sml_get_proc_param_status_word to #" << task_.tsk_proxy_);

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
			CYNG_LOG_DEBUG(sp_->logger_, "evt_sml_get_proc_param_memory to #" << task_.tsk_proxy_);

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
			CYNG_LOG_DEBUG(sp_->logger_, "evt_sml_get_proc_param_wmbus_status to #" << task_.tsk_proxy_);

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
			CYNG_LOG_DEBUG(sp_->logger_, "evt_sml_get_proc_param_wmbus_config to #" << task_.tsk_proxy_);

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
			CYNG_LOG_DEBUG(sp_->logger_, "evt_sml_get_proc_param_ipt_status to #" << task_.tsk_proxy_);

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
			CYNG_LOG_DEBUG(sp_->logger_, "evt_sml_get_proc_param_ipt_param to #" << task_.tsk_proxy_);

			//
			//	message slot (5)
			//
			task_.get_proc_param_ipt_param(sp_->mux_, evt.vec_);

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
			CYNG_LOG_DEBUG(sp_->logger_, "evt_sml_attention_msg to #" << task_.tsk_proxy_);

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
			evt_init_complete::evt_init_complete(std::pair<std::size_t, bool> r, std::uint16_t watchdog)
				: tsk_(r.first)
				, success_(r.second)
				, watchdog_(watchdog)
			{}

			evt_watchdog_started::evt_watchdog_started(std::pair<std::size_t, bool> r)
				: tsk_(r.first)
			, success_(r.second)
			{}

			evt_proxy_started::evt_proxy_started(std::pair<std::size_t, bool> r)
				: tsk_(r.first)
				, success_(r.second)
			{}

			evt_req_login_public::evt_req_login_public(std::tuple
				<
				boost::uuids::uuid,		//	[0] peer tag
				std::string,			//	[1] name
				std::string				//	[2] pwd
				> tpl)
				: tag_(std::get<0>(tpl))
				, name_(std::get<1>(tpl))
				, pwd_(std::get<2>(tpl))
			{}

			//
			//	EVENT: scrambled login
			//
			evt_req_login_scrambled::evt_req_login_scrambled(boost::uuids::uuid tag
				, std::string const& name
				, std::string const& pwd
				, scramble_key const& sk)
				: tag_(tag)
				, name_(name)
				, pwd_(pwd)
				, sk_(sk)
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
				, sequence_type seq
				, std::string number)
				: tag_(tag)
				, seq_(seq)
				, number_(number)
			{}


			//
			//	EVENT: IP-T connection open response
			//
			evt_ipt_res_open_connection::evt_ipt_res_open_connection(std::size_t tsk, std::size_t slot, response_type res)
				: tsk_(tsk)
			, slot_(slot)
			, res_(res)
			{}


			//
			//	EVENT: IP-T connection close request
			//
			evt_ipt_req_close_connection::evt_ipt_req_close_connection(std::tuple<
				boost::uuids::uuid,		//	[0] session tag
				sequence_type			//	[1] seq
				>tpl )
			: tag_(std::get<0>(tpl))
				, seq_(std::get<1>(tpl))
			{}

			//
			//	EVENT: IP-T connection close response
			//
			evt_ipt_res_close_connection::evt_ipt_res_close_connection(std::size_t tsk, std::size_t slot, response_type res)
				: tsk_(tsk)
				, slot_(slot)
				, res_(res)
			{}


			//
			//	EVENT: incoming connection open request
			//
			evt_client_req_open_connection::evt_client_req_open_connection(std::pair<std::size_t, bool> r
				, boost::uuids::uuid origin_tag
				, bool local
				, std::size_t seq
				, cyng::param_map_t master
				, cyng::param_map_t client)
			: tsk_(r.first)
				, success_(r.second)
				, origin_tag_(origin_tag)
				, local_(local)
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
			evt_client_req_close_connection::evt_client_req_close_connection(std::pair<std::size_t, bool> r,
				std::uint64_t seq,			//	[1] cluster sequence
				boost::uuids::uuid tag,		//	[2] origin-tag - compare to "origin-tag"
				bool shutdown,				//	[3] shutdown flag
				cyng::param_map_t master,	//	[4] master
				cyng::param_map_t client)	//	[5] client
			: tsk_(r.first)
				, success_(r.second)
				, seq_(seq)
				, tag_(tag)
				, shutdown_(shutdown)
				, master_(master)
				, client_(client)
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
			//	EVENT: evt_sml_attention_msg
			//
			evt_sml_attention_msg::evt_sml_attention_msg(cyng::vector_t vec)
				: vec_(vec)
			{}

			//
			//	STATE: idle
			//
			state_idle::state_idle()
				: tsk_gatekeeper_(cyng::async::NO_TASK)
			{}

			void state_idle::stop(cyng::async::mux& mux)
			{
				if (cyng::async::NO_TASK != tsk_gatekeeper_)	mux.stop(tsk_gatekeeper_);
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
			: tsk_connection_open_(cyng::async::NO_TASK)
				, tag_(boost::uuids::nil_uuid())
				, type_(E_UNDEF)
				, seq_(0u)
				, master_params_()
				, client_params_()
			{}

			void state_wait_for_open_response::init(std::size_t tsk
				, boost::uuids::uuid tag
				, bool local
				, std::uint64_t seq
				, cyng::param_map_t master
				, cyng::param_map_t client)
			{
				tsk_connection_open_ = tsk;
				tag_ = tag;
				type_ = local ? E_LOCAL : E_REMOTE;
				seq_ = seq;
				master_params_ = master;
				client_params_ = client;
			}

			void state_wait_for_open_response::reset()
			{
				tsk_connection_open_ = cyng::async::NO_TASK;
				tag_ = boost::uuids::nil_uuid();
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
				, local_(false)
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
				local_ = local;
			}

			void state_wait_for_close_response::reset()
			{
				tsk_connection_close_ = cyng::async::NO_TASK;
				tag_ = boost::uuids::nil_uuid();
				shutdown_ = false;
				seq_ = 0u;
				master_params_.clear();
				client_params_.clear();
				local_ = false;
			}

			bool state_wait_for_close_response::is_connection_local() const
			{
				auto const dom = cyng::make_reader(master_params_);
				return cyng::value_cast(dom.get("local-connect"), false);
			}

			void state_wait_for_close_response::terminate(bus::shared_type bus, response_type res)
			{
				if (!shutdown_ && bus->is_online())
				{
					if (is_connection_local()) {

						//	otherwise connection type is inconsistent
						BOOST_ASSERT(local_);

						//
						//	remove from connection_map_
						//
						bus->vm_.async_run(cyng::generate_invoke("server.clear.connection.map", tag_));
					}

					//	send "client.res.close.connection" to SMF master
					bus->vm_.async_run(client_res_close_connection(tag_
						, seq_
						, ipt::tp_res_close_connection_policy::is_success(res)
						, master_params_
						, client_params_));
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
					, cyng::param_map_factory("tp-layer", "ipt")("start", std::chrono::system_clock::now())
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

			void state_connected_task::attention_msg(cyng::async::mux& mux, cyng::vector_t vec)
			{
				mux.post(tsk_proxy_, 6, cyng::tuple_t{ vec.at(3), vec.at(4) });
			}

		}

	}
}



