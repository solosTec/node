/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Sylko Olzscher
 *
 */
#ifndef SMF_IMEGA_H
#define SMF_IMEGA_H

#include <string>

namespace smf {
    namespace imega {

        /**
         * There are three options to obtain a password:
         * - use the global defined password
         * - use MNAME from the login
         * - use TELNB from the login
         */
        enum struct policy {
            GLOBAL,
            MNAME, // swibi/MNAME,
            TELNB  // sgsw/TELNB
        };

        policy to_policy(std::string);

    } // namespace imega
} // namespace smf

#include <smf/imega/parser.h>
#include <smf/imega/serializer.h>

#endif // SMF_IMEGA_H
