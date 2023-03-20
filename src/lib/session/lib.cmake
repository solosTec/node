# 
#	reset 
#
set (session_lib)

set (session_lib_cpp
    src/lib/session/src/session.cpp
    src/lib/session/src/tasks/gatekeeper.cpp
)
    
set (session_lib_h
    include/smf/session/session.hpp
    include/smf/session/tasks/gatekeeper.hpp
)


# define the session lib
set (session_lib
  ${session_lib_cpp}
  ${session_lib_h}
)

