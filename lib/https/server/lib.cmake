# top level files
set (https_srv_lib)

set (https_srv_cpp

	src/websocket.cpp
	src/server.cpp
	src/session.cpp
	src/detector.cpp
	src/detect_ssl.hpp
	src/connections.cpp
)

set (https_srv_h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/https/srv/https.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/https/srv/websocket.hpp
	${CMAKE_SOURCE_DIR}/src/main/include/smf/https/srv/websocket.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/https/srv/server.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/https/srv/session.hpp
	${CMAKE_SOURCE_DIR}/src/main/include/smf/https/srv/session.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/https/srv/detector.h
#	${CMAKE_SOURCE_DIR}/src/main/include/smf/https/srv/handle_request.hpp
	${CMAKE_SOURCE_DIR}/src/main/include/smf/https/srv/ssl_stream.hpp
	${CMAKE_SOURCE_DIR}/src/main/include/smf/https/srv/connections.h
)

set (http_parser 
	${CMAKE_SOURCE_DIR}/lib/http/server/src/parser/multi_part.cpp
	${CMAKE_SOURCE_DIR}/lib/http/server/src/parser/content_parser.hpp
	${CMAKE_SOURCE_DIR}/lib/http/server/src/parser/content_parser.cpp

	${CMAKE_SOURCE_DIR}/src/main/include/smf/http/srv/parser/multi_part.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/http/srv/parser/content_type.hpp
	${CMAKE_SOURCE_DIR}/src/main/include/smf/http/srv/parser/content_disposition.hpp
	${CMAKE_SOURCE_DIR}/src/main/include/smf/http/srv/parser/content_parser.h
)

set (http_shared
	${CMAKE_SOURCE_DIR}/src/main/include/smf/http/srv/cm_interface.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/http/srv/auth.h
	${CMAKE_SOURCE_DIR}/lib/http/server/src/auth.cpp
	${CMAKE_SOURCE_DIR}/src/main/include/smf/http/srv/path_cat.h
	${CMAKE_SOURCE_DIR}/lib/http/server/src/path_cat.cpp
	${CMAKE_SOURCE_DIR}/src/main/include/smf/http/srv/mime_type.h
	${CMAKE_SOURCE_DIR}/lib/http/server/src/mime_type.cpp
	${CMAKE_SOURCE_DIR}/src/main/include/smf/http/srv/url.h
	${CMAKE_SOURCE_DIR}/lib/http/server/src/url.cpp
	${CMAKE_SOURCE_DIR}/src/main/include/smf/http/srv/generator.h
	${CMAKE_SOURCE_DIR}/lib/http/server/src/generator.cpp
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

