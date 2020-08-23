
#
# https server library
#
if(${PROJECT_NAME}_SSL_SUPPORT)
	include (lib.cmake)
	add_library(smf_https_srv ${GLOBAL_LIBRARY_TYPE} ${https_srv_lib})
	target_include_directories(smf_https_srv
		PRIVATE
			"$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/main/include;${CMAKE_BINARY_DIR}>"
			${CYNG_INCLUDE_DIRS}
			${OPENSSL_INCLUDE_DIR}
			${CRYPT_INCLUDE_DIR}
	)
        
endif()
