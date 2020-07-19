# top level files
set (cluster_lib)

set (cluster_cpp

	src/bus.cpp
	src/config.cpp
)

set (cluster_h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/cluster/bus.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/cluster/config.h
)

set (cluster_shared

	${CMAKE_SOURCE_DIR}/src/main/include/smf/cluster/generator.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/cluster/serializer.h
	${CMAKE_SOURCE_DIR}/lib/shared/src/generator.cpp
	${CMAKE_SOURCE_DIR}/lib/shared/src/serializer.cpp
)

source_group("shared" FILES ${cluster_shared})

# define the main program
set (cluster_lib
  ${cluster_cpp}
  ${cluster_h}
  ${cluster_shared}
)

