/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SML_SELECT_H
#define SMF_SML_SELECT_H

#include <cyng/obj/intrinsics/container.h>
#include <cyng/obj/intrinsics/obis.h>
#include <cyng/obj/object.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace smf {
    namespace sml {
        //
        //  select device information from a get proc parameter response.
        //
        void select_devices(cyng::tuple_t const &);

        /**
         * similiar to read_param_tree() but with ignoring the attribute
         */
        std::tuple<cyng::obis, cyng::tuple_t> get_list(cyng::object const &obj);

    } // namespace sml
} // namespace smf
#endif
