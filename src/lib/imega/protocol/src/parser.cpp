#include <smf/imega/parser.h>

#include <ios>
#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/core/ignore_unused.hpp>

namespace smf {
    namespace imega {
        parser::parser()
            : state_(state::INITIAL) {}
    } // namespace imega
} // namespace smf
