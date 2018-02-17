# top level files
set (http_srv_lib)

set (http_srv_cpp

	lib/http/server/src/path_cat.cpp
	lib/http/server/src/websocket.cpp
	lib/http/server/src/server.cpp
	lib/http/server/src/session.cpp
	lib/http/server/src/mime_type.cpp

)

set (http_srv_h
	src/main/include/smf/http/srv/path_cat.h
	src/main/include/smf/http/srv/websocket.h
	src/main/include/smf/http/srv/server.h
	src/main/include/smf/http/srv/session.h
	src/main/include/smf/http/srv/handle_request.hpp
	src/main/include/smf/http/srv/mime_type.h
)


# define the main program
set (http_srv_lib
  ${http_srv_cpp}
  ${http_srv_h}
)

