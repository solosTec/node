/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_STORAGE_H
#define SMF_SEGW_STORAGE_H

#include <storage_functions.h>

#include <cyng/obj/intrinsics/obis.h>
#include <cyng/task/task_fwd.h>

namespace smf {

    /**
     * manage SQL tables
     */
    class storage {
      public:
        storage(cyng::db::session);

        bool cfg_insert(cyng::object const &, cyng::object const &);
        bool cfg_update(cyng::object const &, cyng::object const &);
        bool cfg_remove(cyng::object const &);
        cyng::object cfg_read(cyng::object const &, cyng::object def);

        void generate_op_log(
            std::uint64_t status,
            std::uint32_t evt,
            cyng::obis code,
            cyng::buffer_t srv,
            std::string target,
            std::uint8_t nr,
            std::string description);

      private:
        cyng::db::session db_;
    };
} // namespace smf

#endif
