# top level files
set (http_srv_lib)

set (http_srv_cpp

	lib/http/server/src/websocket.cpp
	lib/http/server/src/server.cpp
	lib/http/server/src/session.cpp
	lib/http/server/src/connections.cpp
	lib/http/server/src/mail_config.cpp
)

set (http_srv_h
	src/main/include/smf/http/srv/websocket.h
	src/main/include/smf/http/srv/server.h
	src/main/include/smf/http/srv/session.h
	src/main/include/smf/http/srv/connections.h
	src/main/include/smf/http/srv/mail_config.h
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
	src/main/include/smf/http/srv/path_cat.h
	lib/http/server/src/path_cat.cpp
	src/main/include/smf/http/srv/mime_type.h
	lib/http/server/src/mime_type.cpp
)

source_group("parser" FILES ${http_parser})
source_group("shared" FILES ${http_shared})

if(${PROJECT_NAME}_SSL_SUPPORT)
	list(APPEND http_shared src/main/include/smf/http/srv/auth.h)
	list(APPEND http_shared lib/http/server/src/auth.cpp)
endif()

# define the main program
set (http_srv_lib
  ${http_srv_cpp}
  ${http_srv_h}
  ${http_parser}
  ${http_shared}
)

