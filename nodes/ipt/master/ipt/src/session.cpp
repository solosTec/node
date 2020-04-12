/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 


#include "session.h"
#include "tasks/open_connection.h"
#include "tasks/close_connection.h"
#include "../../../../shared/tasks/gatekeeper.h"
#include "../../shared/tasks/gateway_proxy.h"
#include "../../shared/tasks/watchdog.h"
#include <NODE_project_info.h>

#include <smf/cluster/generator.h>
#include <smf/ipt/response.hpp>
#include <smf/ipt/scramble_key_io.hpp>
#include <cyng/io/serializer.h>
#include <cyng/io/io_bytes.hpp>
#include <cyng/value_cast.hpp>
#include <cyng/tuple_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/dom/reader.h>
#include <cyng/async/task/task_builder.hpp>
#include <cyng/factory/set_factory.h>
#ifdef SMF_IO_LOG
#include <cyng/io/hex_dump.hpp>
#include <boost/uuid/uuid_io.hpp>
#endif
#include <boost/uuid/nil_generator.hpp>

namespace node 
{
	namespace ipt
	{
		session::session(boost::asio::ip::tcp::socket&& socket
			, cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, bus::shared_type bus
			, boost::uuids::uuid tag
			, std::chrono::seconds timeout
			, scramble_key const& sk
			, std::uint16_t watchdog
			, bool sml_log)
		: session_stub(std::move(socket), mux, logger, bus, tag, timeout)
			, parser_([this](cyng::vector_t&& prg) {
				//CYNG_LOG_DEBUG(logger_, prg.size() << " ipt instructions received");
				//CYNG_LOG_TRACE(logger_, vm_.tag() << ": " << cyng::io::to_str(prg));
				vm_.async_run(std::move(prg));
			}, sk)
			, serializer_(socket_, vm_, sk)
			, timeout_(timeout)
			, state_(this)
			, proxy_comm_(state_, vm_)
#ifdef SMF_IO_LOG
			, log_counter_(0)
#endif
		{

			vm_.register_function("session.store.relation", 3, std::bind(&session::store_relation, this, std::placeholders::_1));
			vm_.register_function("session.remove.relation", 1, std::bind(&session::remove_relation, this, std::placeholders::_1));

			vm_.register_function("session.start.watchdog", 1, [this](cyng::context& ctx) {

				const cyng::vector_t frame = ctx.get_frame();

				state_.react(state::evt_watchdog_started(cyng::async::start_task_sync<node::watchdog>(mux_
					, logger_
					, vm_
					, cyng::value_cast<std::uint16_t>(frame.at(0), 12)
					, cyng::value_cast<std::string>(frame.at(1), ""))));
				
			});

			vm_.register_function("client.req.gateway", 11, std::bind(&session::client_req_gateway, this, std::placeholders::_1));
			vm_.register_function("client.req.proxy", 10, std::bind(&session::client_req_proxy, this, std::placeholders::_1));

			vm_.register_function("session.start.proxy", 0, [this, sml_log](cyng::context& ctx) {

				//
				//	start SML proxy 
				//
				state_.react(state::evt_proxy_started(cyng::async::start_task_sync<node::gateway_proxy>(mux_
					, logger_
					, bus_
					, vm_
					, timeout_
					, sml_log)));

			});

			vm_.register_function("sml.log", 1, [this](cyng::context& ctx){
				const cyng::vector_t frame = ctx.get_frame();
				CYNG_LOG_INFO(logger_, "sml.log - " << cyng::value_cast<std::string>(frame.at(0), ""));
			});
			
			//
			//	register request handler
			//	client.req.transmit.data.forward
			vm_.register_function("ipt.req.transmit.data", 2, std::bind(&session::ipt_req_transmit_data, this, std::placeholders::_1));
			vm_.register_function("client.req.transmit.data.forward", 4, std::bind(&session::client_req_transmit_data_forward, this, std::placeholders::_1));


			//	transport
			//	transport - push channel open
			//TP_REQ_OPEN_PUSH_CHANNEL = 0x9000,
			vm_.register_function("ipt.req.open.push.channel", 8, std::bind(&session::ipt_req_open_push_channel, this, std::placeholders::_1));
			vm_.register_function("client.res.open.push.channel", 8, std::bind(&session::client_res_open_push_channel, this, std::placeholders::_1));

			//TP_RES_OPEN_PUSH_CHANNEL = 0x1000,	//!<	response
			vm_.register_function("ipt.res.open.push.channel", 9, std::bind(&session::ipt_res_open_push_channel, this, std::placeholders::_1));

			//	transport - push channel close
			//TP_REQ_CLOSE_PUSH_CHANNEL = 0x9001,	//!<	request
			vm_.register_function("ipt.req.close.push.channel", 3, std::bind(&session::ipt_req_close_push_channel, this, std::placeholders::_1));
			vm_.register_function("client.res.close.push.channel", 5, std::bind(&session::client_res_close_push_channel, this, std::placeholders::_1));
			//TP_RES_CLOSE_PUSH_CHANNEL = 0x1001,	//!<	response

			//	transport - push channel data transfer
			//TP_REQ_PUSHDATA_TRANSFER = 0x9002,	//!<	request
			vm_.register_function("ipt.req.transfer.pushdata", 7, std::bind(&session::ipt_req_transfer_pushdata, this, std::placeholders::_1));
			vm_.register_function("client.req.transfer.pushdata.forward", 6, std::bind(&session::client_req_transfer_pushdata_forward, this, std::placeholders::_1));

			//TP_RES_PUSHDATA_TRANSFER = 0x1002,	//!<	response
			vm_.register_function("ipt.res.transfer.pushdata", 7, std::bind(&session::ipt_res_transfer_pushdata, this, std::placeholders::_1));
			vm_.register_function("client.res.transfer.pushdata", 6, std::bind(&session::client_res_transfer_pushdata, this, std::placeholders::_1));

			//	transport - connection open
			//TP_REQ_OPEN_CONNECTION = 0x9003,	//!<	request
			vm_.register_function("ipt.req.open.connection", 3, std::bind(&session::ipt_req_open_connection, this, std::placeholders::_1));
			vm_.register_function("client.req.open.connection.forward", 5, std::bind(&session::client_req_open_connection_forward, this, std::placeholders::_1));
			//TP_RES_OPEN_CONNECTION = 0x1003,	//!<	response
			vm_.register_function("ipt.res.open.connection", 3, std::bind(&session::ipt_res_open_connection, this, std::placeholders::_1));
			vm_.register_function("client.res.open.connection", 5, std::bind(&session::client_res_open_connection, this, std::placeholders::_1));
			vm_.register_function("client.res.open.connection.forward", 5, std::bind(&session::client_res_open_connection_forward, this, std::placeholders::_1));

			//	transport - connection close
			//TP_REQ_CLOSE_CONNECTION = 0x9004,	//!<	request
			vm_.register_function("ipt.req.close.connection", 2, std::bind(&session::ipt_req_close_connection, this, std::placeholders::_1));
			vm_.register_function("client.req.close.connection.forward", 6, std::bind(&session::client_req_close_connection_forward, this, std::placeholders::_1));
			//TP_RES_CLOSE_CONNECTION = 0x1004,	//!<	response
			vm_.register_function("ipt.res.close.connection", 0, std::bind(&session::ipt_res_close_connection, this, std::placeholders::_1));
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
			vm_.register_function("ipt.req.protocol.version", 2, std::bind(&session::ipt_req_protocol_version, this, std::placeholders::_1));
			vm_.register_function("ipt.res.protocol.version", 3, std::bind(&session::ipt_res_protocol_version, this, std::placeholders::_1));

			//	application - device firmware version
			//APP_REQ_SOFTWARE_VERSION = 0xA001,	//!<	request
			//APP_RES_SOFTWARE_VERSION = 0x2001,	//!<	response
			vm_.register_function("ipt.req.software.version", 2, std::bind(&session::ipt_req_software_version, this, std::placeholders::_1));
			vm_.register_function("ipt.res.software.version", 3, std::bind(&session::ipt_res_software_version, this, std::placeholders::_1));

			//	application - device identifier
			//APP_REQ_DEVICE_IDENTIFIER = 0xA003,	//!<	request
			//APP_RES_DEVICE_IDENTIFIER = 0x2003,
			vm_.register_function("ipt.req.device.id", 0, std::bind(&session::ipt_req_device_id, this, std::placeholders::_1));
			vm_.register_function("ipt.res.dev.id", 3, std::bind(&session::ipt_res_dev_id, this, std::placeholders::_1));

			//	application - network status
			//APP_REQ_NETWORK_STATUS = 0xA004,	//!<	request
			//APP_RES_NETWORK_STATUS = 0x2004,	//!<	response
			vm_.register_function("ipt.req.net.stat", 2, std::bind(&session::ipt_req_net_stat, this, std::placeholders::_1));
			vm_.register_function("ipt.res.network.stat", 10, std::bind(&session::ipt_res_network_stat, this, std::placeholders::_1));

			//	application - IP statistic
			//APP_REQ_IP_STATISTICS = 0xA005,	//!<	request
			//APP_RES_IP_STATISTICS = 0x2005,	//!<	response
			vm_.register_function("ipt.req.ip.statistics", 2, std::bind(&session::ipt_req_ip_statistics, this, std::placeholders::_1));
			vm_.register_function("ipt.res.ip.statistics", 5, std::bind(&session::ipt_res_ip_statistics, this, std::placeholders::_1));

			//	application - device authentification
			//APP_REQ_DEVICE_AUTHENTIFICATION = 0xA006,	//!<	request
			//APP_RES_DEVICE_AUTHENTIFICATION = 0x2006,	//!<	response
			vm_.register_function("ipt.req.dev.auth", 2, std::bind(&session::ipt_req_dev_auth, this, std::placeholders::_1));
			vm_.register_function("ipt.res.dev.auth", 6, std::bind(&session::ipt_res_dev_auth, this, std::placeholders::_1));

			//	application - device time
			//APP_REQ_DEVICE_TIME = 0xA007,	//!<	request
			//APP_RES_DEVICE_TIME = 0x2007,	//!<	response
			vm_.register_function("ipt.req.dev.time", 2, std::bind(&session::ipt_req_dev_time, this, std::placeholders::_1));
			vm_.register_function("ipt.res.dev.time", 3, std::bind(&session::ipt_res_dev_time, this, std::placeholders::_1));

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
			vm_.register_function("ipt.req.login.public", 2, std::bind(&session::ipt_req_login_public, this, std::placeholders::_1));
			vm_.register_function("ipt.req.login.scrambled", 3, std::bind(&session::ipt_req_login_scrambled, this, std::placeholders::_1));
			vm_.register_function("client.res.login", 6, std::bind(&session::client_res_login, this, std::placeholders::_1));

			//	control - maintenance
			//MAINTENANCE_REQUEST = 0xC003,	//!<	request *** deprecated ***
			//MAINTENANCE_RESPONSE = 0x4003,	//!<	response *** deprecated ***

			//	control - logout
			//CTRL_REQ_LOGOUT = 0xC004,	//!<	request *** deprecated ***
			//CTRL_RES_LOGOUT = 0x4004,	//!<	response *** deprecated ***
			vm_.register_function("ipt.req.logout", 2, std::bind(&session::ipt_req_logout, this, std::placeholders::_1));
			vm_.register_function("ipt.res.logout", 2, std::bind(&session::ipt_res_logout, this, std::placeholders::_1));

			//	control - push target register
			//CTRL_REQ_REGISTER_TARGET = 0xC005,	//!<	request
			vm_.register_function("ipt.req.register.push.target", 5, std::bind(&session::ipt_req_register_push_target, this, std::placeholders::_1));
			vm_.register_function("client.res.register.push.target", 6, std::bind(&session::client_res_register_push_target, this, std::placeholders::_1));
			//CTRL_RES_REGISTER_TARGET = 0x4005,	//!<	response

			//	control - push target deregister
			//CTRL_REQ_DEREGISTER_TARGET = 0xC006,	//!<	request
			//CTRL_RES_DEREGISTER_TARGET = 0x4006,	//!<	response
			vm_.register_function("ipt.req.deregister.push.target", 3, std::bind(&session::ipt_req_deregister_push_target, this, std::placeholders::_1));
			vm_.register_function("client.res.deregister.push.target", 6, std::bind(&session::client_res_deregister_push_target, this, std::placeholders::_1));

			//	control - watchdog
			//CTRL_REQ_WATCHDOG = 0xC008,	//!<	request
			//CTRL_RES_WATCHDOG = 0x4008,	//!<	response
			vm_.register_function("ipt.req.watchdog", 2, std::bind(&session::ipt_req_watchdog, this, std::placeholders::_1));
			vm_.register_function("ipt.res.watchdog", 0, std::bind(&session::ipt_res_watchdog, this, std::placeholders::_1));

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
			vm_.register_function("ipt.unknown.cmd", 3, std::bind(&session::ipt_unknown_cmd, this, std::placeholders::_1));

			//
			//	statistical data
			//
			vm_.async_run(cyng::generate_invoke("log.msg.debug", cyng::invoke("lib.size"), " callbacks registered"));

			//
			//	initialize state engine
			//
			state_.react(state::evt_init_complete(cyng::async::start_task_sync<node::gatekeeper>(mux_
				, logger_
				, vm_
				, timeout)
			, watchdog));

		}

		session::~session()
		{
			//std::cerr << std::endl << "~session(" << vm_.tag() << ")" << std::endl;
			CYNG_LOG_DEBUG(logger_, "~session(" << vm_.tag() << ")");
		}

		cyng::buffer_t session::parse(read_buffer_const_iterator begin, read_buffer_const_iterator end)
		{
			const auto bytes_transferred = std::distance(begin, end);
			vm_.async_run(cyng::generate_invoke("log.msg.trace", "ipt connection received ", bytes_transferred, cyng::invoke("log.fmt.byte")));

			//
			//	size == parsed bytes
			//
			const auto buffer = parser_.read(begin, end);
			//BOOST_ASSERT(size == bytes_transferred);
			//boost::ignore_unused(size);	//	release version

#ifdef SMF_IO_DEBUG
			{
				cyng::io::hex_dump hd;
				std::stringstream ss;
				hd(ss, buffer.begin(), buffer.end());
				CYNG_LOG_TRACE(logger_, "ipt input dump \n" << ss.str());
			}
#endif
			return buffer;
		}

		void session::shutdown()
		{
			//
			//	Clear connection map and stop proxy and watchdog tasks
			//
			state_.react(state::evt_shutdown());

			//
			//	stop all tasks
			//
			CYNG_LOG_TRACE(logger_, vm_.tag() << " stop " << task_db_.size() << " task(s)");
			for (auto const& tsk : task_db_) {

				//
				//	stop tasks
				//
				CYNG_LOG_TRACE(logger_, vm_.tag() << " task #" << tsk.second.first << " will be stopped");
				mux_.stop(tsk.second.first);
			}

			//
			//	reset IP-T parser
			//
			try {
				CYNG_LOG_TRACE(logger_, vm_.tag() << " reset parser");
				parser_.clear();
			}
			catch (std::exception const& ex) {
				CYNG_LOG_FATAL(logger_, vm_.tag() << " reset parser failed: " << ex.what());
			}

			//
			//	gracefull shutdown
			//	device/party closed connection or network shutdown
			//
			session_stub::shutdown();
		}

		void session::store_relation(cyng::context& ctx)
		{
			//	[1,2,0]
			//
			//	* ipt sequence number
			//	* task id
			//	* channel
			//	
			const cyng::vector_t frame = ctx.get_frame();
			auto const tpl = cyng::tuple_cast<
				sequence_type,		//	[0] ipt sequence
				std::size_t,		//	[1] task id
				std::size_t			//	[2] channel
			>(frame);

			if (task_db_.find(std::get<0>(tpl)) != task_db_.end()) {

				CYNG_LOG_WARNING(logger_, "session.store.relation - slot "
					<< +std::get<0>(tpl)
					<< " already occupied with #"
					<< std::get<1>(tpl)
					<< '/'
					<< task_db_.size());
			}
			else {

				task_db_.emplace(std::piecewise_construct
					, std::forward_as_tuple(std::get<0>(tpl))
					, std::forward_as_tuple(std::get<1>(tpl), std::get<2>(tpl)));

				CYNG_LOG_INFO(logger_, "session.store.relation "
					<< +std::get<0>(tpl)
					<< " => #"
					<< std::get<1>(tpl)
					<< ':'
					<< std::get<2>(tpl)
					<< '/'
					<< task_db_.size());
			}
		}

		void session::remove_relation(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			auto const tpl = cyng::tuple_cast<
				std::size_t			//	[0] task id
			>(frame);

			for (auto pos = task_db_.begin(); pos != task_db_.end(); ++pos) {
				if (pos->second.first == std::get<0>(tpl)) {

					CYNG_LOG_DEBUG(logger_, "session.remove.relation "
						<< +pos->first
						<< " => #"
						<< pos->second.first
						<< ':'
						<< pos->second.second);

					//
					//	remove from task db
					//
					pos = task_db_.erase(pos);

					break;
				}
			}
		}

		void session::client_req_gateway(cyng::context& ctx)
		{
			//	[c469c819-8b75-4dd7-a7b2-d74e228488c7,9f773865-e4af-489a-8824-8f78a2311278,4,[df735c77-797f-4ce8-bb74-86280f9884a9],[{("section":[status:word,srv:visible,srv:active,firmware,memory,root-wMBus-status,IF_wMBUS,ipt-status,ipt-config])}],0500153B022980,operator,operator]
			//	[c469c819-8b75-4dd7-a7b2-d74e228488c7,9f773865-e4af-489a-8824-8f78a2311278,5,[df735c77-797f-4ce8-bb74-86280f9884a9],[{("name":smf-form-gw-ipt-srv),("value":0500153B022980)},{("name":smf-gw-ipt-host-1),("value":waiting...)},{("name":smf-gw-ipt-local-1),("value":4)},{("name":smf-gw-ipt-remote-1),("value":3)},{("name":smf-gw-ipt-name-1),("value":waiting...)},{("name":smf-gw-ipt-pwd-1),("value":asdasd)},{("name":smf-gw-ipt-host-2),("value":waiting...)},{("name":smf-gw-ipt-local-2),("value":3)},{("name":smf-gw-ipt-remote-2),("value":3)},{("name":smf-gw-ipt-name-2),("value":holgär)},{("name":smf-gw-ipt-pwd-2),("value":asdasd)},{("section":[ipt])}],0500153B022980,operator,operator]
			//
			//	* [uuid] ident
			//	* [uuid] source
			//	* [u64] seq
			//	* [vec] TGateway key
			//	* [uuid] web-socket tag
			//	* [str] channel
			//	* [vec] sections
			//	* [vec] parameter
			//	* [buffer] server
			//	* [string] user
			//	* [string] pwd
			//
			auto const frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

			//
			//	update proxy queue
			//
			state_.react(state::evt_gateway(cyng::to_tuple(frame)));
		}

		void session::client_req_proxy(cyng::context& ctx)
		{
			//	[88342aae-0408-43b0-8608-7689cc673a85,9f773865-e4af-489a-8824-8f78a2311278,12,b930b77f-6dd5-4664-a8de-d797acb846f7,disable.cache,[ec563e58-f0d6-4d6a-8a13-63f639f6c9a7],[access,meter],0500153B0223B3,operator,operator]
			//
			//	* [uuid] ident
			//	* [uuid] source
			//	* [u64] seq
			//	* [uuid] origin (web-socket)
			//	* [str] job
			//	* [vec] gateway pk
			//	* [vec] sections
			//	* [buffer] server
			//	* [string] user
			//	* [string] pwd
			//
			auto const frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

			//
			//	update proxy queue
			//
			state_.react(state::evt_proxy(cyng::to_tuple(frame)));

		}

		void session::ipt_req_login_public(cyng::context& ctx)
		{
			BOOST_ASSERT(ctx.tag() == vm_.tag());
			//
			//	update session state
			//	[LSMTest5,LSMTest5,<sk>]
			//
			ctx.queue(state_.react(state::evt_req_login_public(cyng::tuple_cast<
				boost::uuids::uuid,		//	[0] peer tag
				std::string,			//	[1] name
				std::string,			//	[2] pwd
				boost::asio::ip::tcp::endpoint,
				boost::asio::ip::tcp::endpoint
			>(ctx.get_frame()))));
		}

		void session::ipt_req_login_scrambled(cyng::context& ctx)
		{
			//	[tag,LSMTest5,LSMTest5,<sk>]
			const cyng::vector_t frame = ctx.get_frame();

			ctx.queue(state_.react(state::evt_req_login_scrambled(cyng::value_cast(frame.at(0), boost::uuids::nil_uuid())
				, cyng::value_cast<std::string>(frame.at(1), "")
				, cyng::value_cast<std::string>(frame.at(2), "")
				, cyng::value_cast<scramble_key>(frame.at(3), scramble_key::null_scramble_key_)
				, cyng::value_cast(frame.at(4), boost::asio::ip::tcp::endpoint())
				, cyng::value_cast(frame.at(5), boost::asio::ip::tcp::endpoint()))));
		}

		void session::ipt_req_logout(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_WARNING(logger_, "ipt.req.logout - deprecated " << cyng::io::to_str(frame));
		}

		void session::ipt_res_logout(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_WARNING(logger_, "ipt.res.logout - deprecated " << cyng::io::to_str(frame));

			//
			//	Nothing more to do. Variosafe manager will close connection anyway.
			//	But we can tell the server to close the IP connection intentionally
			//
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
				ctx.run(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame));

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
				ctx.queue(cyng::generate_invoke("res.open.push.channel"
					, frame.at(1)
					, static_cast<response_type>(tp_res_open_push_channel_policy::UNREACHABLE)
					, static_cast<std::uint32_t>(0)	//	channel
					, static_cast<std::uint32_t>(0)	//	source
					, static_cast<std::uint16_t>(0)	//	packet size
					, static_cast<std::uint8_t>(0)	//	window size
					, static_cast<std::uint8_t>(0)	//	status
					, static_cast<std::uint32_t>(0)));	//	count
				ctx.queue(cyng::generate_invoke("stream.flush"));
			}

			//
			//	update watchdog timer
			//
			state_.react(state::evt_activity());
		}

		void session::ipt_res_open_push_channel(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			//CYNG_LOG_INFO(logger_, "ipt.res.open.push.channel " << cyng::io::to_str(frame));
			if (bus_->is_online())
			{
				ctx.run(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame));

				cyng::param_map_t bag, options;
				bag["tp-layer"] = cyng::make_object("ipt");
				bag["seq"] = frame.at(1);

				auto const tpl = cyng::tuple_cast<
					boost::uuids::uuid,		//	[0] tag
					sequence_type,			//	[1] ipt seq
					response_type,			//	[2] response
					std::uint32_t,			//	[3] channel
					std::uint32_t,			//	[4] source
					std::uint16_t,			//	[5] packet size
					std::uint8_t,			//	[6] window size
					std::uint8_t,			//	[7] status
					std::uint32_t			//	[8] target count
				>(frame);

				//auto res = cyng::value_cast<response_type>(frame.at(2), tp_res_open_push_channel_policy::UNREACHABLE);

				bus_->vm_.async_run(node::client_res_open_push_channel(cyng::value_cast(frame.at(0), boost::uuids::nil_uuid())
					, 0u //	sequence
					, tp_res_open_push_channel_policy::is_success(std::get<2>(tpl))
					, std::get<3>(tpl)	//	channel
					, std::get<4>(tpl)	//	source
					, std::get<8>(tpl)	//	count
					, options
					, bag));
			}
			else
			{
				CYNG_LOG_ERROR(logger_, "ipt.res.open.push.channel - no master " << cyng::io::to_str(frame));
			}

			//
			//	update watchdog timer
			//
			state_.react(state::evt_activity());
		}

		void session::ipt_req_close_push_channel(cyng::context& ctx)
		{
			//	[b1b6a46c-bb14-4722-bc3e-3cf8d6e74c00,bf,d268409a]
			//
			//	* session tag
			//	* ipt sequence
			//	* channel 
			const cyng::vector_t frame = ctx.get_frame();
			//CYNG_LOG_INFO(logger_, "ipt.req.close.push.channel " << cyng::io::to_str(frame));

			if (bus_->is_online())
			{
				ctx.run(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame));

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
				ctx.queue(cyng::generate_invoke("res.close.push.channel"
					, frame.at(1)
					, static_cast<response_type>(tp_res_close_push_channel_policy::BROKEN)
					, frame.at(2)));
				ctx.queue(cyng::generate_invoke("stream.flush"));
			}

			//
			//	update watchdog timer
			//
			state_.react(state::evt_activity());
		}

		void session::ipt_req_transmit_data(cyng::context& ctx)
		{
			//	[ipt.req.transmit.data,
			//	[c5eae235-d5f5-413f-a008-5d317d8baab7,1B1B1B1B01010101768106313830313...1B1B1B1B1A034276]]
			const cyng::vector_t frame = ctx.get_frame();

			const boost::uuids::uuid tag = cyng::value_cast(frame.at(0), boost::uuids::nil_uuid());
			BOOST_ASSERT(tag == ctx.tag());

			//
			//	data destination depends on connection state
			//
			state_.react(state::evt_data(frame.at(1)));
		}

		void session::client_req_transmit_data_forward(cyng::context& ctx)
		{
			//	[client.req.transmit.data.forward,
			//	[95a4ccf9-1171-4ff0-ad64-d06cb74da24e,8,
			//	%(("tp-layer":ipt)),
			//	1B1B1B1B01010101768106313830313330323133...0000001B1B1B1B1A0353AD]]
			//
			//	* peer
			//	* cluster sequence
			//	* bag
			//	* data
			//
			const cyng::vector_t frame = ctx.get_frame();
			ctx.run(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame));

#ifdef SMF_IO_LOG
		
			std::stringstream ss;
			ss
				<< "ipt-"
				<< boost::uuids::to_string(ctx.tag())
				;
			boost::filesystem::path const sub_dir(ss.str());
			boost::filesystem::path const dir_out(boost::filesystem::temp_directory_path() / sub_dir);

			if (!boost::filesystem::is_directory(dir_out)) {

				//
				//	create directory if not exist
				//
				boost::system::error_code ec;
				boost::filesystem::create_directory(dir_out, ec);
			}

			ss.str("");
			ss
				<< "sx-"
				<< std::setw(4)
				<< std::setfill('0')
				<< std::dec
				<< ++log_counter_
				<< ".log"
				;

			boost::filesystem::path const file_name(ss.str());

			std::string const path = (dir_out / file_name).string();
			std::ofstream of(path, std::ios::out | std::ios::app);
			if (of.is_open())
			{
				cyng::io::hex_dump hd;
				auto ptr = cyng::object_cast<cyng::buffer_t>(frame.at(3));
				hd(of, ptr->begin(), ptr->end());

				CYNG_LOG_TRACE(logger_, "write debug log " << file_name);
				of.close();
			}
#endif
			ctx.queue(cyng::generate_invoke("ipt.transfer.data", frame.at(3)));
			ctx.queue(cyng::generate_invoke("stream.flush"));

		}

		void session::ipt_res_watchdog(cyng::context& ctx)
		{
			//BOOST_ASSERT_MSG(bus_->is_online(), "no master");

			//	[]
			const cyng::vector_t frame = ctx.get_frame();
			ctx.run(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame));

			//
			//	update watchdog timer
			//
			state_.react(state::evt_activity());
		}

		void session::ipt_req_watchdog(cyng::context& ctx)
		{
			//	[0696ccad-af35-4e13-a4b8-e2f0f273e9e5,3]
			const cyng::vector_t frame = ctx.get_frame();
			ctx.queue(cyng::generate_invoke("log.msg.info", ctx.get_name(), " - ", frame));
			ctx.queue(cyng::generate_invoke("res.watchdog", frame.at(1)));
			ctx.queue(cyng::generate_invoke("stream.flush"));	

			//
			//	update watchdog timer
			//
			state_.react(state::evt_activity());
		}

		void session::ipt_req_protocol_version(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			ctx.queue(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame));
			ctx.queue(cyng::generate_invoke("res.protocol.version", frame.at(1), static_cast<std::uint8_t>(1)));
			ctx.queue(cyng::generate_invoke("stream.flush"));

			//
			//	update watchdog timer
			//
			state_.react(state::evt_activity());
		}

		void session::ipt_res_protocol_version(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			ctx.queue(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame));

			//
			//	update watchdog timer
			//
			state_.react(state::evt_activity());
		}

		void session::ipt_req_software_version(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			ctx.queue(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame));
			ctx.queue(cyng::generate_invoke("res.software.version", frame.at(1), NODE_VERSION));
			ctx.queue(cyng::generate_invoke("stream.flush"));

			//
			//	update watchdog timer
			//
			state_.react(state::evt_activity());
		}

		void session::ipt_res_software_version(cyng::context& ctx)
		{
			//	 [b22572a5-1233-43da-967b-aa6b16be002e,2,0.2.2018030]
			const cyng::vector_t frame = ctx.get_frame();
			if (bus_->is_online())
			{
				ctx.run(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame));

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
				ctx.queue(cyng::generate_invoke("log.msg.error", "ipt.res.software.version - no master", frame));
			}

			//
			//	update watchdog timer
			//
			state_.react(state::evt_activity());
		}

		void session::ipt_res_dev_id(cyng::context& ctx)
		{
			//	[65134d13-2d67-4208-bfe3-de8e2bd093d2,3,ipt:store]
			const cyng::vector_t frame = ctx.get_frame();
			//
			//	device ID
			//
			auto id = cyng::value_cast<std::string>(frame.at(2), "");
			if (bus_->is_online())
			{
				ctx.run(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame));

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
				ctx.queue(cyng::generate_invoke("log.msg.error", "ipt.res.device.id - no master ", id));
			}

			//
			//	update watchdog timer
			//
			state_.react(state::evt_activity());
		}

		void session::ipt_req_device_id(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			ctx.queue(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame));
			ctx.queue(cyng::generate_invoke("res.device.id", frame.at(1), "ipt:master"));
			ctx.queue(cyng::generate_invoke("stream.flush"));

			//
			//	update watchdog timer
			//
			state_.react(state::evt_activity());
		}

		void session::ipt_req_net_stat(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			ctx.queue(cyng::generate_invoke("log.msg.debug", "ipt.req.net.stat", frame));
			ctx.queue(cyng::generate_invoke("res.unknown.command", frame.at(1), static_cast<std::uint16_t>(code::APP_REQ_NETWORK_STATUS)));
			ctx.queue(cyng::generate_invoke("stream.flush"));

			//
			//	update watchdog timer
			//
			state_.react(state::evt_activity());
		}

		void session::ipt_res_network_stat(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			ctx.queue(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame));

			//
			//	update watchdog timer
			//
			state_.react(state::evt_activity());
		}

		void session::ipt_req_ip_statistics(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			ctx.queue(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame));
			ctx.queue(cyng::generate_invoke("res.unknown.command", frame.at(1), static_cast<std::uint16_t>(code::APP_REQ_IP_STATISTICS)));
			ctx.queue(cyng::generate_invoke("stream.flush"));

			//
			//	update watchdog timer
			//
			state_.react(state::evt_activity());
		}

		void session::ipt_res_ip_statistics(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			ctx.queue(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame));

			//
			//	update watchdog timer
			//
			state_.react(state::evt_activity());
		}

		void session::ipt_req_dev_auth(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			ctx.queue(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame));
			ctx.queue(cyng::generate_invoke("res.unknown.command", frame.at(1), static_cast<std::uint16_t>(code::APP_REQ_DEVICE_AUTHENTIFICATION)));
			ctx.queue(cyng::generate_invoke("stream.flush"));

			//
			//	update watchdog timer
			//
			state_.react(state::evt_activity());
		}

		void session::ipt_res_dev_auth(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			ctx.queue(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame));

			//
			//	update watchdog timer
			//
			state_.react(state::evt_activity());
		}

		void session::ipt_req_dev_time(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			ctx.queue(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame));
			ctx.queue(cyng::generate_invoke("res.device.time", frame.at(1)));
			ctx.queue(cyng::generate_invoke("stream.flush"));

			//
			//	update watchdog timer
			//
			state_.react(state::evt_activity());
		}

		void session::ipt_res_dev_time(cyng::context& ctx)
		{
			//	[420fa09c-e9a7-4092-bbdb-368b02f18e84,6,00000000]
			const cyng::vector_t frame = ctx.get_frame();
			if (bus_->is_online())
			{
				ctx.run(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame));

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
				ctx.queue(cyng::generate_invoke("log.msg.error", "ipt.res.device.time - no master", frame));
			}

			//
			//	update watchdog timer
			//
			state_.react(state::evt_activity());
		}

		void session::ipt_unknown_cmd(cyng::context& ctx)
		{
			//	 [9c08c93e-b2fe-4738-9fb4-4b16ed57a06e,6,00000000]
			const cyng::vector_t frame = ctx.get_frame();
			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,		//	[0] tag
				sequence_type,			//	[1] ipt seq
				std::uint16_t			//	[2] command
			>(frame);

			ctx.queue(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame));

			//
			//	update watchdog timer
			//
			state_.react(state::evt_activity());
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
			ctx.queue(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame));

			auto const tpl = cyng::tuple_cast<
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
			auto dom_options = cyng::make_reader(std::get<6>(tpl));
			const response_type res = cyng::value_cast<response_type>(dom_options.get("response-code"), 0);
			const std::uint16_t p_size = cyng::value_cast<std::uint16_t>(dom_options.get("packet-size"), 0);
			const std::uint8_t w_size = cyng::value_cast<std::uint8_t>(dom_options.get("window-size"), 0);
			const std::uint8_t status = cyng::value_cast<std::uint8_t>(dom_options.get("channel-status"), 0);

			//
			//	dom/bag reader
			//
			auto dom_bag = cyng::make_reader(std::get<7>(tpl));
			const sequence_type seq = cyng::value_cast<sequence_type>(dom_bag.get("seq"), 0);

			const std::uint32_t channel = std::get<3>(tpl);
			const std::uint32_t source = std::get<4>(tpl);
			const std::uint32_t count = std::get<5>(tpl);

			ctx.queue(cyng::generate_invoke("res.open.push.channel", seq, res, channel, source, p_size, w_size, status, count));
			ctx.queue(cyng::generate_invoke("stream.flush"));

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
			ctx.queue(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame));

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
			auto dom_bag = cyng::make_reader(std::get<4>(tpl));
			const sequence_type seq = cyng::value_cast<sequence_type>(dom_bag.get("seq"), 0);

			const response_type res = (std::get<2>(tpl))
				? tp_res_close_push_channel_policy::SUCCESS
				: tp_res_close_push_channel_policy::BROKEN
				;

			ctx	.queue(cyng::generate_invoke("res.close.push.channel", seq, res, std::get<3>(tpl)))
				.queue(cyng::generate_invoke("stream.flush"));

		}

		void session::ipt_req_register_push_target(cyng::context& ctx)
		{
			//	[3fb2afc3-7fef-4c5c-b272-234fe1de45fa,2,power@solostec]
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
				CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

				auto bag = cyng::param_map_factory("tp-layer", "ipt")
					("seq", frame.at(1))
					("pSize", frame.at(3))
					("wSize", frame.at(4))
					.operator cyng::param_map_t();

				bus_->vm_.async_run(client_req_register_push_target(cyng::value_cast(frame.at(0), boost::uuids::nil_uuid())
					, cyng::value_cast<std::string>(frame.at(2), "")	//	target name
					, bag));
			}
			else
			{
				CYNG_LOG_ERROR(logger_, ctx.get_name() << " - no master " << cyng::io::to_str(frame));
				ctx	.queue(cyng::generate_invoke("res.register.push.target"
					, frame.at(1)
					, static_cast<response_type>(ctrl_res_register_target_policy::GENERAL_ERROR)
					, static_cast<std::uint32_t>(0)))
					.queue(cyng::generate_invoke("stream.flush"));

			}

			//
			//	update watchdog timer
			//
			state_.react(state::evt_activity());
		}

		void session::ipt_req_deregister_push_target(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			if (bus_->is_online())
			{
				CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

				auto bag = cyng::param_map_factory("tp-layer", "ipt")
					("seq", frame.at(1))
					.operator cyng::param_map_t();

				bus_->vm_.async_run(client_req_deregister_push_target(cyng::value_cast(frame.at(0), boost::uuids::nil_uuid())
					, cyng::value_cast<std::string>(frame.at(2), "")	//	target name
					, bag));
			}
			else
			{
				CYNG_LOG_ERROR(logger_, ctx.get_name() << " - no master " << cyng::io::to_str(frame));
				ctx	.queue(cyng::generate_invoke("res.deregister.push.target"
					, frame.at(1)		//	seq
					, static_cast<response_type>(ctrl_res_deregister_target_policy::GENERAL_ERROR)
					, frame.at(2)))	//	target name
					.queue(cyng::generate_invoke("stream.flush"));
			}

			//
			//	update watchdog timer
			//
			state_.react(state::evt_activity());
		}

		void session::client_res_deregister_push_target(cyng::context& ctx)
		{
			//	[255eaa0f-c0d6-4c6e-a1c4-576592ca371c,5,true,power@solostec,%(("response-code":1)),%(("seq":2),("tp-layer":ipt))]]
			//
			//	* remote peer
			//	* cluster bus sequence
			//	* success
			//	* target name
			//	* options
			//	* bag
			//	
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

			//
			//	dom reader
			//
			auto dom = cyng::make_reader(frame);

			const sequence_type seq = cyng::value_cast<sequence_type>(dom[5].get("seq"), 0);
			const response_type res = cyng::value_cast<response_type>(dom[4].get("response-code"), 0);
			const std::string target = cyng::value_cast<std::string>(dom.get(3), "");

			ctx	.queue(cyng::generate_invoke("res.deregister.push.target", seq, res, target))
				.queue(cyng::generate_invoke("stream.flush"));

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
			CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

			//
			//	dom reader
			//
			auto dom = cyng::make_reader(frame);

			const sequence_type seq = cyng::value_cast<sequence_type>(dom[5].get("seq"), 0);
			const response_type res = cyng::value_cast<response_type>(dom[4].get("response-code"), 0);
			const std::uint32_t channel = cyng::value_cast<std::uint32_t>(dom.get(3), 0);	

			ctx	.queue(cyng::generate_invoke("res.register.push.target", seq, res, channel))
				.queue(cyng::generate_invoke("stream.flush"));

		}

		void session::ipt_req_open_connection(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			if (bus_->is_online())
			{
				CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

				bus_->vm_.async_run(client_req_open_connection(cyng::value_cast(frame.at(0), boost::uuids::nil_uuid())
					, cyng::value_cast<std::string>(frame.at(2), "")	//	number
					, cyng::param_map_factory("tp-layer", "ipt")("origin-tag", ctx.tag())("seq", frame.at(1))("start", std::chrono::system_clock::now())));
			}
            else
			{
				ctx	.queue(cyng::generate_invoke("log.msg.error", ctx.get_name(), frame))
					.queue(cyng::generate_invoke("res.open.connection", frame.at(1), static_cast<response_type>(tp_res_open_connection_policy::NO_MASTER)))
					.queue(cyng::generate_invoke("stream.flush"));
			}

			//
			//	update watchdog timer
			//
			state_.react(state::evt_activity());
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
			//	* master
			//		- caller device name
			//		- origin session tag
			//		- local/remote connection
			//		- ...
			//	* client
			//		- start
			//

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,		//	[0] peer
				std::uint64_t,			//	[1] cluster sequence
				std::string,			//	[2] number to call
				cyng::param_map_t,		//	[3] master
				cyng::param_map_t		//	[4] client
			>(ctx.get_frame());

			//
			//	dom reader (options)
			//
			auto const dom = cyng::make_reader(std::get<3>(tpl));
			auto const local_connect = cyng::value_cast(dom.get("local-connect"), false);

			//
			//	Use state machine to respond to ip-t response.
			//	task only detect timeout and closes session
			//

			ctx.queue(state_.react(state::evt_client_req_open_connection(cyng::async::start_task_sync<open_connection>(mux_
					, logger_
					, vm_
					, std::get<2>(tpl)	//	number
					, timeout_)
				, local_connect
				, std::get<1>(tpl)	//	cluster bus sequence
				, std::get<3>(tpl)	//	master
				, std::get<4>(tpl)	//	client
			)));
		}

		void session::client_req_close_connection_forward(cyng::context& ctx)
		{
			//
			//
			auto const frame = ctx.get_frame();
			CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

			//	[	86e6f4a5-fb44-49cd-ac25-96677e9f2e8a,8,c439ade5-f75f-4a28-a69b-aa9e5827a1f9,false,
			//		%(("local-connect":true),("local-peer":9462fd76-02e8-4494-ac3c-ee96e0011604)),
			//		%(("origin-tag":c439ade5-f75f-4a28-a69b-aa9e5827a1f9),("seq":1),("start":2018-09-24 12:41:32.61626400),("tp-layer":ipt))
			//	]
			//
			//	* [uuid] peer (cluster bus)
			//	* [u64] cluster bus sequence
			//	* [uuid] origin/remote tag
			//	* [bool] shutdown mode
			//	* [pmap] master
			//	* [pmap] client

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,		//	[0] peer
				std::uint64_t,			//	[1] cluster sequence
				boost::uuids::uuid,		//	[2] origin-tag - compare to "origin-tag"
				bool,					//	[3] shutdown flag
				cyng::param_map_t,		//	[4] master
				cyng::param_map_t		//	[5] client
			>(frame);

			//
			//	in shutdown mode no response should be sent.
			//
			//const bool shutdown = std::get<3>(tpl);

#ifdef _DEBUG
			//
			//	dom reader
			//
			auto dom = cyng::make_reader(std::get<5>(tpl));
			auto origin_tag = cyng::value_cast(dom.get("origin-tag"), boost::uuids::nil_uuid());
			BOOST_ASSERT_MSG(std::get<2>(tpl) == origin_tag, "inconsistent data - client.req.close.connection.forward");
			BOOST_ASSERT_MSG(ctx.tag() != origin_tag, "sender and receiver can't be the same");
#endif

			state_.react(state::evt_client_req_close_connection(cyng::async::start_task_sync<close_connection>(mux_
					, logger_
					, vm_
					, timeout_)
				, std::get<1>(tpl)		//	cluster bus sequence
				, std::get<2>(tpl)		//	origin tag
				, std::get<3>(tpl)		//	shutdown flag
				, std::get<4>(tpl)		//	master
				, std::get<5>(tpl)));	//	client
		}

		void session::client_res_close_connection_forward(cyng::context& ctx)
		{
			//
			//	[d6aac617-2f91-47ab-87a3-287f584f55a4,24,true,
			//	%(("local-connect":false),("local-peer":22206aa4-0bc3-40c0-9ef4-17b3733a3aae),("origin-tag":5c2fa815-82bc-46dd-a3b9-8b07124c4994)),
			//	%()]
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
			//	[2fcae30a-04c0-4881-9e15-d090cef7bd6b
			//	,1
			//	,true
			//	,%(("device-name":receiver-0000),("local-connect":true),("local-peer":21ee7236-7273-45ba-8bd0-e37c74f2afd7),("origin-tag":c5be645d-f604-40dd-a953-42cd40b590b1),("remote-peer":21ee7236-7273-45ba-8bd0-e37c74f2afd7),("remote-tag":2de96e3f-1c7c-4481-ab27-872e79d4e551))
			//	,%(("origin-tag":c5be645d-f604-40dd-a953-42cd40b590b1),("seq":1),("start":2018-09-24 17:09:41.30037200),("tp-layer":ipt))]
			//
			//	* peer
			//	* cluster sequence
			//	* success
			//	* options
			//	* bag - original data

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
			auto const frame = ctx.get_frame();

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
			auto dom = cyng::make_reader(std::get<5>(tpl));
			const sequence_type seq = cyng::value_cast<sequence_type>(dom.get("seq"), 0);

			//
			//	set acknownlegde flag
			//
			std::uint8_t status = cyng::value_cast<std::uint8_t>(dom.get("status"), 0);
			//BOOST_ASSERT_MSG(status == 0xc1, "invalid push channel status");
			if (status != 0xc1)
			{
				ctx.queue(cyng::generate_invoke("log.msg.warning"
					, ctx.get_name()
					, " - status: 0x"
					, status));

			}
			status |= tp_res_pushdata_transfer_policy::ACK;

			//
			//	response
			//
			response_type const res = (std::get<4>(tpl) != 0)
				? tp_res_pushdata_transfer_policy::SUCCESS
				: tp_res_pushdata_transfer_policy::BROKEN
				;

			if (std::get<4>(tpl) != 0)
			{
				CYNG_LOG_DEBUG(logger_, ctx.get_name() 
					<< " - source: "
					<< std::get<2>(tpl)
					<< ", channel: "
					<< std::get<3>(tpl)
					<< ", count: "
					<< std::get<4>(tpl)
					<< " OK")
			}
			else
			{
				CYNG_LOG_WARNING(logger_, ctx.get_name() 
					<< " - failed: " 
					<< cyng::io::to_str(frame));
			}

			ctx	.queue(cyng::generate_invoke("res.transfer.push.data"
				, seq
				, res
				, std::get<2>(tpl)	//	source
				, std::get<3>(tpl)	//	channel
				, status
				, cyng::value_cast<std::uint8_t>(dom.get("block"), 0)
			))
				.queue(cyng::generate_invoke("stream.flush"));
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

			ctx.queue(cyng::generate_invoke("log.msg.debug", ctx.get_name()
				, " - channel "
				, std::get<2>(tpl)
				, ", source "
				, std::get<3>(tpl)
				, ", "
				, std::get<5>(tpl).size()
				, " bytes"));

			//
			//	dom reader
			//
			auto dom = cyng::make_reader(std::get<5>(tpl));
			auto status = cyng::value_cast<std::uint8_t>(dom.get("status"), 0);
			auto block = cyng::value_cast<std::uint8_t>(dom.get("block"), 0);

			ctx	.queue(cyng::generate_invoke("req.transfer.push.data"
				, std::get<4>(tpl)	//	channel
				, std::get<3>(tpl)	//	source
				, status 
				, block 
				, std::get<6>(tpl)))	//	data
			
				.queue(cyng::generate_invoke("stream.flush"));
		}

		void session::ipt_req_close_connection(cyng::context& ctx)
		{
			//	[c439ade5-f75f-4a28-a69b-aa9e5827a1f9,1]
			//
			//	* [uuid] session tag
			//	* [u8] seq
			//
			ctx.queue(state_.react(state::evt_ipt_req_close_connection(cyng::tuple_cast<
				boost::uuids::uuid,		//	[0] session tag
				sequence_type			//	[1] seq
			>(ctx.get_frame()))));
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

			ctx.queue(state_.react(state::evt_client_res_open_connection(cyng::tuple_cast<
				boost::uuids::uuid,		//	[0] peer tag
				std::uint64_t,			//	[1] cluster seq
				bool,					//	[2] success
				cyng::param_map_t,		//	[3] master
				cyng::param_map_t		//	[4] client
			>(ctx.get_frame()))));

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
			//ctx.queue(cyng::generate_invoke("log.msg.info", "ipt.res.open.connection", frame));

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
				const auto tsk = pos->second.first;
				const auto slot = pos->second.second;
				CYNG_LOG_DEBUG(logger_, vm().tag() << " ipt.res.open.connection "
					<< +pos->first
					<< " => #"
					<< tsk
					<< ':'
					<< slot);

				//
				//	update state
				//
				state_.react(state::evt_ipt_res_open_connection(tsk, slot, res));

				//
				//	remove entry
				//
				task_db_.erase(pos);
			}
			else
			{
				//ctx.queue(cyng::generate_invoke("log.msg.error", "ipt.res.open.connection - empty sequence/task relation", seq));
				CYNG_LOG_ERROR(logger_, vm().tag() << " - " << ctx.get_name() << ": empty sequence/task relation" << +seq);

			}
		}

		void session::ipt_res_close_connection(cyng::context& ctx)
		{
			//	[3,1]
			//	
			//	* [u8] seq
			//	* [u8] response
			const cyng::vector_t frame = ctx.get_frame();
			ctx.queue(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame));

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
				CYNG_LOG_INFO(logger_, "stop task "
					<< +pos->first
					<< " => #"
					<< pos->second.first
					<< ':'
					<< pos->second.second);

				const auto tsk = pos->second.first;
				const auto slot = pos->second.second;

				//
				//	update state
				//
				CYNG_LOG_TRACE(logger_, "stop close connection task #" << tsk);
				state_.react(state::evt_ipt_res_close_connection(tsk, slot, res));

				//mux_.post(tsk, slot, cyng::tuple_factory(std::get<2>(tpl)));

				//
				//	remove entry
				//
				task_db_.erase(pos);
			}
			else
			{
				ctx.queue(cyng::generate_invoke("log.msg.error", "empty sequence/task relation ", +seq));
			}


			//
			//	If device doesn't send a response the <close_connection> task will close this session.
			//
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
				//
				//	ToDo: log less data
				//
				//ctx.run(cyng::generate_invoke("log.msg.info", "ipt.req.transfer.pushdata", frame));

				auto bag = cyng::param_map_factory("tp-layer", "ipt")
					("seq", frame.at(1))
					("status", frame.at(4))
					("block", frame.at(5))
					.operator cyng::param_map_t();

				bus_->vm_.async_run(client_req_transfer_pushdata(cyng::value_cast(frame.at(0), boost::uuids::nil_uuid())
					, cyng::value_cast<std::uint32_t>(frame.at(2), 0)
					, cyng::value_cast<std::uint32_t>(frame.at(3), 0)
					, frame.at(6)
					, bag));
			}
			else
			{
				ctx.queue(cyng::generate_invoke("log.msg.error", "ipt.req.transfer.pushdata - offline", frame));
				ctx.queue(cyng::generate_invoke("res.transfer.push.data"
					, frame.at(1)	//	seq
					, static_cast<response_type>(tp_res_pushdata_transfer_policy::UNREACHABLE)
					, frame.at(2)	//	channel
					, frame.at(3)	//	source
					, frame.at(4)	//	status
					, frame.at(5)	//	block
				));
				ctx.queue(cyng::generate_invoke("stream.flush"));
			}

			//
			//	update watchdog timer
			//
			state_.react(state::evt_activity());
		}

		void session::ipt_res_transfer_pushdata(cyng::context& ctx)
		{
			//	* session tag
			//	* ipt sequence
			//	* channel
			//	* source
			//	* status
			//	* block

			const cyng::vector_t frame = ctx.get_frame();

			if (bus_->is_online())
			{
				ctx.run(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame));

				auto bag = cyng::param_map_factory("tp-layer", "ipt")
					("seq", frame.at(1))
					("status", frame.at(4))
					("block", frame.at(5))
					.operator cyng::param_map_t();

				//
				//	master generates already response 
				//

				//bus_->vm_.async_run(node::client_res_transfer_pushdata(cyng::value_cast(frame.at(0), boost::uuids::nil_uuid())
				//	, cyng::value_cast<std::uint32_t>(frame.at(2), 0)
				//	, cyng::value_cast<std::uint32_t>(frame.at(3), 0)
				//	, frame.at(6)
				//	, bag));
			}
			else
			{
				ctx.queue(cyng::generate_invoke("log.msg.warning - offline", "ipt.res.transfer.pushdata", frame));
				ctx.queue(cyng::generate_invoke("stream.flush"));
			}

			//
			//	update watchdog timer
			//
			state_.react(state::evt_activity());
		}

		cyng::object make_session(boost::asio::ip::tcp::socket&& socket
			, cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, bus::shared_type bus
			, boost::uuids::uuid tag
			, std::chrono::seconds const& timeout
			, scramble_key const& sk
			, std::uint16_t watchdog
			, bool sml_log)
		{
			return cyng::make_object<session>(std::move(socket)
				, mux
				, logger
				, bus
				, tag
				, timeout
				, sk
				, watchdog
				, sml_log);
		}

	}
}


namespace cyng
{
	namespace traits
	{

#if !defined(__CPP_SUPPORT_N2235)
		const char type_tag<node::ipt::session>::name[] = "ipt::session";
#endif
	}	// traits	
}



