/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/obis/db.h>

#include <boost/assert.hpp>

namespace smf {
    namespace obis {

        std::string get_name(cyng::obis o) {

            switch (o.to_uint64()) {

#include <smf/obis/db.ipp>

            default:
                break;
            }

            return "not-found";
        }

        std::string get_description(cyng::obis o) {
            switch (o.to_uint64()) {

#include <smf/obis/descr.ipp>

            default:
                break;
            }

            return "";
        }

    } // namespace obis
} // namespace smf
