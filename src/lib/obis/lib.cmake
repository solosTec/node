# 
#	reset 
#
set (obis_lib)

set (obis_cpp
    src/lib/obis/src/defs.cpp
    src/lib/obis/src/conv.cpp
    src/lib/obis/src/db.cpp
    src/lib/obis/src/tree.cpp
    src/lib/obis/src/list.cpp
    src/lib/obis/src/profile.cpp
)
    
set (obis_h
    ${CMAKE_BINARY_DIR}/include/smf.h
    include/smf/obis/defs.h   
    include/smf/obis/defs.ipp
    include/smf/obis/conv.h   
    include/smf/obis/db.h   
    include/smf/obis/tree.hpp   
    include/smf/obis/list.h
    include/smf/obis/profile.h
)


# define the obis lib
set (obis_lib
  ${obis_cpp}
  ${obis_h}
)

