/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2020 Sylko Olzscher 
 * 
 */ 

#include "config_cache.h"

namespace node 
{
	config_cache::config_cache(cyng::buffer_t srv, sml::obis_path&& sections)
        : srv_(srv)
        , sections_(std::move(sections))
	{}

    bool config_cache::update(sml::obis root, cyng::param_map_t const& params)
    {
        //sections_
        return false;
    }

}
