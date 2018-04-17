/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 
#ifndef NODE_HTTP_HANDLE_REQUEST_H
#define NODE_HTTP_HANDLE_REQUEST_H

#include <NODE_project_info.h>
#include "path_cat.h"
#include "mime_type.h"
#include "auth.h"
#include <cyng/compatibility/io_service.h>
#include <cyng/log.h>
#include <boost/beast/http.hpp>

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
	handle_request(
		boost::beast::string_view doc_root,
		auth_dirs const& ad,
		boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>>&& req,
		Send&& send,
		cyng::logging::log_ptr logger)
	{
		// Returns a bad request response
		auto const bad_request =
		[&req, logger](boost::beast::string_view why)
		{
			CYNG_LOG_WARNING(logger, "400 - bad request: " << why);	
			boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::bad_request, req.version()};
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
			boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::not_found, req.version()};
			res.set(boost::beast::http::field::server, NODE::version_string);
			res.set(boost::beast::http::field::content_type, "text/html");
			res.keep_alive(req.keep_alive());
			res.body() = "The resource '" + target.to_string() + "' was not found.";
			res.prepare_payload();
			return res;
		};

		// Returns a 500 - server error response
		auto const server_error =
		[&req, logger](boost::beast::string_view what)
		{
			CYNG_LOG_WARNING(logger, "500 - server error: " << what);	
			boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::internal_server_error, req.version()};
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
			boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::unauthorized, req.version()};
			res.set(boost::beast::http::field::server, NODE::version_string);
			res.set(boost::beast::http::field::content_type, "text/plain");
			res.set(boost::beast::http::field::www_authenticate, boost::beast::string_param("Basic realm=", "\"" + realm + "\""));
			res.keep_alive(req.keep_alive());
			res.body() = "401 - not authorized: '" + target.to_string() + "'";
			res.prepare_payload();
			return res;
		};

		// Make sure we can handle the method
		if( req.method() != boost::beast::http::verb::get &&
			req.method() != boost::beast::http::verb::head &&
			req.method() != boost::beast::http::verb::post)
		{
			return send(bad_request("Unknown HTTP-method"));
		}

		// Request path must be absolute and not contain "..".
		if( req.target().empty() ||
			req.target()[0] != '/' ||
			req.target().find("..") != boost::beast::string_view::npos)
		{
			return send(bad_request("Illegal request-target"));
		}

		//
		//	test authorization
		//
		auto auth = authorization_required(req.target(), ad);
		if (auth.second)
		{
			//	authorization required 
			
			auto pos = req.find(boost::beast::http::field::authorization);
			if (pos == req.end())
			{
				return send(not_authorized(req.target(), auth.first.realm_));
			}
			//	Authorization: <type> <credentials>
			if (!authorization_test(pos->value(), auth.first))
			{
				return send(not_authorized(req.target(), auth.first.realm_));
			}
		}
		
		CYNG_LOG_TRACE(logger, "request: " << req.target());	

		// Build the path to the requested file
		std::string path = path_cat(doc_root, req.target());
		if(req.target().back() == '/')
		{
			path.append("index.html");
		}

		
		// Attempt to open the file
		boost::beast::error_code ec;
		boost::beast::http::file_body::value_type body;
		body.open(path.c_str(), boost::beast::file_mode::scan, ec);

		// Handle the case where the file doesn't exist
		if(ec == boost::system::errc::no_such_file_or_directory)
		{
			return send(not_found(req.target()));
		}

		// Handle an unknown error
		if(ec)
		{
			return send(server_error(ec.message()));
		}

		
		// Respond to HEAD request
		if(req.method() == boost::beast::http::verb::head)
		{
			boost::beast::http::response<boost::beast::http::empty_body> res{boost::beast::http::status::ok, req.version()};
			res.set(boost::beast::http::field::server, NODE::version_string);
			res.set(boost::beast::http::field::content_type, mime_type(path));
// 			res.set(boost::beast::http::field::access_control_allow_origin, "*");
// 			res.set(boost::beast::http::field::access_control_allow_methods, "GET, POST");
			res.content_length(body.size());
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
			// 		res.set(boost::beast::http::field::access_control_allow_origin, "*");
			// 		res.set(boost::beast::http::field::access_control_allow_methods, "GET, POST");
			res.content_length(body.size());
			res.keep_alive(req.keep_alive());
			return send(std::move(res));
		}
		else if (req.method() == boost::beast::http::verb::post)
		{
			boost::beast::http::response<boost::beast::http::file_body> res{
				std::piecewise_construct,
				std::make_tuple(std::move(body)),
				std::make_tuple(boost::beast::http::status::ok, req.version()) };
			res.set(boost::beast::http::field::server, NODE::version_string);
			res.set(boost::beast::http::field::content_type, mime_type(path));
			res.content_length(body.size());
			res.keep_alive(req.keep_alive());
			return send(std::move(res));
		}
		return send(std::move(req));
	}
	
}

#endif

