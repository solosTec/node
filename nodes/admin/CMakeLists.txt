include (prg.cmake)

add_executable(dash ${node_dash})
target_include_directories(dash
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/main/include;${CMAKE_BINARY_DIR};${CMAKE_CURRENT_SOURCE_DIR}/shared/src>"
		${CYNG_INCLUDE_DIRS}
		${CRYPT_INCLUDE_DIR}
		${BOOST_INCLUDE_DIR}
		${PUGIXML_INCLUDE_DIR}
)
target_link_libraries(dash 
	smf_info 
	smf_cluster
	smf_protocol_sml
	smf_http_srv
	${SMF_CRYPTO_LIBRARY}
	${CYNG_LIBRARIES} 
	${OPENSSL_LIBRARIES}
	"$<$<PLATFORM_ID:Linux>:${Boost_THREAD_LIBRARY};${Boost_SYSTEM_LIBRARY};${Boost_FILESYSTEM_LIBRARY};${Boost_PROGRAM_OPTIONS_LIBRARY};${Boost_RANDOM_LIBRARY};pthread>"
	$<$<AND:$<VERSION_LESS_EQUAL:${CMAKE_CXX_COMPILER_VERSION},9.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
	$<$<AND:$<VERSION_LESS:${CMAKE_CXX_COMPILER_VERSION},9.0>,$<CXX_COMPILER_ID:Clang>>:stdc++fs>   
	"$<$<PLATFORM_ID:Windows>:odbc32.lib>"
)

#
# dashboard (HTTPS)
#
add_executable(dashs ${node_dashs})
target_include_directories(dashs
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/main/include;${CMAKE_BINARY_DIR};${CMAKE_CURRENT_SOURCE_DIR}/shared/src>"
		${CYNG_INCLUDE_DIRS}
		${OPENSSL_INCLUDE_DIR}
		${CRYPT_INCLUDE_DIR}
		${BOOST_INCLUDE_DIR}
		${PUGIXML_INCLUDE_DIR}
)
target_link_libraries(dashs 
	smf_info 
	smf_cluster
	smf_protocol_sml
	smf_https_srv
	${SMF_CRYPTO_LIBRARY}
	${CYNG_LIBRARIES} 
	${OPENSSL_LIBRARIES}
	"$<$<PLATFORM_ID:Linux>:${Boost_THREAD_LIBRARY};${Boost_SYSTEM_LIBRARY};${Boost_FILESYSTEM_LIBRARY};${Boost_PROGRAM_OPTIONS_LIBRARY};${Boost_RANDOM_LIBRARY};${CMAKE_DL_LIBS};pthread>"
	$<$<AND:$<VERSION_LESS_EQUAL:${CMAKE_CXX_COMPILER_VERSION},9.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
	$<$<AND:$<VERSION_LESS:${CMAKE_CXX_COMPILER_VERSION},9.0>,$<CXX_COMPILER_ID:Clang>>:stdc++fs>   
	"$<$<PLATFORM_ID:Windows>:odbc32.lib>"
)

configure_file (
	"${CMAKE_CURRENT_SOURCE_DIR}/shared/assets/oui.csv"
	"${CMAKE_CURRENT_BINARY_DIR}/oui.csv")
