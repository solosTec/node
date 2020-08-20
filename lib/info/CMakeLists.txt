#
#	programm info info
#
include (lib.cmake)
add_library(smf_info ${GLOBAL_LIBRARY_TYPE} ${info_lib})
target_include_directories(smf_info
	PUBLIC
		${CMAKE_SOURCE_DIR}/nodes
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/main/include;${CMAKE_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
)

