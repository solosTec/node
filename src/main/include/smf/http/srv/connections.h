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
			friend class websocket_session;

		public:
			connection_manager(const connection_manager&) = delete;
			connection_manager& operator=(const connection_manager&) = delete;

			/**
			 * Construct a connection manager.
			 */
			connection_manager(cyng::logging::log_ptr, bus::shared_type);

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
			void run_on_ws(boost::uuids::uuid tag, cyng::vector_t&& prg);

			bool add_channel(boost::uuids::uuid tag, std::string const& channel);
			void process_event(std::string const& channel, cyng::vector_t&&);

		private:
			/**
			 * put a websocket object on stack
			 */
			void push_ws(cyng::context& ctx);
			cyng::object get_ws(boost::uuids::uuid) const;

		private:
			cyng::logging::log_ptr logger_;

			/// The managed connections.
			std::set<session_ptr> connections_;
			std::map<boost::uuids::uuid, cyng::object>	ws_;

			/**
			 * channel list
			 */
			std::multimap<std::string, cyng::object>	listener_;

			mutable cyng::async::shared_mutex mutex_;

		};
	}
}

#endif

