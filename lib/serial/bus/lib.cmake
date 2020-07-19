# top level files
set (serial_bus_lib)

set (serial_bus_cpp

	src/baudrate.cpp
	src/parity.cpp
	src/stopbits.cpp
	src/flow_control.cpp
)

set (serial_bus_h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/serial/baudrate.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/serial/parity.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/serial/stopbits.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/serial/flow_control.h
)

# define the main program
set (serial_bus_lib
  ${serial_bus_cpp}
  ${serial_bus_h}
)

