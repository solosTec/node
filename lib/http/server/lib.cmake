# top level files
set (http_srv_lib)

set (http_srv_cpp

	src/websocket.cpp
	src/server.cpp
	src/session.cpp
	src/connections.cpp
	src/mail_config.cpp
)

set (http_srv_h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/http/srv/websocket.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/http/srv/server.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/http/srv/session.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/http/srv/connections.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/http/srv/mail_config.h
)

set (http_parser 
	src/parser/multi_part.cpp
	src/parser/content_parser.hpp
	src/parser/content_parser.cpp

	${CMAKE_SOURCE_DIR}/src/main/include/smf/http/srv/parser/multi_part.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/http/srv/parser/content_type.hpp
	${CMAKE_SOURCE_DIR}/src/main/include/smf/http/srv/parser/content_disposition.hpp
	${CMAKE_SOURCE_DIR}/src/main/include/smf/http/srv/parser/content_parser.h
)

set (http_shared
	${CMAKE_SOURCE_DIR}/src/main/include/smf/http/srv/cm_interface.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/http/srv/path_cat.h
	src/path_cat.cpp
	${CMAKE_SOURCE_DIR}/src/main/include/smf/http/srv/mime_type.h
	src/mime_type.cpp
	${CMAKE_SOURCE_DIR}/src/main/include/smf/http/srv/url.h
	src/url.cpp
	${CMAKE_SOURCE_DIR}/src/main/include/smf/http/srv/generator.h
	src/generator.cpp
)

source_group("parser" FILES ${http_parser})
source_group("shared" FILES ${http_shared})

if(${PROJECT_NAME}_SSL_SUPPORT)
	list(APPEND http_shared ${CMAKE_SOURCE_DIR}/src/main/include/smf/http/srv/auth.h)
	list(APPEND http_shared src/auth.cpp)
endif()

# define the main program
set (http_srv_lib
  ${http_srv_cpp}
  ${http_srv_h}
  ${http_parser}
  ${http_shared}
)

