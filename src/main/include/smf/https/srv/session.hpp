/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_HTTPS_SRV_SESSION_HPP
#define NODE_LIB_HTTPS_SRV_SESSION_HPP

#include <smf/https/srv/connections.h>
#include <smf/https/srv/websocket.h>
#include <smf/http/srv/path_cat.h>
#include <smf/http/srv/parser/multi_part.h>
#include <smf/http/srv/mime_type.h>
#include <smf/http/srv/auth.h>
#include <smf/http/srv/url.h>

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
#if (BOOST_BEAST_VERSION < 248)
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
#else
							boost::beast::http::async_write(
								self_.derived().stream(),
								msg_,
								boost::beast::bind_front_handler(
									&session::on_write,
									&self_.derived(),
									obj,	//	reference
									msg_.need_eof()));
#endif
						}
					};

					// Allocate and store the work
					items_.push_back(boost::make_unique<work_impl>(self_, std::move(msg)));

					// If there was no previous work, start this one
					if (items_.size() == 1)	{
						(*items_.front())(obj);
					}
				}
			};


		public:
			// Construct the session
			session(cyng::logging::log_ptr logger
				, connections& cm
				, boost::uuids::uuid tag
#if (BOOST_BEAST_VERSION < 248)
				, boost::asio::io_context& ioc
#endif
				, boost::beast::flat_buffer buffer
				, std::uint64_t max_upload_size
				, std::string const& doc_root
				, auth_dirs const& ad)
			: logger_(logger)
				, connection_manager_(cm)
				, tag_(tag)
#if (BOOST_BEAST_VERSION < 248)
				, timer_(ioc, (std::chrono::steady_clock::time_point::max)())
				, strand_(ioc.get_executor())
#endif
				, buffer_(std::move(buffer))
				, max_upload_size_((max_upload_size < (10 * 1024)) ? (10 * 1024) : max_upload_size)
				, doc_root_(doc_root)
				, auth_dirs_(ad)
				, queue_(*this)
				, authorized_(false)
			{
				if (max_upload_size < (10 * 1024)) {
					CYNG_LOG_WARNING(logger_, "low upload size: " << max_upload_size << " bytes");
				}
			}

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
#if (BOOST_BEAST_VERSION < 248)
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
#else
				// Construct a new parser for each message
				parser_.emplace();

				// Apply a reasonable limit to the allowed size
				// of the body in bytes to prevent abuse.
				parser_->body_limit(this->max_upload_size_);

				// Set the timeout.
				boost::beast::get_lowest_layer(
					derived().stream()).expires_after(std::chrono::seconds(30));

				// Read a request using the parser-oriented interface
				boost::beast::http::async_read(
					derived().stream(),
					buffer_,
					*parser_,
					boost::beast::bind_front_handler(
						&session::on_read,
						&derived(),
						obj));

#endif
			}

#if (BOOST_BEAST_VERSION < 248)
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
#endif

#if (BOOST_BEAST_VERSION < 248)
			void on_read(cyng::object obj, boost::system::error_code ec)
#else
			void on_read(cyng::object obj, boost::beast::error_code ec, std::size_t bytes_transferred)
#endif
			{
#if (BOOST_BEAST_VERSION < 248)
				// Happens when the timer closes the socket
				if (ec == boost::asio::error::operation_aborted)	{
					CYNG_LOG_WARNING(logger_, tag() << " - timer aborted session");
					connection_manager_.stop_session(tag());
					return;
				}
#endif

				// This means they closed the connection
				if (ec == boost::beast::http::error::end_of_stream)	{
					CYNG_LOG_WARNING(logger_, tag() << " - session was closed");
					return derived().do_eof(obj);
				}

				if (ec)	{
					CYNG_LOG_ERROR(logger_, tag() << " - read: " << ec.message());
					connection_manager_.stop_session(tag());
					return;
				}

				// See if it is a WebSocket Upgrade
#if (BOOST_BEAST_VERSION < 248)
				if (boost::beast::websocket::is_upgrade(req_))
				{
					// Transfer the stream to a new WebSocket session
					CYNG_LOG_TRACE(logger_, tag() << " -> upgrade");

					//
					//	upgrade
					//
					connection_manager_.upgrade(tag(), derived().release_stream(), std::move(req_));
					timer_.cancel();
				}
				else
				{

					//
					// Send the response
					//
					handle_request(obj, std::move(req_));

					// If we aren't at the queue limit, try to pipeline another request
					if (!queue_.is_full()) {
						do_read(obj);
					}
				}
#else
				if (boost::beast::websocket::is_upgrade(parser_->get()))
				{
					//	Disable the timeout.
					//	The websocket::stream uses its own timeout settings.
					boost::beast::get_lowest_layer(derived().stream()).expires_never();

					//
					//	Upgrade.
					//	Create a websocket session, transferring ownership
					//	of both the socket and the HTTP request.
					//
					connection_manager_.upgrade(tag(), derived().release_stream(), parser_->release());
				}
				else
				{
					// Send the response
					handle_request(obj, parser_->release());

					// If we aren't at the queue limit, try to pipeline another request
					if (!queue_.is_full()) {
						do_read(obj);
					}
				}
#endif
			}

#if (BOOST_BEAST_VERSION < 248)
			void on_write(cyng::object obj, boost::system::error_code ec, bool close)
#else
			void on_write(cyng::object obj, bool close, boost::beast::error_code ec, std::size_t bytes_transferred)
#endif
			{

#if (BOOST_BEAST_VERSION < 248)
				// Happens when the timer closes the socket
				if (ec == boost::asio::error::operation_aborted)
				{
					connection_manager_.stop_session(tag());
					return;
				}
#endif

				if (ec)
				{
					CYNG_LOG_FATAL(logger_, "write: " << ec.message());
					connection_manager_.stop_session(tag());
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

			void temporary_redirect(cyng::object obj, std::string const& location)
			{
				boost::beast::http::response<boost::beast::http::string_body> res{ boost::beast::http::status::temporary_redirect, 11 };
				res.set(boost::beast::http::field::server, NODE::version_string);
				res.set(boost::beast::http::field::location, location);
				res.body() = std::string("");
				res.prepare_payload();
				queue_(obj, std::move(res));

			}

			void trigger_download(cyng::object obj, cyng::filesystem::path const& path, std::string const& attachment)
			{

				boost::beast::error_code ec;
				boost::beast::http::file_body::value_type body;
				body.open(path.string().c_str(), boost::beast::file_mode::scan, ec);
				if (ec == boost::system::errc::no_such_file_or_directory)
				{
					boost::beast::http::response<boost::beast::http::string_body> res{ boost::beast::http::status::not_found, 11 };
					res.set(boost::beast::http::field::server, NODE::version_string);
					res.set(boost::beast::http::field::content_type, "text/html");
					res.keep_alive(true);
					res.body() = "The resource '" + path.string() + "' was not found.";
					res.prepare_payload();

					return queue_(obj, std::move(res));
				}
				// Cache the size since we need it after the move
				auto const size = body.size();

				boost::beast::http::response<boost::beast::http::file_body> res{
					std::piecewise_construct,
					std::make_tuple(std::move(body)),
					std::make_tuple(boost::beast::http::status::ok, 11) };

				res.content_length(size);


				res.set(boost::beast::http::field::server, NODE::version_string);
				res.set(boost::beast::http::field::content_description, "File Transfer");
				//res.set(boost::beast::http::field::content_type, "application/octet-stream");
				res.set(boost::beast::http::field::content_type, "application/force-download");
				res.set(boost::beast::http::field::content_disposition, "attachment; filename='" + attachment + "'");
				res.set(boost::beast::http::field::expires, "0");
				res.set(boost::beast::http::field::cache_control, "must-revalidate, post-check=0, pre-check=0");

				auto const ext = path.extension().string();
				if (boost::algorithm::equals(ext, ".xml")) {
					res.set(boost::beast::http::field::content_type, "text/xml");
				}
				else if (boost::algorithm::equals(ext, ".csv")) {
					res.set(boost::beast::http::field::content_type, "text/csv");
				}
				else if (boost::algorithm::equals(ext, ".pdf")) {
					res.set(boost::beast::http::field::content_type, "application/pdf");
				}
				else {
					res.set(boost::beast::http::field::content_type, "application/octet-stream");
				}

				queue_(obj, std::move(res));
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

					//
					//	test authorization
					//
					for (auto const& ad : auth_dirs_) {
						if (boost::algorithm::starts_with(req.target(), ad.first)) {

							//
							//	Test auth token
							//
							auto pos = req.find("Authorization");
							if (pos == req.end()) {

								//
								//	authorization required
								//
								CYNG_LOG_WARNING(logger_, "authorization required: "
									<< ad.first
									<< " / "
									<< req.target());

								//
								//	send auth request
								//
								return queue_(obj, send_not_authorized(req.version()
									, req.keep_alive()
									, req.target().to_string()
									, ad.second.type_
									, ad.second.realm_));
							}
							else {

								//
								//	authorized
								//	Basic c29sOm1lbGlzc2E=
								//
								if (authorization_test(pos->value(), ad.second)) {
									CYNG_LOG_DEBUG(logger_, "authorized with "
										<< pos->value());

									if (!authorized_) {
										
#if (BOOST_BEAST_VERSION < 248)
										connection_manager_.vm().async_run(cyng::generate_invoke("http.authorized"
											, tag()
											, true
											, ad.second.type_
											, ad.second.user_
											, ad.first
											, derived().stream().lowest_layer().remote_endpoint()));
#else
										//connection_manager_.vm().async_run(cyng::generate_invoke("http.authorized"
										//	, tag()
										//	, true
										//	, ad.second.type_
										//	, ad.second.user_
										//	, ad.first
										//	, derived().stream().next_layer().socket().remote_endpoint()));
#endif
										authorized_ = true;
									}
								}
								else {
									CYNG_LOG_WARNING(logger_, "authorization failed "
										<< pos->value());
#if (BOOST_BEAST_VERSION < 248)
									connection_manager_.vm().async_run(cyng::generate_invoke("http.authorized"
										, tag()
										, false
										, ad.second.type_
										, ad.second.user_
										, ad.first
										, derived().stream().lowest_layer().remote_endpoint()));
#else
									//connection_manager_.vm().async_run(cyng::generate_invoke("http.authorized"
									//	, tag()
									//	, false
									//	, ad.second.type_
									//	, ad.second.user_
									//	, ad.first
									//	, derived().stream().next_layer().socket().remote_endpoint()));
#endif

									//
									//	send auth request
									//
									return queue_(obj, send_not_authorized(req.version()
										, req.keep_alive()
										, req.target().to_string()
										, ad.second.type_
										, ad.second.realm_));

								}
							}
							break;
						}
					}

					//	apply redirections
					std::string target = req.target().to_string();
					connection_manager_.redirect(target);

					//
					// Build the path to the requested file
					//
					std::string path = path_cat(doc_root_, target);

					//
					// Attempt to open the file
					//
					boost::beast::error_code ec;
					boost::beast::http::file_body::value_type body;
					std::string const url = decode_url(path);
					body.open(url.c_str(), boost::beast::file_mode::scan, ec);

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
					auto target = req.target();	//	/upload/config/device/

					boost::beast::string_view content_type = req[boost::beast::http::field::content_type];
					CYNG_LOG_INFO(logger_, "content type " << content_type);
#ifdef _DEBUG
					CYNG_LOG_DEBUG(logger_, "payload \n" << req.body());
#endif
					CYNG_LOG_INFO(logger_, *req.payload_size() << " bytes posted to " << target);
					std::uint64_t payload_size = *req.payload_size();

					if (boost::algorithm::equals(content_type, "application/xml"))
					{
						connection_manager_.vm().async_run(cyng::generate_invoke("http.post.xml"
							, tag_
							, req.version()
							, std::string(target.begin(), target.end())
							, payload_size
							, std::string(req.body().begin(), req.body().end())));

					}
					//	Content-Type:application/json; charset=UTF-8
					else if (boost::algorithm::starts_with(content_type, "application/json"))
					{
						connection_manager_.vm().async_run(cyng::generate_invoke("http.post.json"
							, tag_
							, req.version()
							, std::string(target.begin(), target.end())
							, payload_size
							, std::string(req.body().begin(), req.body().end())));

					}
					else if (boost::algorithm::starts_with(content_type, "application/x-www-form-urlencoded"))
					{
						//	already encoded
						connection_manager_.vm().async_run(cyng::generate_invoke("http.post.form.urlencoded"
							, tag_
							, req.version()
							, std::string(target.begin(), target.end())
							, payload_size
							, std::string(req.body().begin(), req.body().end())));

					}
					else if (boost::algorithm::starts_with(content_type, "multipart/form-data"))
					{
						//
						//	payload parser
						//
						if (req.payload_size()) {

							node::http::multi_part_parser mpp([&](cyng::vector_t&& prg) {

								//	executed by HTTP/s session
								connection_manager_.vm().async_run(std::move(prg));

							}	, logger_
								, payload_size
								, target
								, tag_);

							//
							//	open new upload sequence
							//
							connection_manager_.vm().async_run(cyng::generate_invoke("http.upload.start"
								, tag_
								, req.version()
								, std::string(target.begin(), target.end())
								, payload_size));

							//
							//	parse payload and generate program sequences
							//
							mpp.parse(req.body().begin(), req.body().end());
						}
						else {
							CYNG_LOG_WARNING(logger_, "no payload for " << target);
						}
					}
					else
					{
						CYNG_LOG_WARNING(logger_, "unknown MIME content type: " << content_type);
					}
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

			boost::beast::http::response<boost::beast::http::string_body> send_not_authorized(std::uint32_t version
				, bool keep_alive
				, std::string target
				, std::string type
				, std::string realm)
			{
				CYNG_LOG_WARNING(logger_, "401 - unauthorized: " << target);

				std::string body =
					"<!DOCTYPE html>\n"
					"<html lang=en>\n"
					"\t<head>\n"
					"\t\t<title>not authorized</title>\n"
					"\t</head>\n"
					"\t<body style=\"font-family:'Courier New', Arial, sans-serif\">\n"
					"\t\t<h1>&#x1F6AB; " + target + "</h1>\n"
					"\t</body>\n"
					"</html>\n"
					;

				boost::beast::http::response<boost::beast::http::string_body> res{ boost::beast::http::status::unauthorized, version };
				res.set(boost::beast::http::field::server, NODE::version_string);
				res.set(boost::beast::http::field::content_type, "text/html");
				res.set(boost::beast::http::field::www_authenticate, "Basic realm=\"" + realm + "\"");
				res.keep_alive(keep_alive);
				res.body() = body;
				res.prepare_payload();
				return res;
			}

		protected:
			cyng::logging::log_ptr logger_;
			connections& connection_manager_;
			boost::uuids::uuid const tag_;
#if (BOOST_BEAST_VERSION < 248)
			boost::asio::steady_timer timer_;
			boost::asio::strand<boost::asio::io_context::executor_type> strand_;
#endif
			boost::beast::flat_buffer buffer_;

		private:
			std::uint64_t const max_upload_size_;
			std::string const doc_root_;
			auth_dirs const&  auth_dirs_;
#if (BOOST_BEAST_VERSION < 248)
			boost::beast::http::request<boost::beast::http::string_body> req_;
#else
			// The parser is stored in an optional container so we can
			// construct it from scratch it at the beginning of each new message.
			boost::optional<boost::beast::http::request_parser<boost::beast::http::string_body>> parser_;
#endif
			queue queue_;
			bool authorized_;

		};
	}
}

#endif
