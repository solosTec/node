include (prg.cmake)

#
# This program is only for testing puposes
# generic HTTP server
#

add_executable(http ${node_http})
target_include_directories(http
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/main/include;${CMAKE_BINARY_DIR}>"
		${BOOST_INCLUDE_DIR}
		${CYNG_INCLUDE_DIRS}
		${CRYPT_INCLUDE_DIR}
)
target_link_libraries(http 
	smf_info 
	smf_http_srv
	${CYNG_LIBRARIES} 
	${OPENSSL_LIBRARIES}
	${SMF_CRYPTO_LIBRARY}
	"$<$<PLATFORM_ID:Linux>:${Boost_LIBRARIES};pthread>"
)
	
#
# This program is only for testing puposes
# generic HTTPS server
#
add_executable(https ${node_https})
target_include_directories(https
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/main/include;${CMAKE_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
		${OPENSSL_INCLUDE_DIR}
		${CRYPT_INCLUDE_DIR}
		${BOOST_INCLUDE_DIR}
)
target_link_libraries(https 
	smf_info 
	smf_https_srv
	${CYNG_LIBRARIES} 
	${OPENSSL_LIBRARIES}
	${SMF_CRYPTO_LIBRARY}
	"$<$<PLATFORM_ID:Linux>:${Boost_LIBRARIES};pthread>"
)


