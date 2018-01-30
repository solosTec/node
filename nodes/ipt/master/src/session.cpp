/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 


#include "session.h"
#include "tasks/open_connection.h"
#include <NODE_project_info.h>
#include <smf/cluster/generator.h>
#include <smf/ipt/response.hpp>
#include <smf/ipt/scramble_key_io.hpp>
#include <cyng/vm/domain/log_domain.h>
#include <cyng/vm/domain/store_domain.h>
#include <cyng/io/serializer.h>
#include <cyng/value_cast.hpp>
#include <cyng/dom/reader.h>
#include <cyng/async/task/task_builder.hpp>
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
			, task_db_()
		{
			//
			//	register logger domain
			//
			cyng::register_logger(logger_, vm_);
			vm_.run(cyng::generate_invoke("log.msg.info", "log domain is running"));

			vm_.run(cyng::register_function("session.store.relation", 0, std::bind(&session::store_relation, this, std::placeholders::_1)));

			//
			//	register request handler
			//	client.req.transmit.data.forward
			vm_.run(cyng::register_function("ipt.req.transmit.data", 2, std::bind(&session::ipt_req_transmit_data, this, std::placeholders::_1)));
			vm_.run(cyng::register_function("client.req.transmit.data.forward", 5, std::bind(&session::client_req_transmit_data_forward, this, std::placeholders::_1)));


			//	transport
			//	transport - push channel open
			//TP_REQ_OPEN_PUSH_CHANNEL = 0x9000,
			vm_.run(cyng::register_function("ipt.req.open.push.channel", 8, std::bind(&session::ipt_req_open_push_channel, this, std::placeholders::_1)));
			vm_.run(cyng::register_function("client.res.open.push.channel", 9, std::bind(&session::client_res_open_push_channel, this, std::placeholders::_1)));

			//TP_RES_OPEN_PUSH_CHANNEL = 0x1000,	//!<	response

			//	transport - push channel close
			//TP_REQ_CLOSE_PUSH_CHANNEL = 0x9001,	//!<	request
			vm_.run(cyng::register_function("ipt.req.close.push.channel", 0, std::bind(&session::ipt_req_close_push_channel, this, std::placeholders::_1)));
			vm_.run(cyng::register_function("client.res.close.push.channel", 1, std::bind(&session::client_res_close_push_channel, this, std::placeholders::_1)));
			//TP_RES_CLOSE_PUSH_CHANNEL = 0x1001,	//!<	response

			//	transport - push channel data transfer
			//TP_REQ_PUSHDATA_TRANSFER = 0x9002,	//!<	request
			//TP_RES_PUSHDATA_TRANSFER = 0x1002,	//!<	response

			//	transport - connection open
			//TP_REQ_OPEN_CONNECTION = 0x9003,	//!<	request
			vm_.run(cyng::register_function("ipt.req.open.connection", 3, std::bind(&session::ipt_req_open_connection, this, std::placeholders::_1)));
			vm_.run(cyng::register_function("client.req.open.connection.forward", 4, std::bind(&session::client_req_open_connection_forward, this, std::placeholders::_1)));
			//TP_RES_OPEN_CONNECTION = 0x1003,	//!<	response
			vm_.run(cyng::register_function("ipt.res.open.connection", 3, std::bind(&session::ipt_res_open_connection, this, std::placeholders::_1)));
			vm_.run(cyng::register_function("client.res.open.connection", 6, std::bind(&session::client_res_open_connection, this, std::placeholders::_1)));
			vm_.run(cyng::register_function("client.res.open.connection.forward", 6, std::bind(&session::client_res_open_connection_forward, this, std::placeholders::_1)));

			//	transport - connection close
			//TP_REQ_CLOSE_CONNECTION = 0x9004,	//!<	request
			vm_.run(cyng::register_function("ipt.req.close.connection", 0, std::bind(&session::ipt_req_close_connection, this, std::placeholders::_1)));
			//TP_RES_CLOSE_CONNECTION = 0x1004,	//!<	response

			//	open stream channel
			//TP_REQ_OPEN_STREAM_CHANNEL = 0x9006,
			//TP_RES_OPEN_STREAM_CHANNEL = 0x1006,

			//	close stream channel
			//TP_REQ_CLOSE_STREAM_CHANNEL = 0x9007,
			//TP_RES_CLOSE_STREAM_CHANNEL = 0x1007,

			//	stream channel data transfer
			//TP_REQ_STREAMDATA_TRANSFER = 0x9008,
			//TP_RES_STREAMDATA_TRANSFER = 0x1008,

			//	transport - connection data transfer *** non-standard ***
			//	0x900B:	//	request
			//	0x100B:	//	response

			//	application
			//	application - protocol version
			//APP_REQ_PROTOCOL_VERSION = 0xA000,	//!<	request
			//APP_RES_PROTOCOL_VERSION = 0x2000,	//!<	response

			//	application - device firmware version
			//APP_REQ_SOFTWARE_VERSION = 0xA001,	//!<	request
			//APP_RES_SOFTWARE_VERSION = 0x2001,	//!<	response

			//	application - device identifier
			//APP_REQ_DEVICE_IDENTIFIER = 0xA003,	//!<	request
			//APP_RES_DEVICE_IDENTIFIER = 0x2003,

			//	application - network status
			//APP_REQ_NETWORK_STATUS = 0xA004,	//!<	request
			//APP_RES_NETWORK_STATUS = 0x2004,	//!<	response

			//	application - IP statistic
			//APP_REQ_IP_STATISTICS = 0xA005,	//!<	request
			//APP_RES_IP_STATISTICS = 0x2005,	//!<	response

			//	application - device authentification
			//APP_REQ_DEVICE_AUTHENTIFICATION = 0xA006,	//!<	request
			//APP_RES_DEVICE_AUTHENTIFICATION = 0x2006,	//!<	response

			//	application - device time
			//APP_REQ_DEVICE_TIME = 0xA007,	//!<	request
			//APP_RES_DEVICE_TIME = 0x2007,	//!<	response

			//	application - push-target namelist
			//APP_REQ_PUSH_TARGET_NAMELIST = 0xA008,	//!<	request
			//APP_RES_PUSH_TARGET_NAMELIST = 0x2008,	//!<	response

			//	application - push-target echo
			//APP_REQ_PUSH_TARGET_ECHO = 0xA009,	//!<	*** deprecated ***
			//APP_RES_PUSH_TARGET_ECHO = 0x2009,	//!<	*** deprecated ***

			//	application - traceroute
			//APP_REQ_TRACEROUTE = 0xA00A,	//!<	*** deprecated ***
			//APP_RES_TRACEROUTE = 0x200A,	//!<	*** deprecated ***

			//	control
			//CTRL_RES_LOGIN_PUBLIC = 0x4001,	//!<	public login response
			//CTRL_RES_LOGIN_SCRAMBLED = 0x4002,	//!<	scrambled login response
			//CTRL_REQ_LOGIN_PUBLIC = 0xC001,	//!<	public login request
			//CTRL_REQ_LOGIN_SCRAMBLED = 0xC002,	//!<	scrambled login request
			vm_.run(cyng::register_function("ipt.req.login.public", 2, std::bind(&session::ipt_req_login_public, this, std::placeholders::_1)));
			vm_.run(cyng::register_function("ipt.req.login.scrambled", 3, std::bind(&session::ipt_req_login_scrambled, this, std::placeholders::_1)));
			vm_.run(cyng::register_function("client.res.login", 4, std::bind(&session::client_res_login, this, std::placeholders::_1)));

			//	control - maintenance
			//MAINTENANCE_REQUEST = 0xC003,	//!<	request *** deprecated ***
			//MAINTENANCE_RESPONSE = 0x4003,	//!<	response *** deprecated ***

			//	control - logout
			//CTRL_REQ_LOGOUT = 0xC004,	//!<	request *** deprecated ***
			//CTRL_RES_LOGOUT = 0x4004,	//!<	response *** deprecated ***

			//	control - push target register
			//CTRL_REQ_REGISTER_TARGET = 0xC005,	//!<	request
			vm_.run(cyng::register_function("ipt.req.register.push.target", 5, std::bind(&session::ipt_req_register_push_target, this, std::placeholders::_1)));
			vm_.run(cyng::register_function("client.res.register.push.target", 7, std::bind(&session::client_res_register_push_target, this, std::placeholders::_1)));
			//CTRL_RES_REGISTER_TARGET = 0x4005,	//!<	response

			//	control - push target deregister
			//CTRL_REQ_DEREGISTER_TARGET = 0xC006,	//!<	request
			//CTRL_RES_DEREGISTER_TARGET = 0x4006,	//!<	response

			//	control - watchdog
			//CTRL_REQ_WATCHDOG = 0xC008,	//!<	request
			//CTRL_RES_WATCHDOG = 0x4008,	//!<	response
			vm_.run(cyng::register_function("ipt.res.watchdog", 0, std::bind(&session::ipt_res_watchdog, this, std::placeholders::_1)));

			//	control - multi public login request
			//MULTI_CTRL_REQ_LOGIN_PUBLIC = 0xC009,	//!<	request
			//MULTI_CTRL_RES_LOGIN_PUBLIC = 0x4009,	//!<	response

			//	control - multi public login request
			//MULTI_CTRL_REQ_LOGIN_SCRAMBLED = 0xC00A,	//!<	request
			//MULTI_CTRL_RES_LOGIN_SCRAMBLED = 0x400A,	//!<	response

			//	stream source register
			//CTRL_REQ_REGISTER_STREAM_SOURCE = 0xC00B,
			//CTRL_RES_REGISTER_STREAM_SOURCE = 0x400B,

			//	stream source deregister
			//CTRL_REQ_DEREGISTER_STREAM_SOURCE = 0xC00C,
			//CTRL_RES_DEREGISTER_STREAM_SOURCE = 0x400C,

			//	server mode
			//SERVER_MODE_REQUEST = 0xC010,	//!<	request
			//SERVER_MODE_RESPONSE = 0x4010,	//!<	response

			//	server mode reconnect
			//SERVER_MODE_RECONNECT_REQUEST = 0xC011,	//!<	request
			//SERVER_MODE_RECONNECT_RESPONSE = 0x4011,	//!<	response


			//UNKNOWN = 0x7fff,	//!<	unknown command

			
			//
			//	ToDo: start maintenance task
			//
		}

		void session::store_relation(cyng::context& ctx)
		{
			//	[1,2]
			//
			//	* ipt sequence number
			//	* task id
			//	
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "session.store.relation " << cyng::io::to_str(frame));

			task_db_.emplace(cyng::value_cast<sequence_type>(frame.at(0), 0), cyng::value_cast<std::size_t>(frame.at(1), 0));
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
				bag["security"] = cyng::make_object("public");

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

				vm_.run(cyng::generate_invoke("res.login.public", res, watchdog_, ""));
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
				bag["security"] = cyng::make_object("scrambled");

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
				ctx.run(cyng::generate_invoke("log.msg.info", "ipt.req.open.push.channel", frame));

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

		void session::ipt_req_close_push_channel(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "ipt_req_close_push_channel " << cyng::io::to_str(frame));
		}

		void session::ipt_req_transmit_data(cyng::context& ctx)
		{
			//	[ipt.req.transmit.data,
			//	[c5eae235-d5f5-413f-a008-5d317d8baab7,1B1B1B1B01010101768106313830313...1B1B1B1B1A034276]]
			const cyng::vector_t frame = ctx.get_frame();
			if (bus_->is_online())
			{
				const boost::uuids::uuid tag = cyng::value_cast(frame.at(0), boost::uuids::nil_uuid());

				cyng::param_map_t bag;
				bag["tp-layer"] = cyng::make_object("ipt");
				//bag["app-layer"] = cyng::make_object("sml");

				//bus_->vm_.async_run(cyng::generate_invoke("log.msg.info", "ipt.req.transmit.data", frame));
				bus_->vm_.async_run(client_req_transmit_data(tag
					, bag
					, frame.at(1)));
			}
			else
			{
				ctx.run(cyng::generate_invoke("log.msg.warning", "ipt.req.transmit.data", "no master", frame));
			}
		}

		void session::client_req_transmit_data_forward(cyng::context& ctx)
		{
			//	[client.req.transmit.data.forward,
			//	[812518c9-c532-413f-a1c8-6d4a73de35b9,95a4ccf9-1171-4ff0-ad64-d06cb74da24e,8,
			//	%(("tp-layer":ipt)),
			//	1B1B1B1B01010101768106313830313330323133...0000001B1B1B1B1A0353AD]]
			//
			//	* session tag
			//	* peer
			//	* cluster sequence
			//	* bag
			//	* data
			//
			const cyng::vector_t frame = ctx.get_frame();
			ctx.run(cyng::generate_invoke("log.msg.info", "client.req.transmit.data.forward", frame));

			CYNG_LOG_INFO(logger_, "---=== T R A N S F E R ===---");

			vm_.async_run(cyng::generate_invoke("ipt.transfer.data", frame.at(4)));
			vm_.async_run(cyng::generate_invoke("stream.flush"));
			
		}

		void session::ipt_res_watchdog(cyng::context& ctx)
		{
			//BOOST_ASSERT_MSG(bus_->is_online(), "no master");

			//	[]
			const cyng::vector_t frame = ctx.get_frame();
			//ctx.run(cyng::generate_invoke("log.msg.info", "ipt.res.watchdog", frame));
			if (bus_->is_online())
			{
				ctx.run(cyng::generate_invoke("log.msg.info", "ipt.res.watchdog", frame));
			}
			else
			{
				ctx.run(cyng::generate_invoke("log.msg.warning", "ipt.res.watchdog", "no master", frame));

			}

		}

		void session::client_res_login(cyng::context& ctx)
		//void session::client_res_login(std::uint64_t seq, bool success, std::string msg)
		{
			const cyng::vector_t frame = ctx.get_frame();

			//	* seq
			//	* success
			//	* msg
			//	* boost::algorithm::equals(security, "scrambled")	//	public/scrambled

			const sequence_type seq = cyng::value_cast<sequence_type>(frame.at(0), 0);
			const bool success = cyng::value_cast(frame.at(1), false);
			const std::string msg = cyng::value_cast<std::string>(frame.at(2), "");
			const bool scrambled = cyng::value_cast(frame.at(3), false);

			const response_type res = success 
				? ctrl_res_login_public_policy::SUCCESS
				: ctrl_res_login_public_policy::UNKNOWN_ACCOUNT
				;

			if (scrambled)
			{
				vm_.async_run(cyng::generate_invoke("res.login.scrambled", res, watchdog_, ""));
			}
			else
			{
				vm_.async_run(cyng::generate_invoke("res.login.public", res, watchdog_, ""));
			}
			vm_.async_run(cyng::generate_invoke("stream.flush"));

			if (success)
			{
				vm_.async_run(cyng::generate_invoke("log.msg.info"
					, "send login response"
					, msg
					, "watchdog"
					, watchdog_));
			}
			else
			{
				vm_.async_run(cyng::generate_invoke("log.msg.warning"
					, "send login response"
					, msg));

				//
				//	close session
				//
				vm_.async_run(cyng::generate_invoke("ip.tcp.socket.shutdown"));
				vm_.async_run(cyng::generate_invoke("ip.tcp.socket.close"));
			}

		}

		void session::client_res_open_push_channel(cyng::context& ctx)
		{
			//	[85058f73-a243-42a7-908c-85b3a3f29f62,01027bb4-8964-4577-938f-785f50016ebb,4,false,0,0,("channel-status": ),("packet-size":0),("response-code": ),("window-size": ),("seq"),("tp-layer":ipt)]
			//
			//	* session tag
			//	* peer
			//	* cluster bus sequence
			//	* success flag
			//	* channel
			//	* source
			//	* count
			//	* options
			//	* bag
			//	
			const cyng::vector_t frame = ctx.get_frame();
			vm_.async_run(cyng::generate_invoke("log.msg.info", "client.res.open.push.channel", frame));

			//
			//	dom reader
			//
			auto dom = cyng::make_reader(frame);

			//	options
			const response_type res = cyng::value_cast<response_type>(dom[6].get("response-code"), 0);
			const std::uint16_t p_size = cyng::value_cast<std::uint16_t>(dom[6].get("packet-size"), 0);
			const std::uint8_t w_size = cyng::value_cast<std::uint8_t>(dom[6].get("window-size"), 0);
			const std::uint8_t status = cyng::value_cast<std::uint8_t>(dom[6].get("channel-status"), 0);

			//	bag
			const sequence_type seq = cyng::value_cast<sequence_type>(dom[7].get("seq"), 0);

			const std::uint32_t channel = cyng::value_cast<std::uint32_t>(dom.get(4), 0);
			const std::uint32_t source = cyng::value_cast<std::uint32_t>(dom.get(5), 0);
			const std::uint32_t count = cyng::value_cast<std::uint32_t>(dom.get(6), 0);


			vm_.async_run(cyng::generate_invoke("res.open.push.channel", seq, res, channel, source, p_size, w_size, status, count));
			vm_.async_run(cyng::generate_invoke("stream.flush"));


		}
		void session::client_res_close_push_channel(cyng::context& ctx)
		{
			//	[]
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "client.res.open.push.channel(2) " << cyng::io::to_str(frame));

		}

		void session::ipt_req_register_push_target(cyng::context& ctx)
		{
			//	[3fb2afc3-7fef-4c5c-b272-234fe1de45fa,2,data.sink.2,65535,1]
			//
			//	* session tag
			//	* sequence
			//	* target name
			//	* packet size
			//	* window size
			//	
			const cyng::vector_t frame = ctx.get_frame();

			if (bus_->is_online())
			{
				ctx.run(cyng::generate_invoke("log.msg.info", "ipt.req.register.push.target", frame));

				cyng::param_map_t bag;
				bag["tp-layer"] = cyng::make_object("ipt");
				bag["seq"] = frame.at(1);
				bag["p-size"] = frame.at(3);
				bag["w-size"] = frame.at(4);
				bus_->vm_.async_run(client_req_register_push_target(cyng::value_cast(frame.at(0), boost::uuids::nil_uuid())
					, cyng::value_cast<std::string>(frame.at(2), "")
					, bag));
			}
			else
			{
				CYNG_LOG_ERROR(logger_, "ipt.req.register.push.target - no master " << cyng::io::to_str(frame));
				vm_.async_run(cyng::generate_invoke("res.register.push.target"
					, frame.at(1)
					, static_cast<response_type>(ctrl_res_register_target_policy::GENERAL_ERROR)
					, static_cast<std::uint32_t>(0)));
				vm_.async_run(cyng::generate_invoke("stream.flush"));

			}

		}
		void session::client_res_register_push_target(cyng::context& ctx)
		{
			//	[377de26e-1190-4d12-b87e-374b5a163d66,2bd281df-ba1b-43f6-9c79-f8c55f730c04,3,false,0,("response-code":2),("p-size":65535),("seq":2),("tp-layer":ipt),("w-size":1)]
			//
			//	* session tag
			//	* remote peer
			//	* cluster bus sequence
			//	* success
			//	* channel id
			//	* options
			//	* bag
			//	
			const cyng::vector_t frame = ctx.get_frame();
			vm_.async_run(cyng::generate_invoke("log.msg.info", "ipt.res.register.push.target", frame));

			//
			//	dom reader
			//
			auto dom = cyng::make_reader(frame);

			const sequence_type seq = cyng::value_cast<sequence_type>(dom[6].get("seq"), 0);
			const response_type res = cyng::value_cast<response_type>(dom[5].get("response-code"), 0);
			const std::uint32_t channel = cyng::value_cast<std::uint32_t>(dom.get(4), 0);	

			vm_.async_run(cyng::generate_invoke("res.register.push.target", seq, res, channel));
			vm_.async_run(cyng::generate_invoke("stream.flush"));

		}

		void session::ipt_req_open_connection(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			if (bus_->is_online())
			{
				ctx.run(cyng::generate_invoke("log.msg.info", "client.req.open.connection", frame));

				cyng::param_map_t bag;
				bag["tp-layer"] = cyng::make_object("ipt");
				bag["seq"] = frame.at(1);
				bus_->vm_.async_run(client_req_open_connection(cyng::value_cast(frame.at(0), boost::uuids::nil_uuid())
					, cyng::value_cast<std::string>(frame.at(2), "")	//	number
					, bag));
			}
			else
			{
				ctx.run(cyng::generate_invoke("log.msg.error", "client.req.open.connection", frame));
				vm_.async_run(cyng::generate_invoke("res.open.connection", frame.at(1), static_cast<response_type>(tp_res_open_connection_policy::NO_MASTER)));
				vm_.async_run(cyng::generate_invoke("stream.flush"));
			}
		}

		void session::client_req_open_connection_forward(cyng::context& ctx)
		{
			//	[client.req.open.connection.forward,
			//	[df08ad56-c59f-4a3b-bded-3e2c72d03f8e,e6a65d73-90bf-4134-ac14-d31dda5fafe8,1,1024,
			//	%(("device-name":data-store),("local-peer":25f0022c-cc9c-4028-9caf-9836b00690ee),("origin-tag":3a85a604-3397-4c1e-aa17-3c1d120098d5),("remote-peer":25f0022c-cc9c-4028-9caf-9836b00690ee),("remote-tag":df08ad56-c59f-4a3b-bded-3e2c72d03f8e)),
			//	%(("seq":1),("tp-layer":ipt))]]
			//
			//	* session tag
			//	* peer
			//	* cluster bus sequence
			//	* numer to call
			//	* options
			//		- caller device name
			//		- origin session tag
			//		- ...
			//	* bag
			const cyng::vector_t frame = ctx.get_frame();
			//ctx.run(cyng::generate_invoke("log.msg.trace", "client.req.open.connection.forward", frame));

			//
			//	dom reader
			//
			auto dom = cyng::make_reader(frame);

			cyng::param_map_t tmp;
			const std::size_t tsk = cyng::async::start_task_sync<open_connection>(mux_
				, logger_
				, bus_
				, vm_
				, cyng::value_cast(dom[4].get("origin-tag"), boost::uuids::nil_uuid())
				, cyng::value_cast<std::size_t>(dom.get(2), 0)	//	cluster bus sequence
				, cyng::value_cast<std::string>(dom.get(3), "")	//	number
				, cyng::value_cast(dom.get(4), tmp)	//	options
				, cyng::value_cast(dom.get(5), tmp)	//	bag
				, timeout_).first;

		}

		void session::client_res_open_connection_forward(cyng::context& ctx)
		{
			//	[client.res.open.connection.forward,
			//	[6baa9ee2-4812-4698-9eb6-c79a3df19d51,1a4ac8fd-2997-43c8-8b4a-e2dae1e167b8,1,false,
			//	%(),
			//	%(("seq":1),("tp-layer":ipt))]]
			//
			//	* session tag
			//	* peer
			//	* cluster sequence
			//	* success
			//	* options
			//	* bag
			//ctx.run(cyng::generate_invoke("log.msg.trace", "client.res.open.connection.forward", ctx.get_frame()));

			const cyng::vector_t frame = ctx.get_frame();

			//
			//	dom reader
			//
			auto dom = cyng::make_reader(frame);

			const bool success = cyng::value_cast(dom.get(3), false);
			const sequence_type seq = cyng::value_cast<sequence_type>(dom[5].get("seq"), 0);
			const response_type res = success
				? ipt::tp_res_open_connection_policy::DIALUP_SUCCESS
				: ipt::tp_res_open_connection_policy::DIALUP_FAILED
				;

			if (success)
			{
				ctx.attach(cyng::generate_invoke("log.msg.info", "client.res.open.connection.forward", ctx.get_frame()));
			}
			else
			{
				ctx.attach(cyng::generate_invoke("log.msg.warning", "client.res.open.connection.forward", ctx.get_frame()));

			}
			ipt::tp_res_open_connection_policy::DIALUP_FAILED;
			vm_.async_run(cyng::generate_invoke("res.open.connection", seq, res));
			vm_.async_run(cyng::generate_invoke("stream.flush"));

		}

		void session::ipt_req_close_connection(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			if (bus_->is_online())
			{
				ctx.run(cyng::generate_invoke("log.msg.info", "client.req.close.connection", frame));

				cyng::param_map_t bag;
				bag["tp-layer"] = cyng::make_object("ipt");
				bag["seq"] = frame.at(1);
				bus_->vm_.async_run(client_req_close_connection(cyng::value_cast(frame.at(0), boost::uuids::nil_uuid())
					, bag));
			}
			else
			{
				ctx.run(cyng::generate_invoke("log.msg.error", "client.req.close.connection", frame));
				vm_.async_run(cyng::generate_invoke("res.close.connection", frame.at(1), static_cast<response_type>(tp_res_close_connection_policy::CONNECTION_CLEARING_FORBIDDEN)));
				vm_.async_run(cyng::generate_invoke("stream.flush"));
			}
		}

		void session::client_res_open_connection(cyng::context& ctx)
		{
			//	[client.res.open.connection,[767e4b41-3df2-402e-9cbc-d20de610000a,efeb7b5d-aec7-4e91-8a1b-c88f8fd4e01d,2,false,%(("device-name":LSMTest1),("response-code":0)),%(("seq":1),("tp-layer":ipt))]]
			//
			//	* session tag
			//	* remote peer
			//	* cluster bus sequence
			//	* success
			//	* options
			//	* bag
			//	
			const cyng::vector_t frame = ctx.get_frame();
			vm_.async_run(cyng::generate_invoke("log.msg.info", "client.res.open.connection", frame));

			//
			//	dom reader
			//
			auto dom = cyng::make_reader(frame);

			const sequence_type seq = cyng::value_cast<sequence_type>(dom[5].get("seq"), 0);
			const response_type res = cyng::value_cast<response_type>(dom[4].get("response-code"), 0);

			vm_.async_run(cyng::generate_invoke("res.open.connection", seq, res));
			vm_.async_run(cyng::generate_invoke("stream.flush"));

		}

		void session::ipt_res_open_connection(cyng::context& ctx)
		{
			//	[ipt.res.open.connection,[6c869d79-eb8a-4df8-b509-8be87f5d3bc9,1,2]]
			//
			//	* session tag
			//	* ipt sequence
			//	* ipt response
			//
			const cyng::vector_t frame = ctx.get_frame();
			vm_.async_run(cyng::generate_invoke("log.msg.info", "ipt.res.open.connection", frame));

			//
			//	dom reader
			//
			auto dom = cyng::make_reader(frame);

			const sequence_type seq = cyng::value_cast<sequence_type>(dom.get(1), 0);
			const response_type res = cyng::value_cast<response_type>(dom.get(2), 0);

			//
			//	search related task
			//
			auto pos = task_db_.find(seq);
			if (pos != task_db_.end())
			{
				const auto tsk = pos->second;
				vm_.async_run(cyng::generate_invoke("log.msg.trace", "continue task", tsk));
				mux_.send(tsk, 0, cyng::tuple_factory(res));

				//
				//	remove entry
				//
				task_db_.erase(pos);
			}
			else
			{
				vm_.async_run(cyng::generate_invoke("log.msg.error", "task related to sequence not found", seq));
			}
		}

	}
}



