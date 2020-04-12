/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2020 Sylko Olzscher 
 * 
 */ 

#include "config_cache.h"
#include <smf/sml/srv_id_io.h>

#include <cyng/object.h>

namespace node 
{
	config_cache::config_cache(cyng::buffer_t srv, sml::obis_path&& sections)
        : srv_(srv)
        , sections_(init_sections(std::move(sections)))
	{ }

    bool config_cache::update(sml::obis root, cyng::param_map_t const& params)
    {
        //
        //  selective update
        //
        auto pos = sections_.find(sml::obis_path{ root });
        if (pos != sections_.end()) {
            pos->second = params;
            return true;
        }
        return false;
    }

    bool config_cache::update(sml::obis_path path, cyng::param_map_t const& params)
    {
        //
        //  selective update
        //
        auto pos = sections_.find(path);
        if (pos != sections_.end()) {
            pos->second = params;
            return true;
        }
        return false;
    }

    //  static
    config_cache::sections_t config_cache::init_sections(sml::obis_path&& slots)
    {
        sections_t secs;
        for (auto const& code : slots) {
            secs.emplace(sml::obis_path{ code }, cyng::param_map_t());
        }
        return secs;
    }

    std::string config_cache::get_server() const
    {
        return sml::from_server_id(srv_);
    }

    bool config_cache::is_cached(sml::obis code) const
    {
        return is_cached(sml::obis_path{ code });
    }

    bool config_cache::is_cached(sml::obis_path const& path) const
    {
        return sections_.find(path) != sections_.end();
    }


    void config_cache::add(sml::obis_path&& path)
    {
        if (!is_cached(path)) {
            sections_.emplace(path, cyng::param_map_t());
        }
    }

    void config_cache::remove(sml::obis_path&& slots)
    {
        for (auto const& code : slots) {
            if (is_cached(code)) {
                sections_.erase(sml::obis_path{ code });
            }
        }
    }

    void config_cache::clear()
    {
        sections_.clear();
    }

    cyng::param_map_t config_cache::get_section(sml::obis_path const& path) const
    {
        auto const pos = sections_.find(path);
        return (pos != sections_.end())
            ? pos->second 
            : cyng::param_map_t{}
        ;
    }

}
