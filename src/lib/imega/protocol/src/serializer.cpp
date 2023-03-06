#include <smf/imega/serializer.h>

#include <cyng/io/serialize.h>
#ifdef SMF_IMEGA_DEBUG
#include <cyng/io/hex_dump.hpp>
#endif

// #include <boost/core/ignore_unused.hpp>
// #include <boost/predef.h>

namespace smf {
    namespace imega {
        serializer::serializer() {}

        cyng::buffer_t serializer::raw_data(cyng::buffer_t &&data) { return data; }
    } // namespace imega
} // namespace smf
