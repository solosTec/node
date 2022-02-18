# 
#	reset 
#
set (cluster_bus)

set (cluster_bus_cpp
    src/lib/cluster/bus/src/config.cpp
    src/lib/cluster/bus/src/bus.cpp
)
    
set (cluster_bus_h
    include/smf/cluster/config.h
    include/smf/cluster/bus.h
    include/smf/cluster/features.h
)


# define the docscript lib
set (cluster_bus
  ${cluster_bus_cpp}
  ${cluster_bus_h}
)

