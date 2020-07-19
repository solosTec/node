#
#	IEC 62056-21 protocol library
#
include (lib.cmake)
add_library(smf_protocol_iec ${GLOBAL_LIBRARY_TYPE} ${iec_protocol_lib})
target_include_directories(smf_protocol_iec
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/main/include;${CMAKE_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
)
