/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/https/srv/websocket.h>

namespace node
{
	namespace https
	{
		plain_websocket::plain_websocket(cyng::logging::log_ptr logger
			, connections& cm
			, boost::uuids::uuid tag
#if (BOOST_BEAST_VERSION < 248)
			, boost::asio::ip::tcp::socket socket
#else
			, boost::beast::tcp_stream&& stream
#endif
		)
#if (BOOST_BEAST_VERSION < 248)
		: websocket_session<plain_websocket>(logger, cm, tag, socket.get_executor().context())
			, ws_(std::move(socket))
#else
		: websocket_session<plain_websocket>(logger, cm, tag)
			, ws_(std::move(stream))
#endif
		{
			setup_stream(ws_);
		}

		// Called by the base class
#if (BOOST_BEAST_VERSION < 248)
		boost::beast::websocket::stream<boost::asio::ip::tcp::socket>& plain_websocket::ws()
		{
			return ws_;
		}
#else
		boost::beast::websocket::stream<boost::beast::tcp_stream>& plain_websocket::ws()
		{
			return ws_;
		}
#endif


#if (BOOST_BEAST_VERSION < 248)
		void plain_websocket::do_timeout(cyng::object obj)
		{
			// This is so the close can have a timeout
			if (close_)
			{
				return;
			}
			close_ = true;

			// Set the timer
			timer_.expires_after(std::chrono::seconds(15));

			// Close the WebSocket Connection
			ws_.async_close(
				boost::beast::websocket::close_code::normal,
				boost::asio::bind_executor(
					strand_,
					std::bind(
						&plain_websocket::on_close,
						//shared_from_this(),
						this,
						obj,		//	reference
						std::placeholders::_1)));
		}

		void plain_websocket::on_close(cyng::object obj, boost::system::error_code ec)
		{
			// Happens when close times out
			if (ec == boost::asio::error::operation_aborted)
			{
				return;
			}

			if (ec)
			{
				CYNG_LOG_FATAL(logger_, "close: " << ec.message());
				//return fail(ec, "close");
				return;
			}

			// At this point the connection is gracefully closed
		}
#endif

		ssl_websocket::ssl_websocket(cyng::logging::log_ptr logger
			, connections& cm
			, boost::uuids::uuid tag
#if (BOOST_BEAST_VERSION < 248)
			, boost::beast::ssl_stream<boost::asio::ip::tcp::socket> stream
#else
			, boost::beast::ssl_stream<boost::beast::tcp_stream>&& stream
#endif
		)
#if (BOOST_BEAST_VERSION < 248)
		: websocket_session<ssl_websocket>(logger, cm, tag, stream.get_executor().context())
#else
		: websocket_session<ssl_websocket>(logger, cm, tag)
#endif
			, ws_(std::move(stream))
#if (BOOST_BEAST_VERSION < 248)
			, strand_(ws_.get_executor())
#endif
		{}

		// Called by the base class
#if (BOOST_BEAST_VERSION < 248)
		boost::beast::websocket::stream<boost::beast::ssl_stream<boost::asio::ip::tcp::socket>>& ssl_websocket::ws()
		{
			return ws_;
		}
#else
		boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>>& ssl_websocket::ws()
		{
			return ws_;
		}
#endif

#if (BOOST_BEAST_VERSION < 248)
		void ssl_websocket::do_eof(cyng::object obj)
		{
			eof_ = true;

			// Set the timer
			timer_.expires_after(std::chrono::seconds(15));

			// Perform the SSL shutdown
			ws_.next_layer().async_shutdown(
				boost::asio::bind_executor(
					strand_,
					std::bind(
						&ssl_websocket::on_shutdown,
						//shared_from_this(),
						this,
						obj,	//	reference
						std::placeholders::_1)));
		}

		void ssl_websocket::on_shutdown(cyng::object obj, boost::system::error_code ec)
		{
			// Happens when the shutdown times out
			if (ec == boost::asio::error::operation_aborted)
			{
				return;
			}

			if (ec)
			{
				CYNG_LOG_FATAL(logger_, "shutdown: " << ec.message());
				//return fail(ec, "shutdown");
				return;
			}

			// At this point the connection is closed gracefully
		}

		void ssl_websocket::do_timeout(cyng::object obj)
		{
			// If this is true it means we timed out performing the shutdown
			if (eof_)
			{
				return;
			}

			// Start the timer again
			timer_.expires_at((std::chrono::steady_clock::time_point::max)());
			on_timer(obj, boost::system::error_code{});
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
		const char type_tag<node::https::plain_websocket>::name[] = "plain-websocket";
		const char type_tag<node::https::ssl_websocket>::name[] = "ssl-websocket";
#endif
	}	// traits	
}

namespace std
{
	size_t hash<node::https::plain_websocket>::operator()(node::https::plain_websocket const& s) const noexcept
	{
		return s.hash();
	}

	bool equal_to<node::https::plain_websocket>::operator()(node::https::plain_websocket const& s1, node::https::plain_websocket const& s2) const noexcept
	{
		return s1.hash() == s2.hash();
	}

	bool less<node::https::plain_websocket>::operator()(node::https::plain_websocket const& s1, node::https::plain_websocket const& s2) const noexcept
	{
		return s1.hash() < s2.hash();
	}

	size_t hash<node::https::ssl_websocket>::operator()(node::https::ssl_websocket const& s) const noexcept
	{
		return s.hash();
	}

	bool equal_to<node::https::ssl_websocket>::operator()(node::https::ssl_websocket const& s1, node::https::ssl_websocket const& s2) const noexcept
	{
		return s1.hash() == s2.hash();
	}

	bool less<node::https::ssl_websocket>::operator()(node::https::ssl_websocket const& s1, node::https::ssl_websocket const& s2) const noexcept
	{
		return s1.hash() < s2.hash();
	}

}
