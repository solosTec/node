# top level files
set (sml_bus_lib)

set (sml_bus_cpp

	lib/sml/bus/src/serializer.cpp
#	lib/sml/bus/src/generator.cpp
#	lib/sml/bus/src/config.cpp
)

set (sml_bus_h
	src/main/include/smf/sml/defs.h
	src/main/include/smf/sml/bus/serializer.h
#	src/main/include/smf/sml/generator.h
#	src/main/include/smf/sml/config.h
)


#source_group("shared" FILES ${sml_shared})

# define the main program
set (sml_bus_lib
  ${sml_bus_cpp}
  ${sml_bus_h}
)

