# 
#	reset 
#
set (mbus_lib)

set (mbus_cpp
    src/lib/mbus/protocol/src/units.cpp
    src/lib/mbus/protocol/src/aes.cpp
)
    
set (mbus_h
    include/smf/mbus.h    
    include/smf/mbus/units.h
    include/smf/mbus/aes.h
)


# define the mbus lib
set (mbus_lib
  ${mbus_cpp}
  ${mbus_h}
)

