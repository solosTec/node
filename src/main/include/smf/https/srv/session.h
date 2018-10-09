/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_HTTPS_SRV_SESSION_H
#define NODE_LIB_HTTPS_SRV_SESSION_H

#include <smf/https/srv/session.hpp>
#include <smf/https/srv/ssl_stream.hpp>
//#include <smf/https/srv/connections.h>
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
				, boost::uuids::uuid
				, boost::asio::ip::tcp::socket socket
				, boost::beast::flat_buffer buffer
				, std::string const& doc_root);

			virtual ~plain_session();

			// Called by the base class
			boost::asio::ip::tcp::socket& stream();

			// Called by the base class
			boost::asio::ip::tcp::socket release_stream();

			// Start the asynchronous operation
			void run(cyng::object);
			void do_timeout(cyng::object obj);
			void do_eof(cyng::object obj);

		private:
			boost::asio::ip::tcp::socket socket_;
			boost::asio::strand<boost::asio::io_context::executor_type> strand_;
		};

		/** 
		 * Handles an SSL HTTP connection
		 */
		class ssl_session : public session<ssl_session>
		{
		public:
			// Create the session
			ssl_session(cyng::logging::log_ptr logger
				, boost::uuids::uuid
				, boost::asio::ip::tcp::socket socket
				, boost::asio::ssl::context& ctx
				, boost::beast::flat_buffer buffer
				, std::string const& doc_root);

			virtual ~ssl_session();

			// Called by the base class
			ssl_stream<boost::asio::ip::tcp::socket>& stream();

			// Called by the base class
			ssl_stream<boost::asio::ip::tcp::socket> release_stream();

			/**
			 * Start the asynchronous operation
			 */
			void run(cyng::object);
			void do_eof(cyng::object obj);
			void do_timeout(cyng::object obj);

		private:
			void on_handshake(cyng::object obj, boost::system::error_code ec, std::size_t bytes_used);
			void on_shutdown(cyng::object obj, boost::system::error_code ec);

		private:
			ssl_stream<boost::asio::ip::tcp::socket> stream_;
			boost::asio::strand<boost::asio::io_context::executor_type> strand_;
			bool eof_ = false;

		};

		cyng::object make_plain_session(cyng::logging::log_ptr
			, connections& cm
			, boost::asio::ip::tcp::socket&& socket
			, std::string const& doc_root
			, boost::uuids::uuid);

		cyng::object make_ssl_session(cyng::logging::log_ptr
			, connections& cm
			, boost::asio::ip::tcp::socket&& socket
			, std::string const& doc_root
			, boost::uuids::uuid);
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
			using tag = std::integral_constant<std::size_t, PREDEF_SESSION>;
#if defined(CYNG_LEGACY_MODE_ON)
			const static char name[];
#else
			constexpr static char name[] = "plain-session";
#endif
		};

		template <>
		struct reverse_type < PREDEF_SESSION >
		{
			using type = node::https::plain_session;
		};

		template <>
		struct type_tag<node::https::ssl_session>
		{
			using type = node::https::ssl_session;
			using tag = std::integral_constant<std::size_t, PREDEF_SSL_SESSION>;
#if defined(CYNG_LEGACY_MODE_ON)
			const static char name[];
#else
			constexpr static char name[] = "ssl-session";
#endif
		};

		template <>
		struct reverse_type < PREDEF_SSL_SESSION >
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
