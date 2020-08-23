#
#	ipt protocol library
#
include (lib.cmake)
add_library(smf_protocol_ipt ${GLOBAL_LIBRARY_TYPE} ${ipt_protocol_lib})
target_include_directories(smf_protocol_ipt
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/main/include;${CMAKE_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
)

