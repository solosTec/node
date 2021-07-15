# 
#	reset 
#
set (modbus_lib)

set (modbus_cpp
    src/lib/modbus/protocol/src/defs.cpp
)
    
set (modbus_h
    include/smf/modbus/defs.h
)


# define the modbus lib
set (modbus_lib
  ${modbus_cpp}
  ${modbus_h}
)

