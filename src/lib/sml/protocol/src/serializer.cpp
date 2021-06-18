#include <smf/sml/serializer.h>
#include <smf/sml/writer.hpp>

#include <cyng/obj/object.h>

#include <sstream>

#include <boost/assert.hpp>

namespace smf {
    namespace sml {
        cyng::buffer_t serialize(cyng::tuple_t tpl) {
            std::stringstream ss;
            auto const size = cyng::io::serializer<cyng::tuple_t, cyng::io::SML>::write(ss, tpl);
            cyng::buffer_t r;
            r.resize(size);
            ss.read(r.data(), size);
            return r;
        }
    } // namespace sml
} // namespace smf
