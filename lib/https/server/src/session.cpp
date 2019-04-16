/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/https/srv/session.h>
#include <smf/https/srv/connections.h>

#include <cyng/vm/controller.h>
#include <cyng/vm/generator.h>

#include <boost/uuid/uuid_io.hpp>

namespace node
{
	namespace https
	{
		plain_session::plain_session(cyng::logging::log_ptr logger
			, connections& cm
			, boost::uuids::uuid tag
#if (BOOST_BEAST_VERSION < 248)
			, boost::asio::ip::tcp::socket socket
#else
			, boost::beast::tcp_stream&& stream
#endif
			, boost::beast::flat_buffer buffer
			, std::string const& doc_root
			, auth_dirs const& ad)
#if (BOOST_BEAST_VERSION < 248)
		: session<plain_session>(logger, cm, tag, socket.get_executor().context(), std::move(buffer), doc_root, ad)
			, socket_(std::move(socket))
			, strand_(socket_.get_executor())
#else
		: session<plain_session>(logger, cm, tag, std::move(buffer), doc_root, ad)
			, stream_(std::move(stream))
#endif
		{}

		plain_session::~plain_session()
		{}

		// Called by the base class
#if (BOOST_BEAST_VERSION < 248)
		boost::asio::ip::tcp::socket& plain_session::stream()
		{
			return socket_;
		}

		// Called by the base class
		boost::asio::ip::tcp::socket plain_session::release_stream()
		{
			return std::move(socket_);
		}
#else
		boost::beast::tcp_stream& plain_session::stream()
		{
			return stream_;
		}

		boost::beast::tcp_stream plain_session::release_stream()
		{
			return std::move(stream_);
		}
#endif

		// Start the asynchronous operation
		void plain_session::run(cyng::object obj)
		{

#if (BOOST_BEAST_VERSION < 248)
			//
			//	signal session launch
			//
			this->connection_manager_.vm().async_run(cyng::generate_invoke("http.session.launch", tag(), false, stream().lowest_layer().remote_endpoint()));

			// Run the timer. The timer is operated
			// continuously, this simplifies the code.
			on_timer(obj, boost::system::error_code{});
#else
			//
			//	signal session launch
			//
			this->connection_manager_.vm().async_run(cyng::generate_invoke("http.session.launch", tag(), false, stream().socket().remote_endpoint()));
#endif

			do_read(obj);
		}

		void plain_session::do_eof(cyng::object obj)
		{
			// Send a TCP shutdown
			boost::system::error_code ec;
#if (BOOST_BEAST_VERSION < 248)
			socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
#else
			stream_.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
#endif

		}

#if (BOOST_BEAST_VERSION < 248)
		void plain_session::do_timeout(cyng::object obj)
		{
			// Closing the socket cancels all outstanding operations. They
			// will complete with boost::asio::error::operation_aborted
			boost::system::error_code ec;
			socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
			socket_.close(ec);
		}
#endif

		ssl_session::ssl_session(cyng::logging::log_ptr logger
			, connections& cm
			, boost::uuids::uuid tag
#if (BOOST_BEAST_VERSION < 248)
			, boost::asio::ip::tcp::socket socket
#else
			, boost::beast::tcp_stream&& stream
#endif
			, boost::asio::ssl::context& ctx
			, boost::beast::flat_buffer buffer
			, std::string const& doc_root
			, auth_dirs const& ad)
#if (BOOST_BEAST_VERSION < 248)
		: session<ssl_session>(logger, cm, tag, socket.get_executor().context(), std::move(buffer), doc_root, ad)
			, stream_(std::move(socket), ctx)
			, strand_(stream_.get_executor())
#else
		: session<ssl_session>(logger, cm, tag, std::move(buffer), doc_root, ad)
			, stream_(std::move(stream), ctx)
#endif
		{}

		ssl_session::~ssl_session()
		{
			//std::cerr << "ssl_session::~ssl_session()" << std::endl;
		}

#if (BOOST_BEAST_VERSION < 248)
		// Called by the base class
		boost::beast::ssl_stream<boost::asio::ip::tcp::socket>& ssl_session::stream()
		{
			return stream_;
		}

		// Called by the base class
		boost::beast::ssl_stream<boost::asio::ip::tcp::socket> ssl_session::release_stream()
		{
			return std::move(stream_);
		}
#else
		boost::beast::ssl_stream<boost::beast::tcp_stream>& ssl_session::stream()
		{
			return stream_;
		}

		boost::beast::ssl_stream<boost::beast::tcp_stream> ssl_session::release_stream()
		{
			return std::move(stream_);
		}
#endif

		// Start the asynchronous operation
		void ssl_session::run(cyng::object obj)
		{

#if (BOOST_BEAST_VERSION < 248)
			//
			//	signal session launch
			//
			this->connection_manager_.vm().async_run(cyng::generate_invoke("http.session.launch", tag(), true, stream().lowest_layer().remote_endpoint()));

			// Run the timer. The timer is operated
			// continuously, this simplifies the code.
			on_timer(obj, boost::system::error_code{});

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
						obj,	//	hold reference
						std::placeholders::_1,
						std::placeholders::_2)));
#else
			//
			//	signal session launch
			//
			this->connection_manager_.vm().async_run(cyng::generate_invoke("http.session.launch", tag(), true, stream().next_layer().socket().remote_endpoint()));

			// Set the timeout.
			boost::beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

			// Perform the SSL handshake
			// Note, this is the buffered version of the handshake.
			stream_.async_handshake(
				boost::asio::ssl::stream_base::server,
				buffer_.data(),
				boost::beast::bind_front_handler(
					&ssl_session::on_handshake,
					this,
					obj));

#endif
		}

		void ssl_session::on_handshake(cyng::object obj
			, boost::system::error_code ec
			, std::size_t bytes_used)
		{
#if (BOOST_BEAST_VERSION < 248)
			// Happens when the handshake times out
			if (ec == boost::asio::error::operation_aborted)	{
				CYNG_LOG_ERROR(logger_, "handshake timeout ");
				return;
			}
#endif

			if (ec)	{
				CYNG_LOG_FATAL(logger_, "handshake: " << ec.message());
				return;
			}

			// Consume the portion of the buffer used by the handshake
			buffer_.consume(bytes_used);

			do_read(obj);
		}

		void ssl_session::do_eof(cyng::object obj)
		{
#if (BOOST_BEAST_VERSION < 248)
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

#else
			// Set the timeout.
			boost::beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

			// Perform the SSL shutdown
			stream_.async_shutdown(
				boost::beast::bind_front_handler(
					&ssl_session::on_shutdown,
					this,
					obj));

#endif
			//
			//	log SSL shutdown
			//
			CYNG_LOG_WARNING(logger_, tag() << " - SSL shutdown");

		}

		void ssl_session::on_shutdown(cyng::object obj, boost::system::error_code ec)
		{
#if (BOOST_BEAST_VERSION < 248)
			// Happens when the shutdown times out
			if (ec == boost::asio::error::operation_aborted)
			{
				CYNG_LOG_WARNING(logger_, tag() << " - SSL shutdown timeout");
				connection_manager_.stop_session(tag());
				return;
			}
#endif

			if (ec)
			{
				CYNG_LOG_WARNING(logger_, tag() << " - SSL shutdown failed");
				connection_manager_.stop_session(tag());
				return;
			}

			//
			//	let connection manager remove this session
			//
			connection_manager_.stop_session(tag());

			// At this point the connection is closed gracefully
		}

#if (BOOST_BEAST_VERSION < 248)
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
#endif
	}
}

namespace cyng
{
	namespace traits
	{

#if !defined(__CPP_SUPPORT_N2235)
		const char type_tag<node::https::plain_session>::name[] = "plain-session";
		const char type_tag<node::https::ssl_session>::name[] = "ssl-session";
#endif
	}	// traits	
}

namespace std
{
	size_t hash<node::https::plain_session>::operator()(node::https::plain_session const& s) const noexcept
	{
		return s.hash();
	}

	bool equal_to<node::https::plain_session>::operator()(node::https::plain_session const& s1, node::https::plain_session const& s2) const noexcept
	{
		return s1.hash() == s2.hash();
	}

	bool less<node::https::plain_session>::operator()(node::https::plain_session const& s1, node::https::plain_session const& s2) const noexcept
	{
		return s1.hash() < s2.hash();
	}

	size_t hash<node::https::ssl_session>::operator()(node::https::ssl_session const& s) const noexcept
	{
		return s.hash();
	}

	bool equal_to<node::https::ssl_session>::operator()(node::https::ssl_session const& s1, node::https::ssl_session const& s2) const noexcept
	{
		return s1.hash() == s2.hash();
	}

	bool less<node::https::ssl_session>::operator()(node::https::ssl_session const& s1, node::https::ssl_session const& s2) const noexcept
	{
		return s1.hash() < s2.hash();
	}

}

