# top level files
set (https_srv_lib)

set (https_srv_cpp

	lib/https/server/src/path_cat.cpp
#	lib/https/server/src/websocket.cpp
	lib/https/server/src/server.cpp
	lib/https/server/src/session.cpp
	lib/https/server/src/mime_type.cpp
#	lib/https/server/src/server_certificate.hpp

)

set (https_srv_h
	src/main/include/smf/https/srv/path_cat.h
#	src/main/include/smf/https/srv/websocket.h
	src/main/include/smf/https/srv/server.h
	src/main/include/smf/https/srv/session.h
	src/main/include/smf/https/srv/handle_request.hpp
	src/main/include/smf/https/srv/mime_type.h
)


# define the main program
set (https_srv_lib
  ${https_srv_cpp}
  ${https_srv_h}
)

