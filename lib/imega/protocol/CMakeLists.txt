#
#	iMega/CU protocol library
#
include (lib.cmake)
add_library(smf_protocol_imega ${GLOBAL_LIBRARY_TYPE} ${imega_protocol_lib})
target_include_directories(smf_protocol_imega
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/main/include;${CMAKE_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
)
