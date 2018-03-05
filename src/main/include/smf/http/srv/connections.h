/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 
#ifndef NODE_HTTP_CONNECTIONS_H
#define NODE_HTTP_CONNECTIONS_H

#include <cyng/compatibility/io_service.h>
#include <smf/http/srv/session.h>
#include <smf/http/srv/websocket.h>
#include <cyng/compatibility/async.h>
#include <cyng/object.h>
#include <set>
#include <map>

namespace node 
{
	namespace http
	{
		class connection_manager
		{
		public:
			connection_manager(const connection_manager&) = delete;
			connection_manager& operator=(const connection_manager&) = delete;

			/**
			 * Construct a connection manager.
			 */
			connection_manager(cyng::logging::log_ptr);

			/**
			 * Add the specified connection to the manager and start it.
			 * Called by server.
			 */
			void start(session_ptr);

			/**
			 * Upgrade a session to a websocket
			 * Called by session.
			 */
			websocket_session const* upgrade(session_ptr c);

			/// Stop the specified connection.
			void stop(session_ptr c);

			/**
			 * Stop the specified websocket
			 */
			bool stop(websocket_session const*);

			/// Stop all connections.
			void stop_all();

			/**
			 * deliver a message to an websocket
			 */
			void send_ws(boost::uuids::uuid tag, cyng::vector_t&& prg);

		private:
			cyng::logging::log_ptr logger_;

			/// The managed connections.
			std::set<session_ptr> connections_;
			std::map<boost::uuids::uuid, cyng::object>	ws_;

			cyng::async::mutex mutex_;

		};
	}
}

#endif

