/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 
#ifndef NODE_HTTP_CONNECTIONS_H
#define NODE_HTTP_CONNECTIONS_H

#include <smf/http/srv/cm_interface.h>
#ifdef NODE_SSL_INSTALLED
#include <smf/http/srv/auth.h>
#endif

#include <cyng/compatibility/io_service.h>
#include <cyng/compatibility/async.h>
#include <cyng/log.h>
#include <cyng/object.h>
#include <cyng/vm/controller_fwd.h>

#include <map>
#include <array>

#include <boost/uuid/random_generator.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

namespace node 
{
	namespace http
	{
		class connections : public connection_manager_interface
		{
			using container_t = std::map<boost::uuids::uuid, cyng::object>;

		public:
			connections(const connections&) = delete;
			connections& operator=(const connections&) = delete;

			/**
			 * Construct a connection manager.
			 */
			connections(cyng::logging::log_ptr
				, cyng::controller& vm
				, std::string const& doc_root
#ifdef NODE_SSL_INSTALLED
				, auth_dirs const& ad
#endif
				, std::map<std::string, std::string> const&
                , std::size_t timeout
				, std::uint64_t max_upload_size
				, bool https_rewrite
			);

			/**
			 * Provide access to vm controller
			 */
			cyng::controller& vm();

			bool redirect(std::string& path) const;

			/**
			 * Add the specified connection to the manager and start it.
			 * Called by server.
			 */
			void create_session(boost::asio::ip::tcp::socket socket);

			/**
			 * Upgrade a plain HTTP session to web-socket 
			 */
			void upgrade(boost::uuids::uuid
				, boost::asio::ip::tcp::socket socket
				, boost::beast::http::request<boost::beast::http::string_body> req);

			/**
			 * Stop the specified HTTP session.
			 *
			 * @return the session object
			 */
			cyng::object stop_session(boost::uuids::uuid);

			/**
			 * Stop the specified websocket
			 */
			cyng::object stop_ws(boost::uuids::uuid);

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

			/**
			 * Push browser/client to start a download
			 */
			virtual void trigger_download(boost::uuids::uuid tag, boost::filesystem::path const& filename, std::string const& attachment) override;

		private:
			void modify_config(cyng::context& ctx);

			enum session_type {
				HTTP_PLAIN,
				SOCKET_PLAIN,
				SESSION_TYPES,
				NO_SESSION	//	no session found
			};

		private:
			cyng::logging::log_ptr logger_;

			/**
			 * execution engine
			 */
			cyng::controller& vm_;

			/**
			 * document root
			 */
			std::string const doc_root_;

#ifdef NODE_SSL_INSTALLED
			auth_dirs const auth_dirs_;
#endif

			std::map<std::string, std::string> const redirects_;
            std::chrono::seconds timeout_;
			std::uint64_t max_upload_size_;
			bool https_rewrite_;

			/**
			 * Generate unique session tags
			 */
			boost::uuids::random_generator uidgen_;

			/**
			 * Running HTTPs/1.x sessions and web-sockets
			 */
			std::array<container_t, SESSION_TYPES> sessions_;

			/**
			 * Mutex for session containers
			 */
			mutable std::array<cyng::async::shared_mutex, SESSION_TYPES> mutex_;

			/** @brief channel list
			 *
			 * Each channel can be subscribed by a multitude of channels
			 */
			std::multimap<std::string, cyng::object>	listener_;


		};
	}
}

#endif

