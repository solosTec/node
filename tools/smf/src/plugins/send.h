/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2020 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_TOOL_SMF_PLUGIN_SEND_H
#define NODE_TOOL_SMF_PLUGIN_SEND_H

#include <cyng/vm/context.h>
#include <cstdint>
#include <cyng/compatibility/file_system.hpp>
//#include <boost/asio.hpp>
//#include <vector>

namespace node
{
	/**
	 * Read a file binary and send content to a specified ip address.
	 */
	class cli;
	class send
	{
	public:
		send(cli*);

	private:
		void cmd(cyng::context& ctx);
		void transmit(std::string filename, std::string host, std::string port);
	private:
		cli& cli_;
		//const filesystem::path inp_;
		//const int verbose_;
		//std::vector<char> buffer_;
	};
}

#endif	//	CYNG_UNLOG_CU_H
