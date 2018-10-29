/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_HTTPS_SRV_WEBSOCKET_H
#define NODE_LIB_HTTPS_SRV_WEBSOCKET_H

#include <smf/https/srv/websocket.hpp>
#include <smf/https/srv/ssl_stream.hpp>

namespace node
{
	namespace https
	{
		// Handles a plain WebSocket connection
		class plain_websocket : public websocket_session<plain_websocket>
		{
		public:
			// Create the session
			explicit plain_websocket(cyng::logging::log_ptr logger
				, connections&
				, boost::uuids::uuid
				, boost::asio::ip::tcp::socket socket);

			// Called by the base class
			boost::beast::websocket::stream<boost::asio::ip::tcp::socket>& ws();

			// Start the asynchronous operation
			template<class Body, class Allocator>
			void run(cyng::object obj, boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> req)
			{
				// Run the timer. The timer is operated
				// continuously, this simplifies the code.
				//on_timer({});
				on_timer(obj, boost::system::error_code{});

				// Accept the WebSocket upgrade request
				do_accept(obj, std::move(req));
			}

			void do_timeout(cyng::object obj);
			void on_close(cyng::object obj, boost::system::error_code ec);

		private:
			boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws_;
			bool close_ = false;
		};

		// Handles an SSL WebSocket connection
		class ssl_websocket : public websocket_session<ssl_websocket>
		{
		public:
			// Create the http_session
			explicit ssl_websocket(cyng::logging::log_ptr logger
				, connections&
				, boost::uuids::uuid
				, boost::beast::ssl_stream<boost::asio::ip::tcp::socket> stream);

			// Called by the base class
			boost::beast::websocket::stream<boost::beast::ssl_stream<boost::asio::ip::tcp::socket>>& ws();

			// Start the asynchronous operation
			template<class Body, class Allocator>
			void run(cyng::object obj, boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> req)
			{
				// Run the timer. The timer is operated
				// continuously, this simplifies the code.
				//on_timer({});
				on_timer(obj, boost::system::error_code{});

				// Accept the WebSocket upgrade request
				do_accept(obj, std::move(req));
			}

			void do_eof(cyng::object obj);
			void do_timeout(cyng::object obj);

		private:
			void on_shutdown(cyng::object obj, boost::system::error_code ec);

		private:
			boost::beast::websocket::stream<boost::beast::ssl_stream<boost::asio::ip::tcp::socket>> ws_;
			boost::asio::strand<boost::asio::io_context::executor_type> strand_;
			bool eof_ = false;

		};
	}
}

#include <cyng/intrinsics/traits.hpp>
namespace cyng
{
	namespace traits
	{
		template <>
		struct type_tag<node::https::plain_websocket>
		{
			using type = node::https::plain_websocket;
			using tag = std::integral_constant<std::size_t, PREDEF_SOCKET>;
#if defined(CYNG_LEGACY_MODE_ON)
			const static char name[];
#else
			constexpr static char name[] = "plain-websocket";
#endif
		};

		template <>
		struct reverse_type < PREDEF_SOCKET >
		{
			using type = node::https::plain_websocket;
		};

		template <>
		struct type_tag<node::https::ssl_websocket>
		{
			using type = node::https::ssl_websocket;
			using tag = std::integral_constant<std::size_t, PREDEF_SSL_SOCKET>;
#if defined(CYNG_LEGACY_MODE_ON)
			const static char name[];
#else
			constexpr static char name[] = "ssl-websocket";
#endif
		};

		template <>
		struct reverse_type < PREDEF_SSL_SOCKET >
		{
			using type = node::https::ssl_websocket;
		};

	}

}

#include <functional>
//#include <boost/functional/hash.hpp>

namespace std
{
	template<>
	struct hash<node::https::plain_websocket>
	{
		size_t operator()(node::https::plain_websocket const& s) const noexcept;
	};
	template<>
	struct equal_to<node::https::plain_websocket>
	{
		using result_type = bool;
		using first_argument_type = node::https::plain_websocket;
		using second_argument_type = node::https::plain_websocket;

		bool operator()(node::https::plain_websocket const& s1, node::https::plain_websocket const& s2) const noexcept;
	};

	template<>
	struct less<node::https::plain_websocket>
	{
		using result_type = bool;
		using first_argument_type = node::https::plain_websocket;
		using second_argument_type = node::https::plain_websocket;

		bool operator()(node::https::plain_websocket const& s1, node::https::plain_websocket const& s2) const noexcept;
	};
	template<>
	struct hash<node::https::ssl_websocket>
	{
		size_t operator()(node::https::ssl_websocket const& s) const noexcept;
	};
	template<>
	struct equal_to<node::https::ssl_websocket>
	{
		using result_type = bool;
		using first_argument_type = node::https::ssl_websocket;
		using second_argument_type = node::https::ssl_websocket;

		bool operator()(node::https::ssl_websocket const& s1, node::https::ssl_websocket const& s2) const noexcept;
	};
	template<>
	struct less<node::https::ssl_websocket>
	{
		using result_type = bool;
		using first_argument_type = node::https::ssl_websocket;
		using second_argument_type = node::https::ssl_websocket;

		bool operator()(node::https::ssl_websocket const& s1, node::https::ssl_websocket const& s2) const noexcept;
	};

}


#endif
