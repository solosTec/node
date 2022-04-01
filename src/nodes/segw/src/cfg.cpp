/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <cfg.h>
#include <smf/config/schemes.h>

#include <cyng/obj/container_cast.hpp>
#include <cyng/parse/string.h>

#ifdef _DEBUG_SEGW
#include <iostream>
#endif

#include <boost/uuid/nil_generator.hpp>

namespace smf {
    cfg::cfg(cyng::logger logger, cyng::store &cache)
        : kv_store(cache, "cfg", boost::uuids::nil_uuid())
        , logger_(logger)
        , cache_(cache)
        , id_({0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00})
        , status_word_(sml::get_initial_value())
        //  d5884bce-9c2b-40bf-9fc0-5c0e77f708c3
        , name_gen_(config::device_name) {}

    // boost::uuids::uuid cfg::get_tag() const { return tag_; }

    boost::uuids::uuid cfg::get_name(std::string const &name) const { return name_gen_(name); }
    boost::uuids::uuid cfg::get_name(cyng::buffer_t const &name) const { return name_gen_(name.data(), name.size()); }

    cyng::buffer_t cfg::get_srv_id() const { return id_; }

    sml::status_word_t cfg::get_status_word() const { return status_word_; }

    void cfg::loop(std::function<void(std::vector<std::string> &&, cyng::object)> cb) {
        cache_.access(
            [&](cyng::table const *cfg) {
                std::string const delimiter(1, sep);
                cfg->loop([&](cyng::record &&rec, std::size_t) -> bool {
                    if (!rec.empty()) {
                        auto const path = rec.value("path", "");
                        cb(cyng::split(path, delimiter), rec.data().at(0));
                    }

                    return true;
                });
            },
            cyng::access::read("cfg"));
    }

    void cfg::loop(std::string const &filter, std::function<void(std::vector<std::string> &&, cyng::object)> cb) {
        cache_.access(
            [&](cyng::table const *cfg) {
                std::string const delimiter(1, sep);
                cfg->loop([&](cyng::record &&rec, std::size_t) -> bool {
                    if (!rec.empty()) {
                        auto const path = rec.value("path", "");
                        if (boost::algorithm::starts_with(path, filter)) {
                            cb(cyng::split(path, delimiter), rec.data().at(0));
                        }
                    }
                    return true;
                });
            },
            cyng::access::read("cfg"));
    }

} // namespace smf
