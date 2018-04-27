# top level files
set (http_srv_lib)

set (http_srv_cpp

	lib/http/server/src/path_cat.cpp
	lib/http/server/src/websocket.cpp
	lib/http/server/src/server.cpp
	lib/http/server/src/session.cpp
	lib/http/server/src/mime_type.cpp
	lib/http/server/src/connections.cpp

)

set (http_srv_h
	src/main/include/smf/http/srv/path_cat.h
	src/main/include/smf/http/srv/websocket.h
	src/main/include/smf/http/srv/server.h
	src/main/include/smf/http/srv/session.h
	src/main/include/smf/http/srv/handle_request.hpp
	src/main/include/smf/http/srv/mime_type.h
	src/main/include/smf/http/srv/connections.h
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

source_group("parser" FILES ${http_parser})

# define the main program
set (http_srv_lib
  ${http_srv_cpp}
  ${http_srv_h}
  ${http_parser}
)

