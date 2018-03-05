/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/http/srv/session.h>
#include <smf/http/srv/websocket.h>
#include <smf/http/srv/handle_request.hpp>
#include <smf/http/srv/connections.h>

namespace node
{

	namespace http
	{
		session::session(cyng::logging::log_ptr logger
			, connection_manager& cm
			, boost::asio::ip::tcp::socket socket
			, std::string const& doc_root
			, node::bus::shared_type bus
			, cyng::store::db& cache
			, boost::uuids::uuid tag)
		: socket_(std::move(socket))
			, strand_(socket_.get_executor())
			, timer_(socket_.get_executor().context(), (std::chrono::steady_clock::time_point::max)())
			, buffer_()
			, logger_(logger)
			, connection_manager_(cm)
			, doc_root_(doc_root)
			, bus_(bus)
			, cache_(cache)
			, tag_(tag)
			//, vm_(socket_.get_executor().context(), tag)
			, queue_(*this)
		{
		}

		session::~session()
		{
			//std::cerr << "session::~session()" << std::endl;
		}

		// Start the asynchronous operation
		void session::run()
		{
			// Run the timer. The timer is operated
			// continuously, this simplifies the code.
			on_timer({});

			do_read();
		}

		void session::do_read()
		{
			// Set the timer
			timer_.expires_after(std::chrono::seconds(15));

			// Read a request
			boost::beast::http::async_read(socket_, buffer_, req_,
				boost::asio::bind_executor(
					strand_,
					std::bind(
						&session::on_read,
						shared_from_this(),
						std::placeholders::_1)));
		}

		// Called when the timer expires.
		void session::on_timer(boost::system::error_code ec)
		{
			if (ec && ec != boost::asio::error::operation_aborted)
			{
				CYNG_LOG_WARNING(logger_, "session timer aborted");
				return;
			}

			// Verify that the timer really expired since the deadline may have moved.
			if (timer_.expiry() <= std::chrono::steady_clock::now())
			{
				CYNG_LOG_WARNING(logger_, "session timer expired");

				// Closing the socket cancels all outstanding operations. They
				// will complete with boost::asio::error::operation_aborted
				socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
				socket_.close(ec);
				return;
			}

			if (socket_.is_open())
			{
				CYNG_LOG_TRACE(logger_, "session timer started " << socket_.remote_endpoint());
				// Wait on the timer
				timer_.async_wait(
					boost::asio::bind_executor(
						strand_,
						std::bind(
							&session::on_timer,
							shared_from_this(),
							std::placeholders::_1)));
			}
		}

		void session::on_read(boost::system::error_code ec)
		{
			CYNG_LOG_TRACE(logger_, "session read use count " << this->weak_from_this().use_count());

			// Happens when the timer closes the socket
			if (ec == boost::asio::error::operation_aborted)
			{
				CYNG_LOG_WARNING(logger_, "session aborted - read");
				return connection_manager_.stop(shared_from_this());
			}

			// This means they closed the connection
			if (ec == boost::beast::http::error::end_of_stream)
			{
				CYNG_LOG_TRACE(logger_, "session closed - read");
				//return do_close();
				return connection_manager_.stop(shared_from_this());
			}

			if (ec)
			{
				CYNG_LOG_ERROR(logger_, "read error: " << ec.message());
				return connection_manager_.stop(shared_from_this());
			}

			// See if it is a WebSocket Upgrade
			if (boost::beast::websocket::is_upgrade(req_))
			{
				CYNG_LOG_TRACE(logger_, "update session " << socket_.remote_endpoint() << " to websocket");

				// Create a WebSocket websocket_session by transferring the socket
				//std::make_shared<websocket_session>(logger_, std::move(socket_))->do_accept(std::move(req_));
				auto ptr = connection_manager_.upgrade(shared_from_this());
				const_cast<websocket_session*>(ptr)->do_accept(std::move(req_));
				timer_.cancel();
				return;
			}

			// Send the response
			CYNG_LOG_TRACE(logger_, "handle request " << socket_.remote_endpoint());
			handle_request(logger_, doc_root_, std::move(req_), queue_);

			// If we aren't at the queue limit, try to pipeline another request
			if (!queue_.is_full())
			{
				do_read();
			}
		}

		void session::on_write(boost::system::error_code ec, bool close)
		{
			// Happens when the timer closes the socket
			if (ec == boost::asio::error::operation_aborted)
			{
				CYNG_LOG_WARNING(logger_, "session aborted - write");
				return connection_manager_.stop(shared_from_this());
			}

			if (ec)
			{
				CYNG_LOG_ERROR(logger_, "write error: " << ec.message());
				return connection_manager_.stop(shared_from_this());
			}

			if (close)
			{
				// This means we should close the connection, usually because
				// the response indicated the "Connection: close" semantic.
				//return do_close();
				return connection_manager_.stop(shared_from_this());
			}

			// Inform the queue that a write completed
			if (queue_.on_write())
			{
				// Read another request
				do_read();
			}
		}

		void session::do_close()
		{
			// Send a TCP shutdown
			boost::system::error_code ec;
			socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);

			// At this point the connection is closed gracefully
		}

		// Called when a message finishes sending
		// Returns `true` if the caller should initiate a read
		bool session::queue::on_write()
		{
			BOOST_ASSERT(!items_.empty());
			auto const was_full = is_full();
			items_.erase(items_.begin());
			if (!items_.empty())
			{
				(*items_.front())();
			}
			return was_full;
		}

		session::queue::queue(session& self)
			: self_(self)
		{
			static_assert(limit > 0, "queue limit must be positive");
			items_.reserve(limit);
		}

		// Returns `true` if we have reached the queue limit
		bool session::queue::is_full() const
		{
			return items_.size() >= limit;
		}
	}
}

