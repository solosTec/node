/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_HTTPS_SRV_SESSION_H
#define NODE_LIB_HTTPS_SRV_SESSION_H

#include <smf/https/srv/session.hpp>
#if (BOOST_BEAST_VERSION < 248)
#include <smf/https/srv/ssl_stream.hpp>
#else
#include <boost/beast/ssl/ssl_stream.hpp>
#endif
#include <cyng/object.h>

namespace node
{
	namespace https
	{
		//	forward declaration(s):
		class connections;

		/**
		 * Handles a plain HTTP connection
		 */
		class plain_session : public session<plain_session>
		{

		public:
			// Create the http_session
			plain_session(cyng::logging::log_ptr logger
				, connections&
				, boost::uuids::uuid
#if (BOOST_BEAST_VERSION < 248)
				, boost::asio::ip::tcp::socket socket
#else
				, boost::beast::tcp_stream&& stream
#endif
				, boost::beast::flat_buffer buffer
				, std::uint64_t max_upload_size
				, std::string const& doc_root
				, auth_dirs const& ad);

			virtual ~plain_session();

			// Called by the base class
#if (BOOST_BEAST_VERSION < 248)
			boost::asio::ip::tcp::socket& stream();

			// Called by the base class
			boost::asio::ip::tcp::socket release_stream();
			void do_timeout(cyng::object obj);
#else
			boost::beast::tcp_stream& stream();
			boost::beast::tcp_stream release_stream();
#endif

			// Start the asynchronous operation
			void run(cyng::object);
			void do_eof(cyng::object obj);

		private:
#if (BOOST_BEAST_VERSION < 248)
			boost::asio::ip::tcp::socket socket_;
			boost::asio::strand<boost::asio::io_context::executor_type> strand_;
#else
			boost::beast::tcp_stream stream_;
#endif

		};

		/** 
		 * Handles an SSL HTTP connection
		 */
		class ssl_session : public session<ssl_session>
		{
		public:
			// Create the session
			ssl_session(cyng::logging::log_ptr logger
				, connections&
				, boost::uuids::uuid
#if (BOOST_BEAST_VERSION < 248)
				, boost::asio::ip::tcp::socket socket
#else
				, boost::beast::tcp_stream&& stream
#endif
				, boost::asio::ssl::context& ctx
				, boost::beast::flat_buffer buffer
				, std::uint64_t max_upload_size
				, std::string const& doc_root
				, auth_dirs const& ad);

			virtual ~ssl_session();

			// Called by the base class
#if (BOOST_BEAST_VERSION < 248)
			boost::beast::ssl_stream<boost::asio::ip::tcp::socket>& stream();
			boost::beast::ssl_stream<boost::asio::ip::tcp::socket> release_stream();
			void do_timeout(cyng::object obj);
#else
			boost::beast::ssl_stream<boost::beast::tcp_stream>& stream();
			boost::beast::ssl_stream<boost::beast::tcp_stream> release_stream();
#endif

			/**
			 * Start the asynchronous operation
			 */
			void run(cyng::object);
			void do_eof(cyng::object obj);

		private:
			void on_handshake(cyng::object obj, boost::system::error_code ec, std::size_t bytes_used);
			void on_shutdown(cyng::object obj, boost::system::error_code ec);

		private:
#if (BOOST_BEAST_VERSION < 248)
			boost::beast::ssl_stream<boost::asio::ip::tcp::socket> stream_;
			boost::asio::strand<boost::asio::io_context::executor_type> strand_;
			bool eof_ = false;
#else
			boost::beast::ssl_stream<boost::beast::tcp_stream> stream_;
#endif

		};

	}
}

#include <cyng/intrinsics/traits.hpp>
namespace cyng
{
	//
	//	make the session objects compatible with the cyng object system
	//
	namespace traits
	{
		template <>
		struct type_tag<node::https::plain_session>
		{
			using type = node::https::plain_session;
			using tag = std::integral_constant<std::size_t, 
#if defined(__CPP_SUPPORT_N2347)
				static_cast<std::size_t>(traits::predef_type_code::PREDEF_SESSION)
#else
				PREDEF_SESSION
#endif
			>;

#if defined(__CPP_SUPPORT_N2235)
			constexpr static char name[] = "plain-session";
#else
			const static char name[];
#endif
		};

		template <>
		struct reverse_type <
#if defined(__CPP_SUPPORT_N2347)
			static_cast<std::size_t>(traits::predef_type_code::PREDEF_SESSION)
#else
			PREDEF_SESSION
#endif
		>
		{
			using type = node::https::plain_session;
		};

		template <>
		struct type_tag<node::https::ssl_session>
		{
			using type = node::https::ssl_session;
			using tag = std::integral_constant<std::size_t, 
#if defined(__CPP_SUPPORT_N2347)
				static_cast<std::size_t>(traits::predef_type_code::PREDEF_SSL_SESSION)
#else
				PREDEF_SSL_SESSION
#endif
			>;

#if defined(__CPP_SUPPORT_N2235)
			constexpr static char name[] = "ssl-session";
#else
			const static char name[];
#endif
		};

		template <>
		struct reverse_type < 
#if defined(__CPP_SUPPORT_N2347)
			static_cast<std::size_t>(traits::predef_type_code::PREDEF_SSL_SESSION)
#else
			PREDEF_SSL_SESSION
#endif
		>
		{
			using type = node::https::ssl_session;
		};
	}
}

#include <functional>

namespace std
{
	template<>
	struct hash<node::https::plain_session>
	{
		size_t operator()(node::https::plain_session const& s) const noexcept;
	};

	template<>
	struct equal_to<node::https::plain_session>
	{
		//	pre C++17
		using result_type = bool;
		using first_argument_type = node::https::plain_session;
		using second_argument_type = node::https::plain_session;

		bool operator()(node::https::plain_session const& s1, node::https::plain_session const& s2) const noexcept;
	};

	template<>
	struct less<node::https::plain_session>
	{
		//	pre C++17
		using result_type = bool;
		using first_argument_type = node::https::plain_session;
		using second_argument_type = node::https::plain_session;

		bool operator()(node::https::plain_session const& s1, node::https::plain_session const& s2) const noexcept;
	};

	template<>
	struct hash<node::https::ssl_session>
	{
		size_t operator()(node::https::ssl_session const& s) const noexcept;
	};

	template<>
	struct equal_to<node::https::ssl_session>
	{
		//	pre C++17
		using result_type = bool;
		using first_argument_type = node::https::ssl_session;
		using second_argument_type = node::https::ssl_session;

		bool operator()(node::https::ssl_session const& s1, node::https::ssl_session const& s2) const noexcept;
	};

	template<>
	struct less<node::https::ssl_session>
	{
		//	pre C++17
		using result_type = bool;
		using first_argument_type = node::https::ssl_session;
		using second_argument_type = node::https::ssl_session;

		bool operator()(node::https::ssl_session const& s1, node::https::ssl_session const& s2) const noexcept;
	};

}

#endif
