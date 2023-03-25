/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Sylko Olzscher
 *
 */
#ifndef SMF_STORE_CONFIG_GENERATOR_H
#define SMF_STORE_CONFIG_GENERATOR_H

#include <cyng/obj/intrinsics.h>

#include <chrono>

namespace smf {

    /**
     * Create the default configuration.
     * Since the configuration is produced as a DOM like structure it's
     * easy to save as JSON or XML file.
     */
    cyng::vector_t create_default_config(
        std::chrono::system_clock::time_point &&now,
        std::filesystem::path &&tmp,
        std::filesystem::path &&cwd,
        boost::uuids::uuid tag);

    /**
     * Same function as in "report" app
     */
    cyng::prop_t create_report_spec(cyng::obis profile, std::filesystem::path cwd, bool enabled, std::chrono::hours backtrack);
    cyng::prop_t create_lpex_spec(cyng::obis profile, std::filesystem::path cwd, bool enabled, std::chrono::hours backtrack);
    cyng::prop_t create_feed_spec(cyng::obis profile, std::filesystem::path cwd, bool enabled, std::chrono::hours backtrack);
    cyng::prop_t create_cleanup_spec(cyng::obis profile, std::chrono::hours hours, bool enabled);
    cyng::prop_t create_gap_spec(cyng::obis profile, std::filesystem::path const &cwd, std::chrono::hours hours, bool enabled);

} // namespace smf

#endif
