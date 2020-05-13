/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_HTTP_SRV_SESSION_H
#define NODE_LIB_HTTP_SRV_SESSION_H

#ifdef NODE_SSL_INSTALLED
#include <smf/http/srv/auth.h>
#endif

#include <cyng/log.h>

#include <memory>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>

namespace node
{
	namespace http
	{
		//	forward declaration(s):
		class connections;

		class session
		{
			// This queue is used for HTTP pipelining.
			class queue
			{
				enum
				{
					// Maximum number of responses we will queue
					limit = 8
				};

				// The type-erased, saved work item
				struct work
				{
					virtual ~work() = default;
					virtual void operator()() = 0;
				};

				session& self_;
				std::vector<std::unique_ptr<work>> items_;

			public:
				explicit queue(session& self);

				// Returns `true` if we have reached the queue limit
				bool is_full() const;

				// Called when a message finishes sending
				// Returns `true` if the caller should initiate a read
				bool on_write();

				// Called by the HTTP handler to send a response.
				template<bool isRequest, class Body, class Fields>
				void operator()(boost::beast::http::message<isRequest, Body, Fields>&& msg)
				{
					// This holds a work item
					struct work_impl : work
					{
						session& self_;
						boost::beast::http::message<isRequest, Body, Fields> msg_;

						work_impl(session& self,
							boost::beast::http::message<isRequest, Body, Fields>&& msg)
							: self_(self)
							, msg_(std::move(msg))
						{
						}

						void operator()()
						{
#if (BOOST_BEAST_VERSION < 248)
							boost::beast::http::async_write(
								self_.socket_,
								msg_,
								boost::asio::bind_executor(
									self_.strand_,
									std::bind(
										&session::on_write,
										&self_,
										std::placeholders::_1,
										msg_.need_eof())));
#else
							boost::beast::http::async_write(
								self_.stream_,
								msg_,
								boost::beast::bind_front_handler(
									&session::on_write,
									&self_,
									msg_.need_eof()
								)
							);
#endif
						}
					};

					// Allocate and store the work
					items_.emplace_back(new work_impl(self_, std::move(msg)));

					// If there was no previous work, start this one
					if (items_.size() == 1)
					{
						(*items_.front())();
					}
				}
			};


		public:
			// Take ownership of the socket
			explicit session(cyng::logging::log_ptr
				, connections& cm
				, boost::uuids::uuid
				, boost::asio::ip::tcp::socket socket
				, std::chrono::seconds timeout
				, std::uint64_t max_upload_size
				, std::string const& doc_root
#ifdef NODE_SSL_INSTALLED
				, auth_dirs const& ad
#endif
				, bool https_redirect
			);
			virtual ~session();

			boost::uuids::uuid tag() const noexcept;

			// Start the asynchronous operation
			void run(cyng::object);
			void do_read();

#if (BOOST_BEAST_VERSION < 248)
			// Called when the timer expires.
			void on_timer(boost::system::error_code ec, cyng::object);
#endif

#if (BOOST_BEAST_VERSION < 248)
			void on_read(boost::system::error_code ec);
			void on_write(boost::system::error_code ec, bool close);
#else
			void on_read(boost::beast::error_code ec, std::size_t bytes_transferred);
			void on_write(bool close, boost::beast::error_code ec, std::size_t bytes_transferred);
#endif
			void do_close();

			void send_moved(std::string const& location);
			void trigger_download(boost::filesystem::path const& filename, std::string const& attachment);

		private:
			bool check_auth(boost::beast::http::request<boost::beast::http::string_body> const&);

			void handle_request(boost::beast::http::request<boost::beast::http::string_body>&&);

			boost::beast::http::response<boost::beast::http::string_body> send_bad_request(std::uint32_t version
				, bool
				, std::string const& why);
			boost::beast::http::response<boost::beast::http::string_body> send_not_found(std::uint32_t version
				, bool
				, std::string target);
			boost::beast::http::response<boost::beast::http::string_body> send_server_error(std::uint32_t version
				, bool
				, boost::system::error_code ec);
			boost::beast::http::response<boost::beast::http::string_body> send_redirect(std::uint32_t version
				, bool
				, std::string host
				, std::string target);
			boost::beast::http::response<boost::beast::http::empty_body> send_head(std::uint32_t version
				, bool
				, std::string const& path
				, std::uint64_t);
			boost::beast::http::response<boost::beast::http::file_body> send_get(std::uint32_t version
				, bool
				, boost::beast::http::file_body::value_type&&
				, std::string const& path
				, std::uint64_t size);
#ifdef NODE_SSL_INSTALLED
			boost::beast::http::response<boost::beast::http::string_body> send_not_authorized(std::uint32_t version
				, bool keep_alive
				, std::string target
				, std::string realm);
#endif


		private:
			cyng::logging::log_ptr logger_;
			boost::uuids::uuid const tag_;
			std::uint64_t const max_upload_size_;
			std::string const doc_root_;
#ifdef NODE_SSL_INSTALLED
			auth_dirs const& auth_dirs_;
#endif
			bool const https_rewrite_;

#if (BOOST_BEAST_VERSION < 248)
			boost::asio::ip::tcp::socket socket_;
			boost::asio::strand<boost::asio::io_context::executor_type> strand_;
			boost::asio::steady_timer timer_;
			boost::beast::http::request<boost::beast::http::string_body> req_;
#else
			boost::beast::tcp_stream stream_;

			// The parser is stored in an optional container so we can
			// construct it from scratch it at the beginning of each new message.
			boost::optional<boost::beast::http::request_parser<boost::beast::http::string_body>> parser_;
#endif
			std::chrono::nanoseconds timeout_;
			connections& connection_manager_;
			boost::beast::flat_buffer buffer_;
			queue queue_;
            bool shutdown_;
			bool authorized_;
		};
	}
}

#include <cyng/intrinsics/traits.hpp>
#include <cyng/intrinsics/traits/tag.hpp>

namespace cyng
{
	//
	//	make the session object compatible with the cyng object system
	//
	namespace traits
	{
		template <>
		struct type_tag<node::http::session>
		{
			using type = node::http::session;
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
			using type = node::http::session;
		};
	}
}

namespace std
{
	template<>
	struct hash<node::http::session>
	{
		size_t operator()(node::http::session const& t) const noexcept;
	};

	template<>
	struct equal_to<node::http::session>
	{
		//	pre C++17
		using result_type = bool;
		using first_argument_type = node::http::session;
		using second_argument_type = node::http::session;

		bool operator()(node::http::session const& t1, node::http::session const& t2) const noexcept;
	};

	template<>
	struct less<node::http::session>
	{
		//	pre C++17
		using result_type = bool;
		using first_argument_type = node::http::session;
		using second_argument_type = node::http::session;

		bool operator()(node::http::session const& t1, node::http::session const& t2) const noexcept;
	};

}

#endif
