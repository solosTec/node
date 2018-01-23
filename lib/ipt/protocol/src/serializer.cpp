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
			vm.async_run(cyng::register_function("stream.flush", 0, [this, &s](cyng::context& ctx) {

#ifdef SMF_IO_DEBUG
				//	get content of buffer
				boost::asio::const_buffer cbuffer(*buffer_.data().begin());
				const char* p = boost::asio::buffer_cast<const char*>(cbuffer);
				const std::size_t size = boost::asio::buffer_size(cbuffer);

				cyng::io::hex_dump hd;
				hd(std::cerr, p, p + size);
#endif

				boost::system::error_code ec;
				boost::asio::write(s, buffer_, ec);
				ctx.set_register(ec);
			}));

			vm.async_run(cyng::register_function("stream.serialize", 0, [this](cyng::context& ctx) {

				const cyng::vector_t frame = ctx.get_frame();
				for (auto obj : frame)
				{
					cyng::io::serialize_plain(std::cerr, obj);
					cyng::io::serialize_binary(ostream_, obj);
				}

			}));

			//
			//	generated from parser
			//
			vm.async_run(cyng::register_function("ipt.set.sk.def", 1, std::bind(&serializer::set_sk, this, std::placeholders::_1)));

			//
			//	generated from master node (mostly)
			//
			vm.async_run(cyng::register_function("req.login.public", 2, std::bind(&serializer::req_login_public, this, std::placeholders::_1)));
			vm.async_run(cyng::register_function("req.login.scrambled", 3, std::bind(&serializer::req_login_scrambled, this, std::placeholders::_1)));
			vm.async_run(cyng::register_function("res.login.public", 2, std::bind(&serializer::res_login_public, this, std::placeholders::_1)));
			vm.async_run(cyng::register_function("res.login.scrambled", 3, std::bind(&serializer::res_login_scrambled, this, std::placeholders::_1)));

			vm.async_run(cyng::register_function("req.open.push.channel", 1, std::bind(&serializer::req_open_push_channel, this, std::placeholders::_1)));
			vm.async_run(cyng::register_function("res.open.push.channel", 1, std::bind(&serializer::res_open_push_channel, this, std::placeholders::_1)));

			vm.async_run(cyng::register_function("req.close.push.channel", 1, std::bind(&serializer::req_close_push_channel, this, std::placeholders::_1)));
			vm.async_run(cyng::register_function("res.close.push.channel", 1, std::bind(&serializer::res_close_push_channel, this, std::placeholders::_1)));

			vm.async_run(cyng::register_function("req.transfer.push.data", 1, std::bind(&serializer::req_transfer_push_data, this, std::placeholders::_1)));
			vm.async_run(cyng::register_function("res.transfer.push.data", 1, std::bind(&serializer::res_transfer_push_data, this, std::placeholders::_1)));

			vm.async_run(cyng::register_function("req.open.connection", 1, std::bind(&serializer::req_open_connection, this, std::placeholders::_1)));
			vm.async_run(cyng::register_function("res.open.connection", 1, std::bind(&serializer::res_open_connection, this, std::placeholders::_1)));

			vm.async_run(cyng::register_function("req.close.connection", 1, std::bind(&serializer::req_close_connection, this, std::placeholders::_1)));
			vm.async_run(cyng::register_function("res.close.connection", 1, std::bind(&serializer::res_close_connection, this, std::placeholders::_1)));

			vm.async_run(cyng::register_function("req.protocol.version", 1, std::bind(&serializer::req_protocol_version, this, std::placeholders::_1)));
			vm.async_run(cyng::register_function("res.protocol.version", 1, std::bind(&serializer::res_protocol_version, this, std::placeholders::_1)));

			vm.async_run(cyng::register_function("req.software.version", 1, std::bind(&serializer::req_software_version, this, std::placeholders::_1)));
			vm.async_run(cyng::register_function("res.software.version", 1, std::bind(&serializer::res_software_version, this, std::placeholders::_1)));

			vm.async_run(cyng::register_function("req.device.id", 1, std::bind(&serializer::req_device_id, this, std::placeholders::_1)));
			vm.async_run(cyng::register_function("res.device.id", 1, std::bind(&serializer::res_device_id, this, std::placeholders::_1)));

			vm.async_run(cyng::register_function("req.network.status", 1, std::bind(&serializer::req_network_status, this, std::placeholders::_1)));
			vm.async_run(cyng::register_function("res.network.status", 1, std::bind(&serializer::res_network_status, this, std::placeholders::_1)));

			vm.async_run(cyng::register_function("req.ip.statistics", 1, std::bind(&serializer::req_ip_statistics, this, std::placeholders::_1)));
			vm.async_run(cyng::register_function("res.ip.statistics", 1, std::bind(&serializer::res_ip_statistics, this, std::placeholders::_1)));

			vm.async_run(cyng::register_function("req.device.auth", 1, std::bind(&serializer::req_device_auth, this, std::placeholders::_1)));
			vm.async_run(cyng::register_function("res.device.auth", 1, std::bind(&serializer::res_device_auth, this, std::placeholders::_1)));

			vm.async_run(cyng::register_function("req.device.time", 1, std::bind(&serializer::req_device_time, this, std::placeholders::_1)));
			vm.async_run(cyng::register_function("res.device.time", 1, std::bind(&serializer::res_device_time, this, std::placeholders::_1)));

			vm.async_run(cyng::register_function("req.push.target.namelist", 1, std::bind(&serializer::req_push_target_namelist, this, std::placeholders::_1)));
			vm.async_run(cyng::register_function("res.push.target.namelist", 1, std::bind(&serializer::res_push_target_namelist, this, std::placeholders::_1)));

			vm.async_run(cyng::register_function("req.push.target.echo", 1, std::bind(&serializer::req_push_target_echo, this, std::placeholders::_1)));
			vm.async_run(cyng::register_function("res.push.target.echo", 1, std::bind(&serializer::res_push_target_echo, this, std::placeholders::_1)));

			vm.async_run(cyng::register_function("req.traceroute", 1, std::bind(&serializer::req_traceroute, this, std::placeholders::_1)));
			vm.async_run(cyng::register_function("res.traceroute", 1, std::bind(&serializer::res_traceroute, this, std::placeholders::_1)));

			vm.async_run(cyng::register_function("req.maintenance", 1, std::bind(&serializer::req_maintenance, this, std::placeholders::_1)));
			vm.async_run(cyng::register_function("res.maintenance", 1, std::bind(&serializer::res_maintenance, this, std::placeholders::_1)));

			vm.async_run(cyng::register_function("req.logout", 1, std::bind(&serializer::req_logout, this, std::placeholders::_1)));
			vm.async_run(cyng::register_function("res.logout", 1, std::bind(&serializer::res_logout, this, std::placeholders::_1)));

			vm.async_run(cyng::register_function("req.register.push.target", 1, std::bind(&serializer::req_register_push_target, this, std::placeholders::_1)));
			vm.async_run(cyng::register_function("res.register.push.target", 1, std::bind(&serializer::res_register_push_target, this, std::placeholders::_1)));

			vm.async_run(cyng::register_function("req.deregister.push.target", 1, std::bind(&serializer::req_deregister_push_target, this, std::placeholders::_1)));
			vm.async_run(cyng::register_function("res.deregister.push.target", 1, std::bind(&serializer::res_deregister_push_target, this, std::placeholders::_1)));

			vm.async_run(cyng::register_function("req.deregister.push.target", 1, std::bind(&serializer::req_deregister_push_target, this, std::placeholders::_1)));
			vm.async_run(cyng::register_function("res.deregister.push.target", 1, std::bind(&serializer::res_deregister_push_target, this, std::placeholders::_1)));

			vm.async_run(cyng::register_function("req.watchdog", 1, std::bind(&serializer::req_watchdog, this, std::placeholders::_1)));
			vm.async_run(cyng::register_function("res.watchdog", 1, std::bind(&serializer::res_watchdog, this, std::placeholders::_1)));

			vm.async_run(cyng::register_function("req.multi.ctrl.public.login", 1, std::bind(&serializer::req_multi_ctrl_public_login, this, std::placeholders::_1)));
			vm.async_run(cyng::register_function("res.multi.ctrl.public.login", 1, std::bind(&serializer::req_multi_ctrl_public_login, this, std::placeholders::_1)));

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


		}

		void serializer::set_sk(cyng::context& ctx)
		{
			//	[]
			const cyng::vector_t frame = ctx.get_frame();
			//CYNG_LOG_INFO(logger_, "set_sk " << cyng::io::to_str(frame));

			//	set new scramble key
			scrambler_ = cyng::value_cast(frame.at(0), def_key_).key();

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
		}

		void serializer::res_open_push_channel(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			const sequence_type seq = cyng::value_cast<sequence_type>(frame.at(0), 0);
			const response_type res = cyng::value_cast<response_type>(frame.at(0), 0);
			const std::uint32_t channel = cyng::value_cast<uint32_t>(frame.at(1), 0);
			const std::uint32_t source = cyng::value_cast<uint32_t>(frame.at(1), 0);
			const std::uint16_t packet_size = cyng::value_cast<uint16_t>(frame.at(1), 0);
			const std::uint8_t window_size = cyng::value_cast<uint8_t>(frame.at(1), 0);
			const std::uint8_t status = cyng::value_cast<uint8_t>(frame.at(1), 0);
			const std::uint32_t count = cyng::value_cast<uint32_t>(frame.at(1), 0);

			static_assert(17 == sizeof(res)
					+ sizeof(channel)
					+ sizeof(source)
					+ sizeof(packet_size)
					+ sizeof(window_size)
					+ sizeof(status)
					+ sizeof(count), "res_open_push_channel(length assumption invalid)");

			write_header(code::TP_RES_OPEN_PUSH_CHANNEL, seq, 17);
			write_numeric(res);
			write_numeric(channel);
			write_numeric(source);
			write_numeric(packet_size);
			write_numeric(window_size);
			write_numeric(status);
			write_numeric(count);
		}

		void serializer::req_close_push_channel(cyng::context& ctx)
		{

		}

		void serializer::res_close_push_channel(cyng::context& ctx)
		{

		}

		void serializer::req_transfer_push_data(cyng::context& ctx)
		{

		}

		void serializer::res_transfer_push_data(cyng::context& ctx)
		{

		}


		void serializer::req_open_connection(cyng::context& ctx)
		{

		}

		void serializer::res_open_connection(cyng::context& ctx)
		{

		}

		void serializer::req_close_connection(cyng::context& ctx)
		{

		}

		void serializer::res_close_connection(cyng::context& ctx)
		{

		}

		void serializer::req_protocol_version(cyng::context& ctx)
		{

		}

		void serializer::res_protocol_version(cyng::context& ctx)
		{

		}

		void serializer::req_software_version(cyng::context& ctx)
		{

		}

		void serializer::res_software_version(cyng::context& ctx)
		{

		}

		void serializer::req_device_id(cyng::context& ctx)
		{

		}

		void serializer::res_device_id(cyng::context& ctx)
		{

		}

		void serializer::req_network_status(cyng::context& ctx)
		{

		}

		void serializer::res_network_status(cyng::context& ctx)
		{

		}

		void serializer::req_ip_statistics(cyng::context& ctx)
		{

		}

		void serializer::res_ip_statistics(cyng::context& ctx)
		{

		}

		void serializer::req_device_auth(cyng::context& ctx)
		{

		}

		void serializer::res_device_auth(cyng::context& ctx)
		{

		}

		void serializer::req_device_time(cyng::context& ctx)
		{

		}

		void serializer::res_device_time(cyng::context& ctx)
		{

		}

		void serializer::req_push_target_namelist(cyng::context& ctx)
		{

		}

		void serializer::res_push_target_namelist(cyng::context& ctx)
		{

		}

		void serializer::req_push_target_echo(cyng::context& ctx)
		{

		}

		void serializer::res_push_target_echo(cyng::context& ctx)
		{

		}

		void serializer::req_traceroute(cyng::context& ctx)
		{

		}

		void serializer::res_traceroute(cyng::context& ctx)
		{

		}

		void serializer::req_maintenance(cyng::context& ctx)
		{

		}

		void serializer::res_maintenance(cyng::context& ctx)
		{

		}

		void serializer::req_logout(cyng::context& ctx)
		{

		}

		void serializer::res_logout(cyng::context& ctx)
		{

		}

		void serializer::req_register_push_target(cyng::context& ctx)
		{

		}

		void serializer::res_register_push_target(cyng::context& ctx)
		{

		}

		void serializer::req_deregister_push_target(cyng::context& ctx)
		{

		}

		void serializer::res_deregister_push_target(cyng::context& ctx)
		{

		}

		void serializer::req_multi_ctrl_public_login(cyng::context& ctx)
		{

		}

		void serializer::res_multi_ctrl_public_login(cyng::context& ctx)
		{

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
