/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 


#include "session.h"
//#include "tasks/open_connection.h"
//#include "tasks/close_connection.h"
//#include "tasks/gatekeeper.h"
#include <NODE_project_info.h>
#include <smf/cluster/generator.h>
#include <cyng/vm/domain/log_domain.h>
#include <cyng/vm/domain/store_domain.h>
#include <cyng/io/serializer.h>
#include <cyng/value_cast.hpp>
#include <cyng/tuple_cast.hpp>
#include <cyng/dom/reader.h>
#include <cyng/async/task/task_builder.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/nil_generator.hpp>

namespace node 
{
	namespace modem
	{
		session::session(cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, bus::shared_type bus
			, boost::uuids::uuid tag
			, std::chrono::seconds const& timeout
			, bool auto_answer
			, std::chrono::milliseconds guard_time)
		: mux_(mux)
			, logger_(logger)
			, bus_(bus)
			, vm_(mux.get_io_service(), tag)
			, timeout_(timeout)
			, auto_answer_(auto_answer)
			, parser_([this](cyng::vector_t&& prg) {
				CYNG_LOG_INFO(logger_, prg.size() << " modem instructions received");
				CYNG_LOG_TRACE(logger_, vm_.tag() << ": " << cyng::io::to_str(prg));
				vm_.run(std::move(prg));
			}, guard_time)
			//, gate_keeper_(cyng::async::start_task_sync<gatekeeper>(mux_
			//	, logger_
			//	, vm_
			//	, tag
			//	, timeout_).first)
			, connect_state_()
		{
			//
			//	register logger domain
			//
			cyng::register_logger(logger_, vm_);
			vm_.run(cyng::generate_invoke("log.msg.info", "log domain is running"));

			vm_.run(cyng::register_function("session.store.relation", 2, std::bind(&session::store_relation, this, std::placeholders::_1)));
			vm_.run(cyng::register_function("session.update.connection.state", 2, std::bind(&session::update_connection_state, this, std::placeholders::_1)));

			//
			//	register request handler
			//	client.req.transmit.data.forward
			vm_.run(cyng::register_function("modem.req.transmit.data", 2, std::bind(&session::modem_req_transmit_data, this, std::placeholders::_1)));
			vm_.run(cyng::register_function("client.req.transmit.data.forward", 4, std::bind(&session::client_req_transmit_data_forward, this, std::placeholders::_1)));


			//	transport
			//	transport - push channel open
			//TP_REQ_OPEN_PUSH_CHANNEL = 0x9000,
			vm_.run(cyng::register_function("ipt.req.open.push.channel", 8, std::bind(&session::ipt_req_open_push_channel, this, std::placeholders::_1)));
			vm_.run(cyng::register_function("client.res.open.push.channel", 8, std::bind(&session::client_res_open_push_channel, this, std::placeholders::_1)));

			//TP_RES_OPEN_PUSH_CHANNEL = 0x1000,	//!<	response

			//	transport - push channel close
			//TP_REQ_CLOSE_PUSH_CHANNEL = 0x9001,	//!<	request
			vm_.run(cyng::register_function("ipt.req.close.push.channel", 3, std::bind(&session::ipt_req_close_push_channel, this, std::placeholders::_1)));
			vm_.run(cyng::register_function("client.res.close.push.channel", 5, std::bind(&session::client_res_close_push_channel, this, std::placeholders::_1)));
			//TP_RES_CLOSE_PUSH_CHANNEL = 0x1001,	//!<	response

			//	transport - push channel data transfer
			//TP_REQ_PUSHDATA_TRANSFER = 0x9002,	//!<	request
			vm_.run(cyng::register_function("ipt.req.transfer.pushdata", 7, std::bind(&session::ipt_req_transfer_pushdata, this, std::placeholders::_1)));
			vm_.run(cyng::register_function("client.req.transfer.pushdata.forward", 6, std::bind(&session::client_req_transfer_pushdata_forward, this, std::placeholders::_1)));

			//TP_RES_PUSHDATA_TRANSFER = 0x1002,	//!<	response
			vm_.run(cyng::register_function("client.res.transfer.pushdata", 6, std::bind(&session::client_res_transfer_pushdata, this, std::placeholders::_1)));

			//	transport - connection open
			//TP_REQ_OPEN_CONNECTION = 0x9003,	//!<	request
			vm_.run(cyng::register_function("modem.req.open.connection", 2, std::bind(&session::modem_req_open_connection, this, std::placeholders::_1)));
			vm_.run(cyng::register_function("client.req.open.connection.forward", 5, std::bind(&session::client_req_open_connection_forward, this, std::placeholders::_1)));
			//TP_RES_OPEN_CONNECTION = 0x1003,	//!<	response
			vm_.run(cyng::register_function("ipt.res.open.connection", 3, std::bind(&session::ipt_res_open_connection, this, std::placeholders::_1)));
			vm_.run(cyng::register_function("client.res.open.connection", 5, std::bind(&session::client_res_open_connection, this, std::placeholders::_1)));
			vm_.run(cyng::register_function("client.res.open.connection.forward", 5, std::bind(&session::client_res_open_connection_forward, this, std::placeholders::_1)));

			//	transport - connection close
			//TP_REQ_CLOSE_CONNECTION = 0x9004,	//!<	request
			vm_.run(cyng::register_function("modem.req.close.connection", 0, std::bind(&session::modem_req_close_connection, this, std::placeholders::_1)));
			vm_.run(cyng::register_function("client.req.close.connection.forward", 5, std::bind(&session::client_req_close_connection_forward, this, std::placeholders::_1)));
			//TP_RES_CLOSE_CONNECTION = 0x1004,	//!<	response
			vm_.run(cyng::register_function("ipt.res.close.connection", 0, std::bind(&session::ipt_res_close_connection, this, std::placeholders::_1)));
			vm_.run(cyng::register_function("client.res.close.connection.forward", 5, std::bind(&session::client_res_close_connection_forward, this, std::placeholders::_1)));

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
			vm_.run(cyng::register_function("ipt.res.protocol.version", 3, std::bind(&session::ipt_res_protocol_version, this, std::placeholders::_1)));

			//	application - device firmware version
			//APP_REQ_SOFTWARE_VERSION = 0xA001,	//!<	request
			//APP_RES_SOFTWARE_VERSION = 0x2001,	//!<	response
			vm_.run(cyng::register_function("ipt.res.software.version", 3, std::bind(&session::ipt_res_software_version, this, std::placeholders::_1)));

			//	application - device identifier
			//APP_REQ_DEVICE_IDENTIFIER = 0xA003,	//!<	request
			//APP_RES_DEVICE_IDENTIFIER = 0x2003,
			vm_.run(cyng::register_function("ipt.res.dev.id", 3, std::bind(&session::ipt_res_dev_id, this, std::placeholders::_1)));

			//	application - network status
			//APP_REQ_NETWORK_STATUS = 0xA004,	//!<	request
			//APP_RES_NETWORK_STATUS = 0x2004,	//!<	response
			vm_.run(cyng::register_function("ipt.res.network.stat", 10, std::bind(&session::ipt_res_network_stat, this, std::placeholders::_1)));

			//	application - IP statistic
			//APP_REQ_IP_STATISTICS = 0xA005,	//!<	request
			//APP_RES_IP_STATISTICS = 0x2005,	//!<	response
			vm_.run(cyng::register_function("ipt.res.ip.statistics", 5, std::bind(&session::ipt_res_ip_statistics, this, std::placeholders::_1)));

			//	application - device authentification
			//APP_REQ_DEVICE_AUTHENTIFICATION = 0xA006,	//!<	request
			//APP_RES_DEVICE_AUTHENTIFICATION = 0x2006,	//!<	response
			vm_.run(cyng::register_function("ipt.res.dev.auth", 6, std::bind(&session::ipt_res_dev_auth, this, std::placeholders::_1)));

			//	application - device time
			//APP_REQ_DEVICE_TIME = 0xA007,	//!<	request
			//APP_RES_DEVICE_TIME = 0x2007,	//!<	response
			vm_.run(cyng::register_function("ipt.res.dev.time", 3, std::bind(&session::ipt_res_dev_time, this, std::placeholders::_1)));

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
			vm_.run(cyng::register_function("modem.req.login.public", 2, std::bind(&session::modem_req_login, this, std::placeholders::_1)));
			vm_.run(cyng::register_function("client.res.login", 6, std::bind(&session::client_res_login, this, std::placeholders::_1)));

			//	control - maintenance
			//MAINTENANCE_REQUEST = 0xC003,	//!<	request *** deprecated ***
			//MAINTENANCE_RESPONSE = 0x4003,	//!<	response *** deprecated ***

			//	control - logout
			//CTRL_REQ_LOGOUT = 0xC004,	//!<	request *** deprecated ***
			//CTRL_RES_LOGOUT = 0x4004,	//!<	response *** deprecated ***
			vm_.run(cyng::register_function("ipt.req.logout", 2, std::bind(&session::ipt_req_logout, this, std::placeholders::_1)));
			vm_.run(cyng::register_function("ipt.res.logout", 2, std::bind(&session::ipt_res_logout, this, std::placeholders::_1)));

			//	control - push target register
			//CTRL_REQ_REGISTER_TARGET = 0xC005,	//!<	request
			vm_.run(cyng::register_function("ipt.req.register.push.target", 5, std::bind(&session::ipt_req_register_push_target, this, std::placeholders::_1)));
			vm_.run(cyng::register_function("client.res.register.push.target", 6, std::bind(&session::client_res_register_push_target, this, std::placeholders::_1)));
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
			vm_.run(cyng::register_function("ipt.unknown.cmd", 3, std::bind(&session::ipt_unknown_cmd, this, std::placeholders::_1)));

			
			//
			//	ToDo: start maintenance task
			//
		}

		session::~session()
		{}

		void session::stop()
		{
			//parser_.stop():
			vm_.halt();
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

			//task_db_.emplace(cyng::value_cast<sequence_type>(frame.at(0), 0), cyng::value_cast<std::size_t>(frame.at(1), 0));
		}

		void session::update_connection_state(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "session.update.connection.state " << cyng::io::to_str(frame));
			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,		//	[0] remote tag
				bool					//	[1] new state
			>(frame);

			connect_state_.connected_local_ = std::get<1>(tpl);
		}


		void session::modem_req_login(cyng::context& ctx)
		{
			BOOST_ASSERT(ctx.tag() == vm_.tag());

			//	[LSMTest5,LSMTest5,<sk>]
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "modem.req.login.public " << cyng::io::to_str(frame));

			//
			//	send authorization request to master
			//
			if (bus_->is_online())
			{
				//const std::string name = cyng::value_cast<std::string>(frame.at(0));
				cyng::param_map_t bag;
				bag["tp-layer"] = cyng::make_object("modem");
				bag["security"] = cyng::make_object("public");
				bag["time"] = cyng::make_now();

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
				ctx.attach(cyng::generate_invoke("print.error"));
				ctx.attach(cyng::generate_invoke("stream.flush"));
			}


		}

		void session::ipt_req_logout(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_WARNING(logger_, "ipt.req.logout - deprecated " << cyng::io::to_str(frame));
		}

		void session::ipt_res_logout(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "ipt.res.login - deprecated " << cyng::io::to_str(frame));

			//
			//	close session
			//
			ctx.attach(cyng::generate_invoke("ip.tcp.socket.shutdown"));
			ctx.attach(cyng::generate_invoke("ip.tcp.socket.close"));

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
				//ctx.attach(cyng::generate_invoke("res.open.push.channel"
				//	, frame.at(1)
				//	, static_cast<ipt::response_type>(ipt::tp_res_open_push_channel_policy::UNREACHABLE)
				//	, static_cast<std::uint32_t>(0)	//	channel
				//	, static_cast<std::uint32_t>(0)	//	source
				//	, static_cast<std::uint16_t>(0)	//	packet size
				//	, static_cast<std::uint8_t>(0)	//	window size
				//	, static_cast<std::uint8_t>(0)	//	status
				//	, static_cast<std::uint32_t>(0)));	//	count
				//ctx.attach(cyng::generate_invoke("stream.flush"));

			}
		}

		void session::ipt_req_close_push_channel(cyng::context& ctx)
		{
			//	[b1b6a46c-bb14-4722-bc3e-3cf8d6e74c00,bf,d268409a]
			//
			//	* session tag
			//	* ipt sequence
			//	* channel 
			const cyng::vector_t frame = ctx.get_frame();
			//CYNG_LOG_INFO(logger_, "ipt_req_close_push_channel " << cyng::io::to_str(frame));

			if (bus_->is_online())
			{
				ctx.run(cyng::generate_invoke("log.msg.info", "ipt.req.close.push.channel", frame));

				cyng::param_map_t bag;
				bag["tp-layer"] = cyng::make_object("ipt");
				bag["seq"] = frame.at(1);
				bus_->vm_.async_run(client_req_close_push_channel(cyng::value_cast(frame.at(0), boost::uuids::nil_uuid())
					, cyng::value_cast<std::uint32_t>(frame.at(2), 0)
					, bag));
			}
			else
			{				
				CYNG_LOG_ERROR(logger_, "ipt.req.close.push.channel - no master " << cyng::io::to_str(frame));
				//ctx.attach(cyng::generate_invoke("res.close.push.channel"
				//	, frame.at(1)
				//	, static_cast<ipt::response_type>(ipt::tp_res_close_push_channel_policy::BROKEN)
				//	, frame.at(2)));
				//ctx.attach(cyng::generate_invoke("stream.flush"));
			}

		}

		void session::modem_req_transmit_data(cyng::context& ctx)
		{
			//	[ipt.req.transmit.data,
			//	[c5eae235-d5f5-413f-a008-5d317d8baab7,1B1B1B1B01010101768106313830313...1B1B1B1B1A034276]]
			const cyng::vector_t frame = ctx.get_frame();
			if (bus_->is_online())
			{
				const boost::uuids::uuid tag = cyng::value_cast(frame.at(0), boost::uuids::nil_uuid());

				if (connect_state_.connected_local_)
				{
					bus_->vm_.async_run(cyng::generate_invoke("server.transmit.data", tag, frame.at(1)));
				}
				else
				{
					cyng::param_map_t bag;
					bag["tp-layer"] = cyng::make_object("modem");

					bus_->vm_.async_run(client_req_transmit_data(tag
						, bag
						, frame.at(1)));
				}
			}
			else
			{
				ctx.attach(cyng::generate_invoke("log.msg.warning", "modem.req.transmit.data", "no master", frame));
			}
		}

		void session::client_req_transmit_data_forward(cyng::context& ctx)
		{
			//	[client.req.transmit.data.forward,
			//	[812518c9-c532-413f-a1c8-6d4a73de35b9,95a4ccf9-1171-4ff0-ad64-d06cb74da24e,8,
			//	%(("tp-layer":ipt)),
			//	1B1B1B1B01010101768106313830313330323133...0000001B1B1B1B1A0353AD]]
			//
			//	* [session tag] - removed
			//	* peer
			//	* cluster sequence
			//	* bag
			//	* data
			//
			const cyng::vector_t frame = ctx.get_frame();
			ctx.run(cyng::generate_invoke("log.msg.info", "client.req.transmit.data.forward", frame));

			ctx.attach(cyng::generate_invoke("modem.transfer.data", frame.at(3)));
			ctx.attach(cyng::generate_invoke("stream.flush"));
			
		}

		void session::ipt_res_watchdog(cyng::context& ctx)
		{
			//BOOST_ASSERT_MSG(bus_->is_online(), "no master");

			//	[]
			const cyng::vector_t frame = ctx.get_frame();
			//ctx.run(cyng::generate_invoke("log.msg.info", "ipt.res.watchdog", frame));
			if (bus_->is_online())
			{
				ctx.attach(cyng::generate_invoke("log.msg.info", "ipt.res.watchdog", frame));
			}
			else
			{
				ctx.attach(cyng::generate_invoke("log.msg.warning", "ipt.res.watchdog", "no master", frame));

			}
		}

		void session::ipt_res_protocol_version(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			ctx.attach(cyng::generate_invoke("log.msg.debug", "ipt.res.protocol.version", frame));

		}

		void session::ipt_res_software_version(cyng::context& ctx)
		{
			//	 [b22572a5-1233-43da-967b-aa6b16be002e,2,0.2.2018030]
			const cyng::vector_t frame = ctx.get_frame();
			if (bus_->is_online())
			{
				ctx.run(cyng::generate_invoke("log.msg.debug", "ipt.res.software.version", frame));

				cyng::param_map_t bag;
				bag["tp-layer"] = cyng::make_object("ipt");
				bag["seq"] = frame.at(1);
				bus_->vm_.async_run(client_update_attr(cyng::value_cast(frame.at(0), boost::uuids::nil_uuid())
					, "TDevice.vFirmware"
					, frame.at(2)
					, bag));
			}
			else
			{
				ctx.attach(cyng::generate_invoke("log.msg.error", "ipt.res.software.version - no master", frame));
			}
		}

		void session::ipt_res_dev_id(cyng::context& ctx)
		{
			//	[65134d13-2d67-4208-bfe3-de8e2bd093d2,3,ipt:store]
			const cyng::vector_t frame = ctx.get_frame();
			if (bus_->is_online())
			{
				ctx.run(cyng::generate_invoke("log.msg.debug", "ipt.res.device.id", frame));

				cyng::param_map_t bag;
				bag["tp-layer"] = cyng::make_object("ipt");
				bag["seq"] = frame.at(1);
				bus_->vm_.async_run(client_update_attr(cyng::value_cast(frame.at(0), boost::uuids::nil_uuid())
					, "TDevice.id"
					, frame.at(2)
					, bag));
			}
			else
			{
				ctx.attach(cyng::generate_invoke("log.msg.error", "ipt.res.device.id - no master", frame));
			}
		}

		void session::ipt_res_network_stat(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			ctx.attach(cyng::generate_invoke("log.msg.debug", "ipt.res.network.stat", frame));
		}

		void session::ipt_res_ip_statistics(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			ctx.attach(cyng::generate_invoke("log.msg.debug", "ipt.res.ip.statistics", frame));
		}

		void session::ipt_res_dev_auth(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			ctx.attach(cyng::generate_invoke("log.msg.debug", "ipt.res.dev.auth", frame));
		}

		void session::ipt_res_dev_time(cyng::context& ctx)
		{
			//	[420fa09c-e9a7-4092-bbdb-368b02f18e84,6,00000000]
			const cyng::vector_t frame = ctx.get_frame();
			if (bus_->is_online())
			{
				ctx.run(cyng::generate_invoke("log.msg.debug", "ipt.res.dev.time", frame));

				cyng::param_map_t bag;
				bag["tp-layer"] = cyng::make_object("ipt");
				bag["seq"] = frame.at(1);
				bus_->vm_.async_run(client_update_attr(cyng::value_cast(frame.at(0), boost::uuids::nil_uuid())
					, "device.time"
					, frame.at(2)
					, bag));
			}
			else
			{
				ctx.attach(cyng::generate_invoke("log.msg.error", "ipt.res.device.time - no master", frame));
			}
		}

		void session::ipt_unknown_cmd(cyng::context& ctx)
		{
			//	 [9c08c93e-b2fe-4738-9fb4-4b16ed57a06e,6,00000000]
			const cyng::vector_t frame = ctx.get_frame();
			//auto const tpl = cyng::tuple_cast<
			//	boost::uuids::uuid,		//	[0] tag
			//	sequence_type,			//	[1] ipt seq
			//	std::uint16_t			//	[2] command
			//>(frame);

			//ctx.attach(cyng::generate_invoke("log.msg.warning", "ipt.unknown.cmd", std::get<2>(tpl), command_name(std::get<2>(tpl))));
		}

		void session::client_res_login(cyng::context& ctx)
		{
			BOOST_ASSERT(ctx.tag() == vm_.tag());

			//	[068544fb-9513-4cbe-9007-c9dd9892aff6,d03ff1a5-838a-4d71-91a1-fc8880b157a6,17,true,OK,%(("tp-layer":ipt))]
			//
			//	* [client tag] - removed
			//	* peer
			//	* sequence
			//	* response (bool)
			//	* name
			//	* msg (optional)
			//	* query
			//	* bag
			//	
			const cyng::vector_t frame = ctx.get_frame();

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,		//	[0] peer tag
				std::uint64_t,			//	[1] sequence number
				bool,					//	[2] success
				std::string,			//	[3] name
				std::string,			//	[4] msg
				std::uint32_t,			//	[5] query
				cyng::param_map_t		//	[6] bag
			>(frame);

			if (std::get<2>(tpl))
			{
				ctx.attach(cyng::generate_invoke("log.msg.info"
					, "login response"
					, std::get<3>(tpl)	//	name
					, std::get<4>(tpl)));	//	msg
				ctx.attach(cyng::generate_invoke("print.ok"));
				ctx.attach(cyng::generate_invoke("stream.flush"));

				cyng::param_map_t bag;
				bag["tp-layer"] = cyng::make_object("ipt");
				bag["seq"] = frame.at(1);
				bus_->vm_.async_run(client_update_attr(ctx.tag()
					, "TDevice.vFirmware"
					, cyng::make_object(NODE_VERSION)
					, bag));
				bus_->vm_.async_run(client_update_attr(ctx.tag()
					, "TDevice.id"
					, cyng::make_object("modem")
					, bag));

			}
			else
			{
				ctx.attach(cyng::generate_invoke("log.msg.warning"
					, "login response"
					, std::get<3>(tpl)	//	name
					, std::get<4>(tpl)));	//	msg
				ctx.attach(cyng::generate_invoke("print.error"));
				ctx.attach(cyng::generate_invoke("stream.flush"));
				ctx.attach(cyng::generate_invoke("ip.tcp.socket.shutdown"));
				ctx.attach(cyng::generate_invoke("ip.tcp.socket.close"));			}

			//
			//	bag reader
			//
			auto dom = cyng::make_reader(std::get<6>(tpl));

			//
			//	stop gatekeeper
			//
			//mux_.send(gate_keeper_, 0, cyng::tuple_factory(res));

			//const std::string security = cyng::value_cast<std::string>(dom.get("security"), "undef");
			//if (boost::algorithm::equals(security, "scrambled"))
			//{
			//	ctx.attach(cyng::generate_invoke("res.login.scrambled", res, watchdog_, ""));
			//}
			//else
			//{
			//	ctx.attach(cyng::generate_invoke("res.login.public", res, watchdog_, ""));
			//}
			//ctx.attach(cyng::generate_invoke("stream.flush"));

			////	success
			//if (std::get<2>(tpl))
			//{
			//	ctx.attach(cyng::generate_invoke("log.msg.info"
			//		, "send login response"
			//		, std::get<3>(tpl)	//	name
			//		, std::get<4>(tpl)	//	msg
			//		, "watchdog"
			//		, watchdog_));

			//	const auto query = std::get<5>(tpl);
			//	if ((query & QUERY_PROTOCOL_VERSION) == QUERY_PROTOCOL_VERSION)
			//	{
			//		ctx.attach(cyng::generate_invoke("log.msg.debug", "QUERY_PROTOCOL_VERSION"));
			//		ctx.attach(cyng::generate_invoke("req.protocol.version"));
			//	}
			//	if ((query & QUERY_FIRMWARE_VERSION) == QUERY_FIRMWARE_VERSION)
			//	{
			//		ctx.attach(cyng::generate_invoke("log.msg.debug", "QUERY_FIRMWARE_VERSION"));
			//		ctx.attach(cyng::generate_invoke("req.software.version"));
			//	}
			//	if ((query & QUERY_DEVICE_IDENTIFIER) == QUERY_DEVICE_IDENTIFIER)
			//	{
			//		ctx.attach(cyng::generate_invoke("log.msg.debug", "QUERY_DEVICE_IDENTIFIER"));
			//		ctx.attach(cyng::generate_invoke("req.device.id"));
			//	}
			//	if ((query & QUERY_NETWORK_STATUS) == QUERY_NETWORK_STATUS)
			//	{
			//		ctx.attach(cyng::generate_invoke("log.msg.debug", "QUERY_NETWORK_STATUS"));
			//		ctx.attach(cyng::generate_invoke("req.net.status"));
			//	}
			//	if ((query & QUERY_IP_STATISTIC) == QUERY_IP_STATISTIC)
			//	{
			//		ctx.attach(cyng::generate_invoke("log.msg.debug", "QUERY_IP_STATISTIC"));
			//		ctx.attach(cyng::generate_invoke("req.ip.statistics"));
			//	}
			//	if ((query & QUERY_DEVICE_AUTHENTIFICATION) == QUERY_DEVICE_AUTHENTIFICATION)
			//	{
			//		ctx.attach(cyng::generate_invoke("log.msg.debug", "QUERY_DEVICE_AUTHENTIFICATION"));
			//		ctx.attach(cyng::generate_invoke("req.device.auth"));
			//	}
			//	if ((query & QUERY_DEVICE_TIME) == QUERY_DEVICE_TIME)
			//	{
			//		ctx.attach(cyng::generate_invoke("log.msg.debug", "QUERY_DEVICE_TIME"));
			//		ctx.attach(cyng::generate_invoke("req.device.time"));
			//	}
			//	ctx.attach(cyng::generate_invoke("stream.flush"));
			//}
			//else
			//{
			//	ctx.attach(cyng::generate_invoke("log.msg.warning"
   //                 , "received login response"
			//		, std::get<3>(tpl)	//	name
			//		, std::get<4>(tpl)));	//	msg

			//	//
			//	//	close session
			//	//
			//	ctx.attach(cyng::generate_invoke("ip.tcp.socket.shutdown"));
			//	ctx.attach(cyng::generate_invoke("ip.tcp.socket.close"));
			//}

		}

		void session::client_res_open_push_channel(cyng::context& ctx)
		{
			//	[85058f73-a243-42a7-908c-85b3a3f29f62,01027bb4-8964-4577-938f-785f50016ebb,4,false,0,0,("channel-status": ),("packet-size":0),("response-code": ),("window-size": ),("seq"),("tp-layer":ipt)]
			//	[bf91ae46-b6bb-493f-938a-b82789244198,4d8268fb-b21b-40fc-b3df-a85d114e4198,25,false,474ba8c4,a1e24bba,00000000,%(("channel-status":0),("packet-size":ffff),("response-code":2),("window-size":1)),%(("seq":4),("tp-layer":ipt))]
			//
			//	* [session tag] - removed
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
			ctx.attach(cyng::generate_invoke("log.msg.debug", "client.res.open.push.channel", frame));

			auto const tpl = cyng::tuple_cast<
				//boost::uuids::uuid,		//	[0] tag
				boost::uuids::uuid,		//	[1] peer
				std::uint64_t,			//	[2] cluster sequence
				bool,					//	[3] success flag
				std::uint32_t,			//	[4] channel
				std::uint32_t,			//	[5] source
				std::uint32_t,			//	[6] count
				cyng::param_map_t,		//	[7] options
				cyng::param_map_t		//	[8] bag
			>(frame);

			//
			//	dom/options reader
			//
			//auto dom_options = cyng::make_reader(std::get<6>(tpl));
			//const response_type res = cyng::value_cast<response_type>(dom_options.get("response-code"), 0);
			//const std::uint16_t p_size = cyng::value_cast<std::uint16_t>(dom_options.get("packet-size"), 0);
			//const std::uint8_t w_size = cyng::value_cast<std::uint8_t>(dom_options.get("window-size"), 0);
			//const std::uint8_t status = cyng::value_cast<std::uint8_t>(dom_options.get("channel-status"), 0);

			////
			////	dom/bag reader
			////
			//auto dom_bag = cyng::make_reader(std::get<7>(tpl));
			//const sequence_type seq = cyng::value_cast<sequence_type>(dom_bag.get("seq"), 0);

			//const std::uint32_t channel = std::get<3>(tpl);
			//const std::uint32_t source = std::get<4>(tpl);
			//const std::uint32_t count = std::get<5>(tpl);

			//ctx.attach(cyng::generate_invoke("res.open.push.channel", seq, res, channel, source, p_size, w_size, status, count));
			//ctx.attach(cyng::generate_invoke("stream.flush"));
		}

		void session::client_res_close_push_channel(cyng::context& ctx)
		{
			//	[ba2298ad-50d3-44ec-ba2f-ce35451b677d,11495a42-9fd3-4ab2-9f70-3ac6b16f4158,232,true,8c006d5f,%(("seq":4b),("tp-layer":ipt))]
			//
			//	* [session tag] - removed
			//	* peer
			//	* cluster bus sequence
			//	* success flag
			//	* channel
			//	* bag
			//	
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_TRACE(logger_, "client.res.close.push.channel " << cyng::io::to_str(frame));

			auto const tpl = cyng::tuple_cast<
				//boost::uuids::uuid,		//	[0] tag
				boost::uuids::uuid,		//	[1] peer
				std::uint64_t,			//	[2] cluster sequence
				bool,					//	[3] success flag
				std::uint32_t,			//	[4] channel
				cyng::param_map_t		//	[5] bag
			>(frame);

			//
			//	dom/bag reader
			//
			//auto dom_bag = cyng::make_reader(std::get<4>(tpl));
			//const sequence_type seq = cyng::value_cast<sequence_type>(dom_bag.get("seq"), 0);

			//const response_type res = (std::get<2>(tpl))
			//	? ipt::tp_res_close_push_channel_policy::SUCCESS
			//	: ipt::tp_res_close_push_channel_policy::BROKEN
			//	;

			//ctx.attach(cyng::generate_invoke("res.close.push.channel", seq, res, std::get<3>(tpl)))
			//	.attach(cyng::generate_invoke("stream.flush"));

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
				ctx.attach(cyng::generate_invoke("log.msg.info", "ipt.req.register.push.target", frame));

				cyng::param_map_t bag;
				bag["tp-layer"] = cyng::make_object("ipt");
				bag["seq"] = frame.at(1);
				bag["pSize"] = frame.at(3);
				bag["wSize"] = frame.at(4);
				bus_->vm_.async_run(client_req_register_push_target(cyng::value_cast(frame.at(0), boost::uuids::nil_uuid())
					, cyng::value_cast<std::string>(frame.at(2), "")
					, bag));
			}
			else
			{
				CYNG_LOG_ERROR(logger_, "ipt.req.register.push.target - no master " << cyng::io::to_str(frame));
				//ctx.attach(cyng::generate_invoke("res.register.push.target"
				//	, frame.at(1)
				//	, static_cast<response_type>(ctrl_res_register_target_policy::GENERAL_ERROR)
				//	, static_cast<std::uint32_t>(0)));
				//ctx.attach(cyng::generate_invoke("stream.flush"));

			}
		}

		void session::client_res_register_push_target(cyng::context& ctx)
		{
			//	[377de26e-1190-4d12-b87e-374b5a163d66,2bd281df-ba1b-43f6-9c79-f8c55f730c04,3,false,0,("response-code":2),("pSize":65535),("seq":2),("tp-layer":ipt),("wSize":1)]
			//
			//	* [session tag] - removed
			//	* remote peer
			//	* cluster bus sequence
			//	* success
			//	* channel id
			//	* options
			//	* bag
			//	
			const cyng::vector_t frame = ctx.get_frame();
			ctx.attach(cyng::generate_invoke("log.msg.info", "ipt.res.register.push.target", frame));

			//
			//	dom reader
			//
			auto dom = cyng::make_reader(frame);

			//const sequence_type seq = cyng::value_cast<sequence_type>(dom[5].get("seq"), 0);
			//const response_type res = cyng::value_cast<response_type>(dom[4].get("response-code"), 0);
			//const std::uint32_t channel = cyng::value_cast<std::uint32_t>(dom.get(3), 0);	

			//ctx.attach(cyng::generate_invoke("res.register.push.target", seq, res, channel));
			//ctx.attach(cyng::generate_invoke("stream.flush"));

		}

		void session::modem_req_open_connection(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			if (bus_->is_online())
			{
				ctx.run(cyng::generate_invoke("log.msg.info", "modem.req.open.connection", frame));

				cyng::param_map_t bag;
				bag["tp-layer"] = cyng::make_object("modem");
				bus_->vm_.async_run(client_req_open_connection(cyng::value_cast(frame.at(0), boost::uuids::nil_uuid())
					, cyng::value_cast<std::string>(frame.at(1), "")	//	number
					, bag));
			}
            else
			{
				ctx.attach(cyng::generate_invoke("log.msg.error", "modem.req.open.connection", frame));
				ctx.attach(cyng::generate_invoke("print.error"));
				ctx.attach(cyng::generate_invoke("stream.flush"));
			}
		}

		void session::client_req_open_connection_forward(cyng::context& ctx)
		{
			//	
			//	[205757ab-a4a2-4eec-813f-bcda41e5f6bb,9,LSMTest4,
			//	%(("device-name":LSMTest4),("local-connect":true),("local-peer":0fb8ab36-de83-4bfc-a111-acd29370599c),("origin-tag":ecd2457f-dc36-4d87-b8f8-6b104f64aaeb),("remote-peer":0fb8ab36-de83-4bfc-a111-acd29370599c),("remote-tag":34263f4e-95fe-4fd3-a869-7bbb080ad7a9)),
			//	%(("seq":1),("tp-layer":ipt))]]
			//
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

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,		//	[0] peer
				std::uint64_t,			//	[1] cluster sequence
				std::string,			//	[2] number to call
				cyng::param_map_t,		//	[3] options
				cyng::param_map_t		//	[4] bag
			>(frame);

			//
			//	dom reader (options)
			//
			auto dom = cyng::make_reader(std::get<3>(tpl));

			cyng::param_map_t tmp;

			ctx.attach(cyng::generate_invoke("print.ring"));
			if (auto_answer_)
			{
				ctx.attach(cyng::generate_invoke("log.msg.info", "client.req.open.connection.forward - auto answer", ctx.get_frame()));

				//
				//	update parser state
				//
				parser_.set_stream_mode();

				//
				//	send connection open response response
				//
				auto target = cyng::value_cast(dom.get("origin-tag"), boost::uuids::nil_uuid());
				bus_->vm_.async_run(node::client_res_open_connection(target
					, std::get<1>(tpl)
					, true
					, std::get<3>(tpl)
					, std::get<4>(tpl)));

			}
			else
			{
				ctx.attach(cyng::generate_invoke("log.msg.info", "client.req.open.connection.forward - wait for ATA", ctx.get_frame()));

				//
				//	ToDo: start task
				//

				//const std::size_t tsk = cyng::async::start_task_sync<open_connection>(mux_
				//	, logger_
				//	, bus_
				//	, vm_
				//	, cyng::value_cast(dom.get("origin-tag"), boost::uuids::nil_uuid())
				//	, std::get<1>(tpl)	//	cluster bus sequence
				//	, std::get<2>(tpl)	//	number
				//	, std::get<3>(tpl)	//	options
				//	, std::get<4>(tpl)	//	bag
				//	, timeout_).first;

				//CYNG_LOG_TRACE(logger_, "client.req.open.connection.forward - task #" << tsk);
			}

			ctx.attach(cyng::generate_invoke("stream.flush"));

		}

		void session::client_req_close_connection_forward(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			//ctx.run(cyng::generate_invoke("log.msg.trace", "client.req.close.connection.forward", frame));

			//	[d347320a-6028-4fe0-9bde-28abb853f479,cca243a2-c567-4ac0-b327-24b48064d0e3,2,true,
			//	%(("local-connect":true),("local-peer":101b4159-d5ef-4d7f-b52a-1f1bd4caf55d),("origin-tag":ea0903a3-8262-4477-a9d2-87303d0a29b5),("send-response":false)),
			//	%()]
			//
			//	* peer
			//	* cluster bus sequence
			//	* shutdown mode
			//	* options
			//	* bag

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,		//	[0] peer
				std::uint64_t,			//	[1] cluster sequence
				bool,					//	[2] shutdown flag
				cyng::param_map_t,		//	[3] options
				cyng::param_map_t		//	[4] bag
			>(frame);

			//
			//	in shutdown mode no response should be sent.
			//
			auto shutdown = std::get<2>(tpl);

			//
			//	dom reader
			//
			auto dom = cyng::make_reader(std::get<3>(tpl));

			cyng::param_map_t tmp;
			//const std::size_t tsk = cyng::async::start_task_sync<close_connection>(mux_
			//	, logger_
			//	, bus_
			//	, vm_
			//	, shutdown
			//	, cyng::value_cast(dom.get("origin-tag"), boost::uuids::nil_uuid())
			//	, std::get<1>(tpl)	//	cluster bus sequence
			//	, std::get<3>(tpl)	//	options
			//	, std::get<4>(tpl)	//	bag
			//	, timeout_).first;

			//CYNG_LOG_TRACE(logger_, "client.req.close.connection.forward - task #" << tsk);

		}

		void session::client_res_close_connection_forward(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			//
			//	[d6aac617-2f91-47ab-87a3-287f584f55a4,24,true,
			//	%(("local-connect":false),("local-peer":22206aa4-0bc3-40c0-9ef4-17b3733a3aae),("origin-tag":5c2fa815-82bc-46dd-a3b9-8b07124c4994)),
			//	%()]

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,		//	[0] peer
				std::uint64_t,			//	[1] cluster sequence
				bool,					//	[2] shutdown flag
				cyng::param_map_t,		//	[3] options
				cyng::param_map_t		//	[4] bag
			>(frame);

			if (std::get<2>(tpl))
			{
				ctx.run(cyng::generate_invoke("log.msg.trace", "client.res.close.connection.forward", frame));
				ctx.attach(cyng::generate_invoke("print.ok"));
				ctx.attach(cyng::generate_invoke("stream.flush"));
			}
			else
			{
				ctx.run(cyng::generate_invoke("log.msg.warning", "client.res.close.connection.forward", frame));
				ctx.attach(cyng::generate_invoke("print.error"));
				ctx.attach(cyng::generate_invoke("stream.flush"));
			}
		}

		void session::client_res_open_connection_forward(cyng::context& ctx)
		{
			//	[bff22fb4-f410-4239-83ae-40027fb2609e,f07f47e1-7f4d-4398-8fb5-ae1d8942a50e,16,true,
			//	%(("device-name":LSMTest4),("local-connect":true),("local-peer":3d5994f1-60df-4410-b6ba-c2934f8bfd5e),("origin-tag":bff22fb4-f410-4239-83ae-40027fb2609e),("remote-peer":3d5994f1-60df-4410-b6ba-c2934f8bfd5e),("remote-tag":3f972487-e9a9-4e03-92d4-e2df8f6d30c5)),
			//	%(("seq":1),("tp-layer":ipt))]
			//
			//	* peer
			//	* cluster sequence
			//	* success
			//	* options
			//	* bag - original data
			//ctx.run(cyng::generate_invoke("log.msg.trace", "client.res.open.connection.forward", ctx.get_frame()));

			const cyng::vector_t frame = ctx.get_frame();

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,		//	[0] peer
				std::uint64_t,			//	[1] cluster sequence
				bool,					//	[2] success
				cyng::param_map_t,		//	[3] options
				cyng::param_map_t		//	[4] bag
			>(frame);

			const bool success = std::get<2>(tpl);

			if (success)
			{
				ctx.attach(cyng::generate_invoke("log.msg.info", "client.res.open.connection.forward", ctx.get_frame()));
				ctx.attach(cyng::generate_invoke("print.ok"));

				//
				//	dom reader
				//
				auto dom = cyng::make_reader(std::get<3>(tpl));
				BOOST_ASSERT(!connect_state_.connected_local_);
				connect_state_.connected_local_ = cyng::value_cast(dom.get("local-connect"), false);

				//
				//	update parser state
				//
				parser_.set_stream_mode();
			}
			else
			{
				ctx.attach(cyng::generate_invoke("log.msg.warning", "client.res.open.connection.forward", ctx.get_frame()));
				ctx.attach(cyng::generate_invoke("print.no-answer"));
			}

			ctx.attach(cyng::generate_invoke("stream.flush"));

		}

		void session::client_res_transfer_pushdata(cyng::context& ctx)
		{
			//	[708758da-ab11-43dd-bc32-cbbb0e1b4b36,1274e763-c365-4898-a544-3641c3b47534,17,474ba8c4,a1e24bba,2,
			//	%(("block":0),("seq":d8),("status":c1),("tp-layer":ipt))]
			//
			//	* [session tag] - removed
			//	* peer
			//	* cluster sequence
			//	* source
			//	* channel
			//	* count
			//	* bag
			const cyng::vector_t frame = ctx.get_frame();

			auto const tpl = cyng::tuple_cast<
				//boost::uuids::uuid,		//	[0] tag
				boost::uuids::uuid,		//	[1] peer
				std::uint64_t,			//	[2] cluster sequence
				std::uint32_t,			//	[3] source
				std::uint32_t,			//	[4] channel
				std::size_t,			//	[5] count
				cyng::param_map_t		//	[6] bag
			>(frame);

			//
			//	dom reader
			//
			//auto dom = cyng::make_reader(std::get<5>(tpl));
			//const sequence_type seq = cyng::value_cast<sequence_type>(dom.get("seq"), 0);

			////
			////	set acknownlegde flag
			////
			//std::uint8_t status = cyng::value_cast<std::uint8_t>(dom.get("status"), 0);
			//BOOST_ASSERT_MSG(status == 0xc1, "invalid push channel status");
			//status |= ipt::tp_res_pushdata_transfer_policy::ACK;

			////
			////	response
			////
			//response_type res = (std::get<4>(tpl) != 0)
			//	? ipt::tp_res_pushdata_transfer_policy::SUCCESS
			//	: ipt::tp_res_pushdata_transfer_policy::BROKEN
			//	;

			//if (std::get<4>(tpl) != 0)
			//{
			//	ctx.attach(cyng::generate_invoke("log.msg.debug"
			//		, "client.res.transfer.pushdata - source"
			//		, std::get<2>(tpl)
			//		, "channel"
			//		, std::get<3>(tpl)
			//		, "count"
			//		, std::get<4>(tpl)
			//		, "OK"));
			//}
			//else
			//{
			//	ctx.attach(cyng::generate_invoke("log.msg.warning", "client.res.transfer.pushdata - failed", frame));
			//}

			//ctx.attach(cyng::generate_invoke("res.transfer.push.data"
			//	, seq
			//	, res
			//	, std::get<2>(tpl)	//	source
			//	, std::get<3>(tpl)	//	channel
			//	, status
			//	, cyng::value_cast<std::uint8_t>(dom.get("block"), 0)
			//));
			//ctx.attach(cyng::generate_invoke("stream.flush"));
		}

		void session::client_req_transfer_pushdata_forward(cyng::context& ctx)
		{
			//	[284afa0f-d192-47c6-870c-e65a54e276b2,eb11e1fd-de09-4de3-aa4c-e3ad4e2269e3,12,b9d09511,3895afe1,e7e1faee,
			//	%(("block":0),("seq":47),("status":c1),("tp-layer":ipt)),
			//	1B1B1B1B01010101760636383337316200620072...97E010163A4DC00]

			//
			//	* [session tag] - removed
			//	* peer
			//	* cluster sequence
			//	* channel
			//	* source
			//	* target
			//	* bag
			//	* data
			//
			const cyng::vector_t frame = ctx.get_frame();
			//ctx.run(cyng::generate_invoke("log.msg.debug", "client.req.transfer.pushdata.forward", frame));

			auto const tpl = cyng::tuple_cast<
				//boost::uuids::uuid,		//	[0] tag
				boost::uuids::uuid,		//	[1] peer
				std::uint64_t,			//	[2] cluster sequence
				std::uint32_t,			//	[3] channel
				std::uint32_t,			//	[4] source
				std::uint32_t,			//	[5] target
				cyng::param_map_t,		//	[6] bag
				cyng::buffer_t			//	[7] data
			>(frame);

			ctx.attach(cyng::generate_invoke("log.msg.debug", "client.req.transfer.pushdata.forward - channel"
				, std::get<2>(tpl)
				, "source"
				, std::get<3>(tpl)
				, std::get<5>(tpl).size()
				, "bytes"));

			//
			//	dom reader
			//
			auto dom = cyng::make_reader(std::get<5>(tpl));
			auto status = cyng::value_cast<std::uint8_t>(dom.get("status"), 0);
			auto block = cyng::value_cast<std::uint8_t>(dom.get("block"), 0);

			ctx.attach(cyng::generate_invoke("req.transfer.push.data"
				, std::get<4>(tpl)	//	channel
				, std::get<3>(tpl)	//	source
				, status 
				, block 
				, std::get<6>(tpl)));	//	data
			ctx.attach(cyng::generate_invoke("stream.flush"));
		}

		void session::modem_req_close_connection(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			if (bus_->is_online())
			{
				ctx.run(cyng::generate_invoke("log.msg.info", "modem.req.close.connection", frame));

				cyng::param_map_t bag;
				bag["tp-layer"] = cyng::make_object("modem");
				bus_->vm_.async_run(client_req_close_connection(cyng::value_cast(frame.at(0), boost::uuids::nil_uuid())
					, bag));
			}
			else
			{
				ctx.attach(cyng::generate_invoke("log.msg.error", "modem.req.close.connection - no master", frame));
				ctx.attach(cyng::generate_invoke("print.error"));
				ctx.attach(cyng::generate_invoke("stream.flush"));
			}
		}

		void session::client_res_open_connection(cyng::context& ctx)
		{
			//	[client.res.open.connection,[767e4b41-3df2-402e-9cbc-d20de610000a,efeb7b5d-aec7-4e91-8a1b-c88f8fd4e01d,2,false,%(("device-name":LSMTest1),("response-code":0)),%(("seq":1),("tp-layer":ipt))]]
			//
			//	* [session tag] - removed
			//	* remote peer
			//	* cluster bus sequence
			//	* success
			//	* options
			//	* bag
			//	
			const cyng::vector_t frame = ctx.get_frame();
			ctx.attach(cyng::generate_invoke("log.msg.info", "client.res.open.connection", frame));

			//
			//	dom reader
			//
			auto dom = cyng::make_reader(frame);

			//const sequence_type seq = cyng::value_cast<sequence_type>(dom[4].get("seq"), 0);
			//const response_type res = cyng::value_cast<response_type>(dom[3].get("response-code"), 0);

			//ctx.attach(cyng::generate_invoke("res.open.connection", seq, res));
			//ctx.attach(cyng::generate_invoke("stream.flush"));

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
			ctx.attach(cyng::generate_invoke("log.msg.info", "ipt.res.open.connection", frame));

			//
			//	dom reader
			//
			auto dom = cyng::make_reader(frame);

			//const sequence_type seq = cyng::value_cast<sequence_type>(dom.get(1), 0);
			//const response_type res = cyng::value_cast<response_type>(dom.get(2), 0);

			////
			////	search related task
			////
			//auto pos = task_db_.find(seq);
			//if (pos != task_db_.end())
			//{
			//	const auto tsk = pos->second;
   //             ctx.attach(cyng::generate_invoke("log.msg.trace", "continue task", tsk));
			//	mux_.send(tsk, 0, cyng::tuple_factory(res));

			//	//
			//	//	remove entry
			//	//
			//	task_db_.erase(pos);
			//}
			//else
			//{
			//	ctx.attach(cyng::generate_invoke("log.msg.error", "empty sequence/task relation", seq));
			//}
		}

		void session::ipt_res_close_connection(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			ctx.attach(cyng::generate_invoke("log.msg.info", "ipt.res.close.connection", frame));

			//
			//	dom reader
			//
			auto dom = cyng::make_reader(frame);

			//const sequence_type seq = cyng::value_cast<sequence_type>(dom.get(1), 0);
			//const response_type res = cyng::value_cast<response_type>(dom.get(2), 0);

			////
			////	search related task
			////
			//auto pos = task_db_.find(seq);
			//if (pos != task_db_.end())
			//{
			//	const auto tsk = pos->second;
   //             ctx.attach(cyng::generate_invoke("log.msg.trace", "continue task", tsk));
			//	mux_.send(tsk, 0, cyng::tuple_factory(res));

			//	//
			//	//	remove entry
			//	//
			//	task_db_.erase(pos);
			//}
			//else
			//{
			//	ctx.attach(cyng::generate_invoke("log.msg.error", "empty sequence/task relation", seq));
			//}
		}

		void session::ipt_req_transfer_pushdata(cyng::context& ctx)
		{
			//	[48d97c37-74b1-4119-8832-f41d4de391e9,44,474ba8c4,3895afe1,c1,0,1B1B1B1B01010101760834313132...C46010163C87900]
			//	
			//	* session tag
			//	* ipt sequence
			//	* source
			//	* channel
			//	* status
			//	* block
			//	* data
			const cyng::vector_t frame = ctx.get_frame();

			if (bus_->is_online())
			{
				ctx.run(cyng::generate_invoke("log.msg.info", "ipt.req.transfer.pushdata", frame));

				cyng::param_map_t bag;
				bag["tp-layer"] = cyng::make_object("ipt");
				bag["seq"] = frame.at(1);
				bag["status"] = frame.at(4);
				bag["block"] = frame.at(5);
				bus_->vm_.async_run(client_req_transfer_pushdata(cyng::value_cast(frame.at(0), boost::uuids::nil_uuid())
					, cyng::value_cast<std::uint32_t>(frame.at(2), 0)
					, cyng::value_cast<std::uint32_t>(frame.at(3), 0)
					, frame.at(6)
					, bag));
			}
			else
			{
				ctx.attach(cyng::generate_invoke("log.msg.error", "ipt.req.transfer.pushdata", frame));
				//ctx.attach(cyng::generate_invoke("res.close.connection", frame.at(1), static_cast<response_type>(tp_res_close_connection_policy::CONNECTION_CLEARING_FORBIDDEN)));
				//ctx.attach(cyng::generate_invoke("stream.flush"));
			}
		}

		session::connect_state::connect_state()
			: connected_local_(false)
		{}

	}
}



