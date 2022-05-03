//
// blocking_tcp_echo_client.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2022 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/asio.hpp>
#include <boost/program_options.hpp>

#include <cstdlib>
#include <cstring>
#include <iostream>

using boost::asio::ip::tcp;

enum { max_length = 1024 };

int main(int argc, char *argv[]) {
    try {
        // if (argc != 3) {
        //     std::cerr << "Usage: tcp-client <host> <port>\n";
        //     return 1;
        // }

        std::string host = "localhost";
        std::uint16_t port = 14001;
        std::string mode = "medium";
        std::uint32_t packet_size = 1024;

        //
        //  start options
        //
        boost::program_options::options_description generic("Start Options");
        generic.add_options()                 //  add your options here
            ("help,h", "print usage message") //  print help menu
            ("host,H", boost::program_options::value<std::string>(&host)->default_value(host), "target host") // host
            ("port,P",
             boost::program_options::value<std::uint16_t>(&port)->default_value(port),
             "IP port: 1024 < port < 65535") // 1024 < port < 65535
            ("mode,M", boost::program_options::value<std::string>(&mode)->default_value(mode), "low, medium, high") // frequency
            ("packet-size",
             boost::program_options::value<std::uint32_t>(&packet_size)->default_value(packet_size),
             "packet size") // 1024
            ;

        boost::program_options::variables_map vm;
        boost::program_options::store(boost::program_options::command_line_parser(argc, argv).options(generic).run(), vm);
        boost::program_options::notify(vm);

        if (vm.count("help")) {
            std::cout << generic << std::endl;
            return EXIT_SUCCESS;
        }

        boost::asio::io_context io_context;

        tcp::socket s(io_context);
        tcp::resolver resolver(io_context);
        boost::asio::connect(s, resolver.resolve(host, std::to_string(port)));

        std::cout << "Enter message: ";
        char request[max_length];
        std::cin.getline(request, max_length);
        size_t request_length = std::strlen(request);
        boost::asio::write(s, boost::asio::buffer(request, request_length));

        char reply[max_length];
        size_t reply_length = boost::asio::read(s, boost::asio::buffer(reply, request_length));
        std::cout << "Reply is: ";
        std::cout.write(reply, reply_length);
        std::cout << "\n";
    } catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
