/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/http/mime_type.h>

namespace smf {
    namespace http {
        boost::beast::string_view mime_type(boost::beast::string_view path) {
            using boost::beast::iequals;
            auto const ext = [&path] {
                auto const pos = path.rfind(".");
                if (pos == boost::beast::string_view::npos)
                    return boost::beast::string_view{};
                return path.substr(pos);
            }();
            if (iequals(ext, ".htm") || iequals(ext, ".html"))
                return "text/html";
            if (iequals(ext, ".php"))
                return "text/html";
            if (iequals(ext, ".css"))
                return "text/css";
            if (iequals(ext, ".csv"))
                return "text/csv"; //	RFC 7111
            if (iequals(ext, ".txt") || iequals(ext, ".log") || iequals(ext, ".h") || iequals(ext, ".cpp") || iequals(ext, ".c"))
                return "text/plain";
            if (iequals(ext, "roff"))
                return "application/x-troff";
            if (iequals(ext, "latex"))
                return "application/x-latex";

            if (iequals(ext, ".js"))
                return "application/javascript";
            if (iequals(ext, ".json"))
                return "application/json";
            if (iequals(ext, ".xml"))
                return "application/xml";
            if (iequals(ext, ".swf"))
                return "application/x-shockwave-flash";
            if (iequals(ext, ".pdf"))
                return "application/pdf";
            if (iequals(ext, ".zip"))
                return "application/zip";
            if (iequals(ext, ".gz"))
                return "application/x-gzip";
            if (iequals(ext, ".gtar"))
                return "application/x-gtar";
            if (iequals(ext, ".docx"))
                return "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
            if (iequals(ext, ".xlsx"))
                return "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";

            if (iequals(ext, ".flv"))
                return "video/x-flv";
            if (iequals(ext, ".mpeg"))
                return "video/mpeg";
            if (iequals(ext, ".ogv"))
                return "video/ogg";

            if (iequals(ext, ".png"))
                return "image/png";
            if (iequals(ext, ".jpe") || iequals(ext, ".jpeg") || iequals(ext, ".jpg"))
                return "image/jpeg";
            if (iequals(ext, ".gif"))
                return "image/gif";
            if (iequals(ext, ".bmp"))
                return "image/bmp";
            if (iequals(ext, ".ico"))
                return "image/vnd.microsoft.icon";
            if (iequals(ext, ".tiff") || iequals(ext, ".tif"))
                return "image/tiff";
            if (iequals(ext, ".svg") || iequals(ext, ".svgz"))
                return "image/svg+xml";

            if (iequals(ext, ".mid") || iequals(ext, ".midi"))
                return "audio/midi";
            if (iequals(ext, ".ogg"))
                return "audio/ogg"; //	RFC5334, RFC7845
            if (iequals(ext, ".mp3"))
                return "audio/mpeg";
            if (iequals(ext, ".mp4"))
                return "audio/mp4";

            return "application/text";
        }
    } // namespace http
} // namespace smf
