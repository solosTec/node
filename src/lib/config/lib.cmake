# 
#	reset 
#
set (config_lib)

set (config_cpp
    src/lib/config/src/config.cpp
)
    
set (config_h
    ${CMAKE_BINARY_DIR}/include/smf.h
    include/smf/config.h
)


# define the config lib
set (config_lib
  ${config_cpp}
  ${config_h}
)

