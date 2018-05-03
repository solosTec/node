/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_HTTPS_SRV_SESSION_HPP
#define NODE_LIB_HTTPS_SRV_SESSION_HPP

#include <smf/https/srv/websocket.h>
#include <cyng/object.h>

namespace node
{
	namespace https
	{
		template<class Derived>
		class session
		{
			// Access the derived class, this is part of
			// the Curiously Recurring Template Pattern idiom.
			Derived& derived()
			{
				return static_cast<Derived&>(*this);
			}

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
					virtual void operator()(cyng::object obj) = 0;
				};

				session& self_;
				std::vector<std::unique_ptr<work>> items_;

			public:
				explicit queue(session& self)
					: self_(self)
				{
					static_assert(limit > 0, "queue limit must be positive");
					items_.reserve(limit);
				}

				// Returns "true" if we have reached the queue limit
				bool is_full() const
				{
					return items_.size() >= limit;
				}

				// Called when a message finishes sending
				// Returns `true` if the caller should initiate a read
				bool on_write(cyng::object obj)
				{
					BOOST_ASSERT(!items_.empty());
					auto const was_full = is_full();
					items_.erase(items_.begin());
					if (!items_.empty())
					{
						(*items_.front())(obj);
					}
					return was_full;
				}

				// Called by the HTTP handler to send a response.
				template<bool isRequest, class Body, class Fields>
				void operator()(cyng::object obj, boost::beast::http::message<isRequest, Body, Fields>&& msg)
				{
					// This holds a work item
					struct work_impl : work
					{
						session& self_;
						boost::beast::http::message<isRequest, Body, Fields> msg_;

						work_impl(
							session& self,
							boost::beast::http::message<isRequest, Body, Fields>&& msg)
						: self_(self)
							, msg_(std::move(msg))
						{
						}

						void operator()(cyng::object obj)
						{
							boost::beast::http::async_write(
								self_.derived().stream(),
								msg_,
								boost::asio::bind_executor(
									self_.strand_,
									std::bind(
										&session::on_write,
										&self_.derived(),
										//self_.derived().shared_from_this(),
										obj,
										std::placeholders::_1,
										msg_.need_eof())));
						}
					};

					// Allocate and store the work
					items_.push_back(boost::make_unique<work_impl>(self_, std::move(msg)));

					// If there was no previous work, start this one
					if (items_.size() == 1)
					{
						(*items_.front())(obj);
					}
				}
			};


		public:
			// Construct the session
			session(cyng::logging::log_ptr logger
				, session_callback_t cb
				, boost::asio::io_context& ioc
				, boost::beast::flat_buffer buffer
				, std::string const& doc_root
				, std::vector<std::string> const& sub_protocols)
			: logger_(logger)
				, cb_(cb)
				, doc_root_(doc_root)
				, sub_protocols_(sub_protocols)
				, queue_(*this)
				, timer_(ioc, (std::chrono::steady_clock::time_point::max)())
				, strand_(ioc.get_executor())
				, buffer_(std::move(buffer))
			{}

			virtual ~session()
			{}

			void do_read(cyng::object obj)
			{
				// Set the timer
				timer_.expires_after(std::chrono::seconds(15));

				// Make the request empty before reading,
				// otherwise the operation behavior is undefined.
				req_ = {};

				// Read a request
				boost::beast::http::async_read(
					derived().stream(),
					buffer_,
					req_,
					boost::asio::bind_executor(
						strand_,
						std::bind(
							&session::on_read,
							//derived().shared_from_this(),
							&derived(),
							obj, 
							std::placeholders::_1)));
			}

			// Called when the timer expires.
			void on_timer(cyng::object obj, boost::system::error_code ec)
			{
				if (ec && ec != boost::asio::error::operation_aborted)
				{
					CYNG_LOG_FATAL(logger_, "timer: " << ec.message());
					return;
				}

				// Verify that the timer really expired since the deadline may have moved.
				if (timer_.expiry() <= std::chrono::steady_clock::now())
				{
					return derived().do_timeout(obj);
				}

				// Wait on the timer
				timer_.async_wait(
					boost::asio::bind_executor(
						strand_,
						std::bind(
							&session::on_timer,
							this,
							//derived().shared_from_this(),
							obj,
							std::placeholders::_1)));
			}

			void on_read(cyng::object obj, boost::system::error_code ec)
			{
				// Happens when the timer closes the socket
				if (ec == boost::asio::error::operation_aborted)
				{
					return;
				}

				// This means they closed the connection
				if (ec == boost::beast::http::error::end_of_stream)
				{
					return derived().do_eof(obj);
				}

				if (ec)
				{
					CYNG_LOG_ERROR(logger_, "read: " << ec.message());
					return;
				}

				// See if it is a WebSocket Upgrade
				if (boost::beast::websocket::is_upgrade(req_))
				{
					// Transfer the stream to a new WebSocket session
					CYNG_LOG_TRACE(logger_, "upgrade");
					cb_(cyng::generate_invoke("https.upgrade", obj));
					return make_websocket_session(logger_
						, cb_
						, derived().release_stream()
						, std::move(req_)
						, sub_protocols_);
				}

				// Send the response
				handle_request(logger_, doc_root_, std::move(req_), queue_, cb_, obj);

				// If we aren't at the queue limit, try to pipeline another request
				if (!queue_.is_full())
				{
					do_read(obj);
				}
			}

			void on_write(cyng::object obj, boost::system::error_code ec, bool close)
			{
				// Happens when the timer closes the socket
				if (ec == boost::asio::error::operation_aborted)
				{
					return;
				}

				if (ec)
				{
					CYNG_LOG_FATAL(logger_, "write: " << ec.message());
					return;
				}

				if (close)
				{
					// This means we should close the connection, usually because
					// the response indicated the "Connection: close" semantic.
					return derived().do_eof(obj);
				}

				// Inform the queue that a write completed
				if (queue_.on_write(obj))
				{
					// Read another request
					do_read(obj);
				}
			}

			/**
			 * @return session specific hash based on current address
			 */
			std::size_t hash() const noexcept
			{
				static std::hash<decltype(this)>	hasher;
				return hasher(this);
			}

		protected:
			cyng::logging::log_ptr logger_;
			session_callback_t cb_;
			boost::asio::steady_timer timer_;
			boost::asio::strand<boost::asio::io_context::executor_type> strand_;
			boost::beast::flat_buffer buffer_;

		private:
			std::string const& doc_root_;
			std::vector<std::string> const& sub_protocols_;
			boost::beast::http::request<boost::beast::http::string_body> req_;
			queue queue_;

		};
	}
}

#endif