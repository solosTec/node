/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/https/srv/session.h>
//#include <smf/https/srv/websocket.h>
#include <smf/https/srv/handle_request.hpp>

namespace node
{
	namespace https
	{
		session::session(boost::asio::ip::tcp::socket socket,
			boost::asio::ssl::context& ctx,
			std::string const& doc_root)
			: socket_(std::move(socket))
			, stream_(socket_, ctx)
			, strand_(socket_.get_executor())
			, doc_root_(doc_root)
			, lambda_(*this)
		{}

		// Start the asynchronous operation
		void session::run()
		{
			// Perform the boost::asio::ssl handshake
			stream_.async_handshake(
				boost::asio::ssl::stream_base::server,
				boost::asio::bind_executor(
					strand_,
					std::bind(
						&session::on_handshake,
						shared_from_this(),
						std::placeholders::_1)));
		}

		void session::on_handshake(boost::system::error_code ec)
		{
			if (ec)
			{
				std::cerr << "***error: " << ec.message() << std::endl;
				//BOOST_ASSERT_MSG(!ec, "HANDSHAKE");
				return;
			}

			do_read();
		}

		void session::do_read()
		{
			// Make the request empty before reading,
			// otherwise the operation behavior is undefined.
			req_ = {};

			// Read a request
			boost::beast::http::async_read(stream_, buffer_, req_,
				boost::asio::bind_executor(
					strand_,
					std::bind(
						&session::on_read,
						shared_from_this(),
						std::placeholders::_1,
						std::placeholders::_2)));
		}

		void session::on_read(boost::system::error_code ec,
				std::size_t bytes_transferred)
		{
			boost::ignore_unused(bytes_transferred);

			// This means they closed the connection
			if (ec == boost::beast::http::error::end_of_stream)
			{
				return do_close();
			}

			if (ec)
			{
				std::cerr << "***warning: " << ec.message() << std::endl;
				return;
			}

			// See if it is a WebSocket Upgrade
			if (boost::beast::websocket::is_upgrade(req_))
			{
				std::cerr << "***info: upgrade to websocket " << socket_.remote_endpoint() << std::endl;
				//CYNG_LOG_ERROR(logger_, "session::upgrade to websocket " << socket_.remote_endpoint());
				// Create a WebSocket websocket_session by transferring the socket
				//connection_manager_.upgrade(shared_from_this())->do_accept(std::move(req_));;
				return;
			}

			// Send the response
			//BOOST_ASSERT_MSG(false, "handle_request");
			handle_request(doc_root_, std::move(req_), lambda_);
		}

		void session::on_write(
				boost::system::error_code ec,
				std::size_t bytes_transferred,
				bool close)
		{
			boost::ignore_unused(bytes_transferred);

			if (ec)
			{
				BOOST_ASSERT_MSG(!ec, "WRITE");
				return;
			}

			if (close)
			{
				// This means we should close the connection, usually because
				// the response indicated the "Connection: close" semantic.
				return do_close();
			}

			// We're done with the response so delete it
			res_ = nullptr;

			// Read another request
			do_read();
		}

		void session::do_close()
		{
			// Perform the boost::asio::ssl shutdown
			stream_.async_shutdown(
				boost::asio::bind_executor(
					strand_,
					std::bind(
						&session::on_shutdown,
						shared_from_this(),
						std::placeholders::_1)));
		}

		void session::on_shutdown(boost::system::error_code ec)
		{
			if (ec)
			{
				BOOST_ASSERT_MSG(!ec, "SHUTDOWN");
				return;
			}

			// At this point the connection is closed gracefully
		}
	}
}

