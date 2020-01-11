/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 
#ifndef NODE_HTTP_CONNECTIONS_H
#define NODE_HTTP_CONNECTIONS_H

#include <cyng/compatibility/io_service.h>
#include "session.h"
#include "websocket.h"
#include <cyng/log.h>
#include <set>

namespace node 
{
	class connection_manager 
	{
	public:
		connection_manager(const connection_manager&) = delete;
		connection_manager& operator=(const connection_manager&) = delete;

		/// Construct a connection manager.
		connection_manager(cyng::logging::log_ptr);
		
		/// Add the specified connection to the manager and start it.
		void start(session_ptr c);
		
		/// Upgrade a session to a websocket
		wss_ptr upgrade(session_ptr c);

		/// Stop the specified connection.
		void stop(session_ptr c);
		
		/// Stop the specified websocket
		void stop(wss_ptr c);

		/// Stop all connections.
		void stop_all();
		
	
	private:
		cyng::logging::log_ptr logger_;
		
		/// The managed connections.
		std::set<session_ptr> connections_;		
		std::set<wss_ptr> ws_sockets_;		
	};
}

#endif

