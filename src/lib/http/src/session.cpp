/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/http/session.h>
#include <smf/http/ws.h>
#include <smf/http/url.h>
#include <smf/http/mime_type.h>
#include <smf.h>

#include <cyng/log/record.h>
#include <cyng/parse/string.h>

#include <algorithm>

#include <boost/beast/websocket.hpp>
#include <boost/algorithm/string.hpp>

namespace smf {

	namespace http {

        // Append an HTTP rel-path to a local filesystem path.
        // The returned path is normalized for the platform.
        std::string path_cat(
                boost::beast::string_view base,
                boost::beast::string_view path)
        {
            if (base.empty())
                return std::string(path);
            std::string result(base);
#ifdef BOOST_MSVC
            char constexpr path_separator = '\\';
            if (result.back() == path_separator)
                result.resize(result.size() - 1);
            result.append(path.data(), path.size());
            for (auto& c : result)
                if (c == '/')
                    c = path_separator;
#else
            char constexpr path_separator = '/';
            if (result.back() == path_separator)
                result.resize(result.size() - 1);
            result.append(path.data(), path.size());
#endif
            return result;
        }
        session::queue::queue(session& self)
            : self_(self)
        {
            static_assert(limit::value > 0, "queue limit must be positive");
            items_.reserve(limit::value);
        }

        bool session::queue::is_full() const    {
            return items_.size() >= limit::value;
        }

        bool session::queue::on_write()
        {
            BOOST_ASSERT(!items_.empty());
            auto const was_full = is_full();
            items_.erase(items_.begin());
            if (!items_.empty()) {
                (*items_.front())();
            }
            return was_full;
        }


        session::session(boost::asio::ip::tcp::socket&& socket
            , cyng::logger logger
            , std::string doc_root
            , std::map<std::string, std::string> const& redirects_intrinsic
            , auth_dirs const& auths
            , std::uint64_t const max_upload_size
            , std::string const nickname
            , std::chrono::seconds const timeout
            , ws_cb cb)
        : stream_(std::move(socket))
            , buffer_()
            , logger_(logger)
            , doc_root_(doc_root)
            , redirects_intrinsic_(redirects_intrinsic)
            , auths_(auths)
            , max_upload_size_(max_upload_size)
            , nickname_(nickname)
            , timeout_(timeout)
            , ws_cb_(cb)
            , queue_(*this)
            , parser_()
        {
            stream_.expires_after(std::chrono::seconds(20));
        }


        void session::run() {
            // We need to be executing within a strand to perform async operations
              // on the I/O objects in this session. Although not strictly necessary
              // for single-threaded contexts, this example code is written to be
              // thread-safe by default.
            boost::asio::dispatch(
                stream_.get_executor(),
                boost::beast::bind_front_handler(
                    &session::do_read,
                    this->shared_from_this()));
        }

        void session::do_read()
        {
            // Construct a new parser for each message
            parser_.emplace();

            // Apply a reasonable limit to the allowed size
            // of the body in bytes to prevent abuse.
            parser_->body_limit(std::max<std::uint64_t>(max_upload_size_, 10000UL));
            

            // Set the timeout.
            stream_.expires_after(std::max(timeout_, std::chrono::seconds(30)));

            // Read a request using the parser-oriented interface
            boost::beast::http::async_read(
                stream_,
                buffer_,
                *parser_,
                boost::beast::bind_front_handler(&session::on_read, shared_from_this()));
        }

        void session::on_read(boost::beast::error_code ec, std::size_t bytes_transferred) {

            boost::ignore_unused(bytes_transferred);

            //  timeout
            if (ec == boost::asio::error::operation_aborted) {
                CYNG_LOG_WARNING(logger_, "HTTP read timeout " << ec << ": " << ec.message());
                return;
            }

            // This means they closed the connection
            if (ec == boost::beast::http::error::end_of_stream) {
                return do_close();
            }

            if (ec) {
                //return fail(ec, "read");
                CYNG_LOG_WARNING(logger_, "HTTP read " << ec << ": " << ec.message());
                return;
            }

            //
            //	test authorization
            //
            if (!check_auth(parser_->get())) {
                return;
            }

            // See if it is a WebSocket Upgrade
            if (boost::beast::websocket::is_upgrade(parser_->get()))
            {
                //BOOST_ASSERT_MSG(false, "ToDo: implement");
                // Create a websocket session, transferring ownership
                // of both the socket and the HTTP request.
                //std::make_shared<ws>(
                //    stream_.release_socket(), logger_)->do_accept(parser_->release());
                ws_cb_(stream_.release_socket(), parser_->release());
                return;
            }

            // Send the response
            CYNG_LOG_TRACE(logger_, "handle request from " << stream_.socket().remote_endpoint());
            handle_request(parser_->release());

            // If we aren't at the queue limit, try to pipeline another request
            if (!queue_.is_full()) {
                do_read();
            }
        }

        void session::on_write(bool close, boost::beast::error_code ec, std::size_t bytes_transferred) {

            boost::ignore_unused(bytes_transferred);

            if (ec) {
                CYNG_LOG_WARNING(logger_, "HTTP write " << ec << ": " << ec.message());
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

        void session::do_close() {
            // Send a TCP shutdown
            boost::beast::error_code ec;
            stream_.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
            CYNG_LOG_WARNING(logger_, "session::do_close(" << ec.message() << ")");
        }

        void session::handle_request(boost::beast::http::request<boost::beast::http::string_body>&& req) {

            BOOST_ASSERT_MSG(req.version() == 11, "wrong HTTP version");

            //
            //  sanity check
            //
            if (req.target().empty() 
                || req.target()[0] != '/' 
                || req.target().find("..") != boost::beast::string_view::npos)
            {
                return queue_(send_bad_request(req.version()
                    , req.keep_alive()
                    , "Illegal request-target"));
            }

            auto target = req.target().to_string();
            CYNG_LOG_TRACE(logger_, "request target " << target);

            //
            //  apply intrinsic redirects
            //
            intrinsic_redirect(target);

            switch (req.method()) {
            case boost::beast::http::verb::get:
                handle_get_head_request(std::move(req), target, false);
                break;
            case boost::beast::http::verb::head:
                handle_get_head_request(std::move(req), target, true);
                break;
            case boost::beast::http::verb::post:
                handle_post_request(std::move(req), target);
                break;
            case boost::beast::http::verb::options:
                handle_options_request(std::move(req));
                break;
            default:
                return queue_(send_bad_request(req.version()
                    , req.keep_alive()
                    , "Unknown HTTP-method"));
                break;
            }

        }

        void session::intrinsic_redirect(std::string& target) {

            //	skip all characters after '?'
            //	example: /sockjs-node/info?t=1599821614083
            auto const vec = cyng::split(target, "?");

            //  virtual path (not the real one)
            auto const virt_path = (vec.size() == 2)
                ? vec.at(0)
                : target
                ;

            if (vec.size() < 2) {
                if (std::filesystem::is_directory(target)) {
                    target.append("/index.html");
                }
            }

            //
            //  apply intrinsic redirects to virt_path
            //
            auto pos = redirects_intrinsic_.lower_bound(virt_path);
            if (pos != redirects_intrinsic_.end() && pos != redirects_intrinsic_.begin()) {
                --pos;
                auto const& key = pos->first;
                if (boost::algorithm::starts_with(virt_path, key)) {
                    CYNG_LOG_TRACE(logger_, "intrinsic redirect " << virt_path << " ==> " << pos->second);
                    target = pos->second;
                    return; //  complete
                }
            }

#ifdef _DEBUG_HTTP
            //if (virt_path.starts_with("/config")) {
            //    target = "/index.html";
            //}
#endif
        }

        void session::handle_get_head_request(boost::beast::http::request<boost::beast::http::string_body>&& req
            , std::string const& target
            , bool head) {

            // Build the path to the requested file
            std::string path = path_cat(doc_root_, target);
            if (target.back() == '/')   path.append("index.html");

            //
            // Attempt to open the file
            //
            boost::beast::error_code ec;
            boost::beast::http::file_body::value_type body;

            //
            //	uri decoding is coming (https://github.com/vinniefalco/beast/commits/uri)
            //
            std::string const url = decode_url(path);
            body.open(url.c_str(), boost::beast::file_mode::scan, ec);

            // Handle the case where the file doesn't exist
            if (ec == boost::system::errc::no_such_file_or_directory) {

                //
                //	404
                //
                queue_(send_not_found(req.version()
                    , req.keep_alive()
                    , req.target().to_string()));
                return;
            }

            // Handle an unknown error
            if (ec) {

                //
                //	500
                //
                queue_(send_server_error(req.version()
                    , req.keep_alive()
                    , ec));
                return;
            }

            // Cache the size since we need it after the move
            auto const size = body.size();

            if (head) {
                queue_(send_head(req.version(), req.keep_alive(), path, size));
            }
            else {
                queue_(send_get(req.version()
                    , req.keep_alive()
                    , std::move(body)
                    , path
                    , size));
            }

        }

        void session::handle_post_request(boost::beast::http::request<boost::beast::http::string_body>&& req, std::string const& target) {

            boost::beast::string_view content_type = req[boost::beast::http::field::content_type];
            CYNG_LOG_INFO(logger_, "content type " << content_type);

            CYNG_LOG_INFO(logger_, *req.payload_size() << " bytes posted to " << target);
            std::uint64_t payload_size = *req.payload_size();

            if (boost::algorithm::equals(content_type, "application/xml")) {

            }
            else if (boost::algorithm::starts_with(content_type, "application/json")) {

            }
            else if (boost::algorithm::starts_with(content_type, "application/x-www-form-urlencoded")) {

            }
            else if (boost::algorithm::starts_with(content_type, "multipart/form-data")) {

            }

        }

        void session::handle_options_request(boost::beast::http::request<boost::beast::http::string_body>&& req) {
            //  https://developer.mozilla.org/en-US/docs/Web/HTTP/Methods/OPTIONS
            //  204 - no content
            queue_(send_204(req.version()
                , req.keep_alive()
                , "OPTIONS, GET, HEAD, POST" ));

        }

        bool session::check_auth(boost::beast::http::request<boost::beast::http::string_body> const& req)
        {
            auto const target = req.target().to_string();
            if (is_auth_required(target.empty() ? "/" : target, auths_)) {

                //
                //	Test auth token
                //
                auto pos = req.find("Authorization");
                if (pos == req.end()) {

                    //
                    //	authorization required
                    //
                    CYNG_LOG_WARNING(logger_, "missing authorization for resource "
                        << target);

                    //
                    //	send auth request
                    //
                    queue_(send_not_authorized(req.version()
                        , req.keep_alive()
                        , req.target().to_string()
                        , "solos::Tec"));

                    //  authorization failed - but complete
                    return false;

                }
                else {

                    return check_auth(req, pos->value());
                }
            }
            return true;
        }

        bool session::check_auth(boost::beast::http::request<boost::beast::http::string_body> const& req, boost::beast::string_view credentials) {

            CYNG_LOG_DEBUG(logger_, "credentials input: " << credentials.to_string());

            //
            //	authorized as "auth@example.com:secret"
            //	Basic YXV0aEBleGFtcGxlLmNvbTpzZWNyZXQ=
            //
            std::pair<auth, bool> r = is_credentials_valid(credentials, auths_);
            if (r.second) {
                CYNG_LOG_INFO(logger_, "authorized as " << r.first.get_user());

                //
                //  session is authorized
                //
                return true;
            }
            else {
                CYNG_LOG_WARNING(logger_, "authorization failed "
                    << credentials);

                //
                //	send auth request
                //
                queue_(send_not_authorized(req.version()
                    , req.keep_alive()
                    , req.target().to_string()
                    , "solos::Tec"));

            }
            return false;
        }

        boost::beast::http::response<boost::beast::http::string_body> session::send_bad_request(std::uint32_t version
            , bool keep_alive
            , std::string const& why)
        {
            CYNG_LOG_WARNING(logger_, "400 - bad request: " << why);
            boost::beast::http::response<boost::beast::http::string_body> res{ boost::beast::http::status::bad_request, version };
            res.set(boost::beast::http::field::server, SMF_VERSION_SUFFIX);
            res.set(boost::beast::http::field::content_type, "text/html");
            res.keep_alive(keep_alive);
            res.body() = why;
            res.prepare_payload();
            return res;
        }

        boost::beast::http::response<boost::beast::http::string_body> session::send_not_found(std::uint32_t version
            , bool keep_alive
            , std::string target) {
            CYNG_LOG_WARNING(logger_, "404 - not found: " << target);

            std::string body =
                "<!DOCTYPE html>\n"
                "<html lang=en>\n"

                "<head>\n"
                "\t<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, shrink-to-fit=no\">\n"
                "\t<title>401 not authorized</title>\n"
                "</head>\n"

                "<body style=\"color: #444; margin:0;font: normal 14px/20px Arial, Helvetica, sans-serif; height:100%; background-color: #fff;\">\n"
                "\t<div style=\"height:auto; min-height:100%; \">"
                "\t\t<div style=\"text-align: center; width:800px; margin-left: -400px; position:absolute; top: 30%; left:50%;\">\n"
                "\t\t\t<h1 style=\"margin:0; font-size:150px; line-height:150px; font-weight:bold;\">404</h1>\n"
                "\t\t\t<h2 style=\"margin-top:20px;font-size: 30px;\">Not Found</h2>\n"
                "\t\t\t<p>The resource '" + target + "' was not found.</p>\n"
                "\t\t</div>\n"
                "\t</div>\n"

                "</body>\n"
                "</html>\n"
                ;

            boost::beast::http::response<boost::beast::http::string_body> res{ boost::beast::http::status::not_found, version };
            res.set(boost::beast::http::field::server, SMF_VERSION_SUFFIX);
            res.set(boost::beast::http::field::content_type, "text/html");
            res.keep_alive(keep_alive);
            res.body() = body;
            //res.body() = "The resource '" + target + "' was not found.";
            res.prepare_payload();
            return res;
        }

        boost::beast::http::response<boost::beast::http::string_body> session::send_server_error(std::uint32_t version
            , bool keep_alive
            , boost::system::error_code ec)
        {
            CYNG_LOG_WARNING(logger_, "500 - server error: " << ec);
            boost::beast::http::response<boost::beast::http::string_body> res{ boost::beast::http::status::internal_server_error, version };
            res.set(boost::beast::http::field::server, SMF_VERSION_SUFFIX);
            res.set(boost::beast::http::field::content_type, "text/html");
            res.keep_alive(keep_alive);
            res.body() = "An error occurred: '" + ec.message() + "'";
            res.prepare_payload();
            return res;
        }

        boost::beast::http::response<boost::beast::http::string_body> session::send_redirect(std::uint32_t version
            , bool keep_alive
            , std::string host
            , std::string target)
        {
            CYNG_LOG_WARNING(logger_, "301 - moved permanently: " << target);
            boost::beast::http::response<boost::beast::http::string_body> res{ boost::beast::http::status::moved_permanently, version };
            res.set(boost::beast::http::field::server, SMF_VERSION_SUFFIX);
            res.set(boost::beast::http::field::content_type, "text/html");
            res.set(boost::beast::http::field::location, "https://" + host + target);
            res.keep_alive(keep_alive);
            res.body() = "301 - moved permanently";
            res.prepare_payload();
            return res;
        }

        boost::beast::http::response<boost::beast::http::string_body> session::send_204(std::uint32_t version
            , bool keep_alive
            , std::string options) {

            CYNG_LOG_TRACE(logger_, "204 - no content");
            boost::beast::http::response<boost::beast::http::string_body> res{ boost::beast::http::status::moved_permanently, version };
            res.set(boost::beast::http::field::server, SMF_VERSION_SUFFIX);
            res.set(boost::beast::http::field::allow, options);
            res.keep_alive(keep_alive);
            return res;
        }

        boost::beast::http::response<boost::beast::http::string_body> session::send_not_authorized(std::uint32_t version
            , bool keep_alive
            , std::string target
            , std::string realm) {

            CYNG_LOG_WARNING(logger_, "401 - unauthorized: " << target);

            std::string body =
                "<!DOCTYPE html>\n"
                "<html lang=en>\n"

                "<head>\n"
                "\t<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, shrink-to-fit=no\">\n"
                "\t<title>401 not authorized</title>\n"
                "</head>\n"

                "<body style=\"color: #444; margin:0;font: normal 14px/20px Arial, Helvetica, sans-serif; height:100%; background-color: #fff;\">\n"
                "\t<div style=\"height:auto; min-height:100%; \">"
                "\t\t<div style=\"text-align: center; width:800px; margin-left: -400px; position:absolute; top: 30%; left:50%;\">\n"
                "\t\t\t<h1 style=\"margin:0; font-size:150px; line-height:150px; font-weight:bold;\">401</h1>\n"
                "\t\t\t<h2 style=\"margin-top:20px;font-size: 30px;\">Not Authorized</h2>\n"
                "\t\t\t<p>You are not authorized</p>\n"
                "\t\t</div>\n"
                "\t</div>\n"

                "</body>\n"
                "</html>\n"
                ;

            boost::beast::http::response<boost::beast::http::string_body> res{ boost::beast::http::status::unauthorized, version };
            res.set(boost::beast::http::field::server, SMF_VERSION_SUFFIX);
            res.set(boost::beast::http::field::content_type, "text/html");
            res.set(boost::beast::http::field::www_authenticate, "Basic realm=\"" + realm + "\"");
            res.keep_alive(keep_alive);
            res.body() = body;
            res.prepare_payload();
            return res;

        }

        boost::beast::http::response<boost::beast::http::empty_body> session::send_head(std::uint32_t version
            , bool keep_alive
            , std::string const& path
            , std::uint64_t size)
        {
            boost::beast::http::response<boost::beast::http::empty_body> res{ boost::beast::http::status::ok, version };
            res.set(boost::beast::http::field::server, SMF_VERSION_SUFFIX);
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
            res.set(boost::beast::http::field::server, SMF_VERSION_SUFFIX);
            res.set(boost::beast::http::field::content_type, mime_type(path));
            //  ToDo: Permissions-Policy: interest-cohort=()
            res.insert("X-ServerNickName", nickname_);
            res.content_length(size);
            res.keep_alive(keep_alive);
            return res;
        }

	}	//	http
}



