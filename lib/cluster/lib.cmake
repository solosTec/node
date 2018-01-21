# top level files
set (cluster_lib)

set (cluster_cpp

	lib/cluster/src/bus.cpp
	lib/cluster/src/config.cpp
)

set (cluster_h
	src/main/include/smf/cluster/bus.h
	src/main/include/smf/cluster/config.h
)

set (cluster_shared

	src/main/include/smf/cluster/generator.h
	src/main/include/smf/cluster/serializer.h
	lib/shared/src/generator.cpp
	lib/shared/src/serializer.cpp
)

source_group("shared" FILES ${cluster_shared})

# define the main program
set (cluster_lib
  ${cluster_cpp}
  ${cluster_h}
  ${cluster_shared}
)

