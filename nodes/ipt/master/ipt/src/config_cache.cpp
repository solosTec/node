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
        auto pos = sections_.find(root);
        if (pos != sections_.end()) {
            pos->second = params;
            return true;
        }
        return false;
    }

    config_cache::sections_t config_cache::init_sections(sml::obis_path&& path)
    {
        sections_t secs;
        for (auto const& code : path) {
            secs.emplace(code, cyng::param_map_t());
        }
        return secs;
    }

    std::string config_cache::get_server() const
    {
        return sml::from_server_id(srv_);
    }

    bool config_cache::is_cached(sml::obis code) const
    {
        return sections_.find(code) != sections_.end();
    }

}
