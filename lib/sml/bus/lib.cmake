# top level files
set (sml_bus_lib)

set (sml_bus_cpp

	src/serializer.cpp
)

set (sml_bus_h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/sml/defs.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/sml/bus/serializer.h
)


#source_group("shared" FILES ${sml_shared})

# define the main program
set (sml_bus_lib
  ${sml_bus_cpp}
  ${sml_bus_h}
)

