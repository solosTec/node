/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <cfg.h>

#include <cyng/obj/container_cast.hpp>
#include <cyng/parse/string.h>

#ifdef _DEBUG_SEGW
#include <iostream>
#endif

#include <boost/uuid/nil_generator.hpp>

namespace smf {
    cfg::cfg(cyng::logger logger, cyng::store &cache)
        : logger_(logger)
        , cache_(cache)
        , tag_(boost::uuids::nil_uuid())
        , id_({0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00})
        , status_word_(sml::get_initial_value()) {}

    boost::uuids::uuid cfg::get_tag() const { return tag_; }

    cyng::buffer_t cfg::get_srv_id() const { return id_; }

    cyng::object cfg::get_obj(std::string path) { return cache_.get_object("cfg", "val", path); }

    bool cfg::set_obj(std::string name, cyng::object &&obj) {

        bool r = false;
        cache_.access(
            [&](cyng::table *cfg) {
                r = cfg->merge(
                    cyng::key_generator(name),
                    cyng::data_generator(obj),
                    1u,    //	only needed for insert operations
                    tag_); //	tag not available yet
            },
            cyng::access::write("cfg"));
        return r;
    }

    bool cfg::set_value(std::string name, cyng::object obj) { return set_obj(name, std::move(obj)); }

    bool cfg::remove_value(std::string name) {
        bool r = false;
        cache_.access([&](cyng::table *cfg) { r = cfg->erase(cyng::key_generator(name), tag_); }, cyng::access::write("cfg"));
        return r;
    }

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
