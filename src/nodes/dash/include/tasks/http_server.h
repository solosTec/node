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

#include <tuple>
#include <functional>
#include <string>
#include <memory>

#include <boost/uuid/uuid.hpp>

namespace cyng {
	template <typename T >
	class task;

	class channel;
}
namespace smf {

	class http_server
	{
		template <typename T >
		friend class cyng::task;

		using signatures_t = std::tuple<
			std::function<void(cyng::eod)>,
			std::function<void(boost::asio::ip::tcp::endpoint)>
		>;

	public:
		http_server(std::weak_ptr<cyng::channel>
			, boost::asio::io_context&
			, boost::uuids::uuid tag
			, cyng::logger
			, std::string const& document_root
			, std::uint64_t max_upload_size
			, std::string const& nickname
			, std::chrono::seconds timeout);
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

	private:
		signatures_t sigs_;
		std::weak_ptr<cyng::channel> channel_;
		boost::uuids::uuid const tag_;
		cyng::logger logger_;
		std::string const document_root_;
		std::uint64_t const max_upload_size_;
		std::string const nickname_;
		std::chrono::seconds const timeout_;

		/**
		 * listen for incoming connections.
		 */
		http::server server_;

	};

}

#endif
