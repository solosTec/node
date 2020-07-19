#
#	M-Bus/EN 13757-3 protocol/application layer library
#
include (lib.cmake)
add_library(smf_protocol_mbus ${GLOBAL_LIBRARY_TYPE} ${mbus_protocol_lib})
target_include_directories(smf_protocol_mbus
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/main/include;${CMAKE_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
		${CRYPT_INCLUDE_DIR}
		${OPENSSL_INCLUDE_DIR}
)


