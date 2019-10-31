/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 

#include "segw.h"
#include <smf/sml/obis_io.h>
#include <cyng/table/key.hpp>

namespace node
{
	std::string build_cfg_key(sml::obis_path path) {
		return sml::to_hex(path, ':');
	}

	std::string build_cfg_key(sml::obis_path path, std::string leaf) {
		return sml::to_hex(path, ':') + ":" + leaf;
	}

	cyng::vector_t cfg_key(sml::obis_path path)
	{
		return cyng::table::key_generator(build_cfg_key(path));
	}

}

