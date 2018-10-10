/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 
#ifndef NODE_HTTP_CONNECTIONS_H
#define NODE_HTTP_CONNECTIONS_H

#include <smf/http/srv/cm_interface.h>
#include <smf/http/srv/session.h>
#include <smf/http/srv/websocket.h>
#include <cyng/compatibility/io_service.h>
#include <cyng/compatibility/async.h>
#include <cyng/object.h>
#include <set>
#include <map>

namespace node 
{
	namespace http
	{
		class connection_manager : public connection_manager_interface
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
			void start(cyng::object);

			/**
			 * Upgrade a session to a websocket
			 * Called by session.
			 */
			std::pair<cyng::object, cyng::object> upgrade(session*);

			/**
			 * Stop the specified HTTP session.
			 *
			 * @return the session object
			 */
			cyng::object stop(session*);

			/**
			 * Stop the specified websocket
			 */
			bool stop(websocket_session const*);

			/**
			 * stop all HTTP sessions and Web-Sockets
			 */
			void stop_all();

			/**
			 * deliver a message to a websocket
			 */
			virtual bool ws_msg(boost::uuids::uuid tag, std::string const&) override;

			/**
			 * Send HTTP 302 moved response
			 */
			bool http_moved(boost::uuids::uuid tag, std::string const& target);

			/**
			 * A channel is a named data source dhat cann be subsribed by web-sockets
			 */
			virtual bool add_channel(boost::uuids::uuid tag, std::string const& channel) override;

			/**
			 * Push data to all subscribers of the specified channel
			 */
			virtual void push_event(std::string const& channel, std::string const&) override;

			void trigger_download(boost::uuids::uuid tag, std::string const& filename, std::string const& attachment);

		private:
			/**
			 * put a websocket object on stack
			 */
			void push_ws(cyng::context& ctx);
			cyng::object get_ws(boost::uuids::uuid) const;

		private:
			cyng::logging::log_ptr logger_;

			/**
			 * Running HTTP/1.x sessions
			 */
			std::map<boost::uuids::uuid, cyng::object>	sessions_;

			/**
			 * Websockets
			 */
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

