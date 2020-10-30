
#
# IP-T SEGW (gateway)
#
include (prg.cmake)
add_executable(segw ${node_ipt_segw})
target_include_directories(segw
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/main/include;${CMAKE_BINARY_DIR}>"
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
		${CYNG_INCLUDE_DIRS}
		${CRYPT_INCLUDE_DIR}
		${OPENSSL_INCLUDE_DIR}
		${BOOST_INCLUDE_DIR}
)
target_link_libraries(segw 
	smf_info
	smf_protocol_ipt 
	smf_bus_ipt 
	smf_bus_sml 
	smf_bus_serial 
	smf_protocol_sml 
	smf_protocol_mbus 
	smf_protocol_hci 
	${CYNG_LIBRARIES} 
	${SMF_CRYPTO_LIBRARY}
	${OPENSSL_LIBRARIES}
	"$<$<PLATFORM_ID:Linux>:${Boost_THREAD_LIBRARY};${Boost_SYSTEM_LIBRARY};${Boost_FILESYSTEM_LIBRARY};${Boost_PROGRAM_OPTIONS_LIBRARY};${Boost_RANDOM_LIBRARY};${SQLite3_LIBRARY};${CMAKE_DL_LIBS};pthread>"
	$<$<AND:$<VERSION_LESS_EQUAL:${CMAKE_CXX_COMPILER_VERSION},9.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
	$<$<AND:$<VERSION_LESS:${CMAKE_CXX_COMPILER_VERSION},9.0>,$<CXX_COMPILER_ID:Clang>>:stdc++fs>   
	"$<$<PLATFORM_ID:Windows>:odbc32.lib>"
)

