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
		void help();

	private:
		cli& cli_;
	};
}

#endif	//	CYNG_UNLOG_CU_H
