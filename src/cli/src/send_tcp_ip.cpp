#include <send_tcp_ip.h>

#include <boost/asio.hpp>

#include <array>
#include <chrono>
#include <fstream>
#include <iostream>

bool send_tcp_ip(std::string host, std::string service, std::string path) {

    std::ifstream ifs(path, std::ios::binary);
    if (ifs.is_open()) {

        boost::asio::io_service io_context;
        boost::asio::ip::tcp::socket s(io_context);

        boost::asio::ip::tcp::resolver resolver(io_context);
        auto const ep = boost::asio::connect(s, resolver.resolve(host, service));

        std::array<char, 512> buffer;
        for (;;) {
            auto const count = ifs.read(buffer.data(), buffer.size()).gcount();

            if (count != 0) {
                std::cout << "send " << count << " bytes to " << ep << std::endl;

                boost::system::error_code error;
                boost::asio::write(s, boost::asio::buffer(buffer, count), error);
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            } else {
                break;
            }
        }

        return true;
    }
    return false;
}
