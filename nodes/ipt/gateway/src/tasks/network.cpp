/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "network.h"
#include <smf/ipt/response.hpp>
#include <smf/ipt/generator.h>
#include <smf/sml/protocol/value.hpp>
#include <smf/sml/protocol/message.h>
#include <smf/sml/protocol/serializer.h>
#include <smf/sml/srv_id_io.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/crc16.h>
#include <cyng/factory/set_factory.h>
#include <cyng/async/task/task_builder.hpp>
#include <cyng/io/serializer.h>
#include <cyng/vm/generator.h>
#include <cyng/io/serializer.h>
#include <cyng/tuple_cast.hpp>
#include <cyng/sys/memory.h>

#include <boost/uuid/random_generator.hpp>
#ifdef SMF_IO_DEBUG
#include <cyng/io/hex_dump.hpp>
#endif

namespace node
{
	namespace ipt
	{

		network::network(cyng::async::base_task* btp
			, cyng::logging::log_ptr logger
			, master_config_t const& cfg, std::string manufacturer
			, std::string model
			, cyng::mac48 mac)
		: base_(*btp)
			, bus_(bus_factory(btp->mux_, logger, boost::uuids::random_generator()(), scramble_key::default_scramble_key_, btp->get_id(), "ipt:gateway"))
			, logger_(logger)
			, config_(cfg)
			, manufacturer_(manufacturer)
			, model_(model)
			, server_id_(node::sml::to_gateway_srv_id(mac))
			, master_(0)
			, parser_([this](cyng::vector_t&& prg) {
				CYNG_LOG_INFO(logger_, prg.size() << " instructions received");
				CYNG_LOG_TRACE(logger_, cyng::io::to_str(prg));
				bus_->vm_.async_run(std::move(prg));
				//vm_.async_run(std::move(prg));
			}, false)
			, reader_()
			, sml_msg_()
			, group_no_(0)
			, seq_target_map_()
			, channel_target_map_()
		{
			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> is running");

			//
			//	request handler
			//
			bus_->vm_.register_function("network.task.resume", 4, std::bind(&network::task_resume, this, std::placeholders::_1));
			bus_->vm_.register_function("bus.reconfigure", 1, std::bind(&network::reconfigure, this, std::placeholders::_1));
			bus_->vm_.register_function("net.insert.rel", 1, std::bind(&network::insert_rel, this, std::placeholders::_1));

			//
			//	SML transport
			//
			bus_->vm_.register_function("sml.msg", 2, std::bind(&network::sml_msg, this, std::placeholders::_1));
			bus_->vm_.register_function("sml.eom", 2, std::bind(&network::sml_eom, this, std::placeholders::_1));

			//
			//	SML data
			//
			bus_->vm_.register_function("sml.public.open.request", 8, std::bind(&network::sml_public_open_request, this, std::placeholders::_1));
			bus_->vm_.register_function("sml.public.close.request", 3, std::bind(&network::sml_public_close_request, this, std::placeholders::_1));
			bus_->vm_.register_function("sml.get.proc.parameter.request", 8, std::bind(&network::sml_get_proc_parameter_request, this, std::placeholders::_1));
			bus_->vm_.register_function("sml.get.proc.status.word", 6, std::bind(&network::sml_get_proc_status_word, this, std::placeholders::_1));
			bus_->vm_.register_function("sml.get.proc.device.id", 6, std::bind(&network::sml_get_proc_device_id, this, std::placeholders::_1));
			bus_->vm_.register_function("sml.get.proc.device.time", 6, std::bind(&network::sml_get_proc_device_time, this, std::placeholders::_1));
			bus_->vm_.register_function("sml.get.proc.ntp.config", 6, std::bind(&network::sml_get_proc_ntp_config, this, std::placeholders::_1));
			//"sml.get.proc.access.rights"
			//"sml.get.proc.custom.interface"
			//"sml.get.proc.custom.param"
			//"sml.get.proc.wan.config"
			//"sml.get.proc.gsm.config"
			bus_->vm_.register_function("sml.get.proc.ipt.state", 6, std::bind(&network::sml_get_proc_ipt_state, this, std::placeholders::_1));
			bus_->vm_.register_function("sml.get.proc.ipt.param", 6, std::bind(&network::sml_get_proc_ipt_param, this, std::placeholders::_1));
			//"sml.get.proc.gprs.param"
			bus_->vm_.register_function("sml.get.proc.lan.config", 6, std::bind(&network::sml_get_proc_lan_config, this, std::placeholders::_1));
			bus_->vm_.register_function("sml.get.proc.lan.if", 6, std::bind(&network::sml_get_proc_lan_if, this, std::placeholders::_1));
			bus_->vm_.register_function("sml.get.proc.mem.usage", 6, std::bind(&network::sml_get_proc_mem_usage, this, std::placeholders::_1));
			bus_->vm_.register_function("sml.get.proc.active.devices", 6, std::bind(&network::sml_get_proc_active_devices, this, std::placeholders::_1));
			bus_->vm_.register_function("sml.get.proc.visible.devices", 6, std::bind(&network::sml_get_proc_visible_devices, this, std::placeholders::_1));
			bus_->vm_.register_function("sml.get.proc.device.info", 6, std::bind(&network::sml_get_proc_device_info, this, std::placeholders::_1));
		}

		void network::run()
		{
			if (bus_->is_online())
			{
				//
				//	deregister target
				//
				//bus_->vm_.async_run(cyng::generate_invoke("req.deregister.push.target", "DEMO"));

				//
				//	send watchdog response - without request
				//
				bus_->vm_.async_run(cyng::generate_invoke("res.watchdog", static_cast<std::uint8_t>(0)));
				bus_->vm_.async_run(cyng::generate_invoke("stream.flush"));
			}
			else
			{
				//
				//	reset parser and serializer
				//
				bus_->vm_.async_run(cyng::generate_invoke("ipt.reset.parser", config_[master_].sk_));
				bus_->vm_.async_run(cyng::generate_invoke("ipt.reset.serializer", config_[master_].sk_));

				//
				//	login request
				//
				if (config_[master_].scrambled_)
				{
					bus_->vm_.async_run(gen::ipt_req_login_scrambled(config_, master_));
				}
				else
				{
					bus_->vm_.async_run(gen::ipt_req_login_public(config_, master_));
				}
			}
		}

		void network::stop()
		{
			bus_->stop();
			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> is stopped");
		}

		//	slot 0
		cyng::continuation network::process(std::uint16_t watchdog, std::string redirect)
		{
			if (watchdog != 0)
			{
				CYNG_LOG_INFO(logger_, "start watchdog: " << watchdog << " minutes");
				//base_.suspend(std::chrono::minutes(watchdog));
				base_.suspend(std::chrono::seconds(watchdog));
			}

			//
			//	register targets
			//
			//register_targets();

			return cyng::continuation::TASK_CONTINUE;
		}

		cyng::continuation network::process()
		{
			//
			//	switch to other configuration
			//
			reconfigure_impl();

			//
			//	continue task
			//
			return cyng::continuation::TASK_CONTINUE;
		}

		//	slot [2]
		cyng::continuation network::process(sequence_type seq, std::string const& number)
		{
			CYNG_LOG_TRACE(logger_, "incoming call " << +seq << ':' << number);

			BOOST_ASSERT(bus_->is_online());

			//
			//	accept incoming calls
			//
			bus_->vm_.async_run(cyng::generate_invoke("res.open.connection", seq, static_cast<response_type>(ipt::tp_res_open_connection_policy::DIALUP_SUCCESS)));
			bus_->vm_.async_run(cyng::generate_invoke("stream.flush"));

			//
			//	continue task
			//
			return cyng::continuation::TASK_CONTINUE;
		}

		cyng::continuation network::process(sequence_type seq, std::uint32_t channel, std::uint32_t source, cyng::buffer_t const& data)
		{
			//
			//	distribute to output tasks
			//


			//
			//	continue task
			//
			return cyng::continuation::TASK_CONTINUE;
		}

		//	slot [4] - register target response
		cyng::continuation network::process(sequence_type seq, bool success, std::uint32_t channel)
		{
			if (success)
			{
				auto pos = seq_target_map_.find(seq);
				if (pos != seq_target_map_.end())
				{
					CYNG_LOG_INFO(logger_, "channel "
						<< channel
						<< " ==> "
						<< pos->second);

					channel_target_map_.emplace(channel, pos->second);
					seq_target_map_.erase(pos);

					//cyng::vector_t prg;
					//prg
					//	<< cyng::generate_invoke_unwinded("req.deregister.push.target", "DEMO")
					//	//<< cyng::generate_invoke_unwinded("net.remove.rel", cyng::invoke("ipt.push.seq"), "DEMO")
					//	<< cyng::generate_invoke_unwinded("stream.flush")
					//	;

					//bus_->vm_.async_run(std::move(prg));

				}
			}

			//
			//	continue task
			//
			return cyng::continuation::TASK_CONTINUE;
		}

		//	slot [5]
		cyng::continuation network::process(cyng::buffer_t const& data)
		{
			CYNG_LOG_TRACE(logger_, "incoming SML data " << data.size() << " bytes");

			//
			//	parse incoming data
			//
			parser_.read(data.begin(), data.end());

			//
			//	continue task
			//
			return cyng::continuation::TASK_CONTINUE;
		}

		cyng::continuation network::process(sequence_type seq, bool success, std::string const& target)
		{
			CYNG_LOG_INFO(logger_, "target "
				<< target
				<< " deregistered");

			//
			//	continue task
			//
			return cyng::continuation::TASK_CONTINUE;
		}

		void network::task_resume(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			//	[1,0,TDevice,3]
			CYNG_LOG_TRACE(logger_, "resume task - " << cyng::io::to_str(frame));
			std::size_t tsk = cyng::value_cast<std::size_t>(frame.at(0), 0);
			std::size_t slot = cyng::value_cast<std::size_t>(frame.at(1), 0);
			base_.mux_.post(tsk, slot, cyng::tuple_t{ frame.at(2), frame.at(3) });
		}

		void network::reconfigure(cyng::context& ctx)
		{
			reconfigure_impl();
		}

		void network::reconfigure_impl()
		{
			//
			//	switch to other master
			//
			if (config_.size() > 1)
			{
				master_++;
				if (master_ == config_.size())
				{
					master_ = 0;
				}
				CYNG_LOG_INFO(logger_, "switch to redundancy "
					<< config_[master_].host_
					<< ':'
					<< config_[master_].service_);

			}
			else
			{
				CYNG_LOG_ERROR(logger_, "network login failed - no other redundancy available");
			}

			//
			//	trigger reconnect 
			//
			CYNG_LOG_INFO(logger_, "reconnect to network in "
				<< config_[master_].monitor_.count()
				<< " seconds");
			base_.suspend(config_[master_].monitor_);

		}

		void network::insert_rel(cyng::context& ctx)
		{
			//	[5,power@solostec]
			//
			//	* ipt sequence
			//	* target name
			const cyng::vector_t frame = ctx.get_frame();

			auto const tpl = cyng::tuple_cast<
				sequence_type,		//	[0] ipt seq
				std::string			//	[1] target
			>(frame);

			CYNG_LOG_TRACE(logger_, "ipt sequence "
				<< +std::get<0>(tpl)
				<< " ==> "
				<< std::get<1>(tpl));

			seq_target_map_.emplace(std::get<0>(tpl), std::get<1>(tpl));
		}

		void network::register_targets()
		{
			seq_target_map_.clear();
			channel_target_map_.clear();
			CYNG_LOG_INFO(logger_, "register target DEMO" );
			cyng::vector_t prg;
			prg
				<< cyng::generate_invoke_unwinded("req.register.push.target", "DEMO", static_cast<std::uint16_t>(0xffff), static_cast<std::uint8_t>(1))
				<< cyng::generate_invoke_unwinded("net.insert.rel", cyng::invoke("ipt.push.seq"), "DEMO")
				<< cyng::generate_invoke_unwinded("stream.flush")
				;

			bus_->vm_.async_run(std::move(prg));

		}

		void network::sml_msg(cyng::context& ctx)
		{
			//	[{31383035323232323535353231383235322D31,0,0,{0100,{null,005056C00008,3230313830353232323235353532,0500153B02297E,6F70657261746F72,6F70657261746F72,null}},9c91},0]
			//	[{31383035323232323535353231383235322D32,1,0,{0500,{0500153B02297E,6F70657261746F72,6F70657261746F72,{810060050000},null}},f8b5},1]
			//	[{31383035323232323535353231383235322D33,2,0,{0500,{0500153B02297E,6F70657261746F72,6F70657261746F72,{8181C78201FF},null}},4d37},2]
			//	[{31383035323232323535353231383235322D34,0,0,{0200,{null}},e6e8},3]
			//
			const cyng::vector_t frame = ctx.get_frame();
			const std::size_t idx = cyng::value_cast<std::size_t>(frame.at(1), 0);
			//CYNG_LOG_TRACE(logger_, "sml.msg - " << cyng::io::to_str(frame));
			CYNG_LOG_INFO(logger_, "sml.msg #" << idx);

			//
			//	get message body
			//
			cyng::tuple_t msg;
			msg = cyng::value_cast(frame.at(0), msg);
			CYNG_LOG_INFO(logger_, "sml.msg " << cyng::io::to_str(frame));

			//
			//	add generated instruction vector
			//
			ctx.attach(reader_.read(msg, idx));
		}

		void network::sml_eom(cyng::context& ctx)
		{
			//[4aa5,4]
			//
			//	* CRC
			//	* message counter
			//
			const cyng::vector_t frame = ctx.get_frame();
			const std::size_t idx = cyng::value_cast<std::size_t>(frame.at(1), 0);
			//CYNG_LOG_TRACE(logger_, "sml.eom - " << cyng::io::to_str(frame));
			CYNG_LOG_INFO(logger_, "sml.eom #" << idx);

			//
			//	build SML message frame
			//
			cyng::buffer_t buf = node::sml::boxing(sml_msg_);

			//
			//	transfer data
			//
			ctx	.attach(cyng::generate_invoke("ipt.transfer.data", buf))
				.attach(cyng::generate_invoke("stream.flush"));

#ifdef SMF_IO_DEBUG
			cyng::io::hex_dump hd;
			std::stringstream ss;
			hd(ss, buf.begin(), buf.end());
			CYNG_LOG_TRACE(logger_, "response:\n" << ss.str());
#endif

			//
			//	reset reader
			//
			sml_msg_.clear();
			group_no_ = 0;
			reader_.reset();
		}

		void network::append_msg(cyng::tuple_t&& msg)
		{
#ifdef SMF_IO_DEBUG
			CYNG_LOG_TRACE(logger_, "msg: " << cyng::io::to_str(msg));
#endif

			//
			//	linearize and set CRC16
			//
			cyng::buffer_t b = node::sml::linearize(msg);
			node::sml::sml_set_crc16(b);

			//
			//	append to current SML message
			//
			sml_msg_.push_back(b);
		}

		void network::sml_public_open_request(cyng::context& ctx)
		{
			//	[34481794-6866-4776-8789-6f914b4e34e7,180301091943374505-1,0,005056c00008,00:15:3b:02:23:b3,20180301092332,operator,operator]
			//
			//	* pk
			//	* transaction id
			//	* SML message id
			//	* client ID - MAC from client
			//	* server ID - target server/gateway
			//	* request file id
			//	* username
			//	* password
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_TRACE(logger_, "sml.public.open.request " << cyng::io::to_str(frame));

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,	//	[0] pk
				std::string,		//	[1] trx
				std::size_t,		//	[2] SML message id
				std::string,		//	[3] client ID
				std::string,		//	[4] server ID
				std::string,		//	[5] request file id
				std::string,		//	[6] username
				std::string			//	[7] password
			>(frame);
			CYNG_LOG_INFO(logger_, "sml.public.open.request - trx: "
				<< std::get<1>(tpl)
				<< ", msg id: "
				<< std::get<2>(tpl)
				<< ", client id: "
				<< std::get<3>(tpl)
				<< ", server id: "
				<< std::get<4>(tpl)
				<< ", file id: "
				<< std::get<5>(tpl)
				<< ", user: "
				<< std::get<6>(tpl)
				<< ", pwd: "
				<< std::get<7>(tpl))
				;

			//
			//	linearize and set CRC16
			//	append to current SML message
			//
			BOOST_ASSERT_MSG(sml_msg_.empty(), "pending SML data");
			append_msg(node::sml::message(frame.at(1)
				, 0	//	group
				, 0 // abort code
				, node::sml::BODY_OPEN_RESPONSE

				//
				//	generate public open response
				//
				, node::sml::open_response(cyng::make_object()	// codepage
					, frame.at(3)	//	client id
					, frame.at(5)	//	req file id
					, frame.at(4)	//	server id
					, cyng::make_object()	//	 ref time
					, cyng::make_object())));

#ifdef SMF_IO_DEBUG
			//cyng::io::hex_dump hd;
			//std::stringstream ss;
			//hd(ss, b.begin(), b.end());
			//CYNG_LOG_TRACE(logger_, "response:\n" << ss.str());
#endif

		}

		void network::sml_public_close_request(cyng::context& ctx)
		{
			//	[5fb21ea1-ac66-4a00-98b7-88edd57addb7,180523152649286995-4,3]
			//
			//	* pk
			//	* transaction id
			//	* SML message id
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.public.close.request " << cyng::io::to_str(frame));

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,	//	[0] pk
				std::string,		//	[1] trx
				std::size_t			//	[2] msg id
			>(frame);

			//	sml.public.close.request - trx: 180523152649286995-4, msg id:
			CYNG_LOG_INFO(logger_, "sml.public.close.request - trx: "
				<< std::get<1>(tpl)
				<< ", msg id: "
				<< std::get<2>(tpl))
				;

			//
			//	linearize and set CRC16
			//	append to current SML message
			//
			append_msg(node::sml::message(frame.at(1)
				, 0	//	group
				, 0 //	abort code
				, node::sml::BODY_CLOSE_RESPONSE

				//
				//	generate public open response
				//
				, node::sml::close_response(cyng::make_object())));
		}

		void network::sml_get_proc_parameter_request(cyng::context& ctx)
		{
			//	[b5cfc8a0-0bf4-4afe-9598-ded99f71469c,180301094328243436-3,2,05:00:15:3b:02:23:b3,operator,operator,81 81 c7 82 01 ff,null]
			//	[50cfab74-eef0-4c92-89d4-46b28ab9da20,180522233943303816-2,1,00:15:3b:02:29:7e,operator,operator,81 00 60 05 00 00,null]
			//
			//	* pk
			//	* transaction id
			//	* SML message id
			//	* server ID
			//	* username
			//	* password
			//	* OBIS (requested parameter)
			//	* attribute (should be null)
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_WARNING(logger_, "sml.get.proc.parameter.request " << cyng::io::to_str(frame));

			//CODE_ROOT_DEVICE_IDENT

		}

		void network::sml_get_proc_status_word(cyng::context& ctx)
		{
			//	[fd167248-a823-46d6-ba0a-03157a28a104,1805241520113192-2,1,0500153B02297E,operator,operator]
			//
			//	* pk
			//	* transaction id
			//	* SML message id
			//	* server ID
			//	* username
			//	* password
			//	* OBIS (requested parameter)
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.get.proc.status.word " << cyng::io::to_str(frame));

			//	sets the following flags:
			//	0x010070202 - 100000000000001110000001000000010
			//	fataler Fehler              : false
			//	am Funknetz angemeldet      : false
			//	Endkundenschnittstelle      : true
			//	Betriebsbereischaft erreicht: true
			//	am IP-T Server angemeldet   : true
			//	Erweiterungsschnittstelle   : true
			//	DHCP Parameter erfasst      : true
			//	Out of Memory erkannt       : false
			//	Wireless M-Bus Schnittst.   : true
			//	Ethernet-Link vorhanden     : true
			//	Zeitbasis unsicher          : rot
			//	PLC Schnittstelle           : false

			//	0x000070202 - 000000000000001110000001000000010
			//	fataler Fehler              : false
			//	am Funknetz angemeldet      : false
			//	Endkundenschnittstelle      : true
			//	Betriebsbereischaft erreicht: true
			//	am IP-T Server angemeldet   : true
			//	Erweiterungsschnittstelle   : true
			//	DHCP Parameter erfasst      : true
			//	Out of Memory erkannt       : false
			//	Wireless M-Bus Schnittst.   : true
			//	Ethernet-Link vorhanden     : true
			//	Zeitbasis unsicher          : false
			//	PLC Schnittstelle           : false

			//	see also http://www.sagemcom.com/fileadmin/user_upload/Energy/Dr.Neuhaus/Support/ZDUE-MUC/Doc_communs/ZDUE-MUC_Anwenderhandbuch_V2_5.pdf
			//	chapter Globales Statuswort

			//	bit meaning
			//	0	always 0
			//	1	always 1
			//	2-7	always 0
			//	8	1 if fatal error was detected
			//	9	1 if restart was triggered by watchdog reset
			//	10	0 if IP address is available (DHCP)
			//	11	0 if ethernet link is available
			//	12	always 0
			//	13	0 if authorized on IP-T server
			//	14	1 ic case of out of memory
			//	15	always 0
			//	16	1 if Service interface is available (Kundenschnittstelle)
			//	17	1 if extension interface is available (Erweiterungs-Schnittstelle)
			//	18	1 if Wireless M-Bus interface is available
			//	19	1 if PLC is available
			//	20-31	always 0
			//	32	1 if time base is unsure

			//	32.. 28.. 24.. 20.. 16.. 12.. 8..  4..
			//	‭0001 0000 0000 0111 0000 0010 0000 0010‬ = 0x010070202

			//	81 00 60 05 00 00 = OBIS_CLASS_OP_LOG_STATUS_WORD

			//node::sml::parameter_tree(node::sml::OBIS_CLASS_OP_LOG_STATUS_WORD, node::sml::make_value(static_cast<std::uint64_t>(0x000070202)));

			//
			//	linearize and set CRC16
			//	append to current SML message
			//
			append_msg(node::sml::message(frame.at(1)	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, node::sml::BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(frame.at(3)	//	server id
					, node::sml::obis(0x81, 0x00, 0x60, 0x05, 0x00, 0x00)	//	path entry
					, node::sml::parameter_tree(node::sml::OBIS_CLASS_OP_LOG_STATUS_WORD, node::sml::make_value(static_cast<std::uint64_t>(0x070202))))));

		}

		void network::sml_get_proc_device_id(cyng::context& ctx)
		{
			//	[fd167248-a823-46d6-ba0a-03157a28a104,1805241520113192-3,2,0500153B02297E,operator,operator]
			//
			//	* pk
			//	* transaction id
			//	* SML message id
			//	* server ID
			//	* username
			//	* password
			//	* OBIS (requested parameter)
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.get.proc.device.id " << cyng::io::to_str(frame));

			append_msg(node::sml::message(frame.at(1)	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, node::sml::BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(frame.at(3)	//	server id
					, node::sml::obis(0x81, 0x81, 0xC7, 0x82, 0x01, 0xFF)	//	path entry

					//
					//	generate get process parameter response
					//	const static obis	DEFINE_OBIS_CODE(81, 81, C7, 82, 01, FF, CODE_ROOT_DEVICE_IDENT);	
					//	see: 7.3.2.9 Datenstruktur zur Abfrage der Geräte-Identifikation) 
					//
					, node::sml::child_list_tree(node::sml::OBIS_CODE_ROOT_DEVICE_IDENT, {

						//	device class (81 81 C7 82 53 FF == MUC-LAN/DSL)
						node::sml::parameter_tree(node::sml::obis(0x81, 0x81, 0xc7, 0x82, 0x02, 0xff), node::sml::make_value(node::sml::obis(0x81, 0x81, 0xC7, 0x82, 0x53, 0xFF))),
						node::sml::parameter_tree(node::sml::obis(0x81, 0x81, 0xc7, 0x82, 0x03, 0xff), node::sml::make_value(manufacturer_)),	// manufacturer
						node::sml::parameter_tree(node::sml::obis(0x81, 0x81, 0xc7, 0x82, 0x04, 0xff), node::sml::make_value(server_id_)),	// server id

						//	firmware
						node::sml::child_list_tree(node::sml::obis(0x81, 0x81, 0xc7, 0x82, 0x06, 0xff), {
							//	section 1
							node::sml::child_list_tree(node::sml::obis(0x81, 0x81, 0xc7, 0x82, 0x07, 0x01), {
									node::sml::parameter_tree(node::sml::obis(0x81, 0x81, 0xc7, 0x82, 0x08, 0xff), node::sml::make_value("CURRENT_VERSION")),
									node::sml::parameter_tree(node::sml::obis(0x81, 0x81, 0x00, 0x02, 0x00, 0x00), node::sml::make_value(NODE_VERSION)),
									node::sml::parameter_tree(node::sml::obis(0x81, 0x81, 0xc7, 0x82, 0x0e, 0xff), node::sml::make_value(true)) 	//	activated
								}),
							//	section 2
							node::sml::child_list_tree(node::sml::obis(0x81, 0x81, 0xc7, 0x82, 0x07, 0x02),	{
									node::sml::parameter_tree(node::sml::obis(0x81, 0x81, 0xc7, 0x82, 0x08, 0xff), node::sml::make_value("KERNEL")),
									node::sml::parameter_tree(node::sml::obis(0x81, 0x81, 0x00, 0x02, 0x00, 0x00), node::sml::make_value(NODE_PLATFORM)),
									node::sml::parameter_tree(node::sml::obis(0x81, 0x81, 0xc7, 0x82, 0x0e, 0xff), node::sml::make_value(true)) 	//	activated
								})

							}),

						//	hardware
						node::sml::child_list_tree(node::sml::obis(0x81, 0x81, 0xc7, 0x82, 0x09, 0xff), {
							//	Typenschlüssel
							node::sml::parameter_tree(node::sml::obis(0x81, 0x81, 0xc7, 0x82, 0x0a, 0x01), node::sml::make_value("VSES-1KW-221-1F0")),
							node::sml::parameter_tree(node::sml::obis(0x81, 0x81, 0xc7, 0x82, 0x0a, 0x02), node::sml::make_value("serial.number"))
						})
					}))));
		}

		void network::sml_get_proc_mem_usage(cyng::context& ctx)
		{
			//	[fd167248-a823-46d6-ba0a-03157a28a104,1805241520113192-3,2,0500153B02297E,operator,operator]
			//
			//	* pk
			//	* transaction id
			//	* SML message id
			//	* server ID
			//	* username
			//	* password
			//	* OBIS (requested parameter)
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.get.proc.mem.usage " << cyng::io::to_str(frame));

			//76                                                SML_Message(Sequence): 
			//  81063137303531313135343334303434363535382D32    transactionId: 170511154340446558-2
			//  6201                                            groupNo: 1
			//  6200                                            abortOnError: 0
			//  72                                              messageBody(Choice): 
			//	630501                                        messageBody: 1281 => SML_GetProcParameter_Res (0x00000501)
			//	73                                            SML_GetProcParameter_Res(Sequence): 
			//	  080500153B01EC46                            serverId: 05 00 15 3B 01 EC 46 
			//	  71                                          parameterTreePath(SequenceOf): 
			//		070080800010FF                            path_Entry: 00 80 80 00 10 FF 
			//	  73                                          parameterTree(Sequence): 
			//		070080800010FF                            parameterName: 00 80 80 00 10 FF 
			//		01                                        parameterValue: not set
			//		72                                        child_List(SequenceOf): 
			//		  73                                      tree_Entry(Sequence): 
			//			070080800011FF                        parameterName: 00 80 80 00 11 FF 
			//			72                                    parameterValue(Choice): 
			//			  6201                                parameterValue: 1 => smlValue (0x01)
			//			  620E                                smlValue: 14 (std::uint8_t)
			//			01                                    child_List: not set
			//		  73                                      tree_Entry(Sequence): 
			//			070080800012FF                        parameterName: 00 80 80 00 12 FF 
			//			72                                    parameterValue(Choice): 
			//			  6201                                parameterValue: 1 => smlValue (0x01)
			//			  6200                                smlValue: 0
			//			01                                    child_List: not set
			//  631AA8                                          crc16: 6824
			//  00                                              endOfSmlMsg: 00 

			CYNG_LOG_INFO(logger_, "get_used_physical_memory " << cyng::sys::get_used_physical_memory());
			CYNG_LOG_INFO(logger_, "get_total_physical_memory " << cyng::sys::get_total_physical_memory());
			CYNG_LOG_INFO(logger_, "get_used_virtual_memory " << cyng::sys::get_used_virtual_memory());
			CYNG_LOG_INFO(logger_, "get_total_virtual_memory " << cyng::sys::get_total_virtual_memory());

			//	m2m::OBIS_CODE_ROOT_MEMORY_USAGE
			const std::uint8_t mirror = cyng::sys::get_used_virtual_memory_in_percent()
				, tmp = cyng::sys::get_used_physical_memory_in_percent();

			append_msg(node::sml::message(frame.at(1)	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, node::sml::BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(frame.at(3)	//	server id
					, node::sml::obis(0x00, 0x80, 0x80, 0x00, 0x10, 0xFF)	//	path entry

					//
					//	generate get process parameter response
					//	2 uint8 values 
					//
					, node::sml::child_list_tree(node::sml::obis(0x00, 0x80, 0x80, 0x00, 0x10, 0xFF), {

						node::sml::parameter_tree(node::sml::obis(0x00, 0x80, 0x80, 0x00, 0x11, 0xFF), node::sml::make_value(mirror)),	//	mirror
						node::sml::parameter_tree(node::sml::obis(0x00, 0x80, 0x80, 0x00, 0x12, 0xFF), node::sml::make_value(tmp))	// tmp
			}))));
		}

		void network::sml_get_proc_lan_if(cyng::context& ctx)
		{
			//	[fd167248-a823-46d6-ba0a-03157a28a104,1805241520113192-3,2,0500153B02297E,operator,operator]
			//
			//	* pk
			//	* transaction id
			//	* SML message id
			//	* server ID
			//	* username
			//	* password
			//	* OBIS (requested parameter)
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.get.proc.lan.if " << cyng::io::to_str(frame));

		}
		void network::sml_get_proc_lan_config(cyng::context& ctx)
		{
			//	[fd167248-a823-46d6-ba0a-03157a28a104,1805241520113192-3,2,0500153B02297E,operator,operator]
			//
			//	* pk
			//	* transaction id
			//	* SML message id
			//	* server ID
			//	* username
			//	* password
			//	* OBIS (requested parameter)
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.get.proc.lan.config " << cyng::io::to_str(frame));

		}

		void network::sml_get_proc_ntp_config(cyng::context& ctx)
		{
			//	[fd167248-a823-46d6-ba0a-03157a28a104,1805241520113192-3,2,0500153B02297E,operator,operator]
			//
			//	* pk
			//	* transaction id
			//	* SML message id
			//	* server ID
			//	* username
			//	* password
			//	* OBIS (requested parameter)
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.get.proc.ntp.config " << cyng::io::to_str(frame));

			//76                                                SML_Message(Sequence): 
			//  81063137303531323136353635313836333431352D32    transactionId: 170512165651863415-2
			//  6201                                            groupNo: 1
			//  6200                                            abortOnError: 0
			//  72                                              messageBody(Choice): 
			//	630501                                        messageBody: 1281 => SML_GetProcParameter_Res (0x00000501)
			//	73                                            SML_GetProcParameter_Res(Sequence): 
			//	  080500153B01EC46                            serverId: 05 00 15 3B 01 EC 46 
			//	  71                                          parameterTreePath(SequenceOf): 
			//		078181C78801FF                            path_Entry: 81 81 C7 88 01 FF 
			//	  73                                          parameterTree(Sequence): 
			//		078181C78801FF                            parameterName: 81 81 C7 88 01 FF 
			//		01                                        parameterValue: not set
			//		74                                        child_List(SequenceOf): 
			//		  73                                      tree_Entry(Sequence): 
			//			078181C78802FF                        parameterName: 81 81 C7 88 02 FF 
			//			01                                    parameterValue: not set
			//			73                                    child_List(SequenceOf): 
			//			  73                                  tree_Entry(Sequence): 
			//				078181C7880201                    parameterName: 81 81 C7 88 02 01 
			//				72                                parameterValue(Choice): 
			//				  6201                            parameterValue: 1 => smlValue (0x01)
			//				  810170746274696D65312E7074622E6465smlValue: ptbtime1.ptb.de
			//				01                                child_List: not set
			//			  73                                  tree_Entry(Sequence): 
			//				078181C7880202                    parameterName: 81 81 C7 88 02 02 
			//				72                                parameterValue(Choice): 
			//				  6201                            parameterValue: 1 => smlValue (0x01)
			//				  810170746274696D65322E7074622E6465smlValue: ptbtime2.ptb.de
			//				01                                child_List: not set
			//			  73                                  tree_Entry(Sequence): 
			//				078181C7880203                    parameterName: 81 81 C7 88 02 03 
			//				72                                parameterValue(Choice): 
			//				  6201                            parameterValue: 1 => smlValue (0x01)
			//				  810170746274696D65332E7074622E6465smlValue: ptbtime3.ptb.de
			//				01                                child_List: not set
			//		  73                                      tree_Entry(Sequence): 
			//			078181C78803FF                        parameterName: 81 81 C7 88 03 FF 
			//			72                                    parameterValue(Choice): 
			//			  6201                                parameterValue: 1 => smlValue (0x01)
			//			  627B                                smlValue: 123
			//			01                                    child_List: not set
			//		  73                                      tree_Entry(Sequence): 
			//			078181C78806FF                        parameterName: 81 81 C7 88 06 FF 
			//			72                                    parameterValue(Choice): 
			//			  6201                                parameterValue: 1 => smlValue (0x01)
			//			  4201                                smlValue: True
			//			01                                    child_List: not set
			//		  73                                      tree_Entry(Sequence): 
			//			078181C78804FF                        parameterName: 81 81 C7 88 04 FF 
			//			72                                    parameterValue(Choice): 
			//			  6201                                parameterValue: 1 => smlValue (0x01)
			//			  630E10                              smlValue: 3600
			//			01                                    child_List: not set
			//  638154                                        crc16: 33108
			//  00                                            endOfSmlMsg: 00 

			const std::string ntp_primary = "pbtime1.pbt.de", ntp_secondary = "pbtime2.pbt.de", ntp_tertiary = "pbtime3.pbt.de";
			const std::uint16_t ntp_port = 555;	// 123;
			const bool ntp_active = true;
			const std::uint16_t ntp_tz = 3600;

			append_msg(node::sml::message(frame.at(1)	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, node::sml::BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(frame.at(3)	//	server id
					, node::sml::obis(0x81, 0x81, 0xC7, 0x88, 0x01, 0xFF)	//	path entry

					//
					//	generate get process parameter response
					//
					, node::sml::child_list_tree(node::sml::obis(0x81, 0x81, 0xC7, 0x88, 0x01, 0xFF), {

						//	NTP servers
						node::sml::child_list_tree(node::sml::obis(0x81, 0x81, 0xC7, 0x88, 0x02, 0xFF),{
							node::sml::parameter_tree(node::sml::obis(0x81, 0x81, 0xC7, 0x88, 0x02, 0x01), node::sml::make_value(ntp_primary)),	
							node::sml::parameter_tree(node::sml::obis(0x81, 0x81, 0xC7, 0x88, 0x02, 0x02), node::sml::make_value(ntp_secondary)),
							node::sml::parameter_tree(node::sml::obis(0x81, 0x81, 0xC7, 0x88, 0x02, 0x03), node::sml::make_value(ntp_tertiary))
						}),
						node::sml::parameter_tree(node::sml::obis(0x81, 0x81, 0xC7, 0x88, 0x03, 0xFF), node::sml::make_value(ntp_port)),	//	NTP port
						node::sml::parameter_tree(node::sml::obis(0x81, 0x81, 0xC7, 0x88, 0x06, 0xFF), node::sml::make_value(ntp_active)),	// enabled/disabled
						node::sml::parameter_tree(node::sml::obis(0x81, 0x81, 0xC7, 0x88, 0x04, 0xFF), node::sml::make_value(ntp_tz))	// timezone
			}))));

		}

		void network::sml_get_proc_device_time(cyng::context& ctx)
		{
			//	[fd167248-a823-46d6-ba0a-03157a28a104,1805241520113192-3,2,0500153B02297E,operator,operator]
			//
			//	* pk
			//	* transaction id
			//	* SML message id
			//	* server ID
			//	* username
			//	* password
			//	* OBIS (requested parameter)
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.get.proc.device.time " << cyng::io::to_str(frame));
			//76                                                SML_Message(Sequence): 
			//  81063137303531323136353635313836333431352D33    transactionId: 170512165651863415-3
			//  6202                                            groupNo: 2
			//  6200                                            abortOnError: 0
			//  72                                              messageBody(Choice): 
			//	630501                                        messageBody: 1281 => SML_GetProcParameter_Res (0x00000501)
			//	73                                            SML_GetProcParameter_Res(Sequence): 
			//	  080500153B01EC46                            serverId: 05 00 15 3B 01 EC 46 
			//	  71                                          parameterTreePath(SequenceOf): 
			//		078181C78810FF                            path_Entry: 81 81 C7 88 10 FF 
			//	  73                                          parameterTree(Sequence): 
			//		078181C78810FF                            parameterName: 81 81 C7 88 10 FF 
			//		01                                        parameterValue: not set
			//		74                                        child_List(SequenceOf): 
			//		  73                                      tree_Entry(Sequence): 
			//			07010000090B00                        parameterName: 01 00 00 09 0B 00 
			//			72                                    parameterValue(Choice): 
			//			  6204                                parameterValue: 4 => smlTime (0x04)
			//			  72                                  smlTime(Choice): 
			//				6202                              smlTime: 2 => timestamp (0x02)
			//				655915CD3C                        timestamp: 1494601020
			//			01                                    child_List: not set
			//		  73                                      tree_Entry(Sequence): 
			//			070000600800FF                        parameterName: 00 00 60 08 00 FF 
			//			72                                    parameterValue(Choice): 
			//			  6204                                parameterValue: 4 => smlTime (0x04)
			//			  72                                  smlTime(Choice): 
			//				6201                              smlTime: 1 => secIndex (0x01)
			//				6505E5765F                        secIndex: 98924127
			//			01                                    child_List: not set
			//		  73                                      tree_Entry(Sequence): 
			//			07810000090B01                        parameterName: 81 00 00 09 0B 01 
			//			72                                    parameterValue(Choice): 
			//			  6201                                parameterValue: 1 => smlValue (0x01)
			//			  550000003C                          smlValue: 60
			//			01                                    child_List: not set
			//		  73                                      tree_Entry(Sequence): 
			//			07810000090B02                        parameterName: 81 00 00 09 0B 02 
			//			72                                    parameterValue(Choice): 
			//			  6201                                parameterValue: 1 => smlValue (0x01)
			//			  4200                                smlValue: False
			//			01                                    child_List: not set
			//  633752                                          crc16: 14162
			//  00                                              endOfSmlMsg: 00 

			const std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
			const std::time_t now_tt = std::chrono::system_clock::to_time_t(now);
			const std::int32_t tz = 60;
			const bool sync_active = true;

			append_msg(node::sml::message(frame.at(1)	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, node::sml::BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(frame.at(3)	//	server id
					, node::sml::OBIS_CODE_ROOT_DEVICE_TIME	//	path entry 81 81 C7 88 10 FF 

					//
					//	generate get process parameter response
					//
					, node::sml::child_list_tree(node::sml::OBIS_CODE_ROOT_DEVICE_TIME, {

						node::sml::parameter_tree(node::sml::OBIS_CLASS_TIMESTAMP_UTC, node::sml::make_value(now)),	//	timestamp (01 00 00 09 0B 00 )
						node::sml::parameter_tree(node::sml::obis(0x00, 0x00, 0x60, 0x08, 0x00, 0xFF), node::sml::make_sec_index_value(now)),
						node::sml::parameter_tree(node::sml::obis(0x81, 0x00, 0x00, 0x09, 0x0B, 0x01), node::sml::make_value(tz)),
						node::sml::parameter_tree(node::sml::obis(0x81, 0x00, 0x00, 0x09, 0x0B, 0x02), node::sml::make_value(sync_active))
			}))));

		}

		void network::sml_get_proc_active_devices(cyng::context& ctx)
		{
			//	[fd167248-a823-46d6-ba0a-03157a28a104,1805241520113192-3,2,0500153B02297E,operator,operator]
			//
			//	* pk
			//	* transaction id
			//	* SML message id
			//	* server ID
			//	* username
			//	* password
			//	* OBIS (requested parameter)
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.get.proc.active.devices " << cyng::io::to_str(frame));

			//	last received data
			const std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
			const std::string device_class = "---";	//	2D 2D 2D

			append_msg(node::sml::message(frame.at(1)	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, node::sml::BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(frame.at(3)	//	server id
					, node::sml::OBIS_CODE_ROOT_ACTIVE_DEVICES	//	path entry 81 81 11 06 FF FF

					//
					//	generate get process parameter response
					//
					, node::sml::child_list_tree(node::sml::OBIS_CODE_ROOT_ACTIVE_DEVICES, {
						node::sml::child_list_tree(node::sml::obis(0x81, 0x81, 0x11, 0x06, 0x01, 0xFF), {

						node::sml::child_list_tree(node::sml::obis(0x81, 0x81, 0x11, 0x06, 0x01, 0x01),{
							node::sml::parameter_tree(node::sml::obis(0x81, 0x81, 0xC7, 0x82, 0x04, 0xFF), node::sml::make_value(server_id_)),
							node::sml::parameter_tree(node::sml::obis(0x81, 0x81, 0xC7, 0x82, 0x02, 0xFF), node::sml::make_value(device_class)),
							node::sml::parameter_tree(node::sml::OBIS_CLASS_TIMESTAMP_UTC, node::sml::make_value(now))	//	timestamp (01 00 00 09 0B 00 )
						}),
						node::sml::child_list_tree(node::sml::obis(0x81, 0x81, 0x11, 0x06, 0x01, 0x02),{
							node::sml::parameter_tree(node::sml::obis(0x81, 0x81, 0xC7, 0x82, 0x04, 0xFF), node::sml::make_value(cyng::buffer_t{ 0x01, (char)0xA8, 0x15, 0x74, (char)0x31, 0x45, 0x05, (char)0x01, (char)0x02 })),
							node::sml::parameter_tree(node::sml::obis(0x81, 0x81, 0xC7, 0x82, 0x02, 0xFF), node::sml::make_value(device_class)),
							node::sml::parameter_tree(node::sml::OBIS_CLASS_TIMESTAMP_UTC, node::sml::make_value(now))	//	timestamp (01 00 00 09 0B 00 )
						}),
						node::sml::child_list_tree(node::sml::obis(0x81, 0x81, 0x11, 0x06, 0x01, 0x03),{
							node::sml::parameter_tree(node::sml::obis(0x81, 0x81, 0xC7, 0x82, 0x04, 0xFF), node::sml::make_value(cyng::buffer_t{ 0x01, (char)0xA8, 0x15, 0x74, (char)0x31, 0x45, 0x04, (char)0x01, (char)0x02 })),
							node::sml::parameter_tree(node::sml::obis(0x81, 0x81, 0xC7, 0x82, 0x02, 0xFF), node::sml::make_value(device_class)),
							node::sml::parameter_tree(node::sml::OBIS_CLASS_TIMESTAMP_UTC, node::sml::make_value(now))	//	timestamp (01 00 00 09 0B 00 )
						})
					}) 
				}))));
		}

		void network::sml_get_proc_visible_devices(cyng::context& ctx)
		{
			//	[fd167248-a823-46d6-ba0a-03157a28a104,1805241520113192-3,2,0500153B02297E,operator,operator]
			//
			//	* pk
			//	* transaction id
			//	* SML message id
			//	* server ID
			//	* username
			//	* password
			//	* OBIS (requested parameter)
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.get.proc.visible.devices " << cyng::io::to_str(frame));

			//	last received data
			const std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
			const std::string device_class = "---";	//	2D 2D 2D

			append_msg(node::sml::message(frame.at(1)	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, node::sml::BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(frame.at(3)	//	server id
					, node::sml::OBIS_CODE_ROOT_VISIBLE_DEVICES	//	path entry 81 81 10 06 FF FF

					//
					//	generate get process parameter response
					//
					, node::sml::child_list_tree(node::sml::OBIS_CODE_ROOT_VISIBLE_DEVICES, {
						node::sml::child_list_tree(node::sml::obis(0x81, 0x81, 0x10, 0x06, 0x01, 0xFF), {

						node::sml::child_list_tree(node::sml::obis(0x81, 0x81, 0x10, 0x06, 0x01, 0x01),{
							node::sml::parameter_tree(node::sml::obis(0x81, 0x81, 0xC7, 0x82, 0x04, 0xFF), node::sml::make_value(server_id_)),
							node::sml::parameter_tree(node::sml::obis(0x81, 0x81, 0xC7, 0x82, 0x02, 0xFF), node::sml::make_value(device_class)),
							node::sml::parameter_tree(node::sml::OBIS_CLASS_TIMESTAMP_UTC, node::sml::make_value(now))	//	timestamp (01 00 00 09 0B 00 )
						}),
						node::sml::child_list_tree(node::sml::obis(0x81, 0x81, 0x10, 0x06, 0x01, 0x02),{
							node::sml::parameter_tree(node::sml::obis(0x81, 0x81, 0xC7, 0x82, 0x04, 0xFF), node::sml::make_value(cyng::buffer_t{ 0x24, 0x23, 0x02, 0x12, (char)0x90, 0x20, 0x07, (char)0x86 })),
							node::sml::parameter_tree(node::sml::obis(0x81, 0x81, 0xC7, 0x82, 0x02, 0xFF), node::sml::make_value(device_class)),
							node::sml::parameter_tree(node::sml::OBIS_CLASS_TIMESTAMP_UTC, node::sml::make_value(now))	//	timestamp (01 00 00 09 0B 00 )
						}),
						node::sml::child_list_tree(node::sml::obis(0x81, 0x81, 0x10, 0x06, 0x01, 0x02),{
							node::sml::parameter_tree(node::sml::obis(0x81, 0x81, 0xC7, 0x82, 0x04, 0xFF), node::sml::make_value(cyng::buffer_t{ 0x24, 0x23, 0x02, 0x15, (char)0x90, 0x20, 0x07, (char)0x86 })),
							node::sml::parameter_tree(node::sml::obis(0x81, 0x81, 0xC7, 0x82, 0x02, 0xFF), node::sml::make_value(device_class)),
							node::sml::parameter_tree(node::sml::OBIS_CLASS_TIMESTAMP_UTC, node::sml::make_value(now))	//	timestamp (01 00 00 09 0B 00 )
						})
					})
				}))));
		}

		void network::sml_get_proc_device_info(cyng::context& ctx)
		{
			//	[fd167248-a823-46d6-ba0a-03157a28a104,1805241520113192-3,2,0500153B02297E,operator,operator]
			//
			//	* pk
			//	* transaction id
			//	* SML message id
			//	* server ID
			//	* username
			//	* password
			//	* OBIS (requested parameter)
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.get.proc.device.info " << cyng::io::to_str(frame));

			//
			//	linearize and set CRC16
			//	append to current SML message
			//
			append_msg(node::sml::message(frame.at(1)	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, node::sml::BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(frame.at(3)	//	server id
					, node::sml::OBIS_CODE_ROOT_DEVICE_INFO	//	path entry - 81 81 12 06 FF FF
					, node::sml::empty_tree(node::sml::OBIS_CODE_ROOT_DEVICE_INFO))));

		}

		void network::sml_get_proc_ipt_state(cyng::context& ctx)
		{
			//	[fd167248-a823-46d6-ba0a-03157a28a104,1805241520113192-3,2,0500153B02297E,operator,operator]
			//
			//	* pk
			//	* transaction id
			//	* SML message id
			//	* server ID
			//	* username
			//	* password
			//	* OBIS (requested parameter)
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.get.proc.ipt.state " << cyng::io::to_str(frame));

			//76                                                SML_Message(Sequence): 
			//  81063137303531313136303831363537393537312D33    transactionId: 170511160816579571-3
			//  6202                                            groupNo: 2
			//  6200                                            abortOnError: 0
			//  72                                              messageBody(Choice): 
			//	630501                                        messageBody: 1281 => SML_GetProcParameter_Res (0x00000501)
			//	73                                            SML_GetProcParameter_Res(Sequence): 
			//	  080500153B01EC46                            serverId: 05 00 15 3B 01 EC 46 
			//	  71                                          parameterTreePath(SequenceOf): 
			//		0781490D0600FF                            path_Entry: 81 49 0D 06 00 FF 
			//	  73                                          parameterTree(Sequence): 
			//		0781490D0600FF                            parameterName: 81 49 0D 06 00 FF 
			//		01                                        parameterValue: not set
			//		73                                        child_List(SequenceOf): 
			//		  73                                      tree_Entry(Sequence): 
			//			07814917070000                        parameterName: 81 49 17 07 00 00 
			//			72                                    parameterValue(Choice): 
			//			  6201                                parameterValue: 1 => smlValue (0x01)
			//			  652A96A8C0                          smlValue: 714516672
			//			01                                    child_List: not set
			//		  73                                      tree_Entry(Sequence): 
			//			0781491A070000                        parameterName: 81 49 1A 07 00 00 
			//			72                                    parameterValue(Choice): 
			//			  6201                                parameterValue: 1 => smlValue (0x01)
			//			  6368EF                              smlValue: 26863
			//			01                                    child_List: not set
			//		  73                                      tree_Entry(Sequence): 
			//			07814919070000                        parameterName: 81 49 19 07 00 00 
			//			72                                    parameterValue(Choice): 
			//			  6201                                parameterValue: 1 => smlValue (0x01)
			//			  631019                              smlValue: 4121
			//			01                                    child_List: not set
			//  639CB8                                          crc16: 40120
			//  00                                              endOfSmlMsg: 00 

			const std::uint32_t ip_address = bus_->remote_endpoint().address().to_v4().to_ulong();
			const std::uint16_t target_port = bus_->local_endpoint().port()
				, source_port = bus_->remote_endpoint().port();

			//
			//	linearize and set CRC16
			//	append to current SML message
			//
			append_msg(node::sml::message(frame.at(1)	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, node::sml::BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(frame.at(3)	//	server id  
					, node::sml::OBIS_CODE_ROOT_IPT_STATE	//	path entry - 81 49 0D 06 00 FF 
					, node::sml::child_list_tree(node::sml::OBIS_CODE_ROOT_IPT_STATE, {

						node::sml::parameter_tree(node::sml::obis(0x81, 0x49, 0x17, 0x07, 0x00, 0x00), node::sml::make_value(ip_address)),
						node::sml::parameter_tree(node::sml::obis(0x81, 0x49, 0x1A, 0x07, 0x00, 0x00), node::sml::make_value(target_port)),
						node::sml::parameter_tree(node::sml::obis(0x81, 0x49, 0x19, 0x07, 0x00, 0x00), node::sml::make_value(source_port))
			}))));
		}

		void network::sml_get_proc_ipt_param(cyng::context& ctx)
		{
			//	[fd167248-a823-46d6-ba0a-03157a28a104,1805241520113192-3,2,0500153B02297E,operator,operator]
			//
			//	* pk
			//	* transaction id
			//	* SML message id
			//	* server ID
			//	* username
			//	* password
			//	* OBIS (requested parameter)
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.get.proc.ipt.param " << cyng::io::to_str(frame));

			//76                                                SML_Message(Sequence): 
			//  81063137303531313136303831363537393537312D32    transactionId: 170511160816579571-2
			//  6201                                            groupNo: 1
			//  6200                                            abortOnError: 0
			//  72                                              messageBody(Choice): 
			//	630501                                        messageBody: 1281 => SML_GetProcParameter_Res (0x00000501)
			//	73                                            SML_GetProcParameter_Res(Sequence): 
			//	  080500153B01EC46                            serverId: 05 00 15 3B 01 EC 46 
			//	  71                                          parameterTreePath(SequenceOf): 
			//		0781490D0700FF                            path_Entry: 81 49 0D 07 00 FF 
			//	  73                                          parameterTree(Sequence): 
			//		0781490D0700FF                            parameterName: 81 49 0D 07 00 FF 
			//		01                                        parameterValue: not set
			//		76                                        child_List(SequenceOf): 
			//		  73                                      tree_Entry(Sequence): 
			//			0781490D070001                        parameterName: 81 49 0D 07 00 01 
			//			01                                    parameterValue: not set
			//			75                                    child_List(SequenceOf): 
			//			  73                                  tree_Entry(Sequence): 
			//				07814917070001                    parameterName: 81 49 17 07 00 01 
			//				72                                parameterValue(Choice): 
			//				  6201                            parameterValue: 1 => smlValue (0x01)
			//				  652A96A8C0                      smlValue: 714516672
			//				01                                child_List: not set
			//			  73                                  tree_Entry(Sequence): 
			//				0781491A070001                    parameterName: 81 49 1A 07 00 01 
			//				72                                parameterValue(Choice): 
			//				  6201                            parameterValue: 1 => smlValue (0x01)
			//				  6368EF                          smlValue: 26863
			//				01                                child_List: not set
			//			  73                                  tree_Entry(Sequence): 
			//				07814919070001                    parameterName: 81 49 19 07 00 01 
			//				72                                parameterValue(Choice): 
			//				  6201                            parameterValue: 1 => smlValue (0x01)
			//				  6200                            smlValue: 0
			//				01                                child_List: not set
			//			  73                                  tree_Entry(Sequence): 
			//				078149633C0101                    parameterName: 81 49 63 3C 01 01 
			//				72                                parameterValue(Choice): 
			//				  6201                            parameterValue: 1 => smlValue (0x01)
			//				  094C534D5465737432              smlValue: LSMTest2
			//				01                                child_List: not set
			//			  73                                  tree_Entry(Sequence): 
			//				078149633C0201                    parameterName: 81 49 63 3C 02 01 
			//				72                                parameterValue(Choice): 
			//				  6201                            parameterValue: 1 => smlValue (0x01)
			//				  094C534D5465737432              smlValue: LSMTest2
			//				01                                child_List: not set
			//		  73                                      tree_Entry(Sequence): 
			//			0781490D070002                        parameterName: 81 49 0D 07 00 02 
			//			01                                    parameterValue: not set
			//			75                                    child_List(SequenceOf): 
			//			  73                                  tree_Entry(Sequence): 
			//				07814917070002                    parameterName: 81 49 17 07 00 02 
			//				72                                parameterValue(Choice): 
			//				  6201                            parameterValue: 1 => smlValue (0x01)
			//				  65B596A8C0                      smlValue: 3046549696
			//				01                                child_List: not set
			//			  73                                  tree_Entry(Sequence): 
			//				0781491A070002                    parameterName: 81 49 1A 07 00 02 
			//				72                                parameterValue(Choice): 
			//				  6201                            parameterValue: 1 => smlValue (0x01)
			//				  6368F0                          smlValue: 26864
			//				01                                child_List: not set
			//			  73                                  tree_Entry(Sequence): 
			//				07814919070002                    parameterName: 81 49 19 07 00 02 
			//				72                                parameterValue(Choice): 
			//				  6201                            parameterValue: 1 => smlValue (0x01)
			//				  6200                            smlValue: 0
			//				01                                child_List: not set
			//			  73                                  tree_Entry(Sequence): 
			//				078149633C0102                    parameterName: 81 49 63 3C 01 02 
			//				72                                parameterValue(Choice): 
			//				  6201                            parameterValue: 1 => smlValue (0x01)
			//				  094C534D5465737432              smlValue: LSMTest2
			//				01                                child_List: not set
			//			  73                                  tree_Entry(Sequence): 
			//				078149633C0202                    parameterName: 81 49 63 3C 02 02 
			//				72                                parameterValue(Choice): 
			//				  6201                            parameterValue: 1 => smlValue (0x01)
			//				  094C534D5465737432              smlValue: LSMTest2
			//				01                                child_List: not set
			//		  73                                      tree_Entry(Sequence): 
			//			07814827320601                        parameterName: 81 48 27 32 06 01 
			//			72                                    parameterValue(Choice): 
			//			  6201                                parameterValue: 1 => smlValue (0x01)
			//			  6201                                smlValue: 1
			//			01                                    child_List: not set
			//		  73                                      tree_Entry(Sequence): 
			//			07814831320201                        parameterName: 81 48 31 32 02 01 
			//			72                                    parameterValue(Choice): 
			//			  6201                                parameterValue: 1 => smlValue (0x01)
			//			  6278                                smlValue: 120
			//			01                                    child_List: not set
			//		  73                                      tree_Entry(Sequence): 
			//			070080800003FF                        parameterName: 00 80 80 00 03 FF 
			//			72                                    parameterValue(Choice): 
			//			  6201                                parameterValue: 1 => smlValue (0x01)
			//			  4200                                smlValue: False
			//			01                                    child_List: not set
			//		  73                                      tree_Entry(Sequence): 
			//			070080800004FF                        parameterName: 00 80 80 00 04 FF 
			//			01                                    parameterValue: not set
			//			01                                    child_List: not set
			//  63915D                                          crc16: 37213
			//  00                                              endOfSmlMsg: 00 

			const std::uint32_t ip_address_primary_master = 714516672;	//	192.168.150.42
			const std::uint16_t target_port_primary_master = 26862
				, source_port_primary_master = 0;
			const std::string user_pm = config_[0].account_
				, pwd_pm = config_[0].pwd_;

			const std::uint32_t ip_address_secondary_master = 3046549696;	//	192.168.150.181
			const std::uint16_t target_port_secondary_master = 26863
				, source_port_seccondary_master = 0;
			const std::string user_sm = config_[1].account_
				, pwd_sm = config_[1].pwd_;

			const std::uint16_t wait_time = bus_->get_watchdog();	//	minutes
			const std::uint16_t repetitions = 120;	//	counter
			const bool ssl = false;

			//
			//	linearize and set CRC16
			//	append to current SML message
			//
			append_msg(node::sml::message(frame.at(1)	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, node::sml::BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(frame.at(3)	//	server id  
					, node::sml::OBIS_CODE_ROOT_IPT_PARAM	//	path entry - 81 49 0D 07 00 FF 
					, node::sml::child_list_tree(node::sml::OBIS_CODE_ROOT_IPT_PARAM, {

						//	primary master
						node::sml::child_list_tree(node::sml::obis(0x81, 0x49, 0x0D, 0x07, 0x00, 0x01), {
							node::sml::parameter_tree(node::sml::obis(0x81, 0x49, 0x17, 0x07, 0x00, 0x01), node::sml::make_value(ip_address_primary_master)),
							node::sml::parameter_tree(node::sml::obis(0x81, 0x49, 0x1A, 0x07, 0x00, 0x01), node::sml::make_value(target_port_primary_master)),
							node::sml::parameter_tree(node::sml::obis(0x81, 0x49, 0x19, 0x07, 0x00, 0x01), node::sml::make_value(source_port_primary_master)),
							node::sml::parameter_tree(node::sml::obis(0x81, 0x49, 0x63, 0x3C, 0x01, 0x01), node::sml::make_value(user_pm)),
							node::sml::parameter_tree(node::sml::obis(0x81, 0x49, 0x63, 0x3C, 0x02, 0x01), node::sml::make_value(pwd_pm))
						}),

						//	secondary master
						node::sml::child_list_tree(node::sml::obis(0x81, 0x49, 0x0D, 0x07, 0x00, 0x02), {
							node::sml::parameter_tree(node::sml::obis(0x81, 0x49, 0x17, 0x07, 0x00, 0x02), node::sml::make_value(ip_address_secondary_master)),
							node::sml::parameter_tree(node::sml::obis(0x81, 0x49, 0x1A, 0x07, 0x00, 0x02), node::sml::make_value(target_port_secondary_master)),
							node::sml::parameter_tree(node::sml::obis(0x81, 0x49, 0x19, 0x07, 0x00, 0x02), node::sml::make_value(source_port_seccondary_master)),
							node::sml::parameter_tree(node::sml::obis(0x81, 0x49, 0x63, 0x3C, 0x01, 0x02), node::sml::make_value(user_sm)),
							node::sml::parameter_tree(node::sml::obis(0x81, 0x49, 0x63, 0x3C, 0x02, 0x02), node::sml::make_value(pwd_sm))
						}),

						//	waiting time (Wartezeit)
						node::sml::parameter_tree(node::sml::obis(0x81, 0x48, 0x27, 0x32, 0x06, 0x01), node::sml::make_value(wait_time)),

						//	repetitions
						node::sml::parameter_tree(node::sml::obis(0x81, 0x48, 0x31, 0x32, 0x02, 0x01), node::sml::make_value(repetitions)),

						//	SSL
						node::sml::parameter_tree(node::sml::obis(0x00, 0x80, 0x80, 0x00, 0x03, 0xFF), node::sml::make_value(ssl)),

						//	certificates (none)
						node::sml::empty_tree(node::sml::obis(0x00, 0x80, 0x80, 0x00, 0x04, 0xFF))

			}))));

		}

	}
}
