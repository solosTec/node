include (prg.cmake)
add_executable(broker_mbus_radio ${node_mbus_radio})
target_include_directories(broker_mbus_radio
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/main/include;${CMAKE_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
		${BOOST_INCLUDE_DIR}
)
target_link_libraries(broker_mbus_radio 
	smf_info 
	smf_cluster 
	${CYNG_LIBRARIES}
	${OPENSSL_LIBRARIES}
	"$<$<PLATFORM_ID:Linux>:${Boost_THREAD_LIBRARY};${Boost_SYSTEM_LIBRARY};${Boost_FILESYSTEM_LIBRARY};${Boost_PROGRAM_OPTIONS_LIBRARY};${Boost_RANDOM_LIBRARY};${CMAKE_DL_LIBS};${CMAKE_DL_LIBS};pthread>"
    $<$<AND:$<CXX_COMPILER_VERSION:8.2.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
)
