/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <memory>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl/stream.hpp>

namespace node
{
	namespace https
	{
		// Handles an boost::beast::http server connection
		class session : public std::enable_shared_from_this<session>
		{
			// This is the C++11 equivalent of a generic lambda.
			// The function object is used to send an boost::beast::http message.
			struct send_lambda
			{
				session& self_;

				explicit
					send_lambda(session& self)
					: self_(self)
				{
				}

				template<bool isRequest, class Body, class Fields>
				void operator()(boost::beast::http::message<isRequest, Body, Fields>&& msg) const
				{
					// The lifetime of the message has to extend
					// for the duration of the async operation so
					// we use a shared_ptr to manage it.
					auto sp = std::make_shared<
						boost::beast::http::message<isRequest, Body, Fields>>(std::move(msg));

					// Store a type-erased version of the shared
					// pointer in the class to keep it alive.
					self_.res_ = sp;

					// Write the response
					boost::beast::http::async_write(
						self_.stream_,
						*sp,
						boost::asio::bind_executor(
							self_.strand_,
							std::bind(
								&session::on_write,
								self_.shared_from_this(),
								std::placeholders::_1,
								std::placeholders::_2,
								sp->need_eof())));
				}
			};

			boost::asio::ip::tcp::socket socket_;
			boost::asio::ssl::stream<boost::asio::ip::tcp::socket&> stream_;
			boost::asio::strand<boost::asio::io_context::executor_type> strand_;
			boost::beast::flat_buffer buffer_;
			std::string const& doc_root_;
			boost::beast::http::request<boost::beast::http::string_body> req_;
			std::shared_ptr<void> res_;
			send_lambda lambda_;

		public:
			// Take ownership of the socket
			explicit session(boost::asio::ip::tcp::socket socket,
				boost::asio::ssl::context& ctx,
				std::string const& doc_root);

			// Start the asynchronous operation
			void run();
			void on_handshake(boost::system::error_code ec);
			void do_read();
			void on_read(boost::system::error_code ec,
				std::size_t bytes_transferred);
			void on_write(boost::system::error_code ec,
				std::size_t bytes_transferred,
				bool close);
			void do_close();
			void on_shutdown(boost::system::error_code ec);
		};
	}
}