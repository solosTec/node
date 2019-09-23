/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#include "print_version_info.h"
#include <NODE_project_info.h>
#include <string>

namespace node 
{
	int print_version_info(std::ostream& os, std::string const& name)
	{
        os
        << name
		<< " node v"
		<< NODE_VERSION
		<< std::endl
		<< "Copyright (C) 2012-"
		<< NODE_BUILD_YEAR
		<< " S. Olzscher (solos::Tec). All rights reserved."
		<< std::endl
		;
        return EXIT_SUCCESS;
	}
	
    std::string get_cfg_name(std::string const& node)
    {
       return node + "_" + NODE_SUFFIX + ".cfg";
    }
	
}

