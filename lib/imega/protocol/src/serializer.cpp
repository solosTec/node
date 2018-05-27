/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#include <smf/imega/serializer.h>
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
	namespace imega
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
			vm.register_function("imega.transfer.data", 1, std::bind(&serializer::transfer_data, this, std::placeholders::_1));


			//
			//	generated from master node (mostly)
			//
			//vm.register_function("req.login.public", 2, std::bind(&serializer::req_login_public, this, std::placeholders::_1));
			//vm.register_function("req.login.scrambled", 3, std::bind(&serializer::req_login_scrambled, this, std::placeholders::_1));
			//vm.register_function("res.login.public", 2, std::bind(&serializer::res_login_public, this, std::placeholders::_1));
			//vm.register_function("res.login.scrambled", 3, std::bind(&serializer::res_login_scrambled, this, std::placeholders::_1));

			//vm.register_function("req.open.push.channel", 1, std::bind(&serializer::req_open_push_channel, this, std::placeholders::_1));
			//vm.register_function("res.open.push.channel", 8, std::bind(&serializer::res_open_push_channel, this, std::placeholders::_1));

			//vm.register_function("req.close.push.channel", 1, std::bind(&serializer::req_close_push_channel, this, std::placeholders::_1));
			//vm.register_function("res.close.push.channel", 3, std::bind(&serializer::res_close_push_channel, this, std::placeholders::_1));

			//vm.register_function("req.transfer.push.data", 5, std::bind(&serializer::req_transfer_push_data, this, std::placeholders::_1));
			//vm.register_function("res.transfer.push.data", 6, std::bind(&serializer::res_transfer_push_data, this, std::placeholders::_1));

			//vm.register_function("req.open.connection", 1, std::bind(&serializer::req_open_connection, this, std::placeholders::_1));
			//vm.register_function("res.open.connection", 2, std::bind(&serializer::res_open_connection, this, std::placeholders::_1));

			//vm.register_function("req.close.connection", 0, std::bind(&serializer::req_close_connection, this, std::placeholders::_1));
			//vm.register_function("res.close.connection", 2, std::bind(&serializer::res_close_connection, this, std::placeholders::_1));

			//vm.register_function("req.protocol.version", 0, std::bind(&serializer::req_protocol_version, this, std::placeholders::_1));
			//vm.register_function("res.protocol.version", 2, std::bind(&serializer::res_protocol_version, this, std::placeholders::_1));

			//vm.register_function("req.software.version", 0, std::bind(&serializer::req_software_version, this, std::placeholders::_1));
			//vm.register_function("res.software.version", 2, std::bind(&serializer::res_software_version, this, std::placeholders::_1));

			//vm.register_function("req.device.id", 0, std::bind(&serializer::req_device_id, this, std::placeholders::_1));
			//vm.register_function("res.device.id", 2, std::bind(&serializer::res_device_id, this, std::placeholders::_1));

			//vm.register_function("req.net.status", 0, std::bind(&serializer::req_network_status, this, std::placeholders::_1));
			//vm.register_function("res.net.status", 9, std::bind(&serializer::res_network_status, this, std::placeholders::_1));

			//vm.register_function("req.ip.statistics", 0, std::bind(&serializer::req_ip_statistics, this, std::placeholders::_1));
			//vm.register_function("res.ip.statistics", 3, std::bind(&serializer::res_ip_statistics, this, std::placeholders::_1));

			//vm.register_function("req.device.auth", 0, std::bind(&serializer::req_device_auth, this, std::placeholders::_1));
			//vm.register_function("res.device.auth", 5, std::bind(&serializer::res_device_auth, this, std::placeholders::_1));

			//vm.register_function("req.device.time", 0, std::bind(&serializer::req_device_time, this, std::placeholders::_1));
			//vm.register_function("res.device.time", 1, std::bind(&serializer::res_device_time, this, std::placeholders::_1));

			//vm.register_function("req.push.target.namelist", 1, std::bind(&serializer::req_push_target_namelist, this, std::placeholders::_1));
			//vm.register_function("res.push.target.namelist", 1, std::bind(&serializer::res_push_target_namelist, this, std::placeholders::_1));

			//vm.register_function("req.push.target.echo", 1, std::bind(&serializer::req_push_target_echo, this, std::placeholders::_1));
			//vm.register_function("res.push.target.echo", 1, std::bind(&serializer::res_push_target_echo, this, std::placeholders::_1));

			//vm.register_function("req.traceroute", 4, std::bind(&serializer::req_traceroute, this, std::placeholders::_1));
			//vm.register_function("res.traceroute", 1, std::bind(&serializer::res_traceroute, this, std::placeholders::_1));

			//vm.register_function("req.maintenance", 1, std::bind(&serializer::req_maintenance, this, std::placeholders::_1));
			//vm.register_function("res.maintenance", 1, std::bind(&serializer::res_maintenance, this, std::placeholders::_1));

			//vm.register_function("req.logout", 1, std::bind(&serializer::req_logout, this, std::placeholders::_1));
			//vm.register_function("res.logout", 1, std::bind(&serializer::res_logout, this, std::placeholders::_1));

			//vm.register_function("req.register.push.target", 3, std::bind(&serializer::req_register_push_target, this, std::placeholders::_1));
			//vm.register_function("res.register.push.target", 3, std::bind(&serializer::res_register_push_target, this, std::placeholders::_1));

			//vm.register_function("req.deregister.push.target", 1, std::bind(&serializer::req_deregister_push_target, this, std::placeholders::_1));
			//vm.register_function("res.deregister.push.target", 3, std::bind(&serializer::res_deregister_push_target, this, std::placeholders::_1));

			//vm.register_function("req.watchdog", 1, std::bind(&serializer::req_watchdog, this, std::placeholders::_1));
			//vm.register_function("res.watchdog", 1, std::bind(&serializer::res_watchdog, this, std::placeholders::_1));

			//vm.register_function("req.multi.ctrl.public.login", 1, std::bind(&serializer::req_multi_ctrl_public_login, this, std::placeholders::_1));
			//vm.register_function("res.multi.ctrl.public.login", 1, std::bind(&serializer::req_multi_ctrl_public_login, this, std::placeholders::_1));

			//UNKNOWN = 0x7fff,	//!<	unknown command
			//vm.register_function("res.unknown.command", 1, std::bind(&serializer::res_unknown_command, this, std::placeholders::_1));


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
			ctx.attach(cyng::generate_invoke("log.msg.info", data.size(), "imega bytes transferred"));
#endif
		}


		void serializer::write(std::string const& v)
		{
			put(v.c_str(), v.length());
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
