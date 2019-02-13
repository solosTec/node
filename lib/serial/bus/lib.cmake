# top level files
set (serial_bus_lib)

set (serial_bus_cpp

	lib/serial/bus/src/baudrate.cpp
)

set (serial_bus_h
	src/main/include/smf/serial/baudrate.h
)

# define the main program
set (serial_bus_lib
  ${serial_bus_cpp}
  ${serial_bus_h}
)

