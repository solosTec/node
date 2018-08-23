/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_GATEWAY_SERVER_H
#define NODE_IPT_GATEWAY_SERVER_H

#include <smf/sml/status.h>
#include <smf/ipt/config.h>
#include <cyng/async/mux.h>
#include <cyng/log.h>
#include <cyng/intrinsics/mac.h>
#include <cyng/store/db.h>
#include <boost/uuid/uuid.hpp>
#include <unordered_map>

namespace node
{
	class server
	{
	public:
		server(cyng::async::mux&
			, cyng::logging::log_ptr logger
			, sml::status& status_word
			, cyng::store::db& config_db
			, boost::uuids::uuid tag
			, ipt::master_config_t const& cfg
			, std::string account
			, std::string pwd
			, std::string manufacturer
			, std::string model
			, cyng::mac48);

		/**
		* start listening
		*/
		void run(std::string const&, std::string const&);

		/**
		* close acceptor
		*/
		void close();

	private:
		/// Perform an asynchronous accept operation.
		void do_accept();

	private:
		/*
		 * task manager and running I/O context
		 */
		cyng::async::mux& mux_;

		/**
		 * The logger
		 */
		cyng::logging::log_ptr logger_;

		/**
		 * Global status word
		 */
		sml::status&	status_word_;

		/**
		 * configuration db
		 */
		cyng::store::db& config_db_;
		const ipt::master_config_t& cfg_;

		//	credentials
		const std::string account_;
		const std::string pwd_;

		//
		//	hardware
		//
		const std::string manufacturer_;
		const std::string model_;
		const cyng::mac48 server_id_;

		/// Acceptor used to listen for incoming connections.
		boost::asio::ip::tcp::acceptor acceptor_;

#if (BOOST_VERSION < 106600)
		boost::asio::ip::tcp::socket socket_;
#endif

	};
}

#endif

