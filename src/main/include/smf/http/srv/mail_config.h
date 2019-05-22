/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 
#ifndef NODE_HTTP_MAIL_CONFIG_H
#define NODE_HTTP_MAIL_CONFIG_H

// #include <NODE_project_info.h>
// #include <cyng/compatibility/io_service.h>
// #include <boost/beast/websocket.hpp>
// #include <boost/asio/steady_timer.hpp>
// #include <memory>
#include <cyng/object.h>
#include <vector>
#include <string>
#include <cstdint>
#include <iostream>  
// #include <boost/filesystem.hpp>

namespace node 
{
	struct mail_address 
	{
		std::string name_;
		std::string address_;
		mail_address();
		mail_address(std::string const&, std::string const&);
	};
	
	struct mail_config
	{
		std::string host_;
		std::uint16_t port_;
		std::string auth_name_;
		std::string auth_pwd_;
		std::string auth_method_;
		mail_address sender_;
		std::vector<mail_address> recipients_;
		
	};
	
	void init(cyng::object, mail_config&);
}

namespace std 
{
	ostream& operator<<(ostream& os, const node::mail_config& mc);  
}

#endif
