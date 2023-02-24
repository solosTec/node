#include <smf/modem/serializer.h>

#include <cyng/io/serialize.h>
#ifdef SMF_MODEM_DEBUG
#include <cyng/io/hex_dump.hpp>
#endif

#include <sstream>

namespace smf {
    namespace modem {

        cyng::buffer_t serializer::ok() { return write("OK\r\n"); }

        cyng::buffer_t serializer::error() { return write("ERROR\r\n"); }

        cyng::buffer_t serializer::busy() { return write("BUSY\r\n"); }

        cyng::buffer_t serializer::no_dialtone() { return write("NO DIALTONE\r\n"); }

        cyng::buffer_t serializer::no_carrier() { return write("NO CARRIER\r\n"); }

        cyng::buffer_t serializer::no_answer() { return write("NO ANSWER\r\n"); }

        cyng::buffer_t serializer::ring() { return write("RING\r\n"); }

        cyng::buffer_t serializer::connect() { return write("CONNECT\r\n"); }

        cyng::buffer_t serializer::ep(boost::asio::ip::tcp::endpoint p) {
            auto const address = p.address().to_string();
            auto const port = p.port();

            //
            //	address:port
            //
            std::stringstream ss;
            ss << address << ':' << port;
            return write(ss.str());
        }

        cyng::buffer_t serializer::write(std::string const &str) { return cyng::to_buffer(str); }

    } // namespace modem
} // namespace smf
