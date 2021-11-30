#include <smf/modem/serializer.h>

#include <cyng/io/serialize.h>
#ifdef SMF_MODEM_DEBUG
#include <cyng/io/hex_dump.hpp>
#endif

//#include <boost/core/ignore_unused.hpp>
//#include <boost/predef.h>

namespace smf {
    namespace modem {
        serializer::serializer() {}

        cyng::buffer_t serializer::ok() const { return write("OK\r\n"); }

        cyng::buffer_t serializer::error() const { return write("ERROR\r\n"); }

        cyng::buffer_t serializer::busy() const { return write("BUSY\r\n"); }

        cyng::buffer_t serializer::no_dialtone() const { return write("NO DIALTONE\r\n"); }

        cyng::buffer_t serializer::no_carrier() const { return write("NO CARRIER\r\n"); }

        cyng::buffer_t serializer::no_answer() const { return write("NO ANSWER\r\n"); }

        cyng::buffer_t serializer::ring() const { return write("RING\r\n"); }

        cyng::buffer_t serializer::connect() const { return write("CONNECT\r\n"); }

        cyng::buffer_t serializer::write(std::string const &str) const { return cyng::to_buffer(str); }

    } // namespace modem
} // namespace smf
