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
#include <smf/http/srv/parser/multi_part.h>
#include <cyng/vm/generator.h>
#include <boost/uuid/uuid_io.hpp>
#include <boost/functional/hash.hpp>

namespace node
{

	namespace http
	{
		session::session(cyng::logging::log_ptr logger
			, connection_manager& cm
			, boost::asio::ip::tcp::socket socket
			, std::string const& doc_root
			, node::bus::shared_type bus
			, boost::uuids::uuid tag)
		: socket_(std::move(socket))
			, strand_(socket_.get_executor())
			, timer_(socket_.get_executor().context(), (std::chrono::steady_clock::time_point::max)())
			, buffer_()
			, logger_(logger)
			, connection_manager_(cm)
			, doc_root_(doc_root)
			, bus_(bus)
			, tag_(tag)
			, queue_(*this)
            , shutdown_(false)
		{}

		session::~session()
		{
			//std::cerr << "session::~session()" << std::endl;
			CYNG_LOG_DEBUG(logger_, "~session("
				<< tag()
				<< ")");
		}

		boost::uuids::uuid session::tag() const noexcept
		{
			return tag_;
		}

		// Start the asynchronous operation
		void session::run(cyng::object obj)
		{
			BOOST_ASSERT(cyng::object_cast<session>(obj) == this);

			// Make sure we run on the strand
			if (!strand_.running_in_this_thread())
				return boost::asio::post(
					boost::asio::bind_executor(
						strand_,
						std::bind(
							&session::run,
							this,
							obj
						)));

			// Run the timer. The timer is operated
			// continuously, this simplifies the code.
			on_timer(boost::system::error_code{}, obj);

			do_read();
		}

		void session::do_read()
		{
            //
            //  no activities during shutdown
            //
            if (shutdown_)  return;

            // Set the timer
			timer_.expires_after(std::chrono::seconds(15));

			// Make the request empty before reading,
			// otherwise the operation behavior is undefined.
			req_ = {};

			// Read a request
			boost::beast::http::async_read(socket_, buffer_, req_,
				boost::asio::bind_executor(
					strand_,
					std::bind(
						&session::on_read,
						this,
						//shared_from_this(),
						std::placeholders::_1)));
		}

		// Called when the timer expires.
		void session::on_timer(boost::system::error_code ec, cyng::object obj)
		{
			BOOST_ASSERT(cyng::object_cast<session>(obj) == this);

			//
            //  no activities during shutdown
            //
            if (shutdown_)  return;
//#if BOOST_OS_LINUX
//            //
//            //  timer is currently not working on linux.
//            //  further investigation requires
//            //
//            return;
//#endif

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
                boost::system::error_code ec;
                auto rep = socket_.remote_endpoint(ec);
                if (!ec) {
                    CYNG_LOG_TRACE(logger_, "session timer started " << rep);
                }
				// Wait on the timer
				timer_.async_wait(
					boost::asio::bind_executor(
						strand_,
						std::bind(&session::on_timer, this, std::placeholders::_1, obj)));
			}
		}

		void session::on_read(boost::system::error_code ec)
		{
            //
            //  no activities during shutdown
            //
            if (shutdown_)  return;

            //CYNG_LOG_TRACE(logger_, "session read use count " << (this->shared_from_this().use_count() - 1));

			// Happens when the timer closes the socket
			if (ec == boost::asio::error::operation_aborted)
			{
				CYNG_LOG_WARNING(logger_, "session aborted - read");
				connection_manager_.stop(this);
				return;
			}

			// This means they closed the connection
			if (ec == boost::beast::http::error::end_of_stream)
			{
				CYNG_LOG_TRACE(logger_, "session closed - read");
				connection_manager_.stop(this);	//	do_close()
				return;
			}

			if (ec)
			{
				CYNG_LOG_ERROR(logger_, "read error: " << ec.message());
				connection_manager_.stop(this);
				return;
			}

			// See if it is a WebSocket Upgrade
			if (boost::beast::websocket::is_upgrade(req_))
			{
				CYNG_LOG_TRACE(logger_, "update session " 
					<< tag()
					<< " to websocket"
					<< socket_.remote_endpoint());

				// Create a WebSocket websocket_session by transferring the socket
				auto res = connection_manager_.upgrade(this);
				if (!res.second.is_null()) {
					//
					//  There is an object bound to the timer that increases the lifetime of session until times is canceled
					//
					timer_.cancel();
					auto ws_ptr =cyng::object_cast<websocket_session>(res.second);
					if (ws_ptr != nullptr) {
						const_cast<websocket_session*>(ws_ptr)->do_accept(std::move(req_), res.second);
					}
					return;
				}
			}

			// Send the response
			CYNG_LOG_TRACE(logger_, "handle request " << socket_.remote_endpoint());
			handle_request(std::move(req_));

			// If we aren't at the queue limit, try to pipeline another request
			if (!queue_.is_full())
			{
				do_read();
			}
		}

		void session::handle_request(boost::beast::http::request<boost::beast::http::string_body>&& req)
		{
			if (req.method() != boost::beast::http::verb::get &&
				req.method() != boost::beast::http::verb::head &&
				req.method() != boost::beast::http::verb::post)
			{
				return queue_(send_bad_request(req.version()
					, req.keep_alive()
					, "Unknown HTTP-method"));
			}

			if (req.target().empty() ||
				req.target()[0] != '/' ||
				req.target().find("..") != boost::beast::string_view::npos)
			{
				return queue_(send_bad_request(req.version()
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
					return queue_(send_not_found(req.version()
						, req.keep_alive()
						, req.target().to_string()));
				}

				// Handle an unknown error
				if (ec)	{
					return queue_(send_server_error(req.version()
						, req.keep_alive()
						, ec));
				}

				// Cache the size since we need it after the move
				auto const size = body.size();

				if (req.method() == boost::beast::http::verb::head)
				{
					// Respond to HEAD request
					return queue_(send_head(req.version(), req.keep_alive(), path, size));
				}
				else if (req.method() == boost::beast::http::verb::get)
				{
					// Respond to GET request
					return queue_(send_get(req.version()
						, req.keep_alive()
						, std::move(body)
						, path
						, size));
				}
			}
			else if (req.method() == boost::beast::http::verb::post)
			{
				auto target = req.target();	//	/upload/config/device/
				CYNG_LOG_INFO(logger_, *req.payload_size() << " bytes posted to " << target);

				boost::beast::string_view content_type = req[boost::beast::http::field::content_type];
				CYNG_LOG_INFO(logger_, "content type " << content_type);
#ifdef _DEBUG
				CYNG_LOG_DEBUG(logger_, "payload \n" << req.body());
#endif

				//
				//	payload parser
				//
				std::uint32_t payload_size = *req.payload_size();
				http::multi_part_parser mpp([&](cyng::vector_t&& prg) {
					bus_->vm_.async_run(std::move(prg));
				}	, logger_
					, payload_size
					, target
					, tag_);

				//
				//	open new upload sequence
				//
				bus_->vm_.async_run(cyng::generate_invoke("http.upload.start"
					, tag_
					, req.version()
					, std::string(target.begin(), target.end())
					, payload_size));

				//
				//	parse payload and generate program sequences
				//
				mpp.parse(req.body().begin(), req.body().end());

				return;
			}
			return queue_(std::move(req));
		}

		boost::beast::http::response<boost::beast::http::string_body> session::send_server_error(std::uint32_t version
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

		boost::beast::http::response<boost::beast::http::string_body> session::send_not_found(std::uint32_t version
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

		boost::beast::http::response<boost::beast::http::empty_body> session::send_head(std::uint32_t version
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

		boost::beast::http::response<boost::beast::http::file_body> session::send_get(std::uint32_t version
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

		boost::beast::http::response<boost::beast::http::string_body> session::send_bad_request(std::uint32_t version
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

		void session::on_write(boost::system::error_code ec, bool close)
		{
            //
            //  no activities during shutdown
            //
            if (shutdown_)  return;

			// Happens when the timer closes the socket
			if (ec == boost::asio::error::operation_aborted)
			{
				CYNG_LOG_WARNING(logger_, "session aborted - write");
				connection_manager_.stop(this);
				return;
			}

			if (ec)
			{
				CYNG_LOG_ERROR(logger_, "write error: " << ec.message());
				connection_manager_.stop(this);
				return;
			}

			if (close)
			{
				// This means we should close the connection, usually because
				// the response indicated the "Connection: close" semantic.
				//return do_close();
				connection_manager_.stop(this);
				return;
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
            shutdown_ = true;

            timer_.cancel();

            if (socket_.is_open())
            {
                CYNG_LOG_WARNING(logger_, "session::do_close(" << socket_.remote_endpoint() << ")");
                // Send a TCP shutdown
                boost::system::error_code ec;
                socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
                socket_.close(ec);
            }
            else
            {
                CYNG_LOG_WARNING(logger_, "session::do_close(" << tag_ << ")");
            }


			// At this point the connection is closed gracefully
		}

		void session::send_moved(std::string const& location)
		{
			//boost::beast::http::response<boost::beast::http::string_body> res{ boost::beast::http::status::ok, req.version() };
			//res.set(boost::beast::http::field::server, NODE::version_string);
			//res.body() = std::string("");
			//res.prepare_payload();
			////res.content_length(body.size());
			//res.keep_alive(req.keep_alive());

			boost::beast::http::response<boost::beast::http::string_body> res{ boost::beast::http::status::temporary_redirect, 11 };
			res.set(boost::beast::http::field::server, NODE::version_string);
			res.set(boost::beast::http::field::location, location);
			res.body() = std::string("");
			res.prepare_payload();
			//res.content_length(body.size());
			//res.keep_alive(req.keep_alive());

			queue_(std::move(res));
		}

		void session::trigger_download(std::string const& path, std::string const& attachment)
		{

			boost::beast::error_code ec;
			boost::beast::http::file_body::value_type body;
			body.open(path.c_str(), boost::beast::file_mode::scan, ec);
			if (ec == boost::system::errc::no_such_file_or_directory)
			{
				boost::beast::http::response<boost::beast::http::string_body> res{ boost::beast::http::status::not_found, 11 };
				res.set(boost::beast::http::field::server, NODE::version_string);
				res.set(boost::beast::http::field::content_type, "text/html");
				res.keep_alive(true);
				res.body() = "The resource '" + path + "' was not found.";
				res.prepare_payload();

				return queue_(std::move(res));
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
			res.set(boost::beast::http::field::content_type, "application/octet-stream");
			res.set(boost::beast::http::field::content_type, "application/force-download");
			res.set(boost::beast::http::field::content_disposition, "attachment; filename='" + attachment + "'");
			res.set(boost::beast::http::field::expires, 0);
			res.set(boost::beast::http::field::cache_control, "must-revalidate, post-check=0, pre-check=0");

			queue_(std::move(res));
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

		cyng::object make_http_session(cyng::logging::log_ptr logger
			, connection_manager& cm
			, boost::asio::ip::tcp::socket&& socket
			, std::string const& doc_root
			, bus::shared_type bus
			, boost::uuids::uuid tag) {

			return cyng::make_object<session>(logger
				, cm
				, std::move(socket)
				, doc_root
				, bus
				, tag);

		}

	}
}

namespace cyng
{
	namespace traits
	{

#if defined(CYNG_LEGACY_MODE_ON)
		const char type_tag<node::http::session>::name[] = "http";
#endif
	}	// traits	
}

namespace std
{
	size_t hash<node::http::session>::operator()(node::http::session const& t) const noexcept
	{
		return boost::hash<boost::uuids::uuid>()(t.tag());
	}

	bool equal_to<node::http::session>::operator()(node::http::session const& t1, node::http::session const& t2) const noexcept
	{
		return t1.tag() == t2.tag();
	}
}
