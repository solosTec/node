# 
#	reset 
#
set (ipt_bus)

set (ipt_bus_cpp
    src/lib/ipt/bus/src/config.cpp
    src/lib/ipt/bus/src/bus.cpp
)
    
set (ipt_bus_h
    include/smf.h
    include/smf/ipt.h    
    include/smf/ipt/config.h    
    include/smf/ipt/bus.h
)

set (ipt_bus_tasks
    include/smf/ipt/tasks/watchdog.h    
    src/lib/ipt/bus/src/tasks/watchdog.cpp
)

source_group("tasks" FILES ${ipt_bus_tasks})


# define the ipt lib
set (ipt_bus
  ${ipt_bus_cpp}
  ${ipt_bus_h}
  ${ipt_bus_tasks}
)

