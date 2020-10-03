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
		cli_.vm_.register_function("send", 1, std::bind(&send::cmd, this, std::placeholders::_1));
	}
		
	void send::cmd(cyng::context& ctx)
	{
		auto const frame = ctx.get_frame();
		if (frame.size() < 2) {
			std::cout 
				<< "syntax:" 
				<< std::endl 
				<< "send filename host port"
				<< std::endl;

		}
		else {
			auto const filename = cyng::value_cast<std::string>(frame.at(0), "");
			auto const host = cyng::value_cast<std::string>(frame.at(1), "localhost");
			auto const port = cyng::value_cast<std::string>(frame.at(2), "9001");

			cyng::error_code ec;
			auto const file_size = cyng::filesystem::file_size(filename, ec);
			if (!ec) {
				std::ifstream finp(filename, std::ios::binary);
				if (finp.is_open()) {

					boost::asio::io_service ios;
					boost::asio::ip::tcp::resolver rsv(ios);
					auto pos = rsv.resolve(host, port);
					if (!pos.empty()) {

						std::cerr << "send ["
							<< filename
							<< "] to "
							<< pos->endpoint()
							<< std::endl;

						boost::asio::ip::tcp::socket socket(ios);
						socket.connect(*pos);

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
								<< ((total * 100.0)/ file_size)
								<< "%) to "
								<< pos->endpoint()
								<< std::endl;
						}
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

}
