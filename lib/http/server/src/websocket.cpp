/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/http/srv/websocket.h>
#include <boost/assert.hpp>

namespace node
{

	websocket_session::websocket_session(boost::asio::ip::tcp::socket socket)
		: ws_(std::move(socket))
		, strand_(ws_.get_executor())
		, timer_(ws_.get_executor().context(),
		(std::chrono::steady_clock::time_point::max)())
	{}

	void websocket_session::on_accept(boost::system::error_code ec)
	{
		// Happens when the timer closes the socket
		if (ec == boost::asio::error::operation_aborted)
			return;

		if (ec)
		{
			BOOST_ASSERT_MSG(!ec, "FAIL");
			return;
		}
			//return fail(ec, "accept");

		// Read a message
		do_read();
	}

	// Called when the timer expires.
	void websocket_session::on_timer(boost::system::error_code ec)
	{
		if (ec && ec != boost::asio::error::operation_aborted)
		{
			BOOST_ASSERT_MSG(!ec, "TIMER");
			return;
		}

		// See if the timer really expired since the deadline may have moved.
		if (timer_.expiry() <= std::chrono::steady_clock::now())
		{
			// If this is the first time the timer expired,
			// send a ping to see if the other end is there.
			if (ws_.is_open() && ping_state_ == 0)
			{
				// Note that we are sending a ping
				ping_state_ = 1;

				// Set the timer
				timer_.expires_after(std::chrono::seconds(15));

				// Now send the ping
				ws_.async_ping({},
					boost::asio::bind_executor(
						strand_,
						std::bind(
							&websocket_session::on_ping,
							shared_from_this(),
							std::placeholders::_1)));
			}
			else
			{
				// The timer expired while trying to handshake,
				// or we sent a ping and it never completed or
				// we never got back a control frame, so close.

				// Closing the socket cancels all outstanding operations. They
				// will complete with boost::asio::error::operation_aborted
				ws_.next_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
				ws_.next_layer().close(ec);
				return;
			}
		}

		// Wait on the timer
		timer_.async_wait(
			boost::asio::bind_executor(
				strand_,
				std::bind(
					&websocket_session::on_timer,
					shared_from_this(),
					std::placeholders::_1)));
	}

	// Called to indicate activity from the remote peer
	void websocket_session::activity()
	{
		// Note that the connection is alive
		ping_state_ = 0;

		// Set the timer
		timer_.expires_after(std::chrono::seconds(15));
	}

		// Called after a ping is sent.
	void websocket_session::on_ping(boost::system::error_code ec)
	{
		// Happens when the timer closes the socket
		if (ec == boost::asio::error::operation_aborted)
			return;

		if (ec)
		{
			BOOST_ASSERT_MSG(!ec, "PING");
			return;
		}

		// Note that the ping was sent.
		if (ping_state_ == 1)
		{
			ping_state_ = 2;
		}
		else
		{
			// ping_state_ could have been set to 0
			// if an incoming control frame was received
			// at exactly the same time we sent a ping.
			BOOST_ASSERT(ping_state_ == 0);
		}
	}

	void websocket_session::on_control_callback(boost::beast::websocket::frame_type kind,
		boost::beast::string_view payload)
	{
		boost::ignore_unused(kind, payload);

		// Note that there is activity
		activity();
	}

	void websocket_session::do_read()
	{
		// Read a message into our buffer
		ws_.async_read(
			buffer_,
			boost::asio::bind_executor(
				strand_,
				std::bind(
					&websocket_session::on_read,
					shared_from_this(),
					std::placeholders::_1,
					std::placeholders::_2)));
	}

	void websocket_session::on_read(boost::system::error_code ec,
			std::size_t bytes_transferred)
	{
		boost::ignore_unused(bytes_transferred);

		// Happens when the timer closes the socket
		if (ec == boost::asio::error::operation_aborted)
			return;

		// This indicates that the websocket_session was closed
		if (ec == boost::beast::websocket::error::closed)
			return;

		if (ec)
		{
			BOOST_ASSERT_MSG(!ec, "READ");
			return;
		}

		// Note that there is activity
		activity();

		// Echo the message
		ws_.text(ws_.got_text());
		ws_.async_write(
			buffer_.data(),
			boost::asio::bind_executor(
				strand_,
				std::bind(
					&websocket_session::on_write,
					shared_from_this(),
					std::placeholders::_1,
					std::placeholders::_2)));
	}

	void websocket_session::on_write(boost::system::error_code ec,
				std::size_t bytes_transferred)
	{
		boost::ignore_unused(bytes_transferred);

		// Happens when the timer closes the socket
		if (ec == boost::asio::error::operation_aborted)
			return;

		if (ec)
		{
			BOOST_ASSERT_MSG(!ec, "WRITE");
			return;
		}

		// Clear the buffer
		buffer_.consume(buffer_.size());

		// Do another read
		do_read();
	}
}