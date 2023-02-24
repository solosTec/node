#include <smf/imega.h>

#include <boost/algorithm/string.hpp>
namespace smf {
    namespace imega {

        policy to_policy(std::string str) {
            if (boost::algorithm::equals(str, "MNAME")) {
                return policy::MNAME;
            } else if (boost::algorithm::equals(str, "TELNB")) {
                return policy::TELNB;
            }
            return policy::GLOBAL;
        }

    } // namespace imega
} // namespace smf
