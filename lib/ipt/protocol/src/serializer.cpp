/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/ipt/serializer.h>
#include <smf/ipt/codes.h>
#include <cyng/vm/generator.h>
#include <cyng/value_cast.hpp>
#include <cyng/io/serializer.h>
#ifdef SMF_IO_DEBUG
#include <cyng/io/hex_dump.hpp>
#endif
#include <cyng/tuple_cast.hpp>
#include <boost/predef.h>

namespace node
{
	namespace ipt
	{
		serializer::serializer(boost::asio::ip::tcp::socket& s
			, cyng::controller& vm
			, scramble_key const& def)
		: buffer_()
			, ostream_(&buffer_)
			, sgen_()
			, last_seq_(0)
			, scrambler_()
			, def_key_(def)
		{
			vm.register_function("stream.flush", 0, [this, &s](cyng::context& ctx) {

#ifdef SMF_IO_DEBUG
				//	get content of buffer
				boost::asio::const_buffer cbuffer(*buffer_.data().begin());
				const char* p = boost::asio::buffer_cast<const char*>(cbuffer);
				const std::size_t size = boost::asio::buffer_size(cbuffer);

				cyng::io::hex_dump hd;
				hd(std::cerr, p, p + size);
#endif
				//BOOST_ASSERT(s.is_open());

				boost::system::error_code ec;
				boost::asio::write(s, buffer_, ec);
				ctx.set_register(ec);
			});

			vm.register_function("stream.serialize", 0, [this](cyng::context& ctx) {

				const cyng::vector_t frame = ctx.get_frame();
				for (auto obj : frame)
				{
					cyng::io::serialize_plain(std::cerr, obj);
					cyng::io::serialize_binary(ostream_, obj);
				}

			});

			//
			//	generated from parser
			//
			vm.register_function("ipt.set.sk.def", 1, std::bind(&serializer::set_sk, this, std::placeholders::_1));

			//
			//	after reconnect
			//
			vm.register_function("ipt.reset.serializer", 1, std::bind(&serializer::reset, this, std::placeholders::_1));

			//
			//	push last ipt sequece number on stack
			//
			vm.register_function("ipt.push.seq", 0, std::bind(&serializer::push_seq, this, std::placeholders::_1));

			//
			//	transfer data
			//
			vm.register_function("ipt.transfer.data", 1, std::bind(&serializer::transfer_data, this, std::placeholders::_1));

			//
			//	generated from master node (mostly)
			//
			vm.register_function("req.login.public", 2, std::bind(&serializer::req_login_public, this, std::placeholders::_1));
			vm.register_function("req.login.scrambled", 3, std::bind(&serializer::req_login_scrambled, this, std::placeholders::_1));
			vm.register_function("res.login.public", 3, std::bind(&serializer::res_login_public, this, std::placeholders::_1));
			vm.register_function("res.login.scrambled", 3, std::bind(&serializer::res_login_scrambled, this, std::placeholders::_1));

			vm.register_function("req.open.push.channel", 1, std::bind(&serializer::req_open_push_channel, this, std::placeholders::_1));
			vm.register_function("res.open.push.channel", 8, std::bind(&serializer::res_open_push_channel, this, std::placeholders::_1));

			vm.register_function("req.close.push.channel", 1, std::bind(&serializer::req_close_push_channel, this, std::placeholders::_1));
			vm.register_function("res.close.push.channel", 3, std::bind(&serializer::res_close_push_channel, this, std::placeholders::_1));

			vm.register_function("req.transfer.push.data", 5, std::bind(&serializer::req_transfer_push_data, this, std::placeholders::_1));
			vm.register_function("res.transfer.push.data", 6, std::bind(&serializer::res_transfer_push_data, this, std::placeholders::_1));

			vm.register_function("req.open.connection", 1, std::bind(&serializer::req_open_connection, this, std::placeholders::_1));
			vm.register_function("res.open.connection", 2, std::bind(&serializer::res_open_connection, this, std::placeholders::_1));

			vm.register_function("req.close.connection", 0, std::bind(&serializer::req_close_connection, this, std::placeholders::_1));
			vm.register_function("res.close.connection", 2, std::bind(&serializer::res_close_connection, this, std::placeholders::_1));

			vm.register_function("req.protocol.version", 0, std::bind(&serializer::req_protocol_version, this, std::placeholders::_1));
			vm.register_function("res.protocol.version", 2, std::bind(&serializer::res_protocol_version, this, std::placeholders::_1));

			vm.register_function("req.software.version", 0, std::bind(&serializer::req_software_version, this, std::placeholders::_1));
			vm.register_function("res.software.version", 2, std::bind(&serializer::res_software_version, this, std::placeholders::_1));

			vm.register_function("req.device.id", 0, std::bind(&serializer::req_device_id, this, std::placeholders::_1));
			vm.register_function("res.device.id", 2, std::bind(&serializer::res_device_id, this, std::placeholders::_1));

			vm.register_function("req.net.status", 0, std::bind(&serializer::req_network_status, this, std::placeholders::_1));
			vm.register_function("res.net.status", 9, std::bind(&serializer::res_network_status, this, std::placeholders::_1));

			vm.register_function("req.ip.statistics", 0, std::bind(&serializer::req_ip_statistics, this, std::placeholders::_1));
			vm.register_function("res.ip.statistics", 3, std::bind(&serializer::res_ip_statistics, this, std::placeholders::_1));

			vm.register_function("req.device.auth", 0, std::bind(&serializer::req_device_auth, this, std::placeholders::_1));
			vm.register_function("res.device.auth", 5, std::bind(&serializer::res_device_auth, this, std::placeholders::_1));

			vm.register_function("req.device.time", 0, std::bind(&serializer::req_device_time, this, std::placeholders::_1));
			vm.register_function("res.device.time", 1, std::bind(&serializer::res_device_time, this, std::placeholders::_1));

			vm.register_function("req.push.target.namelist", 1, std::bind(&serializer::req_push_target_namelist, this, std::placeholders::_1));
			vm.register_function("res.push.target.namelist", 1, std::bind(&serializer::res_push_target_namelist, this, std::placeholders::_1));

			vm.register_function("req.push.target.echo", 1, std::bind(&serializer::req_push_target_echo, this, std::placeholders::_1));
			vm.register_function("res.push.target.echo", 1, std::bind(&serializer::res_push_target_echo, this, std::placeholders::_1));

			vm.register_function("req.traceroute", 4, std::bind(&serializer::req_traceroute, this, std::placeholders::_1));
			vm.register_function("res.traceroute", 1, std::bind(&serializer::res_traceroute, this, std::placeholders::_1));

			vm.register_function("req.maintenance", 1, std::bind(&serializer::req_maintenance, this, std::placeholders::_1));
			vm.register_function("res.maintenance", 1, std::bind(&serializer::res_maintenance, this, std::placeholders::_1));

			vm.register_function("req.logout", 1, std::bind(&serializer::req_logout, this, std::placeholders::_1));
			vm.register_function("res.logout", 1, std::bind(&serializer::res_logout, this, std::placeholders::_1));

			vm.register_function("req.register.push.target", 3, std::bind(&serializer::req_register_push_target, this, std::placeholders::_1));
			vm.register_function("res.register.push.target", 3, std::bind(&serializer::res_register_push_target, this, std::placeholders::_1));

			vm.register_function("req.deregister.push.target", 1, std::bind(&serializer::req_deregister_push_target, this, std::placeholders::_1));
			vm.register_function("res.deregister.push.target", 3, std::bind(&serializer::res_deregister_push_target, this, std::placeholders::_1));

			vm.register_function("req.watchdog", 1, std::bind(&serializer::req_watchdog, this, std::placeholders::_1));
			vm.register_function("res.watchdog", 1, std::bind(&serializer::res_watchdog, this, std::placeholders::_1));

			vm.register_function("req.multi.ctrl.public.login", 1, std::bind(&serializer::req_multi_ctrl_public_login, this, std::placeholders::_1));
			vm.register_function("res.multi.ctrl.public.login", 1, std::bind(&serializer::req_multi_ctrl_public_login, this, std::placeholders::_1));

			////	control - multi public login request
			//MULTI_CTRL_REQ_LOGIN_SCRAMBLED = 0xC00A,	//!<	request
			//MULTI_CTRL_RES_LOGIN_SCRAMBLED = 0x400A,	//!<	response

			////	server mode
			//SERVER_MODE_REQUEST = 0xC010,	//!<	request
			//SERVER_MODE_RESPONSE = 0x4010,	//!<	response

			////	server mode reconnect
			//SERVER_MODE_RECONNECT_REQUEST = 0xC011,	//!<	request
			//SERVER_MODE_RECONNECT_RESPONSE = 0x4011,	//!<	response

			//UNKNOWN = 0x7fff,	//!<	unknown command
			vm.register_function("res.unknown.command", 1, std::bind(&serializer::res_unknown_command, this, std::placeholders::_1));


		}

		void serializer::set_sk(cyng::context& ctx)
		{
			//	[]
			const cyng::vector_t frame = ctx.get_frame();
			//CYNG_LOG_INFO(logger_, "set_sk " << cyng::io::to_str(frame));

			//	set new scramble key
			scrambler_ = cyng::value_cast(frame.at(0), def_key_).key();

		}

		void serializer::reset(cyng::context& ctx)
		{
			//	[]
			const cyng::vector_t frame = ctx.get_frame();
			//CYNG_LOG_INFO(logger_, "set_sk " << cyng::io::to_str(frame));

			scrambler_.reset();
			sgen_.reset();
			last_seq_ = 0;
			//	reset default scramble key
			//	error: new key is 00000
			def_key_ = cyng::value_cast(frame.at(0), def_key_).key();
		}

		void serializer::push_seq(cyng::context& ctx)
		{
			ctx.push(cyng::make_object(last_seq_));
		}

		void serializer::transfer_data(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			cyng::buffer_t data;
			data = cyng::value_cast(frame.at(0), data);
			write(data);

#ifdef _DEBUG
			ctx.attach(cyng::generate_invoke("log.msg.info", data.size(), "bytes transferred"));
#endif
		}

		void serializer::req_login_public(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			const std::string name = cyng::value_cast<std::string>(frame.at(0), "");
			const std::string pwd = cyng::value_cast<std::string>(frame.at(1), "");
			write_header(code::CTRL_REQ_LOGIN_PUBLIC, 0, name.size() + pwd.size() + 2);
			write(name);
			write(pwd);
		}

		void serializer::req_login_scrambled(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			const std::string name = cyng::value_cast<std::string>(frame.at(0), "");
			const std::string pwd = cyng::value_cast<std::string>(frame.at(1), "");
			const scramble_key key = cyng::value_cast(frame.at(2), def_key_);

			write_header(code::CTRL_REQ_LOGIN_SCRAMBLED, 0, name.size() + pwd.size() + 2 + key.size());

			//	use default scrambled key
			scrambler_ = def_key_.key();

			write(name);
			write(pwd);
			write(key.key());

			//
			//	use new key
			//
			scrambler_ = key.key();
		}

		void serializer::res_login_public(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();

			const response_type res = cyng::value_cast(frame.at(0), response_type(0));
			const std::uint16_t watchdog = cyng::value_cast(frame.at(1), std::uint16_t(23));
			const std::string redirect = cyng::value_cast<std::string>(frame.at(2), "");

			write_header(code::CTRL_RES_LOGIN_PUBLIC, 0, sizeof(res) + sizeof(watchdog) + redirect.size() + 1);

			write_numeric(res);
			write_numeric(watchdog);
			write(redirect);

		}

		void serializer::res_login_scrambled(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();

			const response_type res = cyng::value_cast(frame.at(0), response_type(0));
			const std::uint16_t watchdog = cyng::value_cast(frame.at(1), std::uint16_t(23));
			const std::string redirect = cyng::value_cast<std::string>(frame.at(2), "");

			write_header(code::CTRL_RES_LOGIN_SCRAMBLED, 0, sizeof(res) + sizeof(watchdog) + redirect.size() + 1);

			write_numeric(res);
			write_numeric(watchdog);
			write(redirect);
		}

		void serializer::req_watchdog(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			const sequence_type seq = cyng::value_cast(frame.at(0), sequence_type(0));
			write_header(code::CTRL_REQ_WATCHDOG, seq, 0);
		}

		void serializer::res_watchdog(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			const sequence_type seq = cyng::value_cast(frame.at(0), sequence_type(0));
			write_header(code::CTRL_RES_WATCHDOG, seq, 0);
		}

		void serializer::req_open_push_channel(cyng::context& ctx)
		{
			last_seq_ = sgen_();
			BOOST_ASSERT_MSG(false, "req_open_push_channel is not implemented yet");
		}

		void serializer::res_open_push_channel(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			auto const tpl = cyng::tuple_cast<
				sequence_type,		//	[0] ipt seq
				response_type,		//	[1] response value
				std::uint32_t,		//	[2] channel
				std::uint32_t,		//	[3] source
				std::uint16_t,		//	[4] packet size
				std::uint8_t,		//	[5] window size
				std::uint8_t,		//	[6] status
				std::uint32_t		//	[7] count
			>(frame);

			//static_assert(17 == sizeof(res)
			//		+ sizeof(channel)
			//		+ sizeof(source)
			//		+ sizeof(packet_size)
			//		+ sizeof(window_size)
			//		+ sizeof(status)
			//		+ sizeof(count), "res_open_push_channel(length assumption invalid)");

			//static_assert(18 == sizeof(tpl), "res_open_push_channel(length assumption invalid)");
			//	== 32
			//std::cout << sizeof(tpl) << std::endl;

			write_header(code::TP_RES_OPEN_PUSH_CHANNEL, std::get<0>(tpl), 17);
			write_numeric(std::get<1>(tpl));
			write_numeric(std::get<2>(tpl));
			write_numeric(std::get<3>(tpl));
			write_numeric(std::get<4>(tpl));
			write_numeric(std::get<5>(tpl));
			write_numeric(std::get<6>(tpl));
			write_numeric(std::get<7>(tpl));
		}

		void serializer::req_close_push_channel(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			const std::uint32_t channel = cyng::value_cast<std::uint32_t>(frame.at(0), 0);

			last_seq_ = sgen_();
			write_header(code::TP_REQ_CLOSE_PUSH_CHANNEL, last_seq_, sizeof(channel));
			write_numeric(channel);
		}

		void serializer::res_close_push_channel(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			const sequence_type seq = cyng::value_cast<sequence_type>(frame.at(0), 0);
			const response_type res = cyng::value_cast<response_type>(frame.at(1), 0);
			const std::uint32_t channel = cyng::value_cast<std::uint32_t>(frame.at(2), 0);

			//last_seq_ = sgen_();
			write_header(code::TP_RES_CLOSE_PUSH_CHANNEL, seq, sizeof(res) + sizeof(channel));
			write_numeric(res);
			write_numeric(channel);
		}

		void serializer::req_transfer_push_data(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			const std::uint32_t channel = cyng::value_cast<std::uint32_t>(frame.at(0), 0);
			const std::uint32_t source = cyng::value_cast<std::uint32_t>(frame.at(1), 0);
			const std::uint8_t status = cyng::value_cast<std::uint8_t>(frame.at(2), 0);
			const std::uint8_t block = cyng::value_cast<std::uint8_t>(frame.at(3), 0);
			//const std::uint32_t size = cyng::value_cast<std::uint32_t>(frame.at(4), 0);
			cyng::buffer_t data;
			data = cyng::value_cast<cyng::buffer_t>(frame.at(4), data);
			const std::uint32_t size = data.size();

			last_seq_ = sgen_();
			write_header(code::TP_REQ_PUSHDATA_TRANSFER, last_seq_, sizeof(channel) + sizeof(source) + sizeof(status) + sizeof(block) + sizeof(size) + data.size());
			write_numeric(channel);
			write_numeric(source);
			write_numeric(status);
			write_numeric(block);
			write_numeric(size);
			put(data.data(), data.size());	//	no escaping
		}

		void serializer::res_transfer_push_data(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			auto const tpl = cyng::tuple_cast<
				sequence_type,		//	[0] ipt seq
				response_type,		//	[1] response value
				std::uint32_t,		//	[2] channel
				std::uint32_t,		//	[3] source
				std::uint8_t,		//	[4] status
				std::uint8_t		//	[5] block
			>(frame);

			//static_assert(11 == sizeof(res)
			//	+ sizeof(channel)
			//	+ sizeof(source)
			//	+ sizeof(status)
			//	+ sizeof(block), "res_transfer_push_data(length assumption invalid)");

			write_header(code::TP_RES_PUSHDATA_TRANSFER, std::get<0>(tpl), 11);

			write_numeric(std::get<1>(tpl));
			write_numeric(std::get<2>(tpl));
			write_numeric(std::get<3>(tpl));
			write_numeric(std::get<4>(tpl));
			write_numeric(std::get<5>(tpl));

		}

		void serializer::req_open_connection(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			const std::string number = cyng::value_cast<std::string>(frame.at(0), "");

			last_seq_ = sgen_();
			write_header(code::TP_REQ_OPEN_CONNECTION, last_seq_, number.size() + 1);
			write(number);
		}

		void serializer::res_open_connection(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			const sequence_type seq = cyng::value_cast<sequence_type>(frame.at(0), 0);
			const response_type res = cyng::value_cast<response_type>(frame.at(1), 0);
			write_header(code::TP_RES_OPEN_CONNECTION, seq, sizeof(res));
			write_numeric(res);
		}

		void serializer::req_close_connection(cyng::context& ctx)
		{
			last_seq_ = sgen_();
			write_header(code::TP_REQ_CLOSE_CONNECTION, last_seq_, 0);
		}

		void serializer::res_close_connection(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			const sequence_type seq = cyng::value_cast<sequence_type>(frame.at(0), 0);
			const response_type res = cyng::value_cast<response_type>(frame.at(1), 0);
			write_header(code::TP_RES_CLOSE_CONNECTION, seq, sizeof(res));
			write_numeric(res);
		}

		void serializer::req_protocol_version(cyng::context& ctx)
		{
			last_seq_ = sgen_();
			write_header(code::APP_REQ_PROTOCOL_VERSION, last_seq_, 0);
		}

		void serializer::res_protocol_version(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			const sequence_type seq = cyng::value_cast<sequence_type>(frame.at(0), 0);
			const response_type res = cyng::value_cast<response_type>(frame.at(1), 0);
			write_header(code::APP_RES_PROTOCOL_VERSION, seq, sizeof(res));
			write_numeric(res);
		}

		void serializer::req_software_version(cyng::context& ctx)
		{
			last_seq_ = sgen_();
			write_header(code::APP_REQ_SOFTWARE_VERSION, last_seq_, 0);
		}

		void serializer::res_software_version(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			const sequence_type seq = cyng::value_cast<sequence_type>(frame.at(0), 0);
			const std::string ver = cyng::value_cast<std::string >(frame.at(1), "");
			write_header(code::APP_RES_SOFTWARE_VERSION, seq, ver.size() + 1);
			write(ver);
		}

		void serializer::req_device_id(cyng::context& ctx)
		{
			last_seq_ = sgen_();
			write_header(code::APP_REQ_DEVICE_IDENTIFIER, last_seq_, 0);
		}

		void serializer::res_device_id(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			const sequence_type seq = cyng::value_cast<sequence_type>(frame.at(0), 0);
			const std::string id = cyng::value_cast<std::string >(frame.at(1), "");
			write_header(code::APP_RES_DEVICE_IDENTIFIER, seq, id.size() + 1);
			write(id);
		}

		void serializer::req_network_status(cyng::context& ctx)
		{
			last_seq_ = sgen_();
			write_header(code::APP_REQ_NETWORK_STATUS, last_seq_, 0);
		}

		void serializer::res_network_status(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			const sequence_type seq = cyng::value_cast<sequence_type>(frame.at(0), 0);
			const std::uint8_t dev = cyng::value_cast<std::uint8_t>(frame.at(1), 0);
			const std::uint32_t stat_1 = cyng::value_cast<std::uint32_t>(frame.at(2), 0);
			const std::uint32_t stat_2 = cyng::value_cast<std::uint32_t>(frame.at(3), 0);
			const std::uint32_t stat_3 = cyng::value_cast<std::uint32_t>(frame.at(4), 0);
			const std::uint32_t stat_4 = cyng::value_cast<std::uint32_t>(frame.at(5), 0);
			const std::uint32_t stat_5 = cyng::value_cast<std::uint32_t>(frame.at(6), 0);
			const std::string imsi = cyng::value_cast<std::string >(frame.at(7), "");
			const std::string imei = cyng::value_cast<std::string >(frame.at(8), "");
			write_header(code::APP_RES_NETWORK_STATUS, seq, sizeof(dev) + (sizeof(std::uint32_t) * 5) + imsi.size() + imei.size() + 2);
			write_numeric(dev);
			write_numeric(stat_1);
			write_numeric(stat_2);
			write_numeric(stat_3);
			write_numeric(stat_4);
			write_numeric(stat_5);
			write(imsi);
			write(imei);
		}

		void serializer::req_ip_statistics(cyng::context& ctx)
		{
			last_seq_ = sgen_();
			write_header(code::APP_REQ_IP_STATISTICS, last_seq_, 0);
		}

		void serializer::res_ip_statistics(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			const sequence_type seq = cyng::value_cast<sequence_type>(frame.at(0), 0);
			const response_type res = cyng::value_cast<response_type>(frame.at(1), 0);
			const std::uint64_t rx = cyng::value_cast<std::uint64_t >(frame.at(2), 0);
			const std::uint64_t sx = cyng::value_cast<std::uint64_t >(frame.at(3), 0);
			write_header(code::APP_RES_SOFTWARE_VERSION, seq, sizeof(res) + sizeof(rx) + sizeof(sx));
			write_numeric(res);
			write_numeric(rx);
			write_numeric(sx);
		}

		void serializer::req_device_auth(cyng::context& ctx)
		{
			last_seq_ = sgen_();
			write_header(code::APP_REQ_DEVICE_AUTHENTIFICATION, last_seq_, 0);
		}

		void serializer::res_device_auth(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			const sequence_type seq = cyng::value_cast<sequence_type>(frame.at(0), 0);
			const std::string account = cyng::value_cast<std::string >(frame.at(1), "");
			const std::string password = cyng::value_cast<std::string >(frame.at(2), "");
			const std::string number = cyng::value_cast<std::string >(frame.at(3), "");
			const std::string description = cyng::value_cast<std::string >(frame.at(4), "");
			write_header(code::APP_RES_DEVICE_AUTHENTIFICATION, seq, account.size() + password.size() + number.size() + description.size() + 4);
			write(account);
			write(password);
			write(number);
			write(description);
		}

		void serializer::req_device_time(cyng::context& ctx)
		{
			last_seq_ = sgen_();
			write_header(code::APP_REQ_DEVICE_TIME, last_seq_, 0);
		}

		void serializer::res_device_time(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			const sequence_type seq = cyng::value_cast<sequence_type>(frame.at(0), 0);
#if BOOST_OS_WINDOWS
			std::uint32_t now = (std::uint32_t) ::_time32(NULL);	//	C4244
#else
			std::uint32_t now = (std::uint32_t) ::time(NULL);
#endif	//	_WIN32
			write_header(code::APP_RES_DEVICE_TIME, seq, sizeof(std::uint32_t));
			write_numeric(now);
		}

		void serializer::req_push_target_namelist(cyng::context& ctx)
		{
			BOOST_ASSERT_MSG(false, "push target namelist is not implemented yet");
		}

		void serializer::res_push_target_namelist(cyng::context& ctx)
		{
			BOOST_ASSERT_MSG(false, "push target namelist is not implemented yet");
		}

		void serializer::req_push_target_echo(cyng::context& ctx)
		{
			BOOST_ASSERT_MSG(false, "push target echo is deprecated");
		}

		void serializer::res_push_target_echo(cyng::context& ctx)
		{
			BOOST_ASSERT_MSG(false, "push target echo is deprecated");
		}

		void serializer::req_traceroute(cyng::context& ctx)
		{
			BOOST_ASSERT_MSG(false, "traceroute is deprecated");

			const cyng::vector_t frame = ctx.get_frame();
			const std::uint8_t ipt_link = cyng::value_cast<std::uint8_t >(frame.at(0), 0);
			const std::uint16_t traceroute_idx = cyng::value_cast<std::uint16_t >(frame.at(1), 0);
			const std::uint16_t hop_counter = cyng::value_cast<std::uint16_t >(frame.at(2), 0);
			const std::uint32_t channel = cyng::value_cast<std::uint32_t >(frame.at(3), 0);

			last_seq_ = sgen_();
			write_header(code::APP_REQ_TRACEROUTE, last_seq_, sizeof(ipt_link) + sizeof(traceroute_idx) + sizeof(hop_counter) + sizeof(channel));

			write_numeric(ipt_link);
			write_numeric(traceroute_idx);
			write_numeric(hop_counter);
			write_numeric(channel);

		}

		void serializer::res_traceroute(cyng::context& ctx)
		{
			BOOST_ASSERT_MSG(false, "traceroute is deprecated");
		}

		void serializer::req_maintenance(cyng::context& ctx)
		{
			BOOST_ASSERT_MSG(false, "maintenance is deprecated");
		}

		void serializer::res_maintenance(cyng::context& ctx)
		{
			BOOST_ASSERT_MSG(false, "maintenance is deprecated");
		}

		void serializer::req_logout(cyng::context& ctx)
		{
			BOOST_ASSERT_MSG(false, "logout is deprecated");
		}

		void serializer::res_logout(cyng::context& ctx)
		{
			BOOST_ASSERT_MSG(false, "logout is deprecated");
		}

		void serializer::req_register_push_target(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			const std::string target = cyng::value_cast<std::string >(frame.at(0), "");
			const std::uint16_t p_size = cyng::value_cast<std::uint16_t >(frame.at(1), 0xffff);
			const std::uint8_t w_size = cyng::value_cast<std::uint8_t >(frame.at(2), 1);
			last_seq_ = sgen_();
			write_header(code::CTRL_REQ_REGISTER_TARGET, last_seq_, target.size() + 1 + sizeof(p_size) + w_size);
			write(target);
			write_numeric(p_size);
			write_numeric(w_size);
		}

		void serializer::res_register_push_target(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			const sequence_type seq = cyng::value_cast<sequence_type>(frame.at(0), 0);
			const response_type res = cyng::value_cast<response_type>(frame.at(1), 0);
			const std::uint32_t channel = cyng::value_cast<std::uint32_t >(frame.at(2), 0);
			write_header(code::CTRL_RES_REGISTER_TARGET, seq, sizeof(res) + sizeof(channel));
			write_numeric(res);
			write_numeric(channel);
		}

		void serializer::req_deregister_push_target(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			const std::string target = cyng::value_cast<std::string >(frame.at(0), "");
			last_seq_ = sgen_();
			write_header(code::CTRL_REQ_DEREGISTER_TARGET, last_seq_, target.size() + 1);
			write(target);
		}

		void serializer::res_deregister_push_target(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			const sequence_type seq = cyng::value_cast<sequence_type>(frame.at(0), 0);
			const response_type res = cyng::value_cast<response_type>(frame.at(1), 0);
			const std::string target = cyng::value_cast<std::string >(frame.at(2), "");
			write_header(code::CTRL_RES_DEREGISTER_TARGET, seq, sizeof(res) + target.size() + 1);
			write_numeric(res);
			write(target);
		}

		void serializer::req_multi_ctrl_public_login(cyng::context& ctx)
		{
			BOOST_ASSERT_MSG(false, "req_multi_ctrl_public_login is not implemented yet");
		}

		void serializer::res_multi_ctrl_public_login(cyng::context& ctx)
		{
			BOOST_ASSERT_MSG(false, "req_multi_ctrl_public_login is not implemented yet");
		}

		void serializer::res_unknown_command(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			const sequence_type seq = cyng::value_cast<sequence_type>(frame.at(0), 0);
			const command_type cmd = cyng::value_cast<command_type>(frame.at(1), 0);
			write_header(code::UNKNOWN, seq, sizeof(cmd));
			write_numeric(cmd);
		}

		void serializer::write_header(command_type cmd, sequence_type seq, std::size_t length)
		{
			switch (cmd)
			{
			case code::CTRL_RES_LOGIN_PUBLIC:	case code::CTRL_RES_LOGIN_SCRAMBLED:	//	login response
			case code::CTRL_REQ_LOGIN_PUBLIC:	case code::CTRL_REQ_LOGIN_SCRAMBLED:	//	login request
				break;
			default:
				//	commands are starting with an escape 
				write_numeric<std::uint8_t>(ESCAPE_SIGN);
				break;
			}

			write_numeric(cmd);
			write_numeric(seq);
			write_numeric<std::uint8_t>(0);
			write_numeric(static_cast<std::uint32_t>(length + HEADER_SIZE));
		}

		void serializer::write(std::string const& v)
		{
			put(v.c_str(), v.length());
			write_numeric<std::uint8_t>(0);
		}

		void serializer::write(scramble_key::key_type const& key)
		{
			std::for_each(key.begin(), key.end(), [this](char c) {
				put(c);
			});
		}

		void serializer::write(cyng::buffer_t const& data)
		{
			for (auto c : data)
			{
				if (c == ESCAPE_SIGN)
				{
					//	duplicate escapes
					put(c);
				}
				put(c);
			}
		}
		void serializer::put(const char* p, std::size_t size)
		{
			std::for_each(p, p + size, [this](char c) {
				put(c);
			});

		}
		void serializer::put(char c)
		{
			ostream_.put(scrambler_[c]);
		}


	}
}
