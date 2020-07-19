
#
#	serial bus library (client)
#	at it's core it's the boost asio serial_port implementation
#
include (lib.cmake)
add_library(smf_bus_serial ${GLOBAL_LIBRARY_TYPE} ${serial_bus_lib})
target_include_directories(smf_bus_serial
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/main/include;${CMAKE_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
)

