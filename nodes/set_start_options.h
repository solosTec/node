/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_SET_NODE_START_OPTIONS_H
#define NODE_SET_NODE_START_OPTIONS_H

#include <ostream>
#include <boost/program_options.hpp>
#include <boost/predef.h>	//	requires Boost 1.55
#if BOOST_OS_LINUX
#include <sys/resource.h>
#endif

namespace node 
{
	void set_start_options(boost::program_options::options_description&
	, std::string const& name
	, std::string& json_path
    , unsigned int& pool_size
#if BOOST_OS_LINUX
    , struct rlimit&
#endif
	);
}

#endif
