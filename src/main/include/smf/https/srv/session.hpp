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
#include <boost/beast/http.hpp>

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
										obj,	//	reference
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
				, boost::uuids::uuid tag
				, boost::asio::io_context& ioc
				, boost::beast::flat_buffer buffer
				, std::string const& doc_root)
			: logger_(logger)
				, tag_(tag)
				, timer_(ioc, (std::chrono::steady_clock::time_point::max)())
				, strand_(ioc.get_executor())
				, buffer_(std::move(buffer))
				, doc_root_(doc_root)
				, queue_(*this)
			{}

			virtual ~session()
			{}

			/**
			 * @return session tag
			 */
			boost::uuids::uuid tag() const
			{
				return tag_;
			}

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
							&derived(),
							obj,	//	reference
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
							obj,	//	reference
							std::placeholders::_1)));
			}

			void on_read(cyng::object obj, boost::system::error_code ec)
			{
				// Happens when the timer closes the socket
				if (ec == boost::asio::error::operation_aborted)	{
					CYNG_LOG_WARNING(logger_, tag() << " - timer aborted session");
					return;
				}

				// This means they closed the connection
				if (ec == boost::beast::http::error::end_of_stream)	{
					CYNG_LOG_WARNING(logger_, tag() << " - session was closed");
					return derived().do_eof(obj);
				}

				if (ec)	{
					CYNG_LOG_ERROR(logger_, tag() << " - read: " << ec.message());
					//connection_manager_.stop(this);
					return;
				}

				// See if it is a WebSocket Upgrade
				if (boost::beast::websocket::is_upgrade(req_))
				{
					// Transfer the stream to a new WebSocket session
					CYNG_LOG_TRACE(logger_, tag() << " -> upgrade");
					//
					//	ToDo: substitute cb_
					//
					//cb_(cyng::generate_invoke("https.upgrade", obj));
					//return make_websocket_session(logger_
					//	//, cb_
					//	, derived().release_stream()
					//	, std::move(req_));
				}

				// Send the response
				//
				//	ToDo: substitute cb_
				//
				handle_request(obj, std::move(req_));

				// If we aren't at the queue limit, try to pipeline another request
				if (!queue_.is_full())	{
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

		private:
			void handle_request(cyng::object obj, boost::beast::http::request<boost::beast::http::string_body>&& req)
			{
				if (req.method() != boost::beast::http::verb::get &&
					req.method() != boost::beast::http::verb::head &&
					req.method() != boost::beast::http::verb::post)
				{
					return queue_(obj, send_bad_request(req.version()
						, req.keep_alive()
						, "Unknown HTTP-method"));
				}

				if (req.target().empty() ||
					req.target()[0] != '/' ||
					req.target().find("..") != boost::beast::string_view::npos)
				{
					return queue_(obj, send_bad_request(req.version()
						, req.keep_alive()
						, "Illegal request-target"));
				}

				CYNG_LOG_TRACE(logger_, "HTTP request: " << req.target());

				//
				//	handle GET and HEAD methods immediately
				//
				if (req.method() == boost::beast::http::verb::get ||
					req.method() == boost::beast::http::verb::head)
				{
					// Build the path to the requested file
					std::string path = path_cat(doc_root_, req.target());
					if (req.target().back() == '/')
					{
						path.append("index.html");
					}

					//
					// Attempt to open the file
					//
					boost::beast::error_code ec;
					boost::beast::http::file_body::value_type body;
					body.open(path.c_str(), boost::beast::file_mode::scan, ec);

					// Handle the case where the file doesn't exist
					if (ec == boost::system::errc::no_such_file_or_directory)
					{
						//
						//	ToDo: send system message
						//

						//
						//	404
						//
						return queue_(obj, send_not_found(req.version()
							, req.keep_alive()
							, req.target().to_string()));
					}

					// Handle an unknown error
					if (ec) {
						return queue_(obj, send_server_error(req.version()
							, req.keep_alive()
							, ec));
					}

					// Cache the size since we need it after the move
					auto const size = body.size();

					if (req.method() == boost::beast::http::verb::head)
					{
						// Respond to HEAD request
						return queue_(obj, send_head(req.version(), req.keep_alive(), path, size));
					}
					else if (req.method() == boost::beast::http::verb::get)
					{
						// Respond to GET request
						return queue_(obj, send_get(req.version()
							, req.keep_alive()
							, std::move(body)
							, path
							, size));
					}

				}
				else if (req.method() == boost::beast::http::verb::post)
				{
				}
				return queue_(obj, std::move(req));
			}

			boost::beast::http::response<boost::beast::http::string_body> send_bad_request(std::uint32_t version
				, bool keep_alive
				, std::string const& why)
			{
				CYNG_LOG_WARNING(logger_, "400 - bad request: " << why);
				boost::beast::http::response<boost::beast::http::string_body> res{ boost::beast::http::status::bad_request, version };
				res.set(boost::beast::http::field::server, NODE::version_string);
				res.set(boost::beast::http::field::content_type, "text/html");
				res.keep_alive(keep_alive);
				res.body() = why;
				res.prepare_payload();
				return res;
			}

			boost::beast::http::response<boost::beast::http::string_body> send_not_found(std::uint32_t version
				, bool keep_alive
				, std::string target)
			{
				CYNG_LOG_WARNING(logger_, "404 - not found: " << target);
				boost::beast::http::response<boost::beast::http::string_body> res{ boost::beast::http::status::not_found, version };
				res.set(boost::beast::http::field::server, NODE::version_string);
				res.set(boost::beast::http::field::content_type, "text/html");
				res.keep_alive(keep_alive);
				res.body() = "The resource '" + target + "' was not found.";
				res.prepare_payload();
				return res;
			}

			boost::beast::http::response<boost::beast::http::string_body> send_server_error(std::uint32_t version
				, bool keep_alive
				, boost::system::error_code ec)
			{
				CYNG_LOG_WARNING(logger_, "500 - server error: " << ec);
				boost::beast::http::response<boost::beast::http::string_body> res{ boost::beast::http::status::internal_server_error, version };
				res.set(boost::beast::http::field::server, NODE::version_string);
				res.set(boost::beast::http::field::content_type, "text/html");
				res.keep_alive(keep_alive);
				res.body() = "An error occurred: '" + ec.message() + "'";
				res.prepare_payload();
				return res;
			}

			boost::beast::http::response<boost::beast::http::empty_body> send_head(std::uint32_t version
				, bool keep_alive
				, std::string const& path
				, std::uint64_t size)
			{
				boost::beast::http::response<boost::beast::http::empty_body> res{ boost::beast::http::status::ok, version };
				res.set(boost::beast::http::field::server, NODE::version_string);
				res.set(boost::beast::http::field::content_type, mime_type(path));
				res.content_length(size);
				res.keep_alive(keep_alive);
				return res;
			}

			boost::beast::http::response<boost::beast::http::file_body> send_get(std::uint32_t version
				, bool keep_alive
				, boost::beast::http::file_body::value_type&& body
				, std::string const& path
				, std::uint64_t size)
			{
				boost::beast::http::response<boost::beast::http::file_body> res{
					std::piecewise_construct,
					std::make_tuple(std::move(body)),
					std::make_tuple(boost::beast::http::status::ok, version) };
				res.set(boost::beast::http::field::server, NODE::version_string);
				res.set(boost::beast::http::field::content_type, mime_type(path));
				res.content_length(size);
				res.keep_alive(keep_alive);
				return res;

			}

		protected:
			cyng::logging::log_ptr logger_;
			const boost::uuids::uuid tag_;
			boost::asio::steady_timer timer_;
			boost::asio::strand<boost::asio::io_context::executor_type> strand_;
			boost::beast::flat_buffer buffer_;

		private:
			std::string const& doc_root_;
			boost::beast::http::request<boost::beast::http::string_body> req_;
			queue queue_;

		};
	}
}

#endif
