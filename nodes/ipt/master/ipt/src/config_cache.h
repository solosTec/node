/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2020 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_IPT_MASTER_CONFIG_CACHE_H
#define NODE_IPT_MASTER_CONFIG_CACHE_H

//#include <cyng/vm/controller.h>

namespace node 
{
	/**
	 * Cache for gateway configuration.
	 * This class makes gateway configuration available also when the device
	 * is offline. In addition, it allows the incremental capturing of all 
	 * configuration data and allows a backup to be created.
	 */
	class config_cache
	{
	public:
		config_cache();

	private:
	};

}


#endif

