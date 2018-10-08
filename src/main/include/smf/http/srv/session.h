/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_HTTP_SRV_SESSION_H
#define NODE_LIB_HTTP_SRV_SESSION_H

#include <cyng/log.h>
#include <smf/cluster/bus.h>
#include <memory>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>

namespace node
{
	namespace http
	{
		class connection_manager;
		class session //: public std::enable_shared_from_this<session>
		{
			friend class connection_manager;

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
							boost::beast::http::async_write(
								self_.socket_,
								msg_,
								boost::asio::bind_executor(
									self_.strand_,
									std::bind(
										&session::on_write,
										&self_,
										//self_.shared_from_this(),
										std::placeholders::_1,
										msg_.need_eof())));
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
				, connection_manager& cm
				, boost::asio::ip::tcp::socket socket
				, std::string const& doc_root
				, node::bus::shared_type
				, boost::uuids::uuid);
			virtual ~session();

			boost::uuids::uuid tag() const noexcept;

			// Start the asynchronous operation
			void run(cyng::object);
			void do_read();

			// Called when the timer expires.
			void on_timer(boost::system::error_code ec, cyng::object);

			void on_read(boost::system::error_code ec);
			void on_write(boost::system::error_code ec, bool close);
			void do_close();

			void send_moved(std::string const& location);
			void trigger_download(std::string const& filename, std::string const& attachment);

		private:
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
			boost::beast::http::response<boost::beast::http::empty_body> send_head(std::uint32_t version
				, bool
				, std::string const& path
				, std::uint64_t);
			boost::beast::http::response<boost::beast::http::file_body> send_get(std::uint32_t version
				, bool
				, boost::beast::http::file_body::value_type&&
				, std::string const& path
				, std::uint64_t size);

		private:
			boost::asio::ip::tcp::socket socket_;
			boost::asio::strand<boost::asio::io_context::executor_type> strand_;
			boost::asio::steady_timer timer_;
			boost::beast::flat_buffer buffer_;
			cyng::logging::log_ptr logger_;
			connection_manager& connection_manager_;
			std::string const& doc_root_;
			bus::shared_type bus_;
			const boost::uuids::uuid tag_;
			boost::beast::http::request<boost::beast::http::string_body> req_;
			queue queue_;
            bool shutdown_;
		};

		/**
		* sessions are managed by the session manager as
		* shared pointers
		*/
		//using session_ptr = std::shared_ptr<session>;

		cyng::object make_http_session(cyng::logging::log_ptr
			, connection_manager& cm
			, boost::asio::ip::tcp::socket&& socket
			, std::string const& doc_root
			, bus::shared_type
			, boost::uuids::uuid);


	}
}

#include <cyng/intrinsics/traits.hpp>

namespace cyng
{
	namespace traits
	{
		template <>
		struct type_tag<node::http::session>
		{
			using type = node::http::session;
			using tag = std::integral_constant<std::size_t, PREDEF_SESSION>;
#if defined(CYNG_LEGACY_MODE_ON)
			const static char name[];
#else
			constexpr static char name[] = "http";
#endif
		};

		template <>
		struct reverse_type < PREDEF_SESSION >
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
