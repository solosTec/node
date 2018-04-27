/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_HTTP_SRV_HANDLE_REQUEST_HPP
#define NODE_LIB_HTTP_SRV_HANDLE_REQUEST_HPP

#include <NODE_project_info.h>
#include <smf/http/srv/path_cat.h>
#include <smf/http/srv/mime_type.h>
#include <smf/http/srv/parser/multi_part.h>
#include <cyng/log.h>

namespace node
{

	// This function produces an HTTP response for the given
	// request. The type of the response object depends on the
	// contents of the request, so the interface requires the
	// caller to pass a generic lambda for receiving the response.
	template<
		class Body, class Allocator,
		class Send>
		void
		handle_request(cyng::logging::log_ptr logger
			, boost::beast::string_view doc_root
			, boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>>&& req
			, Send&& send
			, boost::uuids::uuid tag
			, std::function<void(cyng::vector_t&&)> vm)
	{
		// Returns a bad request response
		auto const bad_request = [&req, logger](boost::beast::string_view why) {
			CYNG_LOG_WARNING(logger, "400 - bad request: " << why);
			boost::beast::http::response<boost::beast::http::string_body> res{ boost::beast::http::status::bad_request, req.version() };
			res.set(boost::beast::http::field::server, NODE::version_string);
			res.set(boost::beast::http::field::content_type, "text/html");
			res.keep_alive(req.keep_alive());
			res.body() = why.to_string();
			res.prepare_payload();
			return res;
		};

		// Returns a not found response
		auto const not_found =
			[&req, logger](boost::beast::string_view target)
		{
			CYNG_LOG_WARNING(logger, "404 - not found: " << target);
			boost::beast::http::response<boost::beast::http::string_body> res{ boost::beast::http::status::not_found, req.version() };
			res.set(boost::beast::http::field::server, NODE::version_string);
			res.set(boost::beast::http::field::content_type, "text/html");
			res.keep_alive(req.keep_alive());
			res.body() = "The resource '" + target.to_string() + "' was not found.";
			res.prepare_payload();
			return res;
		};

		// Returns a server error response
		auto const server_error =
			[&req, logger](boost::beast::string_view what)
		{
			CYNG_LOG_WARNING(logger, "500 - server error: " << what);
			boost::beast::http::response<boost::beast::http::string_body> res{ boost::beast::http::status::internal_server_error, req.version() };
			res.set(boost::beast::http::field::server, NODE::version_string);
			res.set(boost::beast::http::field::content_type, "text/html");
			res.keep_alive(req.keep_alive());
			res.body() = "An error occurred: '" + what.to_string() + "'";
			res.prepare_payload();
			return res;
		};

		// Returns 401 - not authorized
		auto const not_authorized =
			[&req, logger](boost::beast::string_view target, std::string const& realm)
		{
			CYNG_LOG_WARNING(logger, "401 - not authorized: " << target);
			boost::beast::http::response<boost::beast::http::string_body> res{ boost::beast::http::status::unauthorized, req.version() };
			res.set(boost::beast::http::field::server, NODE::version_string);
			res.set(boost::beast::http::field::content_type, "text/plain");
			res.set(boost::beast::http::field::www_authenticate, boost::beast::string_param("Basic realm=", "\"" + realm + "\""));
			res.keep_alive(req.keep_alive());
			res.body() = "401 - not authorized: '" + target.to_string() + "'";
			res.prepare_payload();
			return res;
		};

		// Make sure we can handle the method
		if (req.method() != boost::beast::http::verb::get &&
			req.method() != boost::beast::http::verb::head &&
			req.method() != boost::beast::http::verb::post)
		{
			return send(bad_request("Unknown HTTP-method"));
		}

		// Request path must be absolute and not contain "..".
		if (req.target().empty() ||
			req.target()[0] != '/' ||
			req.target().find("..") != boost::beast::string_view::npos)
		{
			return send(bad_request("Illegal request-target"));
		}

		//
		//	ToDo: test authorization
		//

		CYNG_LOG_TRACE(logger, "HTTP request: " << req.target());

		if (req.method() == boost::beast::http::verb::get ||
			req.method() == boost::beast::http::verb::head)
		{

			// Build the path to the requested file
			std::string path = path_cat(doc_root, req.target());
			if (req.target().back() == '/')
			{
				path.append("index.html");
			}

			// Attempt to open the file
			boost::beast::error_code ec;
			boost::beast::http::file_body::value_type body;
			body.open(path.c_str(), boost::beast::file_mode::scan, ec);

			// Handle the case where the file doesn't exist
			if (ec == boost::system::errc::no_such_file_or_directory)
			{
				return send(not_found(req.target()));
			}

			// Handle an unknown error
			if (ec)
			{
				return send(server_error(ec.message()));
			}

			// Cache the size since we need it after the move
			auto const size = body.size();

			// Respond to HEAD request
			if (req.method() == boost::beast::http::verb::head)
			{
				boost::beast::http::response<boost::beast::http::empty_body> res{ boost::beast::http::status::ok, req.version() };
				res.set(boost::beast::http::field::server, NODE::version_string);
				res.set(boost::beast::http::field::content_type, mime_type(path));
				res.content_length(size);
				res.keep_alive(req.keep_alive());
				return send(std::move(res));
			}
			else if (req.method() == boost::beast::http::verb::get)
			{
				// Respond to GET request
				boost::beast::http::response<boost::beast::http::file_body> res{
					std::piecewise_construct,
					std::make_tuple(std::move(body)),
					std::make_tuple(boost::beast::http::status::ok, req.version()) };
				res.set(boost::beast::http::field::server, NODE::version_string);
				res.set(boost::beast::http::field::content_type, mime_type(path));
				res.content_length(size);
				res.keep_alive(req.keep_alive());
				return send(std::move(res));
			}
		}
		else if (req.method() == boost::beast::http::verb::post)
		{
			auto target = req.target();	//	/upload/config/device/
			CYNG_LOG_INFO(logger, *req.payload_size() << " bytes posted to " << target);

			boost::beast::string_view content_type = req[boost::beast::http::field::content_type];
			CYNG_LOG_INFO(logger, "content type " << content_type);
			//if (content_type != "multipart/form-data" && content_type != "application/x-www-form-urlencoded")
			//{
			//	return send(bad_request("Bad request"));
			//}
			CYNG_LOG_DEBUG(logger, "payload \n" << req.body());

			std::uint32_t payload_size = *req.payload_size();
			http::multi_part_parser mpp(vm
				, logger
				, payload_size
				, target
				, tag);


			for (char c : req.body())
			{ 
				mpp.parse(c);
			}
			
			//
			//	consider to send a 302 - Object moved response
			//
			boost::beast::http::response<boost::beast::http::string_body> res{ boost::beast::http::status::ok, req.version() };
			res.set(boost::beast::http::field::server, NODE::version_string);
			res.body() = std::string("");
			res.prepare_payload();
			//res.content_length(body.size());
			res.keep_alive(req.keep_alive());
			return send(std::move(res));
		}
		return send(std::move(req));
	}
}
#endif
