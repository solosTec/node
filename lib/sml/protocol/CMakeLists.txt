#
#	sml protocol library
#
include (lib.cmake)
add_library(smf_protocol_sml ${GLOBAL_LIBRARY_TYPE} ${sml_protocol_lib})
target_include_directories(smf_protocol_sml
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/main/include;${CMAKE_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
		${OPENSSL_INCLUDE_DIR}
)
