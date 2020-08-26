/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 


#include "session.h"
#include "../../shared/tasks/gatekeeper.h"
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
#ifdef SMF_IO_DEBUG
#include <cyng/io/hex_dump.hpp>
#endif

namespace node 
{
	namespace modem
	{
		session::session(boost::asio::ip::tcp::socket&& socket
			, cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, bus::shared_type bus
			, boost::uuids::uuid tag
			, std::chrono::seconds const& timeout
			, bool auto_answer
			, std::chrono::milliseconds guard_time)
		: session_stub(std::move(socket), mux, logger, bus, tag, timeout)
			, parser_([this](cyng::vector_t&& prg) {
				CYNG_LOG_INFO(logger_, prg.size() << " modem instructions received");
				CYNG_LOG_TRACE(logger_, vm_.tag() << ": " << cyng::io::to_str(prg));
				vm_.async_run(std::move(prg));
			}, guard_time)
			, serializer_(socket_, vm_)
			, state_(this, auto_answer)
		{
			vm_.register_function("session.store.relation", 2, std::bind(&session::store_relation, this, std::placeholders::_1));
			//vm_.register_function("session.update.connection.state", 2, std::bind(&session::update_connection_state, this, std::placeholders::_1));

			//
			//	register request handler
			//	client.req.transmit.data.forward
			vm_.register_function("modem.req.transmit.data", 2, std::bind(&session::modem_req_transmit_data, this, std::placeholders::_1));
			vm_.register_function("client.req.transmit.data.forward", 4, std::bind(&session::client_req_transmit_data_forward, this, std::placeholders::_1));


			//	transport
			//	transport - push channel open
			//TP_REQ_OPEN_PUSH_CHANNEL = 0x9000,
			//vm_.register_function("ipt.req.open.push.channel", 8, std::bind(&session::ipt_req_open_push_channel, this, std::placeholders::_1));
			vm_.register_function("client.res.open.push.channel", 8, std::bind(&session::client_res_open_push_channel, this, std::placeholders::_1));

			//TP_RES_OPEN_PUSH_CHANNEL = 0x1000,	//!<	response

			//	transport - push channel close
			//TP_REQ_CLOSE_PUSH_CHANNEL = 0x9001,	//!<	request
			//vm_.register_function("ipt.req.close.push.channel", 3, std::bind(&session::ipt_req_close_push_channel, this, std::placeholders::_1));
			vm_.register_function("client.res.close.push.channel", 5, std::bind(&session::client_res_close_push_channel, this, std::placeholders::_1));
			//TP_RES_CLOSE_PUSH_CHANNEL = 0x1001,	//!<	response

			//	transport - push channel data transfer
			//TP_REQ_PUSHDATA_TRANSFER = 0x9002,	//!<	request
			//vm_.register_function("ipt.req.transfer.pushdata", 7, std::bind(&session::ipt_req_transfer_pushdata, this, std::placeholders::_1));
			vm_.register_function("client.req.transfer.pushdata.forward", 6, std::bind(&session::client_req_transfer_pushdata_forward, this, std::placeholders::_1));

			//TP_RES_PUSHDATA_TRANSFER = 0x1002,	//!<	response
			vm_.register_function("client.res.transfer.pushdata", 6, std::bind(&session::client_res_transfer_pushdata, this, std::placeholders::_1));

			//	transport - connection open
			//TP_REQ_OPEN_CONNECTION = 0x9003,	//!<	request
			vm_.register_function("modem.req.open.connection", 2, std::bind(&session::modem_req_open_connection, this, std::placeholders::_1));
			vm_.register_function("client.req.open.connection.forward", 5, std::bind(&session::client_req_open_connection_forward, this, std::placeholders::_1));
			//TP_RES_OPEN_CONNECTION = 0x1003,	//!<	response
			//vm_.register_function("ipt.res.open.connection", 3, std::bind(&session::ipt_res_open_connection, this, std::placeholders::_1));
			vm_.register_function("client.res.open.connection", 5, std::bind(&session::client_res_open_connection, this, std::placeholders::_1));
			vm_.register_function("client.res.open.connection.forward", 5, std::bind(&session::client_res_open_connection_forward, this, std::placeholders::_1));

			//	transport - connection close
			//TP_REQ_CLOSE_CONNECTION = 0x9004,	//!<	request
			vm_.register_function("modem.req.close.connection", 1, std::bind(&session::modem_req_close_connection, this, std::placeholders::_1));
			vm_.register_function("client.req.close.connection.forward", 6, std::bind(&session::client_req_close_connection_forward, this, std::placeholders::_1));
			//TP_RES_CLOSE_CONNECTION = 0x1004,	//!<	response
			//vm_.register_function("ipt.res.close.connection", 0, std::bind(&session::ipt_res_close_connection, this, std::placeholders::_1));
			vm_.register_function("client.res.close.connection.forward", 5, std::bind(&session::client_res_close_connection_forward, this, std::placeholders::_1));

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
			//vm_.register_function("ipt.res.protocol.version", 3, std::bind(&session::ipt_res_protocol_version, this, std::placeholders::_1));

			//	application - device firmware version
			//APP_REQ_SOFTWARE_VERSION = 0xA001,	//!<	request
			//APP_RES_SOFTWARE_VERSION = 0x2001,	//!<	response
			//vm_.register_function("ipt.res.software.version", 3, std::bind(&session::ipt_res_software_version, this, std::placeholders::_1));

			//	application - device identifier
			//APP_REQ_DEVICE_IDENTIFIER = 0xA003,	//!<	request
			//APP_RES_DEVICE_IDENTIFIER = 0x2003,
			//vm_.register_function("ipt.res.dev.id", 3, std::bind(&session::ipt_res_dev_id, this, std::placeholders::_1));

			//	application - network status
			//APP_REQ_NETWORK_STATUS = 0xA004,	//!<	request
			//APP_RES_NETWORK_STATUS = 0x2004,	//!<	response
			//vm_.register_function("ipt.res.network.stat", 10, std::bind(&session::ipt_res_network_stat, this, std::placeholders::_1));

			//	application - IP statistic
			//APP_REQ_IP_STATISTICS = 0xA005,	//!<	request
			//APP_RES_IP_STATISTICS = 0x2005,	//!<	response
			//vm_.register_function("ipt.res.ip.statistics", 5, std::bind(&session::ipt_res_ip_statistics, this, std::placeholders::_1));

			//	application - device authentification
			//APP_REQ_DEVICE_AUTHENTIFICATION = 0xA006,	//!<	request
			//APP_RES_DEVICE_AUTHENTIFICATION = 0x2006,	//!<	response
			//vm_.register_function("ipt.res.dev.auth", 6, std::bind(&session::ipt_res_dev_auth, this, std::placeholders::_1));

			//	application - device time
			//APP_REQ_DEVICE_TIME = 0xA007,	//!<	request
			//APP_RES_DEVICE_TIME = 0x2007,	//!<	response
			//vm_.register_function("ipt.res.dev.time", 3, std::bind(&session::ipt_res_dev_time, this, std::placeholders::_1));

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
			vm_.register_function("modem.req.login.public", 5, std::bind(&session::modem_req_login, this, std::placeholders::_1));
			vm_.register_function("client.res.login", 6, std::bind(&session::client_res_login, this, std::placeholders::_1));

			//	control - maintenance
			//MAINTENANCE_REQUEST = 0xC003,	//!<	request *** deprecated ***
			//MAINTENANCE_RESPONSE = 0x4003,	//!<	response *** deprecated ***

			//	control - logout
			//CTRL_REQ_LOGOUT = 0xC004,	//!<	request *** deprecated ***
			//CTRL_RES_LOGOUT = 0x4004,	//!<	response *** deprecated ***
			//vm_.register_function("ipt.req.logout", 2, std::bind(&session::ipt_req_logout, this, std::placeholders::_1));
			//vm_.register_function("ipt.res.logout", 2, std::bind(&session::ipt_res_logout, this, std::placeholders::_1));

			//	control - push target register
			//CTRL_REQ_REGISTER_TARGET = 0xC005,	//!<	request
			//vm_.register_function("ipt.req.register.push.target", 5, std::bind(&session::ipt_req_register_push_target, this, std::placeholders::_1));
			vm_.register_function("client.res.register.push.target", 6, std::bind(&session::client_res_register_push_target, this, std::placeholders::_1));
			//CTRL_RES_REGISTER_TARGET = 0x4005,	//!<	response

			//	control - push target deregister
			//CTRL_REQ_DEREGISTER_TARGET = 0xC006,	//!<	request
			//CTRL_RES_DEREGISTER_TARGET = 0x4006,	//!<	response

			//	control - watchdog
			//CTRL_REQ_WATCHDOG = 0xC008,	//!<	request
			//CTRL_RES_WATCHDOG = 0x4008,	//!<	response
			//vm_.register_function("ipt.res.watchdog", 0, std::bind(&session::ipt_res_watchdog, this, std::placeholders::_1));

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
			//vm_.register_function("ipt.unknown.cmd", 3, std::bind(&session::ipt_unknown_cmd, this, std::placeholders::_1));

			vm_.register_function("modem.req.info", 1, std::bind(&session::modem_req_info, this, std::placeholders::_1));

			//
			//	initialize state engine
			//
			state_.react(state::evt_init_complete(cyng::async::start_task_sync<node::gatekeeper>(mux_
				, logger_
				, vm_
				, timeout)));

		}

		session::~session()
		{}

		cyng::buffer_t session::parse(read_buffer_const_iterator begin, read_buffer_const_iterator end)
		{
			const auto bytes_transferred = std::distance(begin, end);
			vm_.async_run(cyng::generate_invoke("log.msg.trace", "modem connection received", bytes_transferred, "bytes"));

			//
			//	size == parsed bytes
			//
			const auto size = parser_.read(begin, end);
			BOOST_ASSERT(size == bytes_transferred);
			boost::ignore_unused(size);	//	release version

#ifdef SMF_IO_DEBUG
			cyng::io::hex_dump hd;
			std::stringstream ss;
			hd(ss, buffer_.data(), buffer_.data() + bytes_transferred);
			CYNG_LOG_TRACE(logger_, "imega input dump \n" << ss.str());
#endif
			return cyng::buffer_t(begin, end);
		}

		void session::shutdown()
		{
			//
			//	Clear connection map and stop running tasks
			//
			state_.react(state::evt_shutdown());

			//
			//	stop all tasks
			//
			//CYNG_LOG_TRACE(logger_, vm_.tag() << " stop " << task_db_.size() << " task(s)");
			//for (auto const& tsk : task_db_) {
			//	mux_.stop(tsk.second.first);
			//}

			//
			//	reset AT parser
			//
			CYNG_LOG_TRACE(logger_, vm_.tag() << " reset parser");
			parser_.clear();

			//
			//	reset AT serializer
			//

			//
			//	call baseclass
			//	gracefull shutdown
			//	device/party closed connection or network shutdown
			//
			session_stub::shutdown();

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

		void session::modem_req_info(cyng::context& ctx)
		{
			//	[de17d93c-ce2e-4d93-bdbc-e38643b934e0,00000009]
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "modem.req.info " << cyng::io::to_str(frame));

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,		//	[0] remote tag
				std::uint32_t			//	[1] code
			>(frame);

			ctx.queue(state_.react(state::evt_info(std::get<1>(tpl))));

		}

		void session::modem_req_login(cyng::context& ctx)
		{
			BOOST_ASSERT(ctx.tag() == vm_.tag());

			//	[LSMTest5,LSMTest5]
			ctx.queue(state_.react(state::evt_req_login(cyng::tuple_cast<
				boost::uuids::uuid,		//	[0] peer tag
				std::string,			//	[1] name
				std::string,			//	[2] pwd
				boost::asio::ip::tcp::endpoint,
				boost::asio::ip::tcp::endpoint
			>(ctx.get_frame()))));
		}

		void session::modem_req_transmit_data(cyng::context& ctx)
		{
			//	[modem.req.transmit.data,
			//	[c5eae235-d5f5-413f-a008-5d317d8baab7,1B1B1B1B01010101768106313830313...1B1B1B1B1A034276]]
			const cyng::vector_t frame = ctx.get_frame();
			BOOST_ASSERT(cyng::value_cast(frame.at(0), boost::uuids::nil_uuid()) == ctx.tag());

			//
			//	data destination depends on connection state
			//
			state_.react(state::evt_data(frame.at(1)));
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

			ctx.queue(cyng::generate_invoke("modem.transfer.data", frame.at(3)));
			ctx.queue(cyng::generate_invoke("stream.flush"));
			
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

			ctx.queue(state_.react(state::evt_res_login(cyng::tuple_cast<
				boost::uuids::uuid,		//	[0] peer tag
				std::uint64_t,			//	[1] sequence number
				bool,					//	[2] success
				std::string,			//	[3] name
				std::string,			//	[4] msg
				std::uint32_t,			//	[5] query
				cyng::param_map_t		//	[6] bag
			>(ctx.get_frame()))));

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
			ctx.queue(cyng::generate_invoke("log.msg.warning", ctx.get_name(), frame));

			//
			//	not implemented
			//
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
			ctx.queue(cyng::generate_invoke("log.msg.warning", ctx.get_name(), frame));

			//
			//	not implemented
			//
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
			ctx.queue(cyng::generate_invoke("log.msg.warning", ctx.get_name(), frame));

			//
			//	not implemented
			//
		}

		void session::modem_req_open_connection(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			if (bus_->is_online())
			{
				ctx.run(cyng::generate_invoke("log.msg.info", ctx.get_name(), frame));

				cyng::param_map_t bag;
				bag["tp-layer"] = cyng::make_object("modem");
				bus_->vm_.async_run(client_req_open_connection(cyng::value_cast(frame.at(0), boost::uuids::nil_uuid())
					, cyng::value_cast<std::string>(frame.at(1), "")	//	number
					, bag));
			}
            else
			{
				ctx.queue(cyng::generate_invoke("log.msg.error", ctx.get_name(), frame));
				ctx.queue(cyng::generate_invoke("print.error"));
				ctx.queue(cyng::generate_invoke("stream.flush"));
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
			auto const dom = cyng::make_reader(std::get<3>(tpl));
			auto const local_connect = cyng::value_cast(dom.get("local-connect"), false);

			ctx.queue(state_.react(state::evt_client_req_open_connection(local_connect
				, std::get<1>(tpl)	//	cluster bus sequence
				, std::get<3>(tpl)	//	master
				, std::get<4>(tpl)	//	client
			)));

		}

		void session::client_req_close_connection_forward(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			ctx.run(cyng::generate_invoke("log.msg.trace", ctx.get_name(), frame));

			//
			//	[5ef23385-4b75-4ed7-997b-a84f7f3e63c0,4,false,
			//	%(("local-connect":true),("local-peer":bdc31cf8-e18e-4d95-ad31-ad821661e857),("origin-tag":5afa7628-caa3-484d-b1de-a4730b53a656)),
			//	%(("tp-layer":modem))]
			//
			//	* peer
			//	* cluster bus sequence
			//	* shutdown mode
			//	* options
			//	* bag

			state_.react(state::evt_client_req_close_connection(cyng::tuple_cast<
				boost::uuids::uuid,		//	[0] peer
				std::uint64_t,			//	[1] cluster sequence
				boost::uuids::uuid,		//	[2] origin-tag
				bool,					//	[3] shutdown flag
				cyng::param_map_t,		//	[4] options
				cyng::param_map_t		//	[5] bag
			>(frame)));	//	client

		}

		void session::client_res_close_connection_forward(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();

			//
			//	[b985e67c-38e2-4850-b584-242c75a5036c,3,true,
			//	%(("local-connect":true),("local-peer":bdc31cf8-e18e-4d95-ad31-ad821661e857),("origin-tag":1f5ba15e-d0fa-402d-a808-3f7ce0df8bd5)),
			//	%(("tp-layer":modem))]
			//
			ctx.queue(state_.react(state::evt_client_res_close_connection(cyng::tuple_cast<
				boost::uuids::uuid,		//	[0] peer
				std::uint64_t,			//	[1] cluster sequence
				bool,					//	[2] success flag from remote session
				cyng::param_map_t,		//	[3] options
				cyng::param_map_t		//	[4] bag
			>(ctx.get_frame()))));

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
			ctx.queue(state_.react(state::evt_client_res_open_connection(cyng::tuple_cast<
				boost::uuids::uuid,		//	[0] peer
				std::uint64_t,			//	[1] cluster sequence
				bool,					//	[2] success flag from remote session
				cyng::param_map_t,		//	[3] options
				cyng::param_map_t		//	[4] bag
			>(ctx.get_frame()))));

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
			ctx.queue(cyng::generate_invoke("log.msg.warning", ctx.get_name(), frame));

			//
			//	not implemented
			//
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
			ctx.queue(cyng::generate_invoke("log.msg.warning", ctx.get_name(), frame));

			//
			//	not implemented
			//
		}

		void session::modem_req_close_connection(cyng::context& ctx)
		{
			//	[99cf07d5-0f3e-4a39-a5ca-bb8484834386]
			const cyng::vector_t frame = ctx.get_frame();
			state_.react(state::evt_modem_req_close_connection(cyng::value_cast(frame.at(0), boost::uuids::nil_uuid())));

			if (bus_->is_online())
			{
				ctx.run(cyng::generate_invoke("log.msg.info", "modem.req.close.connection", frame));

				bus_->vm_.async_run(node::client_req_close_connection(ctx.tag()
					, false //	no shutdown
					, cyng::param_map_factory("tp-layer", "AT")
					("data-layer", "any")
					("origin-tag", ctx.tag())
					("start", std::chrono::system_clock::now())));
			}
			else
			{
				ctx.queue(cyng::generate_invoke("log.msg.error", "modem.req.close.connection - no master", frame));
				ctx.queue(cyng::generate_invoke("print.error"));
				ctx.queue(cyng::generate_invoke("stream.flush"));
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
			ctx.queue(cyng::generate_invoke("log.msg.info", ctx.get_name(), frame));

			ctx.queue(state_.react(state::evt_client_res_open_connection(cyng::tuple_cast<
				boost::uuids::uuid,		//	[0] peer tag
				std::uint64_t,			//	[1] cluster seq
				bool,					//	[2] success
				cyng::param_map_t,		//	[3] master
				cyng::param_map_t		//	[4] client
			>(ctx.get_frame()))));

		}

		cyng::object make_session(boost::asio::ip::tcp::socket&& socket
			, cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, bus::shared_type bus
			, boost::uuids::uuid tag
			, std::chrono::seconds const& timeout
			, bool auto_answer
			, std::chrono::milliseconds guard_time)
		{
			return cyng::make_object<session>(std::move(socket)
				, mux
				, logger
				, bus
				, tag
				, timeout
				, auto_answer
				, guard_time);

		}
	}
}


namespace cyng
{
	namespace traits
	{

#if !defined(__CPP_SUPPORT_N2235)
		const char type_tag<node::modem::session>::name[] = "modem::session";
#endif
	}	// traits	
}

