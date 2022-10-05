/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/config/kv_store.h>

#include <cyng/obj/container_cast.hpp>

#ifdef _DEBUG_SEGW
#include <iostream>
#endif

#include <boost/uuid/nil_generator.hpp>

namespace smf {
    kv_store::kv_store(cyng::store &cache, std::string table_name, boost::uuids::uuid tag)
        : cache_(cache)
        , table_name_(table_name)
        , tag_(tag) {}

    kv_store::kv_store(cyng::store &cache, boost::uuids::uuid tag)
        : kv_store(cache, "config", tag) {} //  delegation

    cyng::object kv_store::get_obj(std::string path) { return cache_.get_object(table_name_, "value", path); }

    std::string kv_store::get_value(std::string name, const char *def) {
        //  convert string to object
        return cyng::value_cast(get_obj(name), std::string(def));
    }

    bool kv_store::set_obj(std::string name, cyng::object &&obj) {

        bool r = false;
        cache_.access(
            [&](cyng::table *kv_store) {
                r = kv_store->merge(
                    cyng::key_generator(name),
                    cyng::data_generator(obj),
                    1u //	only needed for insert operations
                    ,
                    tag_); //	tag not available yet
            },
            cyng::access::write(table_name_));
        return r;
    }

    bool kv_store::set_value(std::string name, cyng::object obj) { return set_obj(name, std::move(obj)); }

    bool kv_store::remove_value(std::string name) {
        bool r = false;
        cache_.access(
            [&](cyng::table *cfg) {
                //  remove record
                r = cfg->erase(cyng::key_generator(name), tag_);
            },
            cyng::access::write(table_name_));
        return r;
    }

    boost::uuids::uuid kv_store::get_tag() const { return tag_; }

    cyng::object tidy_config(std::string key, cyng::object &value) {
        if (boost::algorithm::equals(key, "max.messages") || boost::algorithm::equals(key, "max.events") ||
            boost::algorithm::equals(key, "max.LoRa.records") || boost::algorithm::equals(key, "max.wMBus.records") ||
            boost::algorithm::equals(key, "max.IEC.records") || boost::algorithm::equals(key, "max.bridges")) {
            //  convert to std::uint64_t
            return (value.tag() == cyng::TC_UINT64) ? value : cyng::make_object(cyng::numeric_cast<std::uint64_t>(value, 100));
        } else if (boost::algorithm::equals(key, "def.IEC.interval")) {
            //  convert to std::chrono::minutes
            return (value.tag() == cyng::TC_MINUTE)
                       ? value
                       : cyng::make_object(std::chrono::minutes(cyng::numeric_cast<std::uint64_t>(value, 20)));
        } else {
            ; //  unchanged (mostly boolean)
        }
        return value;
    }

} // namespace smf
