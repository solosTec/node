/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_PRINT_VERSION_INFO_H
#define NODE_PRINT_VERSION_INFO_H

#include  <ostream>
namespace node 
{
	int print_version_info(std::ostream&, std::string const&);
    std::string get_cfg_name(std::string const&);
}

#endif
