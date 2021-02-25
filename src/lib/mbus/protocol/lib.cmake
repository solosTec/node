# 
#	reset 
#
set (mbus_lib)

set (mbus_cpp
    src/lib/mbus/protocol/src/units.cpp
    src/lib/mbus/protocol/src/medium.cpp
    src/lib/mbus/protocol/src/aes.cpp
    src/lib/mbus/protocol/src/flag_id.cpp
    src/lib/mbus/protocol/src/dif.cpp
    src/lib/mbus/protocol/src/vif.cpp
)
    
set (mbus_h
    include/smf/mbus.h    
    include/smf/mbus/field_definitions.h
    include/smf/mbus/units.h
    include/smf/mbus/medium.h
    include/smf/mbus/aes.h
    include/smf/mbus/bcd.hpp
    include/smf/mbus/flag_id.h
    include/smf/mbus/dif.h
    include/smf/mbus/vif.h
)


# define the mbus lib
set (mbus_lib
  ${mbus_cpp}
  ${mbus_h}
)

