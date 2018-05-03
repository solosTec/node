/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/http/srv/mime_type.h>

namespace node
{
	boost::beast::string_view mime_type(boost::beast::string_view path)
	{
		//
		//	lambda to find filename extension
		//
		auto const ext = [&path]
		{
			auto const pos = path.rfind(".");
			if (pos == boost::beast::string_view::npos)
			{
				return boost::beast::string_view{};
			}
			return path.substr(pos);
		}();

		//
		//	get MIME type from file extension
		//
		if (boost::beast::iequals(ext, ".htm"))  return "text/html";
		if (boost::beast::iequals(ext, ".html")) return "text/html";
		if (boost::beast::iequals(ext, ".php"))  return "text/html";
		if (boost::beast::iequals(ext, ".css"))  return "text/css";
		if (boost::beast::iequals(ext, ".csv"))  return "text/csv";	//	RFC 7111
		if (boost::beast::iequals(ext, ".txt"))  return "text/plain";
		if (boost::beast::iequals(ext, ".log"))  return "text/plain";

		if (boost::beast::iequals(ext, ".js"))   return "application/javascript";
		if (boost::beast::iequals(ext, ".json")) return "application/json";
		if (boost::beast::iequals(ext, ".xml"))  return "application/xml";
		if (boost::beast::iequals(ext, ".swf"))  return "application/x-shockwave-flash";
		if (boost::beast::iequals(ext, ".pdf"))  return "application/pdf";
		if (boost::beast::iequals(ext, ".zip"))  return "application/zip";
		if (boost::beast::iequals(ext, ".docx"))  return "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
		if (boost::beast::iequals(ext, ".xlsx"))  return "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";

		if (boost::beast::iequals(ext, ".flv"))  return "video/x-flv";
		if (boost::beast::iequals(ext, ".mpeg"))  return "video/mpeg";
		if (boost::beast::iequals(ext, ".ogv"))  return "video/ogg";

		if (boost::beast::iequals(ext, ".png"))  return "image/png";
		if (boost::beast::iequals(ext, ".jpe"))  return "image/jpeg";
		if (boost::beast::iequals(ext, ".jpeg")) return "image/jpeg";
		if (boost::beast::iequals(ext, ".jpg"))  return "image/jpeg";
		if (boost::beast::iequals(ext, ".gif"))  return "image/gif";
		if (boost::beast::iequals(ext, ".bmp"))  return "image/bmp";
		if (boost::beast::iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
		if (boost::beast::iequals(ext, ".tiff")) return "image/tiff";
		if (boost::beast::iequals(ext, ".tif"))  return "image/tiff";
		if (boost::beast::iequals(ext, ".svg"))  return "image/svg+xml";
		if (boost::beast::iequals(ext, ".svgz")) return "image/svg+xml";

		if (boost::beast::iequals(ext, ".midi")) return "adio/midi";
		if (boost::beast::iequals(ext, ".oga")) return "adio/ogg";	//	RFC5334, RFC7845
		if (boost::beast::iequals(ext, ".mp3")) return "adio/mpeg";
		if (boost::beast::iequals(ext, ".mp4")) return "adio/mp4";

		//
		//	default
		//
		return "application/text";
	}
}
