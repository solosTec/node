# top level files
set (serial_bus_lib)

set (serial_bus_cpp

	lib/serial/bus/src/baudrate.cpp
	lib/serial/bus/src/parity.cpp
	lib/serial/bus/src/stopbits.cpp
	lib/serial/bus/src/flow_control.cpp
)

set (serial_bus_h
	src/main/include/smf/serial/baudrate.h
	src/main/include/smf/serial/parity.h
	src/main/include/smf/serial/stopbits.h
	src/main/include/smf/serial/flow_control.h
)

# define the main program
set (serial_bus_lib
  ${serial_bus_cpp}
  ${serial_bus_h}
)

