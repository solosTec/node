/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_IPT_TASK_HTTP_SERVER_H
#define SMF_IPT_TASK_HTTP_SERVER_H

#include <smf/http/server.h>

#include <cyng/obj/intrinsics/eod.h>
#include <cyng/log/logger.h>
#include <cyng/parse/json/json_parser.h>

#include <tuple>
#include <functional>
#include <string>
#include <memory>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/random_generator.hpp>

namespace smf {

	class http_server
	{
	public:
		using blocklist_type = std::vector<boost::asio::ip::address>;

	public:
		http_server(boost::asio::io_context&
			, boost::uuids::uuid tag
			, cyng::logger
			, std::string const& document_root
			, std::uint64_t max_upload_size
			, std::string const& nickname
			, std::chrono::seconds timeout
			, blocklist_type&&);
		~http_server();

		void stop(cyng::eod);
		void listen(boost::asio::ip::tcp::endpoint);

	private:
		/**
		 * incoming connection
		 */
		void accept(boost::asio::ip::tcp::socket&&);

		/**
		 * upgrade to webscoket
		 */
		void upgrade(boost::asio::ip::tcp::socket socket
			, boost::beast::http::request<boost::beast::http::string_body> req);

		/**
		 * received text/json data from the websocket
		 */
		void on_msg(boost::uuids::uuid tag, std::string);

	private:
		boost::uuids::uuid const tag_;
		cyng::logger logger_;
		std::string const document_root_;
		std::uint64_t const max_upload_size_;
		std::string const nickname_;
		std::chrono::seconds const timeout_;
		std::set<boost::asio::ip::address> const blocklist_;

		/**
		 * listen for incoming connections.
		 */
		http::server server_;

		/**
		 * JSON parser
		 */
		cyng::json::parser	parser_;

		/**
		 * generate unique session tags
		 */
		boost::uuids::random_generator uidgen_;

	};

}

#endif
