/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#include <smf/modem/serializer.h>
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
	namespace modem
	{
		serializer::serializer(boost::asio::ip::tcp::socket& s
			, cyng::controller& vm)
			: buffer_()
			, ostream_(&buffer_)
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
			//	after reconnect
			//
			vm.register_function("ipt.reset.serializer", 1, std::bind(&serializer::reset, this, std::placeholders::_1));

			//
			//	transfer data
			//
			vm.register_function("modem.transfer.data", 1, std::bind(&serializer::transfer_data, this, std::placeholders::_1));

			//
			//	hayes/AT specific commands
			//
			vm.register_function("print.ok", 0, std::bind(&serializer::print_msg, this, std::placeholders::_1, "OK"));
			vm.register_function("print.error", 0, std::bind(&serializer::print_msg, this, std::placeholders::_1, "ERROR"));
			vm.register_function("print.busy", 0, std::bind(&serializer::print_msg, this, std::placeholders::_1, "BUSY"));
			vm.register_function("print.no-dialtone", 0, std::bind(&serializer::print_msg, this, std::placeholders::_1, "NO DIALTONE"));
			vm.register_function("print.no-carrier", 0, std::bind(&serializer::print_msg, this, std::placeholders::_1, "NO CARRIER"));
			vm.register_function("print.no-answer", 0, std::bind(&serializer::print_msg, this, std::placeholders::_1, "NO ANSWER"));
			vm.register_function("print.ring", 0, std::bind(&serializer::print_msg, this, std::placeholders::_1, "RING"));
			vm.register_function("print.connect", 0, std::bind(&serializer::print_msg, this, std::placeholders::_1, "CONNECT"));

			//
			//	generated from master node (mostly)
			//
			vm.register_function("req.login.public", 2, std::bind(&serializer::req_login_public, this, std::placeholders::_1));
			vm.register_function("req.login.scrambled", 3, std::bind(&serializer::req_login_scrambled, this, std::placeholders::_1));
			vm.register_function("res.login.public", 2, std::bind(&serializer::res_login_public, this, std::placeholders::_1));
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


		void serializer::reset(cyng::context& ctx)
		{
			//	[]
			const cyng::vector_t frame = ctx.get_frame();
			//CYNG_LOG_INFO(logger_, "set_sk " << cyng::io::to_str(frame));

		}

		void serializer::transfer_data(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			cyng::buffer_t data;
			data = cyng::value_cast(frame.at(0), data);
			write(data);

#ifdef _DEBUG
			ctx.attach(cyng::generate_invoke("log.msg.info", data.size(), "modem bytes transferred"));
#endif
		}

		void serializer::print_msg(cyng::context& ctx, const char* msg)
		{
			write(std::string(msg));
			//	append <CR> <NL>
			write(std::string("\r\n"));
		}

		void serializer::req_login_public(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			const std::string name = cyng::value_cast<std::string>(frame.at(0), "");
			const std::string pwd = cyng::value_cast<std::string>(frame.at(1), "");
		}

		void serializer::req_login_scrambled(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			const std::string name = cyng::value_cast<std::string>(frame.at(0), "");
			const std::string pwd = cyng::value_cast<std::string>(frame.at(1), "");
		}

		void serializer::res_login_public(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();


		}

		void serializer::res_login_scrambled(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();

		}

		void serializer::req_watchdog(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
		}

		void serializer::res_watchdog(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
		}

		void serializer::req_open_push_channel(cyng::context& ctx)
		{
		}

		void serializer::res_open_push_channel(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
		}

		void serializer::req_close_push_channel(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			const std::uint32_t channel = cyng::value_cast<std::uint32_t>(frame.at(0), 0);
		}

		void serializer::res_close_push_channel(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
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

		}

		void serializer::res_transfer_push_data(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
		}

		void serializer::req_open_connection(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			const std::string number = cyng::value_cast<std::string>(frame.at(0), "");

		}

		void serializer::res_open_connection(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
		}

		void serializer::req_close_connection(cyng::context& ctx)
		{
		}

		void serializer::res_close_connection(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
		}

		void serializer::req_protocol_version(cyng::context& ctx)
		{
		}

		void serializer::res_protocol_version(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
		}

		void serializer::req_software_version(cyng::context& ctx)
		{
		}

		void serializer::res_software_version(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
		}

		void serializer::req_device_id(cyng::context& ctx)
		{
		}

		void serializer::res_device_id(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
		}

		void serializer::req_network_status(cyng::context& ctx)
		{
		}

		void serializer::res_network_status(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
		}

		void serializer::req_ip_statistics(cyng::context& ctx)
		{
		}

		void serializer::res_ip_statistics(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
		}

		void serializer::req_device_auth(cyng::context& ctx)
		{
		}

		void serializer::res_device_auth(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
		}

		void serializer::req_device_time(cyng::context& ctx)
		{
		}

		void serializer::res_device_time(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
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
		}

		void serializer::res_register_push_target(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
		}

		void serializer::req_deregister_push_target(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			const std::string target = cyng::value_cast<std::string >(frame.at(0), "");
		}

		void serializer::res_deregister_push_target(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
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
		}


		void serializer::write(std::string const& v)
		{
			put(v.c_str(), v.length());
			//write_numeric<std::uint8_t>(0);
		}

		void serializer::write(cyng::buffer_t const& data)
		{
			put(data.data(), data.size());
		}
		void serializer::put(const char* p, std::size_t size)
		{
			std::for_each(p, p + size, [this](char c) {
				put(c);
			});
		}

		void serializer::put(char c)
		{
			ostream_.put(c);
		}


	}
}
