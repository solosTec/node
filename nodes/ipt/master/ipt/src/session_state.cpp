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
#include <smf/sml/obis_db.h>
#include <smf/mbus/defs.h>

#include <cyng/dom/reader.h>
#include <cyng/tuple_cast.hpp>
#include <cyng/set_cast.h>
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
			, logger_(sp->logger_)
			, state_(internal_state::IDLE)
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
			case internal_state::IDLE:
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
				idle_.watchdog_ = evt.watchdog_;
			}

			sp_->vm_.register_function("session.redirect", 1, std::bind(&session_state::redirect, this, std::placeholders::_1));
		}

		void session_state::redirect(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			auto tsk = cyng::value_cast<std::size_t>(frame.at(0), cyng::async::NO_TASK);
			CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

			switch (state_) {
			case internal_state::AUTHORIZED:
				if (tsk != cyng::async::NO_TASK) {
					//
					//	update state
					//
					transit(internal_state::CONNECTED_TASK);
					task_.reset(sp_->mux_, tsk);

					CYNG_LOG_INFO(logger_, "session "
						<< ctx.tag()
						<< " connected to task #"
						<< tsk);
				}
				else {
					CYNG_LOG_WARNING(logger_, "session "
						<< ctx.tag()
						<< " cannot dis-connected from task #"
						<< tsk
						<< " (already dis-connected)");
				}
				break;
			case internal_state::CONNECTED_TASK:
				if (tsk == cyng::async::NO_TASK) {
					//
					//	update state
					//
					transit(internal_state::AUTHORIZED);
					task_.reset(sp_->mux_, tsk);

					CYNG_LOG_INFO(logger_, "session "
						<< ctx.tag()
						<< " dis-connected from task #"
						<< tsk);
				}
				else {
					CYNG_LOG_WARNING(logger_, "session "
						<< ctx.tag()
						<< " cannot connected to task #"
						<< tsk
						<< " (already connected)");
				}
				break;
			default:
				//	internal_state::WAIT_FOR_OPEN_RESPONSE:
				//	internal_state::WAIT_FOR_CLOSE_RESPONSE:
				//	internal_state::CONNECTED_LOCAL:
				//	internal_state::CONNECTED_REMOTE:
				CYNG_LOG_WARNING(logger_, "session.redirect "
					<< ctx.tag()
					<< " - session is busy: ");
				break;
			}
		}

		void session_state::react(state::evt_shutdown)
		{
			switch (state_) {
			case internal_state::CONNECTED_LOCAL:
			case internal_state::CONNECTED_REMOTE:
			case internal_state::CONNECTED_TASK:
			case internal_state::WAIT_FOR_CLOSE_RESPONSE:	//	still in connection 

				//
				//	clear connection map
				//
				sp_->bus_->vm_.async_run(cyng::generate_invoke("server.connection-map.clear", sp_->vm_.tag()));

				//
				//	note: NO BREAK here - continue with AUTHORIZED state
				//
#if	defined(__CPP_SUPPORT_P0188R1)
				[[fallthrough]];
#endif

			case internal_state::WAIT_FOR_OPEN_RESPONSE:
			case internal_state::AUTHORIZED:
				authorized_.stop(sp_->mux_);
				break;

			case internal_state::IDLE:
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
			case internal_state::FAILURE:
			case internal_state::AUTHORIZED:
			case internal_state::WAIT_FOR_OPEN_RESPONSE:
			case internal_state::WAIT_FOR_CLOSE_RESPONSE:
			case internal_state::CONNECTED_LOCAL:
			case internal_state::CONNECTED_REMOTE:
			case internal_state::CONNECTED_TASK:
				authorized_.activity(sp_->mux_);
				break;
			//case internal_state::IDLE:
			//case internal_state::SHUTDOWN:
			default:
				break;
			}
		}

		void session_state::react(state::evt_gateway evt)
		{
			switch (state_) {
			case internal_state::AUTHORIZED:
			case internal_state::CONNECTED_TASK:
				if (cyng::async::NO_TASK != authorized_.tsk_proxy_) {
					BOOST_ASSERT_MSG(evt.tpl_.size() == 11, "evt_gateway");

					//
					//	push new entry into (income) queue
					//
					sp_->mux_.post(authorized_.tsk_proxy_, 7u, std::move(evt.tpl_));
				}
				else {
					CYNG_LOG_WARNING(logger_, sp_->vm().tag() << " no task (gateway)");
				}
				break;
			//case internal_state::WAIT_FOR_OPEN_RESPONSE:
			//case internal_state::WAIT_FOR_CLOSE_RESPONSE:
			//case internal_state::CONNECTED_LOCAL:
			//case internal_state::CONNECTED_REMOTE:
			//case internal_state::IDLE:
			//case internal_state::ERROR:
			//case internal_state::SHUTDOWN:
			default:
				signal_wrong_state("evt_gateway");
				break;
			}
		}

		void session_state::react(state::evt_proxy evt)
		{
			switch (state_) {
			case internal_state::AUTHORIZED:
			case internal_state::CONNECTED_TASK:
				if (cyng::async::NO_TASK != authorized_.tsk_proxy_) {
					BOOST_ASSERT_MSG(evt.tpl_.size() == 10, "evt_proxy");
					//
					//	communicate with proxy itself
					//
					sp_->mux_.post(authorized_.tsk_proxy_, 9u, std::move(evt.tpl_));
				}
				else {
					CYNG_LOG_WARNING(logger_, sp_->vm().tag() << " no task (proxy)");
				}
				break;
				//case internal_state::WAIT_FOR_OPEN_RESPONSE:
				//case internal_state::WAIT_FOR_CLOSE_RESPONSE:
				//case internal_state::CONNECTED_LOCAL:
				//case internal_state::CONNECTED_REMOTE:
				//case internal_state::IDLE:
				//case internal_state::ERROR:
				//case internal_state::SHUTDOWN:
			default:
				signal_wrong_state("evt_gateway");
				break;
			}
		}

		void session_state::react(state::evt_data evt)
		{
			auto ptr = cyng::object_cast<cyng::buffer_t>(evt.obj_);
			BOOST_ASSERT(ptr != nullptr);

#ifdef SMF_IO_LOG
			//std::stringstream ss;
			//ss
			//	<< "ipt-rx-"
			//	<< boost::uuids::to_string(tag)
			//	<< "-"
			//	<< std::setw(4)
			//	<< std::setfill('0')
			//	<< std::dec
			//	<< ++log_counter_
			//	<< ".log"
			//	;
			//const std::string file_name = ss.str();
			//std::ofstream of(file_name, std::ios::out | std::ios::app);
			//if (of.is_open())
			//{
			//	cyng::io::hex_dump hd;
			//	auto ptr = cyng::object_cast<cyng::buffer_t>(frame.at(1));
			//	hd(of, ptr->begin(), ptr->end());

			//	CYNG_LOG_TRACE(logger_, "write debug log " << file_name);
			//	of.close();
			//}
#endif

			switch (state_) {
				//case internal_state::IDLE:
				//case internal_state::ERROR:
				//case internal_state::SHUTDOWN:
			case internal_state::AUTHORIZED:
				CYNG_LOG_WARNING(logger_, sp_->vm().tag()
					<< " transmit "
					<< ptr->size()
					<< " bytes in connection state authorized");
				local_.transmit(sp_->bus_, sp_->vm().tag(), evt.obj_);
				break;

				//case internal_state::WAIT_FOR_OPEN_RESPONSE:
			case internal_state::WAIT_FOR_CLOSE_RESPONSE:
				CYNG_LOG_WARNING(logger_, sp_->vm().tag()
					<< " received "
					<< ptr->size()
					<< " bytes while waiting for close response");
				break;

			case internal_state::CONNECTED_LOCAL:
				CYNG_LOG_DEBUG(logger_, sp_->vm().tag()
					<< " transmit "
					<< ptr->size()
					<< " bytes to local session");
				local_.transmit(sp_->bus_, sp_->vm().tag(), evt.obj_);
				break;

			case internal_state::CONNECTED_REMOTE:

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

			case internal_state::CONNECTED_TASK:
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
			case internal_state::IDLE:
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
				//	stop gatekeeper
				//
				idle_.stop(sp_->mux_);

				//
				//	update session state
				//
				transit(internal_state::AUTHORIZED);

				//
				//	start watchdog
				//
				if (idle_.watchdog_ != 0) {
					prg << cyng::generate_invoke_unwinded("session.start.watchdog", idle_.watchdog_, evt.name_);
				}

				//
				//	send query
				//  the template parameter is requires by gcc 5.4.0
				//
				prg << cyng::unwind<cyng::vector_t>(query(evt.query_));

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
				CYNG_LOG_DEBUG(logger_, sp_->vm().tag() << " QUERY_PROTOCOL_VERSION");
				prg << cyng::generate_invoke_unwinded("req.protocol.version");
			}
			if ((query & QUERY_FIRMWARE_VERSION) == QUERY_FIRMWARE_VERSION)
			{
				CYNG_LOG_DEBUG(logger_, sp_->vm().tag() << " QUERY_FIRMWARE_VERSION");
				prg << cyng::generate_invoke_unwinded("req.software.version");
			}
			if ((query & QUERY_DEVICE_IDENTIFIER) == QUERY_DEVICE_IDENTIFIER)
			{
				CYNG_LOG_DEBUG(logger_, sp_->vm().tag() << " QUERY_DEVICE_IDENTIFIER");
				prg << cyng::generate_invoke_unwinded("req.device.id");
			}
			if ((query & QUERY_NETWORK_STATUS) == QUERY_NETWORK_STATUS)
			{
				CYNG_LOG_DEBUG(logger_, sp_->vm().tag() << " QUERY_NETWORK_STATUS");
				prg << cyng::generate_invoke_unwinded("req.net.status");
			}
			if ((query & QUERY_IP_STATISTIC) == QUERY_IP_STATISTIC)
			{
				CYNG_LOG_DEBUG(logger_, sp_->vm().tag() << " QUERY_IP_STATISTIC");
				prg << cyng::generate_invoke_unwinded("req.ip.statistics");
			}
			if ((query & QUERY_DEVICE_AUTHENTIFICATION) == QUERY_DEVICE_AUTHENTIFICATION)
			{
				CYNG_LOG_DEBUG(logger_, sp_->vm().tag() << " QUERY_DEVICE_AUTHENTIFICATION");
				prg << cyng::generate_invoke_unwinded("req.device.auth");
			}
			if ((query & QUERY_DEVICE_TIME) == QUERY_DEVICE_TIME)
			{
				CYNG_LOG_DEBUG(logger_, sp_->vm().tag() << " QUERY_DEVICE_TIME");
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

		cyng::vector_t session_state::react(state::evt_req_login_public evt)
		{
			cyng::vector_t prg;

			switch (state_) {
			case internal_state::IDLE:
				break;
			default:
				signal_wrong_state("evt_req_login_public");
				return prg;
			}

			CYNG_LOG_INFO(logger_, "ipt.req.login.public "
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
						("tp-layer", "ipt")
						("security", "public")
						("time", std::chrono::system_clock::now())
						("local-ep", evt.lep_)
						("remote-ep", evt.rep_)));

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
			case internal_state::IDLE:
				break;
			default:
				signal_wrong_state("evt_req_login_scrambled");
				return prg;
			}

			CYNG_LOG_INFO(logger_, "ipt.req.login.scrambled "
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
						("tp-layer", "ipt")
						("security", "scrambled")
						("time", std::chrono::system_clock::now())
						("local-ep", evt.lep_)
						("remote-ep", evt.rep_)
					));

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
			case internal_state::AUTHORIZED:
				break;
			default:
				signal_wrong_state("evt_client_req_open_connection");
				return prg;
			}

			if (evt.success_) {

				//
				//	update state
				//
				transit(internal_state::WAIT_FOR_OPEN_RESPONSE);
				wait_for_open_response_.init(evt.tsk_, /*evt.origin_tag_, */evt.local_, evt.seq_, evt.master_, evt.client_);
			}

			return prg;
		}

		cyng::vector_t session_state::react(state::evt_client_res_close_connection evt)
		{
			cyng::vector_t prg;

			switch (state_) {
			case internal_state::WAIT_FOR_CLOSE_RESPONSE:
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
				CYNG_LOG_WARNING(logger_, "client.req.close.connection.forward - high cluster lag: " << lag.count() << " millisec");
			}
			else {
				CYNG_LOG_TRACE(logger_, "client.req.close.connection.forward - cluster lag: " << lag.count() << " millisec");
			}

			//
			//	reset connection state
			//
			transit(internal_state::AUTHORIZED);
			wait_for_close_response_.reset();

			return prg;
		}

		cyng::vector_t session_state::react(state::evt_client_res_open_connection evt)
		{
			cyng::vector_t prg;

			switch (state_) {
			//case internal_state::WAIT_FOR_OPEN_RESPONSE:
			case internal_state::AUTHORIZED:
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
				//	hides outer variable dom
				//
				auto dom = cyng::make_reader(evt.master_);
				const auto local = cyng::value_cast(dom.get("local-connect"), false);

				CYNG_LOG_TRACE(logger_, sp_->vm().tag()
					<< " set connection state to "
					<< (local ? "local" : "remote"));

				//
				//	update state
				//
				transit(local ? internal_state::CONNECTED_LOCAL : internal_state::CONNECTED_REMOTE);
				prg << cyng::generate_invoke_unwinded("res.open.connection", seq, static_cast<response_type>(tp_res_open_connection_policy::DIALUP_SUCCESS));
			}
			else
			{
				//
				//	update state
				//	To reset the session state to AUTHORIZED reflect the fact that this session is no longer part in any
				//	attempt to establish a connection.
				//
				transit(internal_state::AUTHORIZED);
				prg << cyng::generate_invoke_unwinded("res.open.connection", seq, static_cast<response_type>(tp_res_open_connection_policy::DIALUP_FAILED));
			}

			prg << cyng::generate_invoke_unwinded("stream.flush");

			return prg;
		}

		cyng::vector_t session_state::react(state::evt_ipt_req_close_connection evt)
		{
			cyng::vector_t prg;

			switch (state_) {
			case internal_state::CONNECTED_LOCAL:
			case internal_state::CONNECTED_REMOTE:
				transit(internal_state::WAIT_FOR_CLOSE_RESPONSE);
				break;
			default:
				signal_wrong_state("evt_ipt_req_close_connection");
				return prg;
			}

			CYNG_LOG_TRACE(logger_, sp_->vm().tag() << " received connection close request");

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
			case internal_state::WAIT_FOR_OPEN_RESPONSE:
				break;
			default:
				signal_wrong_state("evt_ipt_res_open_connection");
				return prg;
			}

			//
			//	stop open connection task
			//
			BOOST_ASSERT(evt.tsk_ == wait_for_open_response_.tsk_connection_open_);
			sp_->mux_.post(evt.tsk_, evt.slot_, cyng::tuple_factory(evt.res_));

			//
			//	update session state
			//
			if (ipt::tp_res_open_connection_policy::is_success(evt.res_)) {
				switch (wait_for_open_response_.type_) {
				case state::state_wait_for_open_response::E_LOCAL:
					sp_->bus_->vm_.async_run(wait_for_open_response_.establish_local_connection());
					transit(internal_state::CONNECTED_LOCAL);
					break;
				case state::state_wait_for_open_response::E_REMOTE:
					transit(internal_state::CONNECTED_REMOTE);
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
				transit(internal_state::AUTHORIZED);
			}
			
			if (sp_->bus_->is_online()) {
				sp_->bus_->vm_.async_run(client_res_open_connection(wait_for_open_response_.get_origin_tag()
					, wait_for_open_response_.seq_	//	cluster sequence
					, ipt::tp_res_open_connection_policy::is_success(evt.res_)
					, wait_for_open_response_.master_params_
					, wait_for_open_response_.client_params_));
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

		cyng::vector_t session_state::react(state::evt_ipt_res_close_connection evt)
		{
			cyng::vector_t prg;

			switch (state_) {
			case internal_state::WAIT_FOR_CLOSE_RESPONSE:
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
			transit(internal_state::AUTHORIZED);

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
			case internal_state::CONNECTED_LOCAL:
			case internal_state::CONNECTED_REMOTE:
				break;
			default:
				signal_wrong_state("evt_client_req_close_connection");
				return prg;
			}

			if (evt.success_) {

				//
				//	initialize next state
				//
				wait_for_close_response_.init(evt.tsk_, evt.tag_, evt.shutdown_, evt.seq_, evt.master_, evt.client_, state_ == internal_state::CONNECTED_LOCAL);

				//
				//	update state
				//
				transit(internal_state::WAIT_FOR_CLOSE_RESPONSE);
			}
			else {
				CYNG_LOG_FATAL(logger_, "cannot start close_connection");
			}

			return prg;
		}

		void session_state::react(state::evt_sml_msg evt)
		{
			switch (state_) {
			case internal_state::CONNECTED_TASK:
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
			//	message slot (3) - get_proc_parameter
			//
			BOOST_ASSERT_MSG(evt.msg_.size() == 5, "evt_sml_msg");
			sp_->mux_.post(task_.tsk_proxy_, 3, std::move(evt.msg_));
		}

		void session_state::react(state::evt_sml_eom evt)
		{
			switch (state_) {
			case internal_state::CONNECTED_TASK:
				break;
			default:
				signal_wrong_state("evt_sml_eom");
				return;
			}

			CYNG_LOG_DEBUG(logger_, "evt_sml_eom to #" << task_.tsk_proxy_);

			//
			//	message slot (1)
			//	CRC, counter, pk
			//
			BOOST_ASSERT_MSG(evt.tpl_.size() == 3, "evt_sml_eom");
			sp_->mux_.post(task_.tsk_proxy_, 1, std::move(evt.tpl_));
		}

		void session_state::react(state::evt_sml_public_close_response evt)
		{
			switch (state_) {
			case internal_state::CONNECTED_TASK:
				break;
			default:
				signal_wrong_state("evt_sml_public_close_response");
				return;
			}

			CYNG_LOG_DEBUG(logger_, "evt_sml_public_close_response to #" << task_.tsk_proxy_);

			//
			//	message slot (4)
			//
			BOOST_ASSERT(evt.vec_.size() == 1);
			//sp_->mux_.post(task_.tsk_proxy_, 4, cyng::tuple_factory(evt.vec_.at(0)));
			task_.close_response(sp_->mux_, evt.vec_);

		}

		void session_state::react(state::evt_sml_get_proc_param_response evt)
		{
			switch (state_) {
			case internal_state::CONNECTED_TASK:
				break;
			default:
				signal_wrong_state("evt_sml_get_proc_param_response");
				return;
			}

			//BOOST_ASSERT(evt.vec_.size() == 8);
			CYNG_LOG_DEBUG(logger_, "evt_sml_get_proc_param_response to #" << task_.tsk_proxy_);

			//
			//	message slot (3)
			//
			task_.get_proc_parameter(sp_->mux_, evt.vec_);
		}

		void session_state::react(state::evt_sml_get_list_response evt)
		{
			switch (state_) {
			case internal_state::CONNECTED_TASK:
				break;
			default:
				signal_wrong_state("evt_sml_get_list_response");
				return;
			}

			//	[]
			CYNG_LOG_DEBUG(logger_, "evt_sml_get_list_response to #" << task_.tsk_proxy_);
			CYNG_LOG_DEBUG(logger_, "evt_sml_get_list_response: "	<< cyng::io::to_str(evt.vec_));

			//
			//	message slot (5)
			//
			task_.get_list_response(sp_->mux_, evt.vec_);

		}

		void session_state::react(state::evt_sml_get_profile_list_response evt)
		{
			switch (state_) {
			case internal_state::CONNECTED_TASK:
				break;
			default:
				signal_wrong_state("evt_sml_get_profile_list_response");
				return;
			}

			//	[]
			CYNG_LOG_DEBUG(logger_, "evt_sml_get_profile_list_response to #" << task_.tsk_proxy_);
			CYNG_LOG_DEBUG(logger_, "evt_sml_get_profile_list_response: " << cyng::io::to_str(evt.vec_));

			//
			//	message slot [8]
			//
			task_.get_profile_list_response(sp_->mux_, evt.vec_);

		}

		void session_state::react(state::evt_sml_attention_msg evt)
		{
			switch (state_) {
			case internal_state::CONNECTED_TASK:
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
			case session_state::internal_state::IDLE:
				os << "IDLE";
				break;
			case session_state::internal_state::FAILURE:
				os << "FAILURE";
				break;
			case session_state::internal_state::SHUTDOWN:
				os << "SHUTDOWN";
				break;
			case session_state::internal_state::AUTHORIZED:
				os << "AUTHORIZED";
				break;
			case session_state::internal_state::WAIT_FOR_OPEN_RESPONSE:
				os << "WAIT_FOR_OPEN_RESPONSE";
				break;
			case session_state::internal_state::WAIT_FOR_CLOSE_RESPONSE:
				os << "WAIT_FOR_CLOSE_RESPONSE";
				break;
			case session_state::internal_state::CONNECTED_LOCAL:
				os << "CONNECTED_LOCAL";
				break;
			case session_state::internal_state::CONNECTED_REMOTE:
				os << "CONNECTED_REMOTE";
				break;
			case session_state::internal_state::CONNECTED_TASK:
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
			//	EVENT: scrambled login
			//
			evt_req_login_scrambled::evt_req_login_scrambled(boost::uuids::uuid tag
				, std::string const& name
				, std::string const& pwd
				, scramble_key const& sk
				, boost::asio::ip::tcp::endpoint lep
				, boost::asio::ip::tcp::endpoint rep)
			: tag_(tag)
				, name_(name)
				, pwd_(pwd)
				, sk_(sk)
				, lep_(lep)
				, rep_(rep)
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
				//, boost::uuids::uuid origin_tag
				, bool local
				, std::size_t seq
				, cyng::param_map_t master
				, cyng::param_map_t client)
			: tsk_(r.first)
				, success_(r.second)
				//, origin_tag_(origin_tag)
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
			//	EVENT: gateway
			//
			evt_gateway::evt_gateway(cyng::tuple_t&& tpl)
				: tpl_(std::forward<cyng::tuple_t>(tpl))
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
			evt_sml_public_close_response::evt_sml_public_close_response(cyng::vector_t vec)
				: vec_(std::move(vec))
			{}

			//
			//	EVENT: evt_sml_get_proc_param_response
			//
			evt_sml_get_proc_param_response::evt_sml_get_proc_param_response(cyng::vector_t&& vec)
				: vec_(std::move(vec))
			{}

			//
			//	EVENT: evt_sml_get_list_response
			//
			evt_sml_get_list_response::evt_sml_get_list_response(cyng::vector_t vec)
				: vec_(vec)
			{}

			//
			//	EVENT: evt_sml_get_profile_list_response
			//
			evt_sml_get_profile_list_response::evt_sml_get_profile_list_response(cyng::vector_t vec)
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
				, type_(E_UNDEF)
				, seq_(0u)
				, master_params_()
				, client_params_()
			{}

			void state_wait_for_open_response::init(std::size_t tsk
				, bool local
				, std::uint64_t seq
				, cyng::param_map_t master
				, cyng::param_map_t client)
			{
				tsk_connection_open_ = tsk;
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
				tsk_connection_open_ = cyng::async::NO_TASK;
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

			void state_wait_for_close_response::terminate(bus::shared_type bus, response_type res)
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
				if (cyng::async::NO_TASK != tsk) mux.post(tsk, 0, cyng::tuple_t{});
			}

			void state_connected_task::transmit(cyng::async::mux& mux, cyng::object obj)
			{
				mux.post(tsk_proxy_, 2, cyng::tuple_factory(obj));
			}

			void state_connected_task::get_proc_parameter(cyng::async::mux& mux, cyng::vector_t vec)
			{
				//
				//	post message to gateway proxy slot [3]
				//
				mux.post(tsk_proxy_, 3, cyng::to_tuple(vec));
			}

			void state_connected_task::get_list_response(cyng::async::mux& mux, cyng::vector_t vec)
			{
				mux.post(tsk_proxy_, 5, cyng::tuple_t{
					vec.at(0),	//	trx
					vec.at(1),	//	server ID - mostly empty
					vec.at(2),	//	OBIS_CODE(99, 00, 00, 00, 00, 03)
					vec.at(3)	//	complete parameter list with values
					});
			}

			void state_connected_task::get_profile_list_response(cyng::async::mux& mux, cyng::vector_t vec)
			{
				//	[4076396-2,null,072ff3ff,072ff3d4,,0500153B02297E,00072202]
				BOOST_ASSERT(vec.size() == 8);

				//	* trx
				//	* actTime
				//	* regPeriod
				//	* valTime
				//	* path (OBIS)
				//	* server id
				//	* status
				//	* params
				mux.post(tsk_proxy_, 8, cyng::to_tuple(vec));
			}

			void state_connected_task::attention_msg(cyng::async::mux& mux, cyng::vector_t vec)
			{
				//
				//	send
				//	* [string] trx
				//	* [string] srv
				//	* [buffer] OBIS code
				//
				mux.post(tsk_proxy_, 6, cyng::tuple_t{ vec.at(0), vec.at(1), vec.at(2) });
			}

			void state_connected_task::close_response(cyng::async::mux& mux, cyng::vector_t vec)
			{
				mux.post(tsk_proxy_, 4, cyng::to_tuple(vec));

			}

		}
	}
}



