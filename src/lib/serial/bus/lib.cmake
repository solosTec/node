# top level files
set (serial_bus)

set (serial_bus_cpp

	src/lib/serial/bus/src/baudrate.cpp
	src/lib/serial/bus/src/parity.cpp
	src/lib/serial/bus/src/stopbits.cpp
	src/lib/serial/bus/src/flow_control.cpp
)

set (serial_bus_h
	include/smf/serial/baudrate.h
	include/smf/serial/parity.h
	include/smf/serial/stopbits.h
	include/smf/serial/flow_control.h
)

# define the main program
set (serial_bus
  ${serial_bus_cpp}
  ${serial_bus_h}
)

