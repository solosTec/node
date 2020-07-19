include (prg.cmake)

add_executable(dash ${node_dash})
target_include_directories(dash
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/main/include;${CMAKE_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
		${CRYPT_INCLUDE_DIR}
		${BOOST_INCLUDE_DIR}
		${PUGIXML_INCLUDE_DIR}
)
target_link_libraries(dash 
	smf_info 
	smf_cluster
	smf_http_srv
	${SMF_CRYPTO_LIBRARY}
	${CYNG_LIBRARIES} 
	${OPENSSL_LIBRARIES}
	"$<$<PLATFORM_ID:Linux>:${Boost_THREAD_LIBRARY};${Boost_SYSTEM_LIBRARY};${Boost_FILESYSTEM_LIBRARY};${Boost_PROGRAM_OPTIONS_LIBRARY};${Boost_RANDOM_LIBRARY};pthread>"
    $<$<AND:$<CXX_COMPILER_VERSION:8.2.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
	"$<$<PLATFORM_ID:Windows>:odbc32.lib>"
)

#
# dashboard (HTTPS)
#
add_executable(dashs ${node_dashs})
target_include_directories(dashs
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/main/include;${CMAKE_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
		${OPENSSL_INCLUDE_DIR}
		${CRYPT_INCLUDE_DIR}
		${BOOST_INCLUDE_DIR}
		${PUGIXML_INCLUDE_DIR}
)
target_link_libraries(dashs 
	smf_info 
	smf_cluster
	smf_https_srv
	${SMF_CRYPTO_LIBRARY}
	${CYNG_LIBRARIES} 
	${OPENSSL_LIBRARIES}
	"$<$<PLATFORM_ID:Linux>:${Boost_THREAD_LIBRARY};${Boost_SYSTEM_LIBRARY};${Boost_FILESYSTEM_LIBRARY};${Boost_PROGRAM_OPTIONS_LIBRARY};${Boost_RANDOM_LIBRARY};${CMAKE_DL_LIBS};pthread>"
    $<$<AND:$<CXX_COMPILER_VERSION:8.2.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
	"$<$<PLATFORM_ID:Windows>:odbc32.lib>"
)
