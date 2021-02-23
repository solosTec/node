# 
#	reset 
#
set (cluster_lib)

set (cluster_cpp
    src/lib/cluster/protocol/src/cluster.cpp
#    src/lib/cluster/src/config.cpp
#    src/lib/cluster/src/bus.cpp
)
    
set (cluster_h
    include/smf/cluster.h
#    include/smf/cluster/config.h
#    include/smf/cluster/bus.h
)


# define the docscript lib
set (cluster_lib
  ${cluster_cpp}
  ${cluster_h}
)

