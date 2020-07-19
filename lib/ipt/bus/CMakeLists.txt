#
#	ipt bus library (client)
#
include (lib.cmake)
add_library(smf_bus_ipt ${GLOBAL_LIBRARY_TYPE} ${ipt_bus_lib})
target_include_directories(smf_bus_ipt
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/main/include;${CMAKE_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
)

