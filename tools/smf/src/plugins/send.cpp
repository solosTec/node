/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2020 Sylko Olzscher 
 * 
 */ 


#include "send.h"
#include "../cli.h"

#include <cyng/io/hex_dump.hpp>
#include <cyng/vm/generator.h>
#include <cyng/io/serializer.h>
#include <cyng/io/io_bytes.hpp>

#include <iostream>
#include <fstream>

namespace node
{
	send::send(cli* cp)
		: cli_(*cp)
	{
		cli_.vm_.register_function("file", 1, std::bind(&send::cmd, this, std::placeholders::_1));
	}
		
	void send::cmd(cyng::context& ctx)
	{
		auto const frame = ctx.get_frame();
		if (frame.empty()) {
			std::cout 
				<< "syntax:" 
				<< std::endl 
				<< "<file> send filename host port"
				<< std::endl;

		}
		else {
			auto const cmd = cyng::value_cast<std::string>(frame.at(0), "send");
			if (boost::algorithm::equals(cmd, "send")) {

				auto const filename = cyng::value_cast<std::string>(frame.at(1), "");
				auto const host = cyng::value_cast<std::string>(frame.at(2), "localhost");
				auto const port = cyng::value_cast<std::string>(frame.at(3), "9001");
				transmit(filename, host, port);
			}
			else if (boost::algorithm::equals(cmd, "help")) {
				std::cout
					<< "file send filename host port"
					<< std::endl
					<< "\t> file send demo.txt demo.net 7564"
					<< std::endl;
			}
		}
	}

	void send::transmit(std::string filename, std::string host, std::string port)
	{
		cyng::error_code ec;
		auto const file_size = cyng::filesystem::file_size(filename, ec);
		if (!ec) {
			std::ifstream finp(filename, std::ios::binary);
			if (finp.is_open()) {

				boost::asio::io_service ios;
				boost::asio::ip::tcp::resolver rsv(ios);

				boost::asio::ip::tcp::socket socket(ios);
				boost::asio::connect(socket, rsv.resolve(host, port));
				if (socket.is_open()) {

					std::cerr << "send ["
						<< filename
						<< "] to "
						<< socket.remote_endpoint()
						<< std::endl;


					std::vector<char> buffer;
					buffer.resize(4096);
					std::size_t total{ 0 };

					while (!finp.read(buffer.data(), buffer.size()).fail()) {

						auto const size = finp.gcount();

						boost::system::error_code ec;
						auto sent = socket.write_some(boost::asio::buffer(buffer, size), ec);
						total += sent;
						BOOST_ASSERT(sent == size);

						std::cerr << "send "
							<< cyng::bytes_to_str(sent)
							<< " ("
							<< std::fixed
							<< std::setprecision(2)
							<< std::setfill('0')
							<< ((total * 100.0) / file_size)
							<< "%) to "

							<< socket.remote_endpoint()
							<< std::endl;
					}
					auto const size = finp.gcount();
					boost::system::error_code ec;
					auto sent = socket.write_some(boost::asio::buffer(buffer, size), ec);
					total += sent;
					std::cerr
						<< "complete with "
						<< cyng::bytes_to_str(total)
						<< std::endl;
				}
				else {
					std::cerr << "cannot resolve " << host << ':' << port << std::endl;
				}
			}
			else {
				std::cerr << "cannot open [" << filename << "]" << std::endl;
			}
		}
		else {
			std::cerr << "unknown file size [" << filename << "]: " << ec.message() << std::endl;
		}
	}

}
