/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/http/srv/session.h>
#include <smf/http/srv/websocket.h>
#include <smf/http/srv/handle_request.hpp>

namespace node
{
	http_session::queue::queue(http_session& self)
	: self_(self)
	{
		static_assert(limit > 0, "queue limit must be positive");
		items_.reserve(limit);
	}

	// Returns `true` if we have reached the queue limit
	bool http_session::queue::is_full() const
	{
		return items_.size() >= limit;
	}

	// Called when a message finishes sending
	// Returns `true` if the caller should initiate a read
	bool http_session::queue::on_write()
	{
		BOOST_ASSERT(!items_.empty());
		auto const was_full = is_full();
		items_.erase(items_.begin());
		if (!items_.empty())
			(*items_.front())();
		return was_full;
	}

	http_session::http_session(
			boost::asio::ip::tcp::socket socket,
			std::string const& doc_root)
		: socket_(std::move(socket))
		, strand_(socket_.get_executor())
		, timer_(socket_.get_executor().context(),
		(std::chrono::steady_clock::time_point::max)())
		, doc_root_(doc_root)
		, queue_(*this)
	{
	}

	// Start the asynchronous operation
	void http_session::run()
	{
		// Run the timer. The timer is operated
		// continuously, this simplifies the code.
		on_timer({});

		do_read();
	}

	void http_session::do_read()
	{
		// Set the timer
		timer_.expires_after(std::chrono::seconds(15));

		// Read a request
		boost::beast::http::async_read(socket_, buffer_, req_,
			boost::asio::bind_executor(
				strand_,
				std::bind(
					&http_session::on_read,
					shared_from_this(),
					std::placeholders::_1)));
	}

	// Called when the timer expires.
	void http_session::on_timer(boost::system::error_code ec)
	{
		if (ec && ec != boost::asio::error::operation_aborted)
		{
			BOOST_ASSERT_MSG(!ec, "TIMER");
			return;
		}

		// Verify that the timer really expired since the deadline may have moved.
		if (timer_.expiry() <= std::chrono::steady_clock::now())
		{
			// Closing the socket cancels all outstanding operations. They
			// will complete with boost::asio::error::operation_aborted
			socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
			socket_.close(ec);
			return;
		}

		// Wait on the timer
		timer_.async_wait(
			boost::asio::bind_executor(
				strand_,
				std::bind(
					&http_session::on_timer,
					shared_from_this(),
					std::placeholders::_1)));
	}

	void http_session::on_read(boost::system::error_code ec)
	{
		// Happens when the timer closes the socket
		if (ec == boost::asio::error::operation_aborted)
		{
			return;
		}

		// This means they closed the connection
		if (ec == boost::beast::http::error::end_of_stream)
		{
			return do_close();
		}

		if (ec)
		{
			//BOOST_ASSERT_MSG(!ec, "READ");
			return;
		}

		// See if it is a WebSocket Upgrade
		if (boost::beast::websocket::is_upgrade(req_))
		{
			// Create a WebSocket websocket_session by transferring the socket
			std::make_shared<websocket_session>(
				std::move(socket_))->do_accept(std::move(req_));
			return;
		}

		// Send the response
		handle_request(doc_root_, std::move(req_), queue_);

		// If we aren't at the queue limit, try to pipeline another request
		if (!queue_.is_full())
			do_read();
	}

	void http_session::on_write(boost::system::error_code ec, bool close)
	{
		// Happens when the timer closes the socket
		if (ec == boost::asio::error::operation_aborted)
			return;

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

		// Inform the queue that a write completed
		if (queue_.on_write())
		{
			// Read another request
			do_read();
		}
	}

	void http_session::do_close()
	{
		// Send a TCP shutdown
		boost::system::error_code ec;
		socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);

		// At this point the connection is closed gracefully
	}

}

