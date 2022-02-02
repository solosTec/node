﻿/*
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
        // void select_devices(cyng::tuple_t const &);

        void select(cyng::tuple_t const &, cyng::obis root, std::function<void(cyng::prop_map_t const &, std::size_t)> cb);
        void collect(cyng::tuple_t const &tpl, cyng::obis o, std::function<void(cyng::prop_map_t const &)> cb);

        /**
         * Collect also nested data like in ROOT_PUSH_OPERATIONS (81 81 c7 8a 01 ff).
         */
        void collect(
            cyng::tuple_t const &tpl,
            cyng::obis_path_t,
            std::function<void(cyng::obis_path_t const &, cyng::attr_t const &)> cb);

    } // namespace sml
} // namespace smf
#endif
