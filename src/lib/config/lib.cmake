# 
#	reset 
#
set (config_lib)

set (config_cpp
    src/lib/config/src/config.cpp
    src/lib/config/src/controller_base.cpp
    src/lib/config/src/schemes.cpp
    src/lib/config/src/kv_store.cpp
    src/lib/config/src/protocols.cpp
)
    
set (config_h
    ${CMAKE_BINARY_DIR}/include/smf.h
    include/smf/config.h    
    include/smf/controller_base.h
    include/smf/config/schemes.h
    include/smf/config/kv_store.h
    include/smf/config/protocols.h
)


# define the config lib
set (config_lib
  ${config_cpp}
  ${config_h}
)

