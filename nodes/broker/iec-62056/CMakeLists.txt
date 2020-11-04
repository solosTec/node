include (prg.cmake)
add_executable(broker_iec ${node_iec_62056})
target_include_directories(broker_iec
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/main/include;${CMAKE_BINARY_DIR}>"
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
		${CYNG_INCLUDE_DIRS}
		${BOOST_INCLUDE_DIR}
)
target_link_libraries(broker_iec 
	smf_info 
	smf_cluster 
	smf_protocol_mbus
	smf_protocol_iec
	smf_protocol_sml
	${CYNG_LIBRARIES}
	${OPENSSL_LIBRARIES}
	"$<$<PLATFORM_ID:Linux>:${Boost_THREAD_LIBRARY};${Boost_SYSTEM_LIBRARY};${Boost_FILESYSTEM_LIBRARY};${Boost_PROGRAM_OPTIONS_LIBRARY};${Boost_RANDOM_LIBRARY};${CMAKE_DL_LIBS};${CMAKE_DL_LIBS};pthread>"
	$<$<AND:$<VERSION_LESS_EQUAL:${CMAKE_CXX_COMPILER_VERSION},9.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
	$<$<AND:$<VERSION_LESS:${CMAKE_CXX_COMPILER_VERSION},9.0>,$<CXX_COMPILER_ID:Clang>>:stdc++fs>   
)

