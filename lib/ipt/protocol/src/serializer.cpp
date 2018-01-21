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

			vm.async_run(cyng::register_function("req.watchdog", 1, std::bind(&serializer::req_watchdog, this, std::placeholders::_1)));
			vm.async_run(cyng::register_function("res.watchdog", 1, std::bind(&serializer::res_watchdog, this, std::placeholders::_1)));

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
