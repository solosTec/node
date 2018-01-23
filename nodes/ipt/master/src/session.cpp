/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 


#include "session.h"
#include <NODE_project_info.h>
#include <smf/cluster/generator.h>
#include <smf/ipt/response.hpp>
#include <smf/ipt/scramble_key_io.hpp>
#include <cyng/vm/domain/log_domain.h>
#include <cyng/vm/domain/store_domain.h>
#include <cyng/io/serializer.h>
#include <cyng/value_cast.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/nil_generator.hpp>

namespace node 
{
	namespace ipt
	{
		session::session(cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, bus::shared_type bus
			, boost::uuids::uuid tag
			, scramble_key const& sk
			, std::uint16_t watchdog
			, std::chrono::seconds const& timeout)
		: mux_(mux)
			, logger_(logger)
			, bus_(bus)
			, vm_(mux.get_io_service(), tag)
			, sk_(sk)
			, watchdog_(watchdog)
			, timeout_(timeout)
			, parser_([this](cyng::vector_t&& prg) {
				CYNG_LOG_INFO(logger_, prg.size() << " instructions received");
				CYNG_LOG_TRACE(logger_, cyng::io::to_str(prg));
				vm_.async_run(std::move(prg));
			}, sk)
		{
			//
			//	register logger domain
			//
			cyng::register_logger(logger_, vm_);
			vm_.async_run(cyng::generate_invoke("log.msg.info", "log domain is running"));

			//
			//	register request handler
			//
			vm_.async_run(cyng::register_function("ipt.req.login.public", 2, std::bind(&session::ipt_req_login_public, this, std::placeholders::_1)));
			vm_.async_run(cyng::register_function("ipt.req.login.scrambled", 3, std::bind(&session::ipt_req_login_scrambled, this, std::placeholders::_1)));
			vm_.async_run(cyng::register_function("ipt.req.open.push.channel", 8, std::bind(&session::ipt_req_open_push_channel, this, std::placeholders::_1)));
			vm_.async_run(cyng::register_function("ipt.req.transmit.data", 1, std::bind(&session::ipt_req_transmit_data, this, std::placeholders::_1)));
			vm_.async_run(cyng::register_function("ipt.res.watchdog", 0, std::bind(&session::ipt_res_watchdog, this, std::placeholders::_1)));
			//client.res.open.push.channel

			//
			//	ToDo: start maintenance task
			//
		}

		void session::ipt_req_login_public(cyng::context& ctx)
		{
			//	[LSMTest5,LSMTest5,<sk>]
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "ipt.req.login.public " << cyng::io::to_str(frame));

			//
			//	send authorization request to master
			//
			if (bus_->is_online())
			{
				//const std::string name = cyng::value_cast<std::string>(frame.at(0));
				cyng::param_map_t bag;
				bag["tp-layer"] = cyng::make_object("ipt");
				//cyng::tuple_factory(cyng::param_factory("log-dir", tmp.string()));
				bus_->vm_.async_run(client_req_login(cyng::value_cast(frame.at(0), boost::uuids::nil_uuid())
					, cyng::value_cast<std::string>(frame.at(1), "")
					, cyng::value_cast<std::string>(frame.at(2), "")
					, "plain" //	login scheme
					, bag));

			}
			else
			{
				CYNG_LOG_WARNING(logger_, "cannot forward authorization request - no master");

				//
				//	reject login - no master
				//
				const response_type res = ctrl_res_login_public_policy::MALFUNCTION;

				vm_.async_run(cyng::generate_invoke("res.login.public", res, watchdog_, ""));
				vm_.async_run(cyng::generate_invoke("stream.flush"));
			}


		}

		void session::ipt_req_login_scrambled(cyng::context& ctx)
		{
			//	[LSMTest5,LSMTest5,<sk>]
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "ipt.req.login.scrambled " << cyng::io::to_str(frame));

			CYNG_LOG_TRACE(logger_, "set new scramble key "
				<< cyng::value_cast<scramble_key>(frame.at(3), scramble_key::null_scramble_key_));

			//
			//	ToDo: test if already authorized
			//

			//
			//	send authorization request to master
			//
			if (bus_->is_online())
			{
				//const std::string name = cyng::value_cast<std::string>(frame.at(0));
				cyng::param_map_t bag;
				bag["tp-layer"] = cyng::make_object("ipt");
				//cyng::tuple_factory(cyng::param_factory("log-dir", tmp.string()));
				bus_->vm_.async_run(client_req_login(cyng::value_cast(frame.at(0), boost::uuids::nil_uuid())
					, cyng::value_cast<std::string>(frame.at(1), "")
					, cyng::value_cast<std::string>(frame.at(2), "")
					, "plain" //	login scheme
					, bag));
				
			}
			else
			{
				CYNG_LOG_WARNING(logger_, "cannot forward authorization request - no master");

				//
				//	reject login - no master
				//
				const response_type res = ctrl_res_login_public_policy::MALFUNCTION;

				vm_.async_run(cyng::generate_invoke("res.login.scrambled", res, watchdog_, ""));
				vm_.async_run(cyng::generate_invoke("stream.flush"));
			}

		}

		void session::ipt_req_open_push_channel(cyng::context& ctx)
		{
			//BOOST_ASSERT_MSG(bus_->is_online(), "no master");

			//	[3fdd94d7-03e3-40ed-835f-79a7ddac2180,,LSM_Events,,,,,60]
			//
			//	* session id
			//	* ipt sequence
			//	* target name
			//	* account
			//	* number
			//	* version
			//	* device id
			//	* max. time out
			//
			const cyng::vector_t frame = ctx.get_frame();

			if (bus_->is_online())
			{
				CYNG_LOG_INFO(logger_, "ipt.req.open.push.channel " << cyng::io::to_str(frame));
				cyng::param_map_t bag;
				bag["tp-layer"] = cyng::make_object("ipt");
				bag["seq"] = frame.at(1);
				bus_->vm_.async_run(client_req_open_push_channel(cyng::value_cast(frame.at(0), boost::uuids::nil_uuid())
					, cyng::value_cast<std::string>(frame.at(2), "")
					, cyng::value_cast<std::string>(frame.at(3), "")
					, cyng::value_cast<std::string>(frame.at(4), "")
					, cyng::value_cast<std::string>(frame.at(5), "")
					, cyng::value_cast<std::string>(frame.at(6), "")
					, cyng::value_cast<std::uint16_t>(frame.at(7), 30)
					, bag));
			}
			else
			{
				CYNG_LOG_ERROR(logger_, "ipt.req.open.push.channel - no master " << cyng::io::to_str(frame));

			}
		}

		void session::ipt_req_transmit_data(cyng::context& ctx)
		{
			BOOST_ASSERT_MSG(bus_->is_online(), "no master");

			//	[]
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "ipt.req.transmit.data " << cyng::io::to_str(frame));
		}

		void session::ipt_res_watchdog(cyng::context& ctx)
		{
			BOOST_ASSERT_MSG(bus_->is_online(), "no master");

			//	[]
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "ipt.res.watchdog " << cyng::io::to_str(frame));

		}


		void session::client_res_login(std::uint64_t seq, bool success, std::string msg)
		{

			const response_type res = success 
				? ctrl_res_login_public_policy::SUCCESS
				: ctrl_res_login_public_policy::UNKNOWN_ACCOUNT
				;

			if (success)
			{
				//CYNG_LOG_TRACE(logger_, "send login response - watchdog: " << watchdog_);
				vm_.async_run(cyng::generate_invoke("log.msg.info"
					, "send login response"
					, msg
					, "watchdog"
					, watchdog_	));
			}
			else
			{
				//CYNG_LOG_WARNING(logger_, "send login response");
				vm_.async_run(cyng::generate_invoke("log.msg.warning"
					, "send login response"
					, msg));
			}

			vm_.async_run(cyng::generate_invoke("res.login.scrambled", res, watchdog_, ""));
			vm_.async_run(cyng::generate_invoke("stream.flush"));
		}
	}
}



