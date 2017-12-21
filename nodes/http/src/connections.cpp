/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#include "connections.h"

namespace node 
{
	connection_manager::connection_manager(cyng::logging::log_ptr logger)
	: logger_(logger)
	, connections_()
	, ws_sockets_()
	{}
	
	void connection_manager::start(session_ptr sp)
	{
		connections_.insert(sp);
		sp->run();		
	}
	
	wss_ptr connection_manager::upgrade(session_ptr sp)
	{
		connections_.erase(sp);
		wss_ptr wssp = std::make_shared<websocket_session>(sp->logger_
			, *this
			, std::move(sp->socket_)
			, sp->mail_config_);
		ws_sockets_.insert(wssp);
		return wssp;
	}
	
	void connection_manager::stop(session_ptr sp)
	{
		connections_.erase(sp);
		sp->do_close();
	}
	
	void connection_manager::stop(wss_ptr wssp)
	{
		ws_sockets_.erase(wssp);
		wssp->do_close();
	}

	void connection_manager::stop_all()
	{
		CYNG_LOG_INFO(logger_, "stop " << connections_.size() << " sessions");
		for (auto sp : connections_)
		{
			sp->do_close();
		}
		CYNG_LOG_INFO(logger_, "stop " << ws_sockets_.size() << " websockets");
		for (auto wssp : ws_sockets_)
		{
			wssp->do_close();
		}
		connections_.clear();
		ws_sockets_.clear();
	}	
}




