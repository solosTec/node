/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_HTTP_CONTROLLER_H
#define NODE_HTTP_CONTROLLER_H

#include <string>
#include <cstdint>

namespace node 
{
	class controller 
	{
	public:
		/**
		 * @param pool_size thread pool size 
		 * @param json_path path to JSON configuration file 
		 */
		controller(std::size_t pool_size, std::string const& json_path);
		
	private:
		const std::size_t pool_size_;
		const std::string json_path_;
	};
}

#endif