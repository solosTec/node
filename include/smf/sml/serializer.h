/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SML_SERIALIZER_H
#define SMF_SML_SERIALIZER_H

#include <smf/sml.h>

#include <cyng/obj/intrinsics/buffer.h>
#include <cyng/obj/intrinsics/container.h>

namespace smf {
    namespace sml {

        /**
         * Convert a tuple of SML data in a buffer
         */
        cyng::buffer_t serialize(cyng::tuple_t);

    } // namespace sml
} // namespace smf
#endif
