# 
#	reset 
#
set (http_lib)

set (http_cpp
    src/lib/http/src/server.cpp
    src/lib/http/src/session.cpp
    src/lib/http/src/ws.cpp
    src/lib/http/src/url.cpp
    src/lib/http/src/mime_type.cpp
    src/lib/http/src/auth.cpp
)
    
set (http_h
    include/smf/http.h
    include/smf/http/server.h
    include/smf/http/session.h
    include/smf/http/ws.h
    include/smf/http/url.h
    include/smf/http/mime_type.h
    include/smf/http/auth.h
)


# define the http lib
set (http_lib
  ${http_cpp}
  ${http_h}
)

