# 
#	reset 
#
set (cluster_lib)

set (cluster_cpp
    src/lib/cluster/src/cluster.cpp
)
    
set (cluster_h
    include/smf/cluster.h
)


# define the docscript lib
set (cluster_lib
  ${cluster_cpp}
  ${cluster_h}
)

