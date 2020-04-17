/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 

#include "segw.h"
#include <smf/sml/obis_io.h>
#include <cyng/table/key.hpp>
#include <numeric>

namespace node
{
	std::string build_cfg_key(sml::obis_path_t path) {
		return sml::to_hex(path, ':');
	}

	std::string build_cfg_key(sml::obis_path_t path, std::string leaf) {
		return sml::to_hex(path, ':') + ":" + leaf;
	}

	std::string build_cfg_key(std::initializer_list<std::string> slist)
	{
		return std::accumulate(slist.begin(), slist.end(), std::string(), [](std::string const& ss, std::string const& s) {
				return ss.empty() 
					? s 
					: ss + ":" + s
					;
		});
	}

	cyng::vector_t cfg_key(sml::obis_path_t path)
	{
		return cyng::table::key_generator(build_cfg_key(path));
	}

}

