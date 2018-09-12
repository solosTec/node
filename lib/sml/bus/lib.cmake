# top level files
set (sml_bus_lib)

set (sml_bus_cpp

	lib/sml/bus/src/serializer.cpp
	lib/sml/bus/src/status.cpp
	lib/sml/bus/src/event.cpp
)

set (sml_bus_h
	src/main/include/smf/sml/defs.h
	src/main/include/smf/sml/bus/serializer.h
	src/main/include/smf/sml/status.h
	src/main/include/smf/sml/event.h
)


#source_group("shared" FILES ${sml_shared})

# define the main program
set (sml_bus_lib
  ${sml_bus_cpp}
  ${sml_bus_h}
)

