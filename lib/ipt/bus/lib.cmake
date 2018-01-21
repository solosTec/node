# top level files
set (ipt_bus_lib)

set (ipt_bus_cpp

	lib/ipt/bus/src/bus.cpp
	lib/ipt/bus/src/config.cpp
)

set (ipt_bus_h
	src/main/include/smf/ipt/defs.h
	src/main/include/smf/ipt/bus.h
	src/main/include/smf/ipt/config.h
)


#source_group("shared" FILES ${ipt_shared})

# define the main program
set (ipt_bus_lib
  ${ipt_bus_cpp}
  ${ipt_bus_h}
#  ${ipt_shared}
)

