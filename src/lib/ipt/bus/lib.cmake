# 
#	reset 
#
set (ipt_bus)

set (ipt_bus_cpp
    src/lib/ipt/bus/src/config.cpp
    src/lib/ipt/bus/src/bus.cpp
    src/lib/ipt/bus/src/transpiler.cpp
)
    
set (ipt_bus_h
    include/smf.h
    include/smf/ipt.h    
    include/smf/ipt/config.h    
    include/smf/ipt/bus.h
    include/smf/ipt/transpiler.h
)


# define the ipt lib
set (ipt_bus
  ${ipt_bus_cpp}
  ${ipt_bus_h}
)

