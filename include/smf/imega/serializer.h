/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Sylko Olzscher
 *
 */

#ifndef SMF_IMEGA_SERIALIZER_H
#define SMF_IMEGA_SERIALIZER_H

#include <smf.h>

#include <cyng/obj/buffer_cast.hpp>
#include <cyng/obj/intrinsics/buffer.h>

#include <type_traits>

namespace smf {
    namespace imega {

        /**
         * Currently the serializer doesn't implement a protocol
         */
        class serializer {

          public:
            serializer();
            /**
             * no serialization al all
             */
            cyng::buffer_t raw_data(cyng::buffer_t &&data);
        };
    } // namespace imega
} // namespace smf

#endif
