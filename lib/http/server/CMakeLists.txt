#
#	http server library
#
include (lib.cmake)
add_library(smf_http_srv ${GLOBAL_LIBRARY_TYPE} ${http_srv_lib})
target_include_directories(smf_http_srv
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/main/include;${CMAKE_BINARY_DIR}>"
		${BOOST_INCLUDE_DIR}
		${CYNG_INCLUDE_DIRS}
		${CRYPT_INCLUDE_DIR}
)
