/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#ifndef NODE_HTTPS_CONNECTIONS_H
#define NODE_HTTPS_CONNECTIONS_H

#include <cyng/compatibility/io_service.h>
#include <smf/https/srv/session.h>
#include <cyng/log.h>
//#include <smf/http/srv/websocket.h>
//#include <cyng/compatibility/async.h>
//#include <cyng/object.h>
//#include <set>
#include <map>
#include <boost/uuid/random_generator.hpp>

namespace node 
{
	namespace https
	{
		class connections
		{
		public:
			connections(cyng::logging::log_ptr
				, std::string const& doc_root);

			void create_session(boost::asio::ip::tcp::socket socket
				, boost::asio::ssl::context& ctx);

			void add_ssl_session(cyng::object);
			void add_plain_session(cyng::object);

		private:
			cyng::logging::log_ptr logger_;

			/**
			 * document root
			 */
			const std::string doc_root_;

			/**
			 * Generate unique session tags
			 */
			boost::uuids::random_generator uidgen_;

			/**
			 * Running HTTP/1.x sessions
			 */
			std::map<boost::uuids::uuid, cyng::object>	sessions_plain_;
			std::map<boost::uuids::uuid, cyng::object>	sessions_ssl_;


		};
	}
}

#endif

