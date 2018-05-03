/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/https/srv/session.h>
#include <smf/https/srv/handle_request.hpp>

namespace node
{
	namespace https
	{
		plain_session::plain_session(cyng::logging::log_ptr logger
			, session_callback_t cb
			, boost::asio::ip::tcp::socket socket
			, boost::beast::flat_buffer buffer
			, std::string const& doc_root
			, std::vector<std::string> const& sub_protocols)
		: session<plain_session>(logger, cb, socket.get_executor().context(), std::move(buffer), doc_root, sub_protocols)
			, socket_(std::move(socket))
			, strand_(socket_.get_executor())
		{}

		plain_session::~plain_session()
		{}

		// Called by the base class
		boost::asio::ip::tcp::socket& plain_session::stream()
		{
			return socket_;
		}

		// Called by the base class
		boost::asio::ip::tcp::socket plain_session::release_stream()
		{
			return std::move(socket_);
		}

		// Start the asynchronous operation
		void plain_session::run(cyng::object obj)
		{
			cb_(cyng::generate_invoke("https.launch.session.plain", obj, socket_.remote_endpoint()));

			// Run the timer. The timer is operated
			// continuously, this simplifies the code.
			//on_timer({});
			on_timer(obj, boost::system::error_code());
			do_read(obj);
		}

		void plain_session::do_eof(cyng::object obj)
		{
			// Send a TCP shutdown
			boost::system::error_code ec;
			socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);

			cb_(cyng::generate_invoke("https.eof.session.plain", obj));
			// At this point the connection is closed gracefully
		}

		void plain_session::do_timeout(cyng::object obj)
		{
			// Closing the socket cancels all outstanding operations. They
			// will complete with boost::asio::error::operation_aborted
			boost::system::error_code ec;
			socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
			socket_.close(ec);
		}

		ssl_session::ssl_session(cyng::logging::log_ptr logger
			, session_callback_t cb
			, boost::asio::ip::tcp::socket socket
			, boost::asio::ssl::context& ctx
			, boost::beast::flat_buffer buffer
			, std::string const& doc_root
			, std::vector<std::string> const& sub_protocols)
		: session<ssl_session>(logger, cb, socket.get_executor().context(), std::move(buffer), doc_root, sub_protocols)
			, stream_(std::move(socket), ctx)
			, strand_(stream_.get_executor())
		{}

		ssl_session::~ssl_session()
		{
			//std::cerr << "ssl_session::~ssl_session()" << std::endl;
		}

		// Called by the base class
		ssl_stream<boost::asio::ip::tcp::socket>& ssl_session::stream()
		{
			return stream_;
		}

		// Called by the base class
		ssl_stream<boost::asio::ip::tcp::socket> ssl_session::release_stream()
		{
			return std::move(stream_);
		}

		// Start the asynchronous operation
		void ssl_session::run(cyng::object obj)
		{
			cb_(cyng::generate_invoke("https.launch.session.ssl", obj, stream_.lowest_layer().remote_endpoint()));

			// Run the timer. The timer is operated
			// continuously, this simplifies the code.
			//on_timer({});
			on_timer(obj, boost::system::error_code());

			// Set the timer
			timer_.expires_after(std::chrono::seconds(15));

			// Perform the SSL handshake
			// Note, this is the buffered version of the handshake.
			stream_.async_handshake(
				boost::asio::ssl::stream_base::server,
				buffer_.data(),
				boost::asio::bind_executor(
					strand_,
					std::bind(
						&ssl_session::on_handshake,
						this, 
						//shared_from_this(),
						obj,
						std::placeholders::_1,
						std::placeholders::_2)));
		}

		void ssl_session::on_handshake(cyng::object obj
			, boost::system::error_code ec
			, std::size_t bytes_used)
		{
			// Happens when the handshake times out
			if (ec == boost::asio::error::operation_aborted)
			{
				return;
			}

			if (ec)
			{
				CYNG_LOG_FATAL(logger_, "handshake: " << ec.message());
				return;
			}

			// Consume the portion of the buffer used by the handshake
			buffer_.consume(bytes_used);

			do_read(obj);
		}

		void ssl_session::do_eof(cyng::object obj)
		{
			eof_ = true;

			// Set the timer
			timer_.expires_after(std::chrono::seconds(15));

			// Perform the SSL shutdown
			stream_.async_shutdown(
				boost::asio::bind_executor(
					strand_,
					std::bind(
						&ssl_session::on_shutdown,
						this, 
						obj,
						std::placeholders::_1)));

			cb_(cyng::generate_invoke("https.eof.session.ssl", obj));

		}

		void ssl_session::on_shutdown(cyng::object obj, boost::system::error_code ec)
		{
			// Happens when the shutdown times out
			if (ec == boost::asio::error::operation_aborted)
			{
				return;
			}

			if (ec)
			{
				//return fail(ec, "shutdown");
				return;
			}

			cb_(cyng::generate_invoke("https.on.shutdown.session.ssl", obj));
			// At this point the connection is closed gracefully
		}

		void ssl_session::do_timeout(cyng::object obj)
		{
			// If this is true it means we timed out performing the shutdown
			if (eof_)
			{
				return;
			}

			// Start the timer again
			timer_.expires_at((std::chrono::steady_clock::time_point::max)());
			//on_timer({});
			on_timer(obj, boost::system::error_code());
			do_eof(obj);
		}

	}
}

namespace cyng
{
	namespace traits
	{

#if defined(CYNG_LEGACY_MODE_ON)
		const char type_tag<node::https::plain_session>::name[] = "plain-session";
		const char type_tag<node::https::ssl_session>::name[] = "ssl-session";
#endif
	}	// traits	

}

