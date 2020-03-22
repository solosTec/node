/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#ifndef NODE_HTTP_CONNECTION_MANAGER_INTERFACE_H
#define NODE_HTTP_CONNECTION_MANAGER_INTERFACE_H

#include <string>
#include <boost/uuid/uuid.hpp>
#include <boost/filesystem/path.hpp>

namespace node 
{
	class connection_manager_interface
	{
	public:
		/**
		 * deliver a message to a websocket
		 */
		virtual bool ws_msg(boost::uuids::uuid tag, std::string const&) = 0;

		/**
		 * Send HTTP 302 moved response
		 */
		virtual bool http_moved(boost::uuids::uuid tag, std::string const& target) = 0;

		/**
		 * A channel is a named data source dhat cann be subsribed by web-sockets
		 */
		virtual bool add_channel(boost::uuids::uuid tag, std::string const& channel) = 0;

		/**
		 * Push data to all subscribers of the specified channel
		 */
		virtual void push_event(std::string const& channel, std::string const& data) = 0;

		/**
		 * Push browser/client to start a download
		 *
		 * @return false if session was noot found
		 */
		virtual bool trigger_download(boost::uuids::uuid tag, boost::filesystem::path const& filename, std::string const& attachment) = 0;

	};
}

#endif

