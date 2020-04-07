/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2020 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_IPT_MASTER_CONFIG_CACHE_H
#define NODE_IPT_MASTER_CONFIG_CACHE_H

#include <smf/sml/intrinsics/obis_factory.hpp>


namespace node 
{
	/**
	 * Cache for gateway configuration.
	 * This class makes gateway configuration available also when the device
	 * is offline. In addition, it allows the incremental capturing of all 
	 * configuration data and allows a backup to be created.
	 *
	 * Each instance of this class stores the configuration of one gateway.
	 */
	class config_cache
	{
	public:
		config_cache(cyng::buffer_t srv, sml::obis_path&&);

		/**
		 * update configuration cache
		 *
		 * @return true if root is part of the actice sections
		 */
		bool update(sml::obis root, cyng::param_map_t const& params);

	private:
		cyng::buffer_t const srv_;
		sml::obis_path const sections_;
	};

}


#endif

