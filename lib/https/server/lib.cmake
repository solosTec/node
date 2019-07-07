# top level files
set (https_srv_lib)

set (https_srv_cpp

	lib/https/server/src/websocket.cpp
	lib/https/server/src/server.cpp
	lib/https/server/src/session.cpp
	lib/https/server/src/detector.cpp
	lib/https/server/src/detect_ssl.hpp
	lib/https/server/src/connections.cpp
)

set (https_srv_h
	src/main/include/smf/https/srv/https.h
	src/main/include/smf/https/srv/websocket.hpp
	src/main/include/smf/https/srv/websocket.h
	src/main/include/smf/https/srv/server.h
	src/main/include/smf/https/srv/session.hpp
	src/main/include/smf/https/srv/session.h
	src/main/include/smf/https/srv/detector.h
#	src/main/include/smf/https/srv/handle_request.hpp
	src/main/include/smf/https/srv/ssl_stream.hpp
	src/main/include/smf/https/srv/connections.h
)

set (http_parser 
	lib/http/server/src/parser/multi_part.cpp
	lib/http/server/src/parser/content_parser.hpp
	lib/http/server/src/parser/content_parser.cpp

	src/main/include/smf/http/srv/parser/multi_part.h
	src/main/include/smf/http/srv/parser/content_type.hpp
	src/main/include/smf/http/srv/parser/content_disposition.hpp
	src/main/include/smf/http/srv/parser/content_parser.h
)

set (http_shared
	src/main/include/smf/http/srv/cm_interface.h
	src/main/include/smf/http/srv/auth.h
	lib/http/server/src/auth.cpp
	src/main/include/smf/http/srv/path_cat.h
	lib/http/server/src/path_cat.cpp
	src/main/include/smf/http/srv/mime_type.h
	lib/http/server/src/mime_type.cpp
	src/main/include/smf/http/srv/url.h
	lib/http/server/src/url.cpp
	src/main/include/smf/http/srv/generator.h
	lib/http/server/src/generator.cpp
)

source_group("parser" FILES ${http_parser})
source_group("shared" FILES ${http_shared})


# define the main program
set (https_srv_lib
  ${https_srv_cpp}
  ${https_srv_h}
  ${http_parser}
  ${http_shared}
)

