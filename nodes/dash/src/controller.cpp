/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#include "controller.h"
#include <cyng/log.h>

namespace node 
{
	controller::controller(std::size_t pool_size, std::string const& json_path)
	: pool_size_(pool_size)
	, json_path_(json_path)
	{}

	int controller::run()
	{
		return EXIT_SUCCESS;
	}	
}
