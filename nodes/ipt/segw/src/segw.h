/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_SEGW_H
#define NODE_SEGW_H

#include <smf/sml/intrinsics/obis.h>
#include <cyng/intrinsics/sets.h>

namespace node
{
	/**
	 * helper function to build a key for table TCfg
	 */
	std::string build_cfg_key(sml::obis_path_t path);

	/**
	 * helper function to build a key for table TCfg
	 */
	std::string build_cfg_key(sml::obis_path_t path, std::string leaf);

	/**
	 * helper function to build a key for table TCfg
	 */
	std::string build_cfg_key(std::initializer_list<std::string> slist);

	/**
	 * build a key for internal table "_Cfg" 
	 */
	cyng::vector_t cfg_key(sml::obis_path_t path);

	/**
	 * pre-configured data sources
	 */
	enum class source : std::uint8_t {
		OTHER = 0,
		WIRELESS_LMN = 1,	//	IF_wMBUS
		WIRED_LMN = 2,		//	rs485
		ETHERNET,
	};

}
#endif
