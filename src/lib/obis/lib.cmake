# 
#	reset 
#
set (obis_lib)

set (obis_cpp
    src/lib/obis/src/defs.cpp
)
    
set (obis_h
    ${CMAKE_BINARY_DIR}/include/smf.h
    include/smf/obis/defs.h   
)


# define the obis lib
set (obis_lib
  ${obis_cpp}
  ${obis_h}
)

