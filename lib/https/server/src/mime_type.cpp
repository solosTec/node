/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/http/srv/mime_type.h>

namespace node
{
	boost::beast::string_view
		mime_type(boost::beast::string_view path)
	{
		using boost::beast::iequals;
		auto const ext = [&path]
		{
			auto const pos = path.rfind(".");
			if (pos == boost::beast::string_view::npos)
				return boost::beast::string_view{};
			return path.substr(pos);
		}();
		if (iequals(ext, ".htm"))  return "text/html";
		if (iequals(ext, ".html")) return "text/html";
		if (iequals(ext, ".php"))  return "text/html";
		if (iequals(ext, ".css"))  return "text/css";
		if (iequals(ext, ".csv"))  return "text/csv";	//	RFC 7111
		if (iequals(ext, ".txt"))  return "text/plain";
		if (iequals(ext, ".log"))  return "text/plain";

		if (iequals(ext, ".js"))   return "application/javascript";
		if (iequals(ext, ".json")) return "application/json";
		if (iequals(ext, ".xml"))  return "application/xml";
		if (iequals(ext, ".swf"))  return "application/x-shockwave-flash";
		if (iequals(ext, ".pdf"))  return "application/pdf";
		if (iequals(ext, ".zip"))  return "application/zip";
		if (iequals(ext, ".docx"))  return "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
		if (iequals(ext, ".xlsx"))  return "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";

		if (iequals(ext, ".flv"))  return "video/x-flv";
		if (iequals(ext, ".mpeg"))  return "video/mpeg";
		if (iequals(ext, ".ogv"))  return "video/ogg";

		if (iequals(ext, ".png"))  return "image/png";
		if (iequals(ext, ".jpe"))  return "image/jpeg";
		if (iequals(ext, ".jpeg")) return "image/jpeg";
		if (iequals(ext, ".jpg"))  return "image/jpeg";
		if (iequals(ext, ".gif"))  return "image/gif";
		if (iequals(ext, ".bmp"))  return "image/bmp";
		if (iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
		if (iequals(ext, ".tiff")) return "image/tiff";
		if (iequals(ext, ".tif"))  return "image/tiff";
		if (iequals(ext, ".svg"))  return "image/svg+xml";
		if (iequals(ext, ".svgz")) return "image/svg+xml";

		if (iequals(ext, ".midi")) return "adio/midi";
		if (iequals(ext, ".oga")) return "adio/ogg";	//	RFC5334, RFC7845
		if (iequals(ext, ".mp3")) return "adio/mpeg";
		if (iequals(ext, ".mp4")) return "adio/mp4";

		return "application/text";
	}
}
